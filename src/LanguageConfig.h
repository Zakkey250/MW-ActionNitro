#pragma once
#include <windows.h>
#include <filesystem>
#include <shlobj.h>
#include "Localization.h"
#pragma comment(lib,"advapi32.lib")
#pragma comment(lib,"shell32.lib")
namespace an {
inline std::wstring savedLanguage(const std::filesystem::path& wsf,const std::filesystem::path& game){
 if(!GetPrivateProfileIntW(L"MISC",L"WriteSettingsToFile",0,wsf.c_str()))return {};
 wchar_t directory[1024]{},language[128]{};
 GetPrivateProfileStringW(L"MISC",L"CustomUserFilesDirectoryInGameDir",L"0",directory,1024,wsf.c_str());
 std::wstring folder(directory);folder=folder.substr(0,folder.find(L';'));
 const auto first=folder.find_first_not_of(L" \t");
 folder=first==folder.npos?L"":folder.substr(first,folder.find_last_not_of(L" \t")-first+1);
 std::filesystem::path base;
 if(folder.empty()||folder==L"0"){
  wchar_t documents[MAX_PATH]{};if(FAILED(SHGetFolderPathW(nullptr,CSIDL_PERSONAL,nullptr,SHGFP_TYPE_CURRENT,documents)))return {};
  base=documents;
 }else base=game/folder;
 const auto settings=base/L"NFS Most Wanted"/L"Settings.ini";
 // Read only the language, never account/registration values in the same file.
 GetPrivateProfileStringW(L"Need for Speed Most Wanted",L"Language",L"",language,128,settings.c_str());
 return language;
}
inline const Locale& loadLanguage(const wchar_t* ini){
 wchar_t explicitLanguage[128]{},widescreen[128]{},registry[128]{},nativeFile[128]{};
 GetPrivateProfileStringW(L"HUD",L"Language",L"auto",explicitLanguage,128,ini);
 const auto wsf=std::filesystem::path(ini).parent_path()/L"NFSMostWanted.WidescreenFix.ini";
 GetPrivateProfileStringW(L"LANGUAGE",L"Language",L"",widescreen,128,wsf.c_str());
 HKEY key=nullptr;
 if(RegOpenKeyExW(HKEY_LOCAL_MACHINE,L"SOFTWARE\\EA Games\\Need for Speed Most Wanted",0,KEY_QUERY_VALUE|KEY_WOW64_32KEY,&key)==ERROR_SUCCESS){
  DWORD size=sizeof(registry),type=0;
  if(RegQueryValueExW(key,L"Language",nullptr,&type,reinterpret_cast<BYTE*>(registry),&size)!=ERROR_SUCCESS||type!=REG_SZ)registry[0]=0;
  registry[127]=0;RegCloseKey(key);
 }
 // Only called after exact supported-host identity. Bounded copies tolerate
 // language table redirects; no game strings are held across initialization.
 auto read=[](uintptr_t address,void* out,size_t size){SIZE_T got=0;return ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(address),out,size,&got)&&got==size;};
 int current=-1;if(read(0x8f41c0,&current,4)&&current>=0&&current<=20){
  for(unsigned i=0;i<10;i++){int id=-1;uint32_t name=0;
   if(!read(0x8f40f8+i*20,&id,4)||id!=current||!read(0x8f4100+i*20,&name,4)||name<0x10000)continue;
   char value[128]{};bool complete=false;for(unsigned j=0;j<127;j++){if(!read(name+j,&value[j],1))break;if(!value[j]){complete=true;break;}}
   if(complete)MultiByteToWideChar(CP_ACP,0,value,-1,nativeFile,128);break;
  }
 }
 return resolveLocale(explicitLanguage,widescreen,registry,nativeFile,savedLanguage(wsf,wsf.parent_path().parent_path()));
}
}
