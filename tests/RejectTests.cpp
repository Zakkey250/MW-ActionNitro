#include <windows.h>
#include <cstdio>
int wmain(int argc,wchar_t** argv){if(argc!=2)return 2;auto module=LoadLibraryW(argv[1]);if(!module)return 3;Sleep(500);FreeLibrary(module);std::puts("Loader completed; verify REJECTED in module log.");return 0;}
