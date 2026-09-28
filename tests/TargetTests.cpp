#include "../src/ExecutableHash.h"
#include "../src/LanguageConfig.h"
#include <cstdlib>
#include <string>
static void require(bool ok,const char* message){if(!ok){std::fprintf(stderr,"FAIL %s\n",message);std::exit(1);}}
int wmain(int argc,wchar_t** argv){
 for(const auto& a:an::executableTargets){
  require(an::findExecutable(a.size,a.sha256)==&a,"known exact target accepted");
  require(!an::findExecutable(a.size+1,a.sha256),"wrong size rejected");
  require(!an::findExecutable(a.size,std::string(64,'0')),"unknown hash with known size rejected");
  for(const auto& b:an::executableTargets)if(a.size!=b.size)require(!an::findExecutable(a.size,b.sha256),"crossed size/hash rejected");
 }
 require(!an::knownExecutableSize(7254894)&&!an::findExecutable(0,""),"unvalidated protected executable rejected");
 char digest[65]{};std::uint64_t bytes=0;wchar_t self[MAX_PATH]{};GetModuleFileNameW(nullptr,self,MAX_PATH);
 require(!an::hashExecutable(self,digest,bytes),"unsupported harness rejected before hashing");
 for(int i=1;i<argc;i++){
  require(an::hashExecutable(argv[i],digest,bytes),"real executable hashed by shipping helper");
  auto target=an::findExecutable(bytes,digest);require(target!=nullptr,"real executable identity accepted");
  std::printf("PASS actual target=%s bytes=%llu sha256=%s\n",target->id,bytes,digest);
  auto ini=std::filesystem::path(argv[i]).parent_path()/L"scripts"/L"NFSMWActionNitro.ini";
  const auto& language=an::loadLanguage(ini.c_str());std::printf("LANGUAGE actual target=%s selected=%ls\n",target->id,language.code);
 }
 std::puts("PASS exact host size/hash policy and production hash reader");
}
