#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace an {
enum Action { NearMiss, OncomingLane, OncomingMiss, Jump, Air, Drift, Slipstream, NearChain, TopSpeed, DriftBonus, Count };
inline const char* names[Count]={"near_miss","oncoming_lane","oncoming_miss","jump","air","drift","slipstream","near_chain","top_speed","drift_bonus"};
inline const wchar_t* labels[Count]={L"ニアミス",L"対向車線",L"対向車ニアミス",L"ジャンプ",L"滞空",L"ドリフト",L"スリップストリーム",L"連続ニアミス",L"トップスピード",L"ドリフト継続"};
struct Vec {float x=0,y=0,z=0;};
inline Vec operator-(Vec a,Vec b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline Vec operator+(Vec a,Vec b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline Vec operator*(Vec a,float b){return {a.x*b,a.y*b,a.z*b};}
inline float dot(Vec a,Vec b){return a.x*b.x+a.z*b.z;}
inline float length(Vec a){return std::sqrt(dot(a,a));}
inline Vec unit(Vec a){float n=length(a);return n>.001f?a*(1/n):Vec{};}
inline bool finite(Vec v){return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);}
struct Car {
 uint64_t id=0; Vec pos,velocity,forward; float halfWidth=0,halfLength=0;
 int road=-1; bool collision=false,traffic=false;
};
struct Sample {
 Car player; float dt=0,up=1,topSpeed=0,nosCapacity=0; unsigned wheels=4;
 bool valid=false,engaged=false,hasNos=false,roadKnown=false,oncoming=false,arcadeDrift=false;
};
struct Config {
 double nearPoints=100,oppositePoints=200,jumpPoints=100,airPerSecond=40,driftPerSecond=50;
 double oncomingPerMetre=1,slipPerMetre=2,secondsPerPoint=.003;
 double chainStep=50,chainMaximum=200;
 // Stock HPR cop HighSpeed.NitrousReward=4 on its 0..100 tank scale.
 double topTankPerSecond=.04,driftBonus=100;
 static constexpr float topSpeedFraction=.75f;
 float minSpeed=12,nearGap=1.5f,slipMax=35,driftAngle=.08f,driftExitAngle=.04f;
 float lateralEntry=1.5f,lateralExit=.7f,driftHold=.15f,driftGrace=.25f;
};
struct Awards {std::array<double,Count> points{};};
struct NearDiagnostics {unsigned crossings=0,queued=0,awarded=0,gapRejected=0,collisionRejected=0;};
class Detector {
 struct Track {Car car; double seen=0,lastAward=-100,pendingAt=0;bool pending=false,opposite=false;};
 std::unordered_map<uint64_t,Track> tracks;
 Car previous{}; bool havePrevious=false,wasGrounded=false,flightValid=false;
 double clock=0,airTime=0,driftTime=0,driftGap=0,slipTime=0,collisionUntil=0,lastNear=-100;
 unsigned nearChain=0;bool driftConfirmed=false;
 uint64_t slipTarget=0;
public:
 NearDiagnostics diagnostics{};
 void reset(){*this=Detector{};}
 Awards step(const Sample& s,const std::vector<Car>& cars,const Config& c){
  Awards out;
  if(!s.valid||!s.hasNos||s.dt<=0||s.dt>.1f||!std::isfinite(s.dt)||!finite(s.player.pos)||!finite(s.player.velocity)) {reset();return out;}
  const auto& p=s.player;const float speed=length(p.velocity);clock+=s.dt;
  if(!havePrevious||previous.id!=p.id){previous=p;havePrevious=true;wasGrounded=s.wheels>=3;return out;}
  const float moved=length(p.pos-previous.pos);
  if(moved>std::max(4.f,speed*s.dt*2+1)||speed>160||std::abs(p.pos.y-previous.pos.y)>10){reset();return out;}
  if(p.collision){collisionUntil=clock+.4;nearChain=0;lastNear=-100;for(auto& t:tracks)t.second.pending=false;}
  const bool clean=clock>=collisionUntil&&s.up>.75f;
  const bool eligible=clean&&!s.engaged&&speed>=c.minSpeed;
  // Arc length from simulation speed and dt preserves fractional metres and is
  // independent of display FPS. Never include a teleport/pause catch-up interval.
  const double metres=.5*(speed+length(previous.velocity))*s.dt;
  if(eligible&&s.wheels>=3&&std::isfinite(s.topSpeed)&&s.topSpeed>=20&&s.topSpeed<=160&&
     speed>=s.topSpeed*c.topSpeedFraction&&std::isfinite(s.nosCapacity)&&s.nosCapacity>0)
   out.points[TopSpeed]=c.topTankPerSecond*s.dt*s.nosCapacity/c.secondsPerPoint;
  if(eligible&&s.wheels>=3&&s.roadKnown&&s.oncoming)out.points[OncomingLane]=metres*c.oncomingPerMetre;
  if(s.wheels==0){
   if(wasGrounded){airTime=0;flightValid=clean&&speed>=c.minSpeed;}
   airTime+=s.dt; if(!clean)flightValid=false;
   if(eligible&&flightValid){const double before=airTime-s.dt;out.points[Air]=std::clamp(airTime-.25,0.,4.75)-std::clamp(before-.25,0.,4.75);out.points[Air]*=c.airPerSecond;}
  } else if(s.wheels>=3){
   if(eligible&&flightValid&&airTime>=.3&&airTime<=5)out.points[Jump]=c.jumpPoints;
   airTime=0;flightValid=false;
  }
  if(s.wheels==0||s.wheels>=3)wasGrounded=s.wheels>=3;
  const auto f=unit(p.forward),v=unit(p.velocity);
  const float beta=std::acos(std::clamp(dot(f,v),-1.f,1.f));
  const float slideSpeed=std::abs(p.velocity.x*f.z-p.velocity.z*f.x);
  const bool drifting=driftTime>0;
  const bool sliding=beta>=(drifting?c.driftExitAngle:c.driftAngle)&&slideSpeed>=(drifting?c.lateralExit:c.lateralEntry);
  const bool safeDrift=eligible&&s.wheels>=2&&beta<1.05f;
  if(safeDrift&&(sliding||s.arcadeDrift)){driftGap=0;}
  else driftGap+=s.dt;
  if(safeDrift&&(sliding||s.arcadeDrift||(drifting&&driftGap<=c.driftGrace))){
   const double before=driftTime;driftTime+=s.dt;
   // The companion already confirmed its drift. A second dwell used to discard
   // short ArcadeDrift events; keep qualification through the physical bridge.
   if(s.arcadeDrift)driftConfirmed=true;
   const double rewardTime=driftConfirmed?s.dt:std::max(0.,driftTime-c.driftHold)-std::max(0.,before-c.driftHold);
   const double speedScale=std::clamp(double(speed)/27.7777778,1.,2.);
   const double angleScale=std::clamp(double(beta)/.34906585,1.,2.);
   out.points[Drift]=rewardTime*c.driftPerSecond*(speedScale+angleScale)*.5;
   out.points[DriftBonus]=(std::floor((driftTime+1e-5)/3)-std::floor((before+1e-5)/3))*c.driftBonus;
  }else {driftTime=0;driftGap=0;driftConfirmed=false;}
  uint64_t best=0;float bestDistance=c.slipMax;
  for(const auto& other:cars){
   if(!other.id||other.id==p.id||!finite(other.pos)||!finite(other.velocity)||other.halfWidth<=0||other.halfLength<=0)continue;
   const Vec rel=other.pos-p.pos;const float ahead=dot(rel,f),lateral=std::abs(rel.x*f.z-rel.z*f.x);
   const float align=dot(f,unit(other.velocity));
   // Same fresh native road segment is mandatory: fail closed across overpasses,
   // divided roads or unavailable navigation. Curve/adjacent-segment support later.
   const bool sameRoad=p.road>=0&&p.road==other.road;
   if(eligible&&s.wheels>=3&&sameRoad&&!other.collision&&length(other.velocity)>c.minSpeed&&align>.95f&&
      std::abs(rel.y)<1.5f&&ahead>p.halfLength+other.halfLength+2&&ahead<bestDistance&&lateral<1.3f){best=other.id;bestDistance=ahead;}
   auto found=tracks.find(other.id);
   if(found!=tracks.end()){
    auto& t=found->second;
    if(other.collision||!eligible){if(t.pending)++diagnostics.collisionRejected;t.pending=false;}
    if(t.pending&&clock-t.pendingAt>=.25&&length(rel)>p.halfLength+other.halfLength+3){
     if(eligible&&!other.collision){out.points[t.opposite?OncomingMiss:NearMiss]+=t.opposite?c.oppositePoints:c.nearPoints;t.lastAward=clock;++diagnostics.awarded;
      nearChain=clock-lastNear<=5?std::min(nearChain+1,100u):1;lastNear=clock;
      out.points[NearChain]+=std::min(c.chainMaximum,c.chainStep*(nearChain-1));
     }
     t.pending=false;
    }
    const Vec oldRel=t.car.pos-previous.pos;
    // Crossing the travel plane avoids losing passes while the car is yawed in a drift.
    const auto travel=unit(p.velocity);const float passAhead=dot(rel,travel);
    const float oldAhead=dot(oldRel,travel);
    const float passAlign=dot(travel,length(other.velocity)>1?unit(other.velocity):unit(other.forward));
    if(other.traffic&&eligible&&s.wheels>=3&&!other.collision&&!t.pending&&clock-t.lastAward>5&&clock-t.seen<.15&&
       oldAhead>0&&passAhead<=0&&std::abs(passAlign)>.75f&&length(p.velocity-other.velocity)>5){
     ++diagnostics.crossings;
     const float a=oldAhead/(oldAhead-passAhead);const Vec cross=oldRel+(rel-oldRel)*a;
     const float side=std::abs(cross.x*travel.z-cross.z*travel.x);
     auto projectedWidth=[&](const Car& car){auto heading=unit(car.forward);return car.halfWidth*std::abs(dot(heading,travel))+car.halfLength*std::abs(heading.x*travel.z-heading.z*travel.x);};
     const float gap=side-projectedWidth(p)-projectedWidth(other);
     if(std::abs(cross.y)<1.5f&&gap>.1f&&gap<c.nearGap){t.pending=true;t.pendingAt=clock;t.opposite=passAlign<-.75f;++diagnostics.queued;}
     else ++diagnostics.gapRejected;
    }
    t.car=other;t.seen=clock;
   }else if(tracks.size()<256)tracks.emplace(other.id,Track{other,clock});
  }
  if(best){if(best!=slipTarget){slipTime=0;slipTarget=best;}const double before=slipTime;slipTime+=s.dt;
   const double qualified=std::max(0.,slipTime-.3)-std::max(0.,before-.3);
   out.points[Slipstream]=metres*c.slipPerMetre*qualified/s.dt;
  }else {slipTime=0;slipTarget=0;}
  for(auto it=tracks.begin();it!=tracks.end();)if(clock-it->second.seen>1)it=tracks.erase(it);else ++it;
  previous=p;return out;
 }
};
inline Awards limitToTank(Awards a,double remainingSeconds,const Config& c){
 double sum=0;for(double x:a.points)sum+=x;
 const double ratio=sum>0?std::clamp(remainingSeconds/(sum*c.secondsPerPoint),0.,1.):0;
 for(double& x:a.points)x*=ratio;return a;
}
}
