// Isolated x86 harness: no game process or installed files are accessed.
#include <Windows.h>
#include <shellapi.h>
#include <atomic>
#include <string>

static std::atomic<unsigned> browserCalls{0};
static std::wstring browserUrl;
static HINSTANCE WINAPI TestShellExecuteW(HWND,LPCWSTR operation,LPCWSTR url,LPCWSTR parameters,LPCWSTR,int) {
    if(operation&&std::wstring(operation)==L"open"&&url&&!parameters){browserUrl=url;++browserCalls;}
    return reinterpret_cast<HINSTANCE>(33);
}
// Exercise the actual button dispatch without launching the user's browser.
#define ShellExecuteW TestShellExecuteW
#include "../src/UpdateNotice.cpp"
#undef ShellExecuteW
#include <iostream>
#include <thread>
#include <chrono>
#include <fstream>
namespace an {
bool StartupNoticeAllowed() noexcept {return true;}
void UpdateLog(const char*) noexcept {}
}
int main(int argc,char** argv) {
    using namespace mod_update;
    unsigned passed=0,failed=0;
    auto expect=[&](bool ok,const char* text){std::cout<<(ok?"PASS ":"FAIL ")<<text<<'\n';ok?++passed:++failed;};
    using nlohmann::json;
    constexpr auto repo="Zakkey250/MW-ActionNitro";
    constexpr auto prefix="MW-ActionNitro-";
    auto row=[&](std::string tag,bool pre=true){return json{{"tag_name",tag},{"draft",false},{"prerelease",pre},
        {"published_at","2026-09-17T00:00:00Z"},{"html_url",std::string("https://github.com/")+repo+"/releases/tag/"+tag},
        {"assets",json::array({json{{"name",std::string(prefix)+tag+".zip"},{"state","uploaded"},{"size",12345}}})}};};
    auto find=[&](json rows,std::string_view current="0.1.0-alpha.59"){return FindUpdate(rows.dump(),current,repo,prefix);};
    for(auto version:{"v0.1.0-alpha.59","0.1.0-beta.1","1.0.0-rc.12","v1.2.3"})expect(ParseVersion(version).has_value(),"strict version accepts supported formats");
    for(auto version:{"v01.0.0","1.0","1.0.0-alpha.01","v1.0.0-alpha.1junk","1.0.0+build","1.0.0-dev","NFSU2EncounterAudioExtractor-v99.0.0","1.0.9999999999999999"})expect(!ParseVersion(version),"malformed/foreign version rejected");
    expect(ParseVersion("0.1.0-alpha.100")->Key()>ParseVersion("0.1.0-alpha.99")->Key(),"numeric not lexical ordering");
    expect(!find(json::array({row("v0.1.0-alpha.57")})),"older public alpha release cannot notify local development build");
    expect(!find(json::array({row("v0.1.0-alpha.59")})),"same version not an update");
    auto latest=find(json::array({row("v0.1.0-alpha.61"),row("v0.1.0-alpha.60")}));
    expect(latest&&latest->tag=="v0.1.0-alpha.61","unordered list selects highest valid newer version");
    expect(find(json::array({row("v0.1.0",false)})).has_value(),"alpha can graduate to stable");
    expect(!find(json::array({row("v2.0.0-alpha.1")}),"1.0.0"),"stable install does not advertise prerelease");
    expect(find(json::array({row("v2.0.0",false)}),"1.0.0").has_value(),"stable install accepts newer stable");
    for(auto field:{"draft","prerelease","published_at","html_url","tag_name","assets"}) {
        auto bad=row("v0.1.0-alpha.60");bad.erase(field);expect(!find(json::array({bad})),"missing required release field rejected");
    }
    auto bad=row("v0.1.0-alpha.60");bad["draft"]=true;expect(!find(json::array({bad})),"draft rejected");
    bad=row("v0.1.0-alpha.60");bad["draft"]="false";expect(!find(json::array({bad})),"string false not accepted as bool");
    bad=row("v0.1.0-alpha.60");bad["prerelease"]=false;expect(!find(json::array({bad})),"inconsistent prerelease flag rejected");
    bad=row("v0.1.0-alpha.60");bad["html_url"]="https://evil.example/releases";expect(!find(json::array({bad})),"foreign release page rejected");
    for(auto name:{"MW-ActionNitro-v0.1.0-alpha.60-source.zip","NFSU2EncounterAudioExtractor-v9.0.0.zip","MW-ActionNitro-v0.1.0-alpha.60.zip.exe"}) {
        bad=row("v0.1.0-alpha.60");bad["assets"][0]["name"]=name;expect(!find(json::array({bad})),"extractor/source/misnamed asset rejected");
    }
    bad=row("v0.1.0-alpha.60");bad["assets"][0]["state"]="new";expect(!find(json::array({bad})),"unfinished asset rejected");
    bad=row("v0.1.0-alpha.60");bad["assets"][0]["size"]=0;expect(!find(json::array({bad})),"empty asset rejected");
    bad=row("v0.1.0-alpha.60");bad["assets"][0]["size"]=-1;expect(!find(json::array({bad})),"negative asset size rejected");
    expect(!FindUpdate("[{\"draft\":true,\"draft\":false}]","0.0.0",repo,prefix),"duplicate JSON keys rejected");
    expect(!FindUpdate("<html>error</html>","0.0.0",repo,prefix)&&!FindUpdate("[{","0.0.0",repo,prefix),"HTML and truncated JSON rejected");
    expect(!FindUpdate(std::string(40,'[')+std::string(40,']'),"0.0.0",repo,prefix),"deep JSON rejected");
    expect(!FindUpdate(std::string(2*1024*1024+1,' '),"0.0.0",repo,prefix),"oversized response rejected");
    for(auto language:{L"ja",L"JP",L"Japanese",L" ja-JP ; note"})expect(Japanese(language),"Japanese language selected");
    for(auto language:{L"",L"en",L"French",L"German",L"Chinese",L"unknown"})expect(!Japanese(language),"non-Japanese and unknown select English");
    expect(QueueName(1)!=QueueName(2),"dialog queues isolated by process");
    std::atomic<unsigned> concurrent=0,maximum=0;
    auto chained=[&]{KernelHandle mutex;mutex.value=CreateMutexW(nullptr,FALSE,QueueName(GetCurrentProcessId()).c_str());
        if(WaitForSingleObject(mutex.value,3000)!=WAIT_OBJECT_0)return;
        auto active=++concurrent;unsigned old=maximum.load();while(old<active&&!maximum.compare_exchange_weak(old,active)){}
        std::this_thread::sleep_for(std::chrono::milliseconds(50));--concurrent;ReleaseMutex(mutex.value);};
    std::thread a(chained),b(chained);a.join();b.join();expect(maximum==1,"two independent named-mutex clients serialize");
    KernelHandle ready,releaseSignal;ready.value=CreateEventW(nullptr,TRUE,FALSE,nullptr);releaseSignal.value=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    std::thread busy([&]{KernelHandle mutex;mutex.value=CreateMutexW(nullptr,FALSE,QueueName(GetCurrentProcessId()).c_str());
        WaitForSingleObject(mutex.value,1000);SetEvent(ready.value);WaitForSingleObject(releaseSignal.value,2000);ReleaseMutex(mutex.value);});
    WaitForSingleObject(ready.value,1000);
    expect(!ShowSerializedNotice(GetModuleHandleW(nullptr),L"Queue test",L"",L"OK",nullptr,100,100),"busy chain times out without opening competing dialog");
    SetEvent(releaseSignal.value);busy.join();
    if(argc>1&&std::string_view(argv[1])=="--live") {
        std::string payload;expect(an::FetchReleases(payload),"read-only real GitHub HTTPS request succeeds");
        auto release=FindUpdate(payload,"0.0.0-alpha.0",repo,prefix);expect(release.has_value(),"live repository has eligible ActionNitro binary release");
        if(release)std::cout<<"LIVE latest="<<release->tag<<'\n';
        expect(!FindUpdate(payload,an::version,repo,prefix),"current local build produces no false notice against live releases");
    }
    if(argc>1&&std::string_view(argv[1])=="--dialog") {
        for(const auto& locale:an::locales) {
            const auto titleText=std::wstring(L"ActionNitro notice fixture — ")+locale.code;
            const wchar_t* title=titleText.c_str();
            const wchar_t* label=locale.open;
            const auto body=an::updateBody(locale,L"0.1.0-alpha.6",L"v0.1.0-alpha.7");
            browserCalls=0;browserUrl.clear();
            std::atomic<bool> labelled=false,layout=false;
            std::thread click([&]{
                const auto deadline=GetTickCount64()+2000;
                while(GetTickCount64()<deadline){
                    HWND window=FindWindowW(L"#32770",title);DWORD pid=0;
                    if(window)GetWindowThreadProcessId(window,&pid);
                    if(pid==GetCurrentProcessId()&&GetWindowLongPtrW(window,DWLP_USER)){
                        wchar_t value[64]{};GetDlgItemTextW(window,102,value,64);
                        if(std::wstring(value)==label){
                            labelled=true;
                            layout=true;
                            for(int id:{101,102,IDOK}){
                                HWND control=GetDlgItem(window,id);HDC dc=GetDC(control);
                                auto font=reinterpret_cast<HFONT>(SendMessageW(control,WM_GETFONT,0,0));auto old=SelectObject(dc,font);
                                RECT actual{};GetClientRect(control,&actual);RECT measured=actual;
                                const wchar_t* text=id==101?body.c_str():(id==102?locale.open:locale.close);
                                DrawTextW(dc,text,-1,&measured,DT_CALCRECT|DT_NOPREFIX|(id==101?DT_WORDBREAK:DT_SINGLELINE));
                                if(measured.right>actual.right||measured.bottom>actual.bottom)layout=false;
                                SelectObject(dc,old);ReleaseDC(control,dc);
                            }
                            SendMessageW(window,WM_COMMAND,MAKEWPARAM(102,BN_CLICKED),0);
                            SendMessageW(window,WM_COMMAND,IDOK,0);return;
                        }
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            });
            const bool shown=ShowSerializedNotice(GetModuleHandleW(nullptr),title,body.c_str(),locale.close,nullptr,1000,2500,
                an::releasesUrl,label,locale.font);
            click.join();
            expect(shown&&labelled&&layout,"localized notice body and both buttons fit their controls");
            expect(browserCalls==1&&browserUrl==an::releasesUrl,"explicit click opens fixed ActionNitro HTTPS release URL exactly once");
        }
        browserCalls=0;
        expect(ShowSerializedNotice(GetModuleHandleW(nullptr),L"ActionNitro no auto-open test",L"Test only.",L"Close",nullptr,1000,150,
            an::releasesUrl,L"Open GitHub")&&browserCalls==0,"display and timeout never open browser automatically");
        expect(ShowSerializedNotice(GetModuleHandleW(nullptr),L"ActionNitro update test — 日本語",L"新しいバージョンが公開されています。\r\n現在: 0.1.0-alpha.59\r\n公開版: v0.1.0-alpha.60\r\nテスト表示です。ダウンロード・インストールは行いません。",L"閉じて続行",nullptr,1000,800),"Japanese Win32 dialog created and auto-closed");
        expect(ShowSerializedNotice(GetModuleHandleW(nullptr),L"ActionNitro update test — English",L"A newer version is available.\r\nInstalled: 0.1.0-alpha.59\r\nAvailable: v0.1.0-alpha.60\r\nTest only. Nothing downloaded or installed.",L"Close and continue",nullptr,1000,800),"English Win32 dialog created and auto-closed");
        expect(!ShowSerializedNotice(GetModuleHandleW(nullptr),L"Forbidden",L"",L"",+[]() noexcept{return false;},100,100),"gameplay gate prevents dialog creation");
    }
    std::cout<<"RESULT passed="<<passed<<" failed="<<failed<<'\n';return failed?1:0;
}
