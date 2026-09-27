#pragma once
#include <cstdint>
namespace an {
// Bound disk writes even when collision/full-tank flags flap every physics step.
// Preserve evidence of brief transitions using a state bitmap and change count.
struct DiagnosticGate {
 int previous=-1;std::uint64_t next=0,seen=0;unsigned changes=0;
 bool poll(int state,std::uint64_t now){
  if(state!=previous){++changes;previous=state;}if(state>=0&&state<64)seen|=std::uint64_t(1)<<state;
  return now>=next&&(changes||now-next>=1750);
 }
 void emitted(std::uint64_t now){next=now+250;seen=0;changes=0;}
};
}
