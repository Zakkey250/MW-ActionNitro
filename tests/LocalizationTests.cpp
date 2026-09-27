#include "../src/LanguageConfig.h"
#include "../src/TextHud.h"
#include "../src/DiagnosticGate.h"
#include <cstdio>
#include <cstdlib>
#include <set>
static void require(bool ok,const char* message){if(!ok){std::fprintf(stderr,"FAIL %s\n",message);std::exit(1);}}
int main(int argc,char** argv){
 require(argc==2,"atlas path");
 require(std::size(an::locales)==18,"18 language variants");
 for(auto name:{L"Chinese.bin",L"Danish.bin",L"Dutch.bin",L"English.bin",L"Finnish.bin",L"French.bin",L"German.bin",L"Italian.bin",L"Japanese.bin",L"Korean.bin",L"Mexican.bin",L"Polish.bin",L"Russian.bin",L"Spanish.bin",L"Swedish.bin"})require(an::findLocale(name)!=nullptr,"every installed game text language");
 for(auto name:{L"English US",L"English UK",L"Chinese (Traditional)",L"Chinese (Simplified)",L"Thai"})require(an::findLocale(name)!=nullptr,"regional and additional WSF languages");
 auto is=[](const an::Locale& l,const wchar_t* code){return std::wstring(l.code)==code;};
 require(is(an::resolveLocale(L"fr",L"German",L"Japanese",L"English.bin"),L"fr"),"explicit HUD language first");
 require(is(an::resolveLocale(L"auto",L"German",L"Japanese",L"English.bin"),L"de"),"WSF override precedes registry");
 require(is(an::resolveLocale(L"auto",L"",L"Japanese",L"English.bin"),L"ja"),"bridge English-slot game follows Japanese registry");
 require(is(an::resolveLocale(L"auto",L"",L"",L"LANGUAGES\\Russian.bin"),L"ru"),"native filename fallback");
 require(is(an::resolveLocale(L"auto",L"unknown",L"unknown",L""),L"en"),"unknown falls back to English");
 require(is(an::resolveLocale(L"unknown",L"Japanese",L"",L""),L"en"),"invalid explicit setting safe English");
 require(an::findLocale(L" ZH_cn ; setting comment ")==an::findLocale(L"zh-CN"),"case separators and comments");
 constexpr unsigned w=1536,h=2304;std::vector<uint32_t> atlas(w*h,0xff182027);
 std::set<std::wstring> codes;const DWORD before=GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS);
 for(size_t i=0;i<std::size(an::locales);++i){const auto& l=an::locales[i];require(codes.insert(l.code).second,"unique locale code");require(an::findLocale(l.code)==&l,"canonical locale alias");
  HDC dc=CreateCompatibleDC(nullptr);HFONT font=CreateFontW(-26,0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,ANTIALIASED_QUALITY,DEFAULT_PITCH,l.font);auto old=SelectObject(dc,font);
  auto checkGlyphs=[&](std::wstring_view t){std::vector<WORD> glyphs(t.size());require(GetGlyphIndicesW(dc,t.data(),int(t.size()),glyphs.data(),GGI_MARK_NONEXISTING_GLYPHS)!=GDI_ERROR,"glyph query");for(size_t g=0;g<t.size();++g)if(t[g]>32)require(glyphs[g]!=0xffff,"font contains all locale characters");};
  for(auto label:l.labels){require(label&&*label,"all ten labels translated");checkGlyphs(label);}
  for(auto t:{l.notice,l.installed,l.available,l.instructions,l.close,l.open}){require(t&&*t,"all update strings translated");checkGlyphs(t);}
  SelectObject(dc,old);DeleteObject(font);DeleteDC(dc);
  an::TextBitmap bitmap;bitmap.setFont(l.font);std::wstring text=std::wstring(l.code)+L"\n";
  for(auto label:l.labels)text+=std::wstring(label)+L" + 999999\n";
  const auto pixels=bitmap.render(text);require(pixels.size()==512*512,"raster produced");
  for(unsigned row=0;row<11;++row){unsigned ink=0;for(unsigned y=row*32;y<(row+1)*32;y++)for(unsigned x=0;x<512;x++){const auto a=pixels[y*512+x]>>24;if(a){++ink;require(x<510,"right edge not clipped");}}require(ink>15,"every label rasterized");}
  require(pixels==bitmap.render(text),"cached raster unchanged");
  for(unsigned y=0;y<364;y++)for(unsigned x=0;x<512;x++){const auto a=pixels[y*512+x]>>24;const auto c=unsigned(24+(231*a/255));atlas[((i/3)*384+y)*w+(i%3)*512+x]=0xff000000|c|(c<<8)|(c<<16);}
  std::printf("PASS locale=%ls font=%ls labels=10 notification=6 glyphs=complete\n",l.code,l.font);
 }
 require(GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS)<=before+2,"no GDI handles leaked");
 an::DiagnosticGate gate;unsigned writes=0;uint64_t states=0;unsigned changes=0;
 for(uint64_t t=0;t<=10000;t++){if(gate.poll((t%2)?3:11,t)){++writes;states|=gate.seen;changes+=gate.changes;gate.emitted(t);}}
 require(writes==41&&changes==10001&&(states&(1ull<<3))&&(states&(1ull<<11)),"10k flapping transitions preserve evidence in <=4 records/second");
 an::DiagnosticGate quiet;unsigned quietWrites=0;for(uint64_t t=0;t<=10000;t++)if(quiet.poll(0,t)){++quietWrites;quiet.emitted(t);}require(quietWrites==6,"steady states log once per two seconds");
 FILE* file=nullptr;fopen_s(&file,argv[1],"wb");require(file!=nullptr,"atlas output");BITMAPFILEHEADER fh{};BITMAPINFOHEADER ih{};fh.bfType=0x4d42;fh.bfOffBits=sizeof(fh)+sizeof(ih);fh.bfSize=fh.bfOffBits+w*h*4;ih.biSize=sizeof(ih);ih.biWidth=w;ih.biHeight=-int(h);ih.biPlanes=1;ih.biBitCount=32;fwrite(&fh,sizeof(fh),1,file);fwrite(&ih,sizeof(ih),1,file);fwrite(atlas.data(),4,atlas.size(),file);fclose(file);
 std::puts("PASS language coverage/selection/glyphs/raster/cache/GDI lifetime/diagnostic throttling");
}
