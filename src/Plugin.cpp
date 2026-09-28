#include <windows.h>
#include <bcrypt.h>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <atomic>
#include "MinHook.h"
#include "Rewards.h"
#include "RoadLane.h"
#include "Performance.h"
#include "DriftStateClient.h"
#include "TextHud.h"
#include "PresentHooks.h"
#include "LanguageConfig.h"
#include "UpdateNotice.h"
#include "Version.h"
#include "DiagnosticGate.h"
#include "ExecutableHash.h"

namespace {
HMODULE module;
wchar_t ini[MAX_PATH]{},logPath[MAX_PATH]{};
an::Config cfg;an::HudConfig hudCfg;an::Detector detector;
volatile LONG active=0;
bool observe=true,hudEnabled=true,driftLink=true;
std::atomic<bool> fault{false},hudFault{false};std::atomic<ULONGLONG> sampleTick{0};
SRWLOCK hudLock=SRWLOCK_INIT;
SRWLOCK renderLock=SRWLOCK_INIT;
struct Row {double points=0;ULONGLONG tick=0;};Row rows[an::Count]{};
an::TextBitmap bitmap;
const an::Locale* language=&an::locales[0];
modperf::Stats perfUpdate,perfHud,perfFrame;
using NosUpdate=float(__thiscall*)(void*,uint32_t,float,uint32_t);
NosUpdate original=nullptr;void* passiveOriginal=nullptr;uint32_t passiveSkip=0x6929e1;
void Log(const char* fmt,...){FILE* f=nullptr;if(_wfopen_s(&f,logPath,L"a")||!f)return;fprintf(f,"[%llu] ",GetTickCount64());va_list a;va_start(a,fmt);vfprintf(f,fmt,a);va_end(a);fputc('\n',f);fclose(f);}
bool Read(uint32_t p,void* dst,size_t n){SIZE_T got=0;return p>=0x10000&&p<=UINT32_MAX-n&&ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(p),dst,n,&got)&&got==n;}
template<class T>bool Get(uint32_t p,T& value){return Read(p,&value,sizeof(value));}
bool Match(uint32_t p,const char* bytes){unsigned char b[64]{};auto n=strlen(bytes)/2;if(n>64||!Read(p,b,n))return false;for(size_t i=0;i<n;i++){unsigned x=0;sscanf_s(bytes+i*2,"%2x",&x);if(b[i]!=x)return false;}return true;}
bool Method(uint32_t obj,unsigned index,uint32_t expected){uint32_t vt=0,f=0;return Get(obj,vt)&&vt>=0x890000&&vt<0x8f0000&&Get(vt+4*index,f)&&f==expected;}
bool Running(){uint32_t flow=0,time=0,state=0;BYTE overlay=0;int a=0,b=0;DWORD pid=0;GetWindowThreadProcessId(GetForegroundWindow(),&pid);
 return pid==GetCurrentProcessId()&&Get(0x925e90,flow)&&flow==6&&Get(0x9885e0,time)&&time&&Get(time+0x2c,state)&&state!=4&&Get(0x9b0fb9,overlay)&&!overlay&&Get(0x9b41fc,a)&&a<=0&&Get(0x9b4220,b)&&b<=0;
}
bool HudVisible(){BYTE table=0;uint32_t h=0,lo=0,hi=0;return Get(0x92fd94,table)&&table&&Get(0x92fd98,h)&&h&&Get(h+0x18,lo)&&Get(h+0x1c,hi)&&(lo||hi);}
void Clear(){detector.reset();sampleTick=0;AcquireSRWLockExclusive(&hudLock);for(auto& r:rows)r={};ReleaseSRWLockExclusive(&hudLock);}
void Draw(IDirect3DDevice9* d){
 modperf::Scope measured(perfHud);
 if(hudFault||!hudEnabled||!Running()||!HudVisible())return;
 const auto now=GetTickCount64(),last=sampleTick.load();if(!last||now-last>250)return;
 D3DDEVICE_CREATION_PARAMETERS info{};DWORD owner=0;if(!d||FAILED(d->GetCreationParameters(&info)))return;GetWindowThreadProcessId(info.hFocusWindow,&owner);if(owner!=GetCurrentProcessId())return;
 static uint64_t lastPresent=0;const auto present=modperf::now();if(lastPresent&&modperf::micros(present-lastPresent)<250000)perfFrame.add(present-lastPresent);lastPresent=present;
 static std::wstring text;static ULONGLONG textTick=0;static unsigned textMask=0;ULONGLONG latest=0;unsigned mask=0;
 Row snapshot[an::Count];AcquireSRWLockShared(&hudLock);std::copy(std::begin(rows),std::end(rows),std::begin(snapshot));ReleaseSRWLockShared(&hudLock);
 for(unsigned i=0;i<an::Count;i++)if(snapshot[i].tick&&now-snapshot[i].tick<1500){mask|=1u<<i;latest=std::max(latest,snapshot[i].tick);}
 if(mask!=textMask||now-textTick>=100){
  text.clear();for(unsigned i=0;i<an::Count;i++)if(mask&(1u<<i)){wchar_t line[160];swprintf_s(line,L"%s + %.0f\n",language->labels[i],std::floor(snapshot[i].points+1e-5));text+=line;}
  textMask=mask;textTick=now;
 }
 if(text.empty())return;
 const float fade=latest&&now-latest>900?float(1500-(now-latest))/600:1;
 if(an::drawText(d,bitmap.render(text),hudCfg,fade,unsigned(std::count(text.begin(),text.end(),L'\n')))){static bool first=true;if(first){Log("HUD_ACTIVE language=%ls independent recovery display",language->code);first=false;}}
}
void GuardedDraw(IDirect3DDevice9* d){if(!TryAcquireSRWLockExclusive(&renderLock))return;__try{Draw(d);}__except(EXCEPTION_EXECUTE_HANDLER){hudFault=true;Log("HUD_FAULT disabled");}ReleaseSRWLockExclusive(&renderLock);}
void RefreshHud(ULONGLONG now){
 static ULONGLONG next=0;static uint32_t previous=0;if(!hudEnabled||hudFault||now<next)return;next=now+1000;
 uint32_t device=0,vt=0,target=0;if(!Get(0x982bdc,device)||!device||!Get(device,vt)||!Get(vt+68,target)||!target||target==previous)return;
 MEMORY_BASIC_INFORMATION page{};if(!VirtualQuery(reinterpret_cast<void*>(target),&page,sizeof(page))||page.State!=MEM_COMMIT||page.AllocationBase==GetModuleHandleW(nullptr)||!(page.Protect&(PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY))){hudFault=true;Log("HUD_REJECT invalid Present");return;}
 const auto status=mcd::PresentHooks::install(reinterpret_cast<void*>(target),GuardedDraw);previous=target;
 if(status==MH_OK)Log("HUD_READY Present=%08X",target);else {hudFault=true;Log("HUD_REJECT status=%d",status);}
}
int Road(uint32_t vehicle,const an::Car& car,uint32_t* snapshot=nullptr){
 uint32_t ai=0,nav=0;
 if(Get(vehicle+0x54,ai)&&ai&&Method(ai,45,0x442a70)){
  nav=reinterpret_cast<uint32_t(__thiscall*)(void*)>(0x442a70)(reinterpret_cast<void*>(ai));
  BYTE valid=0;int16_t segment=-1;an::Vec position{};
  if(nav==ai+0xf4&&Get(nav+0x50,valid)&&valid&&Get(nav+0x8e,segment)&&segment>=0&&Get(nav+0x98,position)&&
     an::finite(position)&&an::length(position-car.pos)<15&&std::abs(position.y-car.pos.y)<3){if(snapshot)*snapshot=nav;return segment;}
 }return -1;
}
bool ReadLane(uint32_t nav,int segment,an::RoadSnapshot& r){
 uint32_t profiles=0,nodes=0,segments=0,np=0,nn=0,ns=0;BYTE loaded=0;
 if(!nav||segment<0||!Get(0x9b3828,loaded)||!loaded||
    !Get(0x9b3884,np)||!np||np>32767||!Get(0x9b387c,nn)||!nn||nn>65535||
    !Get(0x9b3874,ns)||!ns||ns>32767||uint32_t(segment)>=ns||
    !Get(0x9b38b8,profiles)||!profiles||!Get(0x9b38bc,nodes)||!nodes||!Get(0x9b38c0,segments)||!segments||
    !Get(0x9b3a68,r.scale)||!Get(nav+0x8c,r.node)||r.node>1||!Get(nav+0x98,r.position)||
    !Get(nav+0xbc,r.forward)||!Get(nav+0x2c4,r.offset))return false;
 const uint32_t edge=segments+uint32_t(segment)*22;
 if(!Get(edge+10,r.flags))return false;
 for(unsigned i=0;i<2;++i){uint16_t node=0;int16_t profile=-1;
  if(!Get(edge+i*2,node)||node>=nn||!Get(nodes+uint32_t(node)*32+14,profile)||profile<0||uint32_t(profile)>=np||
     !Get(profiles+uint32_t(profile)*64,r.ends[i]))return false;
 }
 return true;
}
bool ReadCar(uint32_t vehicle,an::Car& car,unsigned* wheels=nullptr,float* up=nullptr){
 uint32_t vt=0,driver=0,body=0,ref=0,data=0,owner=0,attribute=0,physics=0,collision=0,cr=0,cd=0;BYTE flags=0,animation=0;
 if(!Get(vehicle,vt)||vt!=0x8aa828||!Get(vehicle+4,owner)||!owner||!Get(vehicle+0x94,driver)||driver>3||
 !Get(vehicle+0xac,physics)||!physics||!Get(vehicle+0xb0,animation)||animation||!Get(vehicle-0x34,body)||!body||
 !Method(body,8,0x671180)||!Method(body,9,0x671190)||!Method(body,18,0x670e50)||!Get(body+0x30,ref)||!Get(ref,data)||
 !Get(data+0x10,car.pos)||!Get(data+0x20,car.velocity)||!Get(data+0x90,car.forward)||!Get(vehicle+0x28,attribute))return false;
 an::Vec dimensions{},upVector{};
 if(!Get(body+0x98,dimensions)||!an::finite(dimensions)||dimensions.x<.5f||dimensions.x>8||dimensions.z<1||dimensions.z>30||
 !an::finite(car.pos)||!an::finite(car.velocity)||!an::finite(car.forward)||!Get(data+0x80,upVector))return false;
 // RB dimension data is a bounding half-extent (e.g. 1.016 x 2.30 m for a car).
 car.halfWidth=dimensions.x;car.halfLength=dimensions.z;car.id=(uint64_t(owner)<<32)|vehicle;
 car.traffic=driver==1;car.collision=true;
 if(Get(vehicle+0x40,collision)&&Method(collision,17,0x670fb0)&&Get(collision+0x28,cr)&&Get(cr,cd)&&Get(cd+0x1c,flags))car.collision=(flags&12)!=0;
 if(wheels&&!Get(vehicle+0x84,*wheels))return false;if(up)*up=upVector.y;
 car.road=-1;
 return true;
}
void Step(uint32_t primary,float dt,uint32_t requested){
 uint32_t vehicle=0,driver=0,engine=0;
 if(!Get(primary+0x48,vehicle)||!vehicle||!Get(vehicle+0x94,driver)||driver!=0)return;
 modperf::Scope measured(perfUpdate);
 if(!Running()||!std::isfinite(dt)||dt<=0||dt>.1f){Clear();return;}
 an::Sample s;s.dt=dt;s.engaged=(requested&0xff)!=0;
 if(!Get(vehicle+0x48,engine)||engine!=primary+0x54||!Method(engine,12,0x6a0470)||!Method(engine,11,0x6a0430)||
 !ReadCar(vehicle,s.player,&s.wheels,&s.up)){Clear();return;}
 s.hasNos=reinterpret_cast<bool(__thiscall*)(void*)>(0x6a0430)(reinterpret_cast<void*>(engine));
 uint32_t props=0;float capacity=0,level=0;
 if(!s.hasNos||!Get(engine+0xd0,props)||!Get(props+0x10,capacity)||!Get(engine+0xa4,level)||!std::isfinite(capacity)||capacity<=0||capacity>120||!std::isfinite(level)||level<0||level>1.001f){Clear();return;}
 uint32_t nav=0;s.valid=true;s.player.road=Road(vehicle,s.player,&nav);s.nosCapacity=capacity;
 s.arcadeDrift=driftLink&&an::arcadeDrift(vehicle);
 // IVehicleAI::GetTopSpeed reads the performance estimate built from vehicle attributes.
 uint32_t speedAI=0;
 if(Get(vehicle+0x54,speedAI)&&Method(speedAI,29,0x431d60)){
  float estimated=0;if(Get(speedAI+0x6ec,estimated)&&std::isfinite(estimated)&&estimated>=20&&estimated<=160)s.topSpeed=estimated;
 }
 an::RoadSnapshot road;const bool roadRead=ReadLane(nav,s.player.road,road);
 const auto laneStatus=roadRead?an::classifyLane(road,s.player):an::LaneStatus::Unknown;
 s.roadKnown=laneStatus!=an::LaneStatus::Unknown;s.oncoming=laneStatus==an::LaneStatus::Oncoming;
 unsigned trafficSeen=0,trafficRead=0,trafficCollision=0;
 static thread_local std::vector<an::Car> neighbours;neighbours.clear();neighbours.reserve(128);uint32_t list=0,count=0;
 if(Get(0x92cd1c,list)&&Get(0x92cd24,count)&&list&&count<=256){for(uint32_t i=0;i<count;i++){
  uint32_t other=0;if(!Get(list+i*4,other)||!other||other==vehicle)continue;
  uint32_t otherDriver=99;Get(other+0x94,otherDriver);if(otherDriver==1)++trafficSeen;
  an::Car car;if(ReadCar(other,car)&&an::length(car.pos-s.player.pos)<100){car.road=Road(other,car);neighbours.push_back(car);if(car.traffic){++trafficRead;if(car.collision)++trafficCollision;}}
 }}
 auto award=detector.step(s,neighbours,cfg);double proposed=0;for(double p:award.points)proposed+=p;
 const auto now=GetTickCount64();sampleTick=now;RefreshHud(now);
 static ULONGLONG nextPerf=0;if(now>=nextPerf){nextPerf=now+10000;perfUpdate.report("nitro_player_update",Log);perfHud.report("nitro_hud_cpu",Log);perfFrame.report("driving_present_interval",Log);}
 static ULONGLONG nextStatus=0;
 static ULONGLONG nextTop=0;
 if(now>=nextTop){nextTop=now+5000;const auto& nd=detector.diagnostics;Log("NEAR_DIAG trafficListed=%u trafficNearbyReadable=%u trafficCollision=%u crossings=%u queued=%u awarded=%u gapReject=%u cancelled=%u",trafficSeen,trafficRead,trafficCollision,nd.crossings,nd.queued,nd.awarded,nd.gapRejected,nd.collisionRejected);Log("TOP_SPEED estimate_mps=%.3f threshold_mps=%.3f qualified=%d tankPerSecond=%.6f source=HPR_STOCK_COP arcadeDrift=%d",s.topSpeed,s.topSpeed*cfg.topSpeedFraction,s.topSpeed>0&&an::length(s.player.velocity)>=s.topSpeed*cfg.topSpeedFraction,cfg.topTankPerSecond,s.arcadeDrift);}

 if(now>=nextStatus){nextStatus=now+5000;Log("SAMPLE dt=%.6f speed=%.3f NOS=%.6f capacity=%.3f wheels=%u road=%d neighbours=%zu halfWidth=%.3f halfLength=%.3f oncomingAdapter=RNpf mode=%s",dt,an::length(s.player.velocity),level,capacity,s.wheels,s.player.road,neighbours.size(),s.player.halfWidth,s.player.halfLength,observe?"observe":"active");
  uint32_t ai=0;if(Get(vehicle+0x54,ai)&&ai){BYTE navBytes[0x2d0]{};if(Read(ai+0xf4,navBytes,sizeof(navBytes))){int16_t segment=0;float laneOffset=0;unsigned path=0,lane=0;memcpy(&segment,navBytes+0x8e,2);memcpy(&laneOffset,navBytes+0x2c4,4);memcpy(&path,navBytes+0x7c,4);memcpy(&lane,navBytes+0x80,4);Log("ROAD_PROBE valid=%u segment=%d node=%d laneIndex=%d pathType=%u laneType=%u laneOffset=%.3f",unsigned(navBytes[0x50]),int(segment),int(int8_t(navBytes[0x8c])),int(int8_t(navBytes[0x2c1])),path,lane,laneOffset);}}
 }
 // Record transitions as well as periodic samples: brief missed linked drifts
 // and unknown road intervals remain diagnosable without per-frame disk writes.
 static an::DiagnosticGate laneLog,driftLog;
 const int laneCode=int(laneStatus);
 if(laneLog.poll(laneCode,now)){
  Log("LANE changes=%u states=%016llX read=%d state=%d segment=%d node=%u flags=%04X offset=%.3f scale=%.8f align=%.3f lateral=%.3f profiles=%u/%u,%u/%u",laneLog.changes,laneLog.seen,roadRead,laneCode,s.player.road,unsigned(road.node),unsigned(road.flags),road.offset,road.scale,an::dot(an::unit(road.forward),an::unit(s.player.velocity)),(s.player.pos.x-road.position.x)*an::unit(road.forward).z-(s.player.pos.z-road.position.z)*an::unit(road.forward).x,unsigned(road.ends[0].count),unsigned(road.ends[0].divider),unsigned(road.ends[1].count),unsigned(road.ends[1].divider));laneLog.emitted(now);
 }
 const int driftCode=(s.arcadeDrift?1:0)|(award.points[an::Drift]>0?2:0)|(s.engaged?4:0)|(s.player.collision?8:0)|(level>=1?16:0);
 if(driftLog.poll(driftCode,now)){Log("DRIFT_DIAG state=%d linked=%d proposed=%.4f NOS=%.6f speed=%.3f beta=%.4f wheels=%u up=%.3f changes=%u states=%016llX",driftCode,s.arcadeDrift,award.points[an::Drift],level,an::length(s.player.velocity),std::acos(std::clamp(an::dot(an::unit(s.player.forward),an::unit(s.player.velocity)),-1.f,1.f)),s.wheels,s.up,driftLog.changes,driftLog.seen);driftLog.emitted(now);}
 if(proposed<=0)return;
 if(observe){static ULONGLONG next=0;if(now>=next){Log("OBSERVE proposed=%.3f nativeNOS=%.6f writes=0",proposed,level);next=now+250;}return;}
 award=an::limitToTank(award,std::max(0.,double(1-level))*capacity,cfg);double points=0;for(double p:award.points)points+=p;if(points<=0)return;
 const float increment=float(points*cfg.secondsPerPoint/capacity);
 reinterpret_cast<void(__thiscall*)(void*,float)>(0x6a0470)(reinterpret_cast<void*>(engine),increment);
 float after=level;if(!Get(engine+0xa4,after)||!std::isfinite(after)||after<level||after>1.001f){fault=true;InterlockedExchange(&active,0);Log("FAULT NOS write result rejected; passive recovery restored");Clear();return;}
 const double actual=double(after-level)*capacity/cfg.secondsPerPoint;
 // Do not round away fractional metres because of float representation in the
 // native gauge. Tank saturation was already applied before calling ChargeNOS.
 const double tolerance=double(capacity)*1.3e-7/cfg.secondsPerPoint;
 const double ratio=std::abs(actual-points)<=tolerance?1.:std::clamp(actual/points,0.,1.);
 AcquireSRWLockExclusive(&hudLock);
 for(unsigned i=0;i<an::Count;i++)if(award.points[i]*ratio>0){auto& r=rows[i];if(now-r.tick>400)r.points=0;r.points+=award.points[i]*ratio;r.tick=now;}
 ReleaseSRWLockExclusive(&hudLock);
 static ULONGLONG nextAward=0;
 static an::Awards accumulated;
 for(unsigned i=0;i<an::Count;i++)accumulated.points[i]+=award.points[i]*ratio;
 if(now>=nextAward||award.points[an::NearMiss]>0||award.points[an::OncomingMiss]>0||award.points[an::Jump]>0||award.points[an::DriftBonus]>0){
  Log("AWARD NOS=%.6f->%.6f intervalPoints near=%.3f lane=%.3f opposite=%.3f jump=%.3f air=%.3f drift=%.3f slip=%.3f chain=%.3f top=%.3f driftBonus=%.3f",level,after,accumulated.points[0],accumulated.points[1],accumulated.points[2],accumulated.points[3],accumulated.points[4],accumulated.points[5],accumulated.points[6],accumulated.points[7],accumulated.points[8],accumulated.points[9]);accumulated={};nextAward=now+250;
 }
}
void GuardedStep(uint32_t primary,float dt,uint32_t requested){__try{Step(primary,dt,requested);}__except(EXCEPTION_EXECUTE_HANDLER){fault=true;InterlockedExchange(&active,0);Log("FAULT detector disabled; passive recovery restored");}}
float __fastcall Update(void* self,void*,uint32_t rpm,float dt,uint32_t engaged){float result=original(self,rpm,dt,engaged);if(!fault)GuardedStep(reinterpret_cast<uint32_t>(self),dt,engaged);return result;}
// Replace only the passive recharge gate, not consumption, capacity or AI NOS.
// Integer-only path leaves the x87 comparison status intact for vanilla fallback.
void __declspec(naked) Passive(){__asm {
 cmp dword ptr [active],0
 je vanilla
 push eax
 mov eax,[esi+48h]
 test eax,eax
 jz restore
 cmp dword ptr [eax+94h],0
 jne restore
 pop eax
 jmp dword ptr [passiveSkip]
 restore:
 pop eax
 vanilla:
 jmp dword ptr [passiveOriginal]
}}
double Number(const wchar_t* section,const wchar_t* key,double fallback,double min,double max){wchar_t b[64];GetPrivateProfileStringW(section,key,L"",b,64,ini);if(!*b)return fallback;wchar_t* end=nullptr;double n=wcstod(b,&end);if(end==b||*end||!std::isfinite(n)||n<min||n>max){Log("CONFIG_REJECT invalid numeric value");throw 1;}return n;}
DWORD WINAPI Initialize(void*){
 GetModuleFileNameW(module,ini,MAX_PATH);auto dot=wcsrchr(ini,L'.');if(!dot)return 0;wcscpy_s(dot,5,L".ini");wcscpy_s(logPath,ini);wcscpy_s(wcsrchr(logPath,L'.'),5,L".log");
 Log("ActionNitro %s pid=%lu",an::version,GetCurrentProcessId());
 if(!GetPrivateProfileIntW(L"ActionNitro",L"Enabled",1,ini)){Log("DISABLED");return 0;}
 char hash[65]{};std::uint64_t fileSize=0;wchar_t executable[MAX_PATH]{};
 const DWORD pathLength=GetModuleFileNameW(nullptr,executable,MAX_PATH);
 const bool hashed=pathLength&&pathLength<MAX_PATH&&an::hashExecutable(executable,hash,fileSize);
 const auto* target=hashed?an::findExecutable(fileSize,hash):nullptr;
 if(!target||reinterpret_cast<uint32_t>(GetModuleHandleW(nullptr))!=0x400000){Log("REJECTED unsupported executable size=%llu hash=%s",fileSize,hash);return 0;}
 Log("HOST target=%s size=%llu sha256=%s",target->id,fileSize,hash);
 observe=GetPrivateProfileIntW(L"ActionNitro",L"ObserveOnly",1,ini)!=0;hudEnabled=GetPrivateProfileIntW(L"HUD",L"Enabled",1,ini)!=0;
 driftLink=GetPrivateProfileIntW(L"ActionNitro",L"OptionalArcadeDrift",1,ini)!=0;
 language=&an::loadLanguage(ini);bitmap.setFont(language->font);Log("LANGUAGE selected=%ls",language->code);
 try {cfg.secondsPerPoint=Number(L"Rewards",L"SecondsPerPoint",.003,.00001,.1);cfg.nearPoints=Number(L"Rewards",L"NearMiss",100,0,10000);cfg.oppositePoints=Number(L"Rewards",L"OncomingMiss",200,0,10000);cfg.jumpPoints=Number(L"Rewards",L"Jump",100,0,10000);cfg.airPerSecond=Number(L"Rewards",L"AirPerSecond",40,0,1000);cfg.driftPerSecond=Number(L"Rewards",L"DriftPerSecond",50,0,1000);
  cfg.chainStep=Number(L"Rewards",L"NearMissChainStep",50,0,10000);cfg.chainMaximum=Number(L"Rewards",L"NearMissChainMaximum",200,0,10000);
  cfg.driftBonus=Number(L"Rewards",L"DriftThreeSecondBonus",100,0,10000);
  hudCfg.right=float(Number(L"HUD",L"Right",30,0,3840));hudCfg.bottom=float(Number(L"HUD",L"Bottom",25,0,2160));hudCfg.width=float(Number(L"HUD",L"Width",430,100,1000));
 }catch(...){return 0;}
 if(!Match(0x765410,"8b4c24048b4424080fb704418b0dbc389b00")||!Match(0x8b5ac4,"02000000")||!Match(0x88d520,"c705683a9b004006483c")||!Match(0x431d60,"d981ec060000c3")||!Match(0x692930,"83ec0c55568bf18b46548d6e54")||!Match(0x6929a4,"dfe0f6c4017536")||!Match(0x6a0470,"568bf18b06ff502c84c0743e")||!Match(0x6a0430,"8b89d0000000d94110")||!Match(0x442a70,"568bf18d4eb4e8a5f9ffff8d86f4000000")||!Match(0x671180,"8b41308b0083c010c3")||!Match(0x670e50,"8b44240481c198000000")) {Log("REJECTED hook/reader guards");return 0;}
 HMODULE pin=nullptr;if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,reinterpret_cast<LPCWSTR>(&Initialize),&pin))return 0;
 if(MH_Initialize()!=MH_OK){Log("REJECTED MinHook initialization");return 0;}
 const auto entry=reinterpret_cast<void*>(0x692930),gate=reinterpret_cast<void*>(0x6929a4);
 if(MH_CreateHook(entry,Update,reinterpret_cast<void**>(&original))!=MH_OK||MH_CreateHook(gate,Passive,&passiveOriginal)!=MH_OK||MH_QueueEnableHook(entry)!=MH_OK||MH_QueueEnableHook(gate)!=MH_OK||MH_ApplyQueued()!=MH_OK){MH_DisableHook(entry);MH_DisableHook(gate);MH_RemoveHook(entry);MH_RemoveHook(gate);Log("REJECTED transactional hook installation");return 0;}
 InterlockedExchange(&active,observe?0:1);Log("READY mode=%s NOSUpdate=00692930 passiveGate=006929A4 noPhysicsHook=1 slipstreamRate=2pt/m oncoming=RNpf_1pt/m topThreshold=75%%",observe?"observe":"active");
 // This is the initialization worker, outside the loader lock and physics/render threads.
 an::CheckForStartupUpdate(module,*language);return 0;
}
}
namespace an {
bool StartupNoticeAllowed() noexcept {uint32_t flow=99;return !fault.load()&&Get(0x925e90,flow)&&flow<6;}
void UpdateLog(const char* message) noexcept {Log("%s",message);}
}
BOOL WINAPI DllMain(HINSTANCE h,DWORD reason,LPVOID){if(reason==DLL_PROCESS_ATTACH){module=h;DisableThreadLibraryCalls(h);HANDLE t=CreateThread(nullptr,0,Initialize,nullptr,0,nullptr);if(t)CloseHandle(t);}return TRUE;}
