// Exercise the actual shipped naked gate against an isolated x86 code fixture.
// No game is started, and no real game file or process is modified.
#include "../src/Plugin.cpp"
#include <cstdlib>
#include "fixtures/DriftStateAPI.h"
static void require(bool ok,const char* why){if(!ok){std::fprintf(stderr,"FAIL %s\n",why);std::exit(1);}}
static float threshold=10;
static void* fixtureGate=nullptr;
static int Invoke(void* primary,float speed){int result=0;__asm {
 push esi
 mov esi,primary
 fld speed
 fcomp dword ptr [threshold]
 mov eax,fixtureGate
 call eax
 mov result,eax
 pop esi
 }return result;}
static void* seenThis=nullptr;static uint32_t seenRpm=0,seenEngaged=0;static float seenDt=0;
__declspec(noinline) float __fastcall Fixture(void* self,void*,uint32_t rpm,float dt,uint32_t engaged){seenThis=self;seenRpm=rpm;seenDt=dt;seenEngaged=engaged;return .125f+dt;}
int main(){
 MWArcadeDriftStateV1 provided{sizeof(provided),1,0,0,0,0,0};
 require(!mcdapi::copy(&provided),"provider empty state rejected");
 mcdapi::publish(42,true,true,GetTickCount64());require(mcdapi::copy(&provided)&&provided.vehicle==42&&provided.drifting==1,"provider read-only snapshot");
 provided.version=2;require(!mcdapi::copy(&provided),"provider unknown ABI rejected");
 provided.version=1;mcdapi::publish(42,true,true,GetTickCount64()-1000);require(!mcdapi::copy(&provided),"provider stale state rejected");
 an::DriftState state{sizeof(state),1,42,1,1,0,1000};
 require(an::validDriftState(state,42,1100),"fresh matching optional drift API");
 require(!an::validDriftState(state,43,1100)&&!an::validDriftState(state,42,1200)&&!an::validDriftState(state,42,900),"wrong vehicle, stale and future API samples rejected");
 require(!an::arcadeDrift(42),"missing companion uses physical fallback");

 auto allocation=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE));require(allocation!=nullptr,"isolated gate memory");auto page=allocation;fixtureGate=page+0x9a4;passiveSkip=reinterpret_cast<uint32_t>(page+0x9e1);
 const unsigned char gate[]={0xdf,0xe0,0xf6,0xc4,1,0x75,0x36,0xb8,1,0,0,0,0xc3};
 memcpy(page+0x9a4,gate,sizeof(gate));const unsigned char skip[]={0x33,0xc0,0xc3};memcpy(page+0x9e1,skip,sizeof(skip));FlushInstructionCache(GetCurrentProcess(),page,4096);
 alignas(16) unsigned char primary[0x180]{},vehicle[0x180]{};*reinterpret_cast<void**>(primary+0x48)=vehicle;auto driver=reinterpret_cast<uint32_t*>(vehicle+0x94);
 require(MH_Initialize()==MH_OK,"MinHook initialization");auto target=page+0x9a4;require(MH_CreateHook(target,Passive,&passiveOriginal)==MH_OK&&MH_EnableHook(target)==MH_OK,"actual gate installation");
 for(unsigned i=0;i<5000;i++){
  active=1;*driver=0;require(Invoke(primary,20)==0&&Invoke(primary,5)==0,"player passive suppression");
  for(uint32_t cls:{1u,2u,3u}){*driver=cls;require(Invoke(primary,20)==1&&Invoke(primary,5)==0,"AI original x87 threshold retained");}
  active=0;*driver=0;require(Invoke(primary,20)==1&&Invoke(primary,5)==0,"observe/fault original recovery restored");
 }
 require(MH_DisableHook(target)==MH_OK&&MH_RemoveHook(target)==MH_OK,"gate restoration");require(!memcmp(page+0x9a4,gate,sizeof(gate)),"original bytes restored");
 require(MH_CreateHook(reinterpret_cast<void*>(Fixture),Update,reinterpret_cast<void**>(&original))==MH_OK&&MH_EnableHook(reinterpret_cast<void*>(Fixture))==MH_OK,"NOS update ABI hook");
 auto fn=reinterpret_cast<NosUpdate>(Fixture);const auto value=fn(primary,0x45bb8000,.016f,0xdeadbe00);
 require(seenThis==primary&&seenRpm==0x45bb8000&&seenDt==.016f&&seenEngaged==0xdeadbe00&&std::abs(value-.141f)<1e-6,"thiscall arguments and float return retained");
 MH_DisableHook(reinterpret_cast<void*>(Fixture));MH_RemoveHook(reinterpret_cast<void*>(Fixture));MH_Uninitialize();VirtualFree(allocation,0,MEM_RELEASE);
 std::puts("PASS native x86 hooks: 5000 iterations, player-only gate, AI x87 threshold, observe/fault fallback, byte restoration, three-argument thiscall and float return");
}
