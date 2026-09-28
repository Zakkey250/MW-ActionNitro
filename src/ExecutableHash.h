#pragma once
#include <windows.h>
#include <bcrypt.h>
#include <cstdio>
#include "ExecutableTargets.h"
namespace an {
inline bool hashExecutable(const wchar_t* path,char (&out)[65],std::uint64_t& fileSize){
 out[0]=0;fileSize=0;
 HANDLE f=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,0,nullptr);if(f==INVALID_HANDLE_VALUE)return false;
 LARGE_INTEGER size{};if(!GetFileSizeEx(f,&size)||size.QuadPart<0||!knownExecutableSize(std::uint64_t(size.QuadPart))){CloseHandle(f);return false;}
 fileSize=std::uint64_t(size.QuadPart);
 BCRYPT_ALG_HANDLE alg=nullptr;BCRYPT_HASH_HANDLE h=nullptr;bool ok=BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;
 if(ok)ok=BCryptCreateHash(alg,&h,nullptr,0,nullptr,0,0)>=0;BYTE buf[32768],digest[32];DWORD n=0;
 while(ok){if(!ReadFile(f,buf,sizeof(buf),&n,nullptr)){ok=false;break;}if(!n)break;ok=BCryptHashData(h,buf,n,0)>=0;}
 if(ok)ok=BCryptFinishHash(h,digest,32,0)>=0;if(h)BCryptDestroyHash(h);if(alg)BCryptCloseAlgorithmProvider(alg,0);CloseHandle(f);
 if(ok)for(size_t i=0;i<32;i++)sprintf_s(out+i*2,65-i*2,"%02x",digest[i]);return ok;
}
}
