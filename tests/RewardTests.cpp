#include "../src/Rewards.h"
#include <cstdio>
#include <cstdlib>
static void check(bool ok,const char* msg){if(!ok){std::fprintf(stderr,"FAIL %s\n",msg);std::exit(1);}}
static an::Sample sample(){an::Sample s;s.valid=s.hasNos=true;s.dt=.01f;s.player.id=1;s.player.forward={0,0,1};s.player.velocity={0,0,20};s.player.halfWidth=1;s.player.halfLength=2;s.player.road=4;return s;}
static double distanceRun(float dt,an::Action action){an::Detector d;an::Config c;auto s=sample();s.dt=dt;s.roadKnown=s.oncoming=true;double total=0;an::Car other=s.player;other.id=2;other.pos.z=15;
 d.step(s,{other},c);for(int i=0;i<int(std::round(10/dt));++i){s.player.pos.z+=20*dt;other.pos.z=s.player.pos.z+15;total+=d.step(s,{other},c).points[action];}return total;}
int main(){
 for(float dt:{1.f/30,1.f/60,1.f/144}){
  check(std::abs(distanceRun(dt,an::OncomingLane)-200)<.01,"oncoming 1m=1pt across frame rates");
  check(std::abs(distanceRun(dt,an::Slipstream)-388)<.01,"slipstream fractional metres after 0.3s qualification");
 }
 an::Detector d;an::Config c;auto s=sample();d.step(s,{},c);s.oncoming=true;s.roadKnown=false;s.player.pos.z+=.2f;check(d.step(s,{},c).points[an::OncomingLane]==0,"unknown road rejected");
 s.roadKnown=true;s.engaged=true;s.player.pos.z+=.2f;check(d.step(s,{},c).points[an::OncomingLane]==0,"no recovery during NOS");
 s.engaged=false;s.player.pos.z+=100;auto a=d.step(s,{},c);check(a.points[an::OncomingLane]==0,"teleport reset");
 s=sample();d.reset();d.step(s,{},c);s.wheels=0;double air=0,jump=0;
 for(int i=0;i<100;i++){s.player.pos.z+=.2f;air+=d.step(s,{},c).points[an::Air];}
 s.wheels=4;s.player.pos.z+=.2f;jump=d.step(s,{},c).points[an::Jump];check(std::abs(air-30)<.01&&jump==100,"air and landing counted once");
 check(d.step(s,{},c).points[an::Jump]==0,"no repeated landing");
 s=sample();d.reset();d.step(s,{},c);s.player.forward={.5f,0,.8660254f};double drift=0;
 for(int i=0;i<100;i++){s.player.pos.z+=.2f;drift+=d.step(s,{},c).points[an::Drift];}check(std::abs(drift-53.125)<.01,"drift angle weighted");
 auto nearRun=[&](bool opposite,bool hit){s=sample();s.dt=.05f;d.reset();an::Car t=s.player;t.id=2;t.traffic=true;t.pos={2.7f,0,12};t.velocity={0,0,opposite?-20.f:10.f};double normal=0,oncoming=0;
  d.step(s,{t},c);for(int i=0;i<80;i++){s.player.pos.z+=s.player.velocity.z*s.dt;t.pos.z+=t.velocity.z*s.dt;s.player.collision=hit;auto r=d.step(s,{t},c);normal+=r.points[an::NearMiss];oncoming+=r.points[an::OncomingMiss];}
  return opposite?oncoming:normal;};
 check(nearRun(false,false)==100,"near miss once");check(nearRun(true,false)==200,"opposite near miss");check(nearRun(true,true)==0,"collision excluded");
 an::Awards full;full.points[an::NearMiss]=100;full.points[an::Jump]=100;auto limited=an::limitToTank(full,.3,c);check(std::abs(limited.points[an::NearMiss]-50)<.001,"capacity clamps actual reported points");
 check(an::limitToTank(full,0,c).points[an::NearMiss]==0,"full tank no reward");
 s=sample();d.reset();an::Car leader=s.player;leader.id=2;leader.road=5;leader.pos.z=15;d.step(s,{leader},c);double slip=0;
 for(int i=0;i<100;i++){s.player.pos.z+=.2f;leader.pos.z+=.2f;slip+=d.step(s,{leader},c).points[an::Slipstream];}check(slip==0,"adjacent road slipstream rejected");
 s=sample();s.hasNos=false;s.roadKnown=s.oncoming=true;d.reset();check(d.step(s,{},c).points[an::OncomingLane]==0,"no NOS equipment");
 s=sample();d.reset();d.step(s,{},c);s.wheels=0;air=0;for(int i=0;i<1000;i++){s.player.pos.z+=.2f;air+=d.step(s,{},c).points[an::Air];}check(std::abs(air-190)<.01,"airtime capped against endless fall");
 // Top-speed conversion is tank-relative; coefficient below is a test fixture, NOT HP data.
 c.topTankPerSecond=.02;s=sample();s.topSpeed=100;s.nosCapacity=10;s.player.velocity.z=75;d.reset();d.step(s,{},c);
 auto top=d.step(s,{},c);check(std::abs(top.points[an::TopSpeed]*c.secondsPerPoint/10-.0002)<1e-8,"75 percent inclusive and tank-normalized rate");
 s.player.velocity.z=74.99f;check(d.step(s,{},c).points[an::TopSpeed]==0,"below 75 percent excluded");
 s.player.velocity.z=100;s.topSpeed=0;check(d.step(s,{},c).points[an::TopSpeed]==0,"missing performance estimate excluded");
 s.topSpeed=100;s.engaged=true;check(d.step(s,{},c).points[an::TopSpeed]==0,"top speed disabled during NOS");
 c.topTankPerSecond=0;s.engaged=false;check(d.step(s,{},c).points[an::TopSpeed]==0,"unverified HP rate produces no award");
 s=sample();d.reset();d.step(s,{},c);s.arcadeDrift=true;drift=0;
 for(int i=0;i<100;i++)drift+=d.step(s,{},c).points[an::Drift];
 check(std::abs(drift-50)<.01,"optional drift state supports continuous reward");
 check(drift>0,"linked drift has no second qualification delay");
 s.arcadeDrift=false;double bridge=0;for(int i=0;i<20;i++)bridge+=d.step(s,{},c).points[an::Drift];
 check(std::abs(bridge-10)<.01,"brief physical drift gap bridged");
 for(int i=0;i<40;i++)d.step(s,{},c);check(d.step(s,{},c).points[an::Drift]==0,"straight driving ends grace");
 s.arcadeDrift=true;s.player.collision=true;check(d.step(s,{},c).points[an::Drift]==0,"collision overrides optional drift state");
 s=sample();d.reset();d.step(s,{},c);s.player.velocity.x=2;drift=0;
 for(int i=0;i<100;i++)drift+=d.step(s,{},c).points[an::Drift];check(drift>40,"physical lateral slide without companion MOD");
 s=sample();s.dt=.05f;d.reset();an::Car t=s.player;t.traffic=true;t.velocity.z=-20;t.pos={2.7f,0,12};t.id=2;
 d.step(s,{t},c);double chain=0;
 for(unsigned pass=0;pass<3;pass++){
  t.id=2+pass;t.pos.z=s.player.pos.z+12;
  for(int j=0;j<30;j++){s.player.pos.z+=1;t.pos.z-=1;auto r=d.step(s,{t},c);chain+=r.points[an::NearChain];}
 }
 check(chain==150,"consecutive confirmed near misses award 0,50,100");
 // Three-second bonus is fixed, inclusive and independent of the continuous multiplier.
 c=an::Config{};
 for(float dt:{1.f/30,1.f/60,1.f/120,1.f/144}){
  s=sample();s.dt=dt;s.arcadeDrift=true;d.reset();d.step(s,{},c);double bonus=0;
  for(int i=0;i<int(std::round(6/dt));++i)bonus+=d.step(s,{},c).points[an::DriftBonus];
  check(bonus==200,"drift 3 and 6 seconds fixed 100 bonus across FPS");
  s.player.collision=true;check(d.step(s,{},c).points[an::DriftBonus]==0,"collision cancels bonus chain");
 }
 // Regression: a real 2.032m wide car passing at 3m centre distance must fit near-gap.
 auto widthRun=[&](bool oldHalf){s=sample();s.dt=.05f;s.player.halfWidth=oldHalf?.508f:1.016f;s.player.halfLength=oldHalf?1.15f:2.3f;d.reset();an::Car t=s.player;t.id=2;t.traffic=true;t.pos={3,0,12};t.velocity.z=-20;double score=0;d.step(s,{t},c);
  for(int i=0;i<40;i++){s.player.pos.z+=1;t.pos.z-=1;score+=d.step(s,{t},c).points[an::OncomingMiss];}return score;};
 check(widthRun(true)==0&&widthRun(false)==200,"native half-extents must not be halved twice");
 s=sample();s.topSpeed=100;s.player.velocity.z=75;s.nosCapacity=5.5f;d.reset();d.step(s,{},c);double tank=0;
 for(int i=0;i<100;i++)tank+=d.step(s,{},c).points[an::TopSpeed]*c.secondsPerPoint/s.nosCapacity;
 check(std::abs(tank-.04)<1e-7,"stock HP cop high speed translated to 4 percent tank per second");
 // A 30ms provider-confirmed drift used to be entirely discarded by the 150ms dwell.
 s=sample();s.arcadeDrift=true;d.reset();d.step(s,{},c);drift=0;
 for(int i=0;i<3;i++)drift+=d.step(s,{},c).points[an::Drift];
 check(std::abs(drift-1.5)<.001,"short confirmed companion event earns immediately");
 s.engaged=true;check(d.step(s,{},c).points[an::Drift]==0,"NOS cancels linked qualification");
 s.engaged=false;s.arcadeDrift=false;s.player.forward={.2f,0,.9797959f};
 check(d.step(s,{},c).points[an::Drift]==0,"reset restores physical-only dwell");
 std::puts("PASS reward tests: distance/FPS/fractional, unknown road, NOS active, teleport, air, jump, drift, near miss, collision, tank cap");
}
