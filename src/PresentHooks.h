#pragma once
#include <d3d9.h>
#include <MinHook.h>
namespace mcd {
// Only D3D implementation entry points. Never patch the game's render code:
// other ASIs discover it by signature during their startup.
class PresentHooks {
 using Present=HRESULT (WINAPI*)(IDirect3DDevice9*,const RECT*,const RECT*,HWND,const RGNDATA*);
 struct Slot {void* target=nullptr;Present original=nullptr;bool enabled=false;};
 inline static Slot slots[4]{};
 inline static void (*callback)(IDirect3DDevice9*)=nullptr;
 inline static thread_local bool inside=false;
 template<unsigned N> static HRESULT WINAPI dispatch(IDirect3DDevice9* d,const RECT* src,const RECT* dst,HWND window,const RGNDATA* dirty){
  const bool outer=!inside;inside=true;
  if(outer&&callback)callback(d);
  const auto hr=slots[N].original(d,src,dst,window,dirty);
  if(outer)inside=false;
  return hr;
 }
public:
 static MH_STATUS install(void* target,void (*draw)(IDirect3DDevice9*)){
  if(!callback)callback=draw;
  else if(callback!=draw)return MH_ERROR_ALREADY_CREATED;
  const Present hooks[]={dispatch<0>,dispatch<1>,dispatch<2>,dispatch<3>};
  for(auto& s:slots)if(s.target==target)return s.enabled?MH_OK:MH_ERROR_DISABLED;
  for(unsigned n=0;n<4;n++)if(!slots[n].target){
   auto& s=slots[n];
   const auto created=MH_CreateHook(target,reinterpret_cast<void*>(hooks[n]),reinterpret_cast<void**>(&s.original));
   if(created!=MH_OK)return created;
   s.target=target;const auto enabled=MH_EnableHook(target);s.enabled=enabled==MH_OK;return enabled;
  }
  return MH_ERROR_MEMORY_ALLOC;
 }
};
}
