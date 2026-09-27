#pragma once
#include <windows.h>
#include <cstdint>
// Optional, read-only inter-MOD ABI. No controller/settings writes exposed.
struct MWArcadeDriftStateV1 {
 uint32_t size,version,vehicle,enabled,drifting,reserved;
 uint64_t tick;
};
static_assert(sizeof(MWArcadeDriftStateV1)==32,"Drift API ABI");
namespace mcdapi {
inline SRWLOCK lock=SRWLOCK_INIT;
inline MWArcadeDriftStateV1 current{sizeof(MWArcadeDriftStateV1),1,0,0,0,0,0};
inline void publish(uint32_t vehicle,bool enabled,bool drifting,uint64_t tick){
 AcquireSRWLockExclusive(&lock);current={sizeof(current),1,vehicle,enabled?1u:0u,drifting?1u:0u,0,tick};ReleaseSRWLockExclusive(&lock);
}
inline bool copy(MWArcadeDriftStateV1* out){
 if(!out||out->size!=sizeof(*out)||out->version!=1)return false;
 AcquireSRWLockShared(&lock);auto value=current;ReleaseSRWLockShared(&lock);
 if(!value.tick||GetTickCount64()-value.tick>150)return false;
 *out=value;return true;
}
}
