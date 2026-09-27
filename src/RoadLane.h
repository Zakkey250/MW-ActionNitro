#pragma once
#include "Rewards.h"

namespace an {
// RNpf, not RNrd: 64-byte cross-section profiles used by WRoadNav.
struct RoadProfile {
 uint8_t count=0,divider=0;uint16_t padding=0;uint32_t lanes[15]{};
};
static_assert(sizeof(RoadProfile)==64,"native RNpf layout");
enum class LaneStatus { Unknown, Legal, Oncoming };
struct RoadSnapshot {
 RoadProfile ends[2];Vec position,forward;
 float offset=0,scale=0;uint16_t flags=0;uint8_t node=0;
};
inline int signed14(uint32_t x){x&=0x3fff;return (x&0x2000)?int(x)-0x4000:int(x);}
inline LaneStatus profileLane(const RoadProfile& p,bool legalSuffix,float offset,float scale,bool reverse){
 if(!p.count||p.count>15||p.divider>p.count)return LaneStatus::Unknown;
 LaneStatus result=LaneStatus::Unknown;
 for(unsigned i=0;i<p.count;++i){
  const uint32_t packed=p.lanes[i];if((packed&15)!=1)continue; // Native Traffic mask = 1 << 1.
  const bool legal=(i>=p.divider)==legalSuffix;
  const float centre=float(signed14(packed>>18))*scale*(legal?1.f:-1.f);
  const float width=float(signed14(packed>>4))*scale;
  if(width<0||std::abs(centre)>100)return LaneStatus::Unknown;
  if(width<1||width>12)continue; // Retail profiles contain 0.1m lane markers too.
  // Small inset excludes lane edges/medians; vehicle centre must be in a traffic lane.
  if(std::abs(offset-centre)<=width*.5f-.2f){
   const auto candidate=(legal!=reverse)?LaneStatus::Legal:LaneStatus::Oncoming;
   if(result!=LaneStatus::Unknown&&result!=candidate)return LaneStatus::Unknown;
   result=candidate;
  }
 }
 return result;
}
inline LaneStatus classifyLane(const RoadSnapshot& r,const Car& car){
 if(r.node>1||(r.flags&1)||!finite(r.position)||!finite(r.forward)||!finite(car.pos)||!finite(car.velocity)||
    !std::isfinite(r.offset)||std::abs(r.offset)>100||!std::isfinite(r.scale)||r.scale<.001f||r.scale>.1f||
    length(r.forward)<.01f||length(car.velocity)<1)return LaneStatus::Unknown;
 const Vec delta=car.pos-r.position,f=unit(r.forward);
 if(length(delta)>15||std::abs(delta.y)>3)return LaneStatus::Unknown;
 const float alignment=dot(f,unit(car.velocity));if(std::abs(alignment)<.8f)return LaneStatus::Unknown;
 // Native nav may be cached. Project current position onto its right vector,
 // matching UpdateRoadNav (44261F..442678), instead of trusting stale laneIndex.
 const float lateral=delta.x*f.z-delta.z*f.x;
 if(std::abs(lateral)>3)return LaneStatus::Unknown;
 const float offset=r.offset+lateral;
 const bool flip=((r.flags>>(r.node?9:10))&1)!=0;
 const bool legalSuffix=(r.node!=0)!=flip;
 // Both endpoint profiles must agree; taper/merge ambiguity fails closed.
 const auto a=profileLane(r.ends[0],legalSuffix,offset,r.scale,alignment<0);
 const auto b=profileLane(r.ends[1],legalSuffix,offset,r.scale,alignment<0);
 return a==b?a:LaneStatus::Unknown;
}
}
