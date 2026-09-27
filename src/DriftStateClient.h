#pragma once
#include <windows.h>
#include <cstdint>
namespace an {
struct DriftState {uint32_t size,version,vehicle,enabled,drifting,reserved;uint64_t tick;};
static_assert(sizeof(DriftState)==32,"Drift state ABI");
inline bool validDriftState(const DriftState& s,uint32_t vehicle,uint64_t now){
 return s.size==sizeof(s)&&s.version==1&&s.vehicle==vehicle&&s.enabled==1&&s.drifting==1&&s.tick&&s.tick<=now&&now-s.tick<=150;
}
inline bool arcadeDrift(uint32_t vehicle){
 HMODULE mod=nullptr;
 if(!GetModuleHandleExW(0,L"MWArcadeDrift.asi",&mod))return false;
 auto fn=reinterpret_cast<BOOL(__cdecl*)(DriftState*)>(GetProcAddress(mod,"MWArcadeDriftGetState"));
 DriftState s{sizeof(s),1,0,0,0,0,0};bool ok=false;
 __try {ok=fn&&fn(&s)&&validDriftState(s,vehicle,GetTickCount64());}__except(EXCEPTION_EXECUTE_HANDLER){ok=false;}
 FreeLibrary(mod);return ok;
}
}
