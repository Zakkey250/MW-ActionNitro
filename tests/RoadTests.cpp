#include "../src/RoadLane.h"
#include <cstdio>
#include <cstdlib>
static void check(bool ok,const char* message){if(!ok){std::fprintf(stderr,"FAIL %s\n",message);std::exit(1);}}
static uint32_t lane(unsigned type,int width,int centre){return type|(uint32_t(width)&0x3fff)<<4|(uint32_t(centre)&0x3fff)<<18;}
static an::RoadSnapshot road(){an::RoadSnapshot r;r.scale=.01f;r.forward={0,0,1};r.offset=2;
 for(auto& p:r.ends){p.count=2;p.divider=1;p.lanes[0]=p.lanes[1]=lane(1,400,200);}return r;}
int main(){
 using S=an::LaneStatus;an::Car c;c.velocity={0,0,20};auto r=road();
 check(an::classifyLane(r,c)==S::Legal,"legal lane");r.offset=-2;check(an::classifyLane(r,c)==S::Oncoming,"opposing lane");
 r.offset=0;check(an::classifyLane(r,c)==S::Unknown,"centre divider margin");
 r.offset=-4;check(an::classifyLane(r,c)==S::Unknown,"shoulder margin");
 r.offset=-7;check(an::classifyLane(r,c)==S::Unknown,"off road");
 r.offset=-2;r.flags=1;check(an::classifyLane(r,c)==S::Unknown,"connector/intersection excluded");r.flags=0;
 c.pos.y=4;check(an::classifyLane(r,c)==S::Unknown,"other elevation excluded");c.pos={};
 c.velocity={20,0,0};check(an::classifyLane(r,c)==S::Unknown,"crossing road excluded");c.velocity={0,0,-20};
 check(an::classifyLane(r,c)==S::Legal,"reversing travel reverses lane legality");c.velocity={0,0,20};
 // Asymmetric one-way cross-section proves direction bits, not left-side heuristics.
 for(auto& p:r.ends){p.count=1;p.divider=0;p.lanes[0]=lane(1,400,0);}r.offset=0;
 check(an::classifyLane(r,c)==S::Oncoming,"wrong-way one-way road");
 r.node=1;check(an::classifyLane(r,c)==S::Legal,"legal one-way road in reverse nav orientation");
 r.flags=0x200;check(an::classifyLane(r,c)==S::Oncoming,"node 1 direction flag");
 r.node=0;r.flags=0x400;check(an::classifyLane(r,c)==S::Legal,"node 0 direction flag");
 r=road();r.ends[1].lanes[1]=lane(3,400,200);r.offset=-2;
 check(an::classifyLane(r,c)==S::Unknown,"both endpoint profiles must contain traffic lane");
 r=road();for(auto& p:r.ends)for(auto& l:p.lanes)l=lane(10,400,200);
 check(an::classifyLane(r,c)==S::Unknown,"non-traffic road excluded");
 r=road();r.offset=.1f;c.pos.x=-2.1f;check(an::classifyLane(r,c)==S::Oncoming,"current lateral position corrects cached nav");
 c.pos.x=4;check(an::classifyLane(r,c)==S::Unknown,"excessively stale lateral nav rejected");c.pos={};
 r=road();r.ends[0].count=16;check(an::classifyLane(r,c)==S::Unknown,"corrupt count bounded");
 r=road();r.scale=NAN;check(an::classifyLane(r,c)==S::Unknown,"nonfinite scale rejected");
 // Optional native-emulator fixtures: each line has profile bytes, node, flags,
 // native traffic-selected offset. No game assets are embedded in this source.
 unsigned count=0,divider=0,node=0,flags=0;float offset=0,scale=0;unsigned checks=0;
 while(std::scanf("%u %u %u %u %f %f",&count,&divider,&node,&flags,&offset,&scale)==6){
  r=road();r.node=uint8_t(node);r.flags=uint16_t(flags);r.offset=offset;r.scale=scale;
  for(auto& p:r.ends){p.count=uint8_t(count);p.divider=uint8_t(divider);}
  for(unsigned i=0;i<15;++i){unsigned packed=0;check(std::scanf("%x",&packed)==1,"fixture input");r.ends[0].lanes[i]=r.ends[1].lanes[i]=packed;}
  if(an::classifyLane(r,c)!=S::Legal)std::fprintf(stderr,"fixture=%u count=%u divider=%u node=%u flags=%x offset=%f scale=%f lane0=%08x status=%d\n",checks,count,divider,node,flags,offset,scale,r.ends[0].lanes[0],int(an::classifyLane(r,c)));
  check(an::classifyLane(r,c)==S::Legal,"native Traffic-selected centre is legal");
  c.velocity.z=-20;check(an::classifyLane(r,c)==S::Oncoming,"native lane driven backwards is oncoming");c.velocity.z=20;++checks;
 }
 std::printf("PASS road classification tests and %u native-selected lane fixtures\n",checks);
}
