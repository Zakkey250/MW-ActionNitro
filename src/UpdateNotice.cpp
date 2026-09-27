#include "UpdateNotice.h"
#include "UpdateRelease.h"
#include "ModUpdateDialog.h"
#include "Version.h"

#include <winhttp.h>
#include <atomic>
#include <filesystem>
#pragma comment(lib,"winhttp.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"advapi32.lib")

namespace an { namespace {
constexpr char repository[]="Zakkey250/MW-ActionNitro";
constexpr char assetPrefix[]="MW-ActionNitro-";
// Fixed trusted destination; never execute a URL supplied by release metadata.
constexpr wchar_t releasesUrl[]=L"https://github.com/Zakkey250/MW-ActionNitro/releases";
constexpr wchar_t apiPath[]=L"/repos/Zakkey250/MW-ActionNitro/releases?per_page=100";
std::atomic<bool> checked{false};
mod_update::KernelHandle checkedEvent;
struct InternetHandle {
    HINTERNET value;
    explicit InternetHandle(HINTERNET v):value(v){}
    ~InternetHandle(){if(value)WinHttpCloseHandle(value);}
    InternetHandle(const InternetHandle&)=delete;
    InternetHandle& operator=(const InternetHandle&)=delete;
};
bool FetchReleases(std::string& output) {
    const auto deadline=GetTickCount64()+10000;
    InternetHandle session(WinHttpOpen(L"MW-ActionNitro-UpdateNotice/1",WINHTTP_ACCESS_TYPE_NO_PROXY,
                                     WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0));
    if(!session.value||!WinHttpSetTimeouts(session.value,1500,2000,2000,2000))return false;
    InternetHandle connection(WinHttpConnect(session.value,L"api.github.com",INTERNET_DEFAULT_HTTPS_PORT,0));
    if(!connection.value)return false;
    InternetHandle request(WinHttpOpenRequest(connection.value,L"GET",apiPath,nullptr,WINHTTP_NO_REFERER,
                                              WINHTTP_DEFAULT_ACCEPT_TYPES,WINHTTP_FLAG_SECURE));
    if(!request.value)return false;
    DWORD redirects=WINHTTP_OPTION_REDIRECT_POLICY_NEVER,auth=WINHTTP_AUTOLOGON_SECURITY_LEVEL_HIGH;
    DWORD disabled=WINHTTP_DISABLE_COOKIES;
    if(!WinHttpSetOption(request.value,WINHTTP_OPTION_REDIRECT_POLICY,&redirects,sizeof(redirects))||
       !WinHttpSetOption(request.value,WINHTTP_OPTION_AUTOLOGON_POLICY,&auth,sizeof(auth))||
       !WinHttpSetOption(request.value,WINHTTP_OPTION_DISABLE_FEATURE,&disabled,sizeof(disabled)))return false;
    constexpr wchar_t headers[]=L"Accept: application/vnd.github+json\r\nX-GitHub-Api-Version: 2022-11-28\r\n";
    if(!WinHttpSendRequest(request.value,headers,static_cast<DWORD>(-1),WINHTTP_NO_REQUEST_DATA,0,0,0)||
       GetTickCount64()>=deadline||!WinHttpReceiveResponse(request.value,nullptr))return false;
    DWORD status=0,size=sizeof(status);
    if(!WinHttpQueryHeaders(request.value,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,WINHTTP_HEADER_NAME_BY_INDEX,
                            &status,&size,WINHTTP_NO_HEADER_INDEX)||status!=200)return false;
    output.clear();char buffer[8192];
    while(GetTickCount64()<deadline) {
        DWORD read=0;
        if(!WinHttpReadData(request.value,buffer,sizeof(buffer),&read))return false;
        if(!read)return !output.empty();
        if(output.size()+read>2*1024*1024)return false;
        output.append(buffer,read);
    }
    return false;
}
} // namespace

void CheckForStartupUpdate(HMODULE module,const Locale& locale) noexcept {
    if(checked.exchange(true))return;
    try {
        wchar_t path[32768]{};const DWORD count=GetModuleFileNameW(module,path,32768);
        if(!count||count>=32768)return;

        auto ini=std::filesystem::path(path);ini.replace_extension(L".ini");
        if(!GetPrivateProfileIntW(L"Updates",L"Enabled",1,ini.c_str())) {UpdateLog("UPDATE_NOTICE disabled");return;}
        const auto eventName=mod_update::QueueName(GetCurrentProcessId())+L".Checked.MW-ActionNitro";
        checkedEvent.value=CreateEventW(nullptr,TRUE,TRUE,eventName.c_str());
        if(!checkedEvent.value||GetLastError()==ERROR_ALREADY_EXISTS)return;
        if(!StartupNoticeAllowed())return;
        std::string payload;
        if(!FetchReleases(payload)){UpdateLog("UPDATE_NOTICE unavailable (offline/HTTP/timeout); startup unaffected");return;}
        const auto update=mod_update::FindUpdate(payload,version,repository,assetPrefix);
        if(!update){UpdateLog("UPDATE_NOTICE no eligible newer release");return;}
        if(!StartupNoticeAllowed()){UpdateLog("UPDATE_NOTICE deferred until next launch; gameplay active");return;}
        wchar_t requested[128]{};
        GetPrivateProfileStringW(L"Updates",L"Language",L"auto",requested,128,ini.c_str());
        const auto* selected=normalizeLanguage(requested)==L"auto"?&locale:findLocale(requested);
        if(!selected)selected=&locales[0];
        const std::wstring current(version,version+std::char_traits<char>::length(version));
        const std::wstring latest(update->tag.begin(),update->tag.end());
        const auto body=updateBody(*selected,current,latest);
        const auto title=std::wstring(L"MW ActionNitro — ")+selected->notice;
        const bool shown=mod_update::ShowSerializedNotice(module,title.c_str(),body.c_str(),selected->close,
            StartupNoticeAllowed,90000,60000,releasesUrl,selected->open,selected->font);
        const auto message=std::string("UPDATE_NOTICE available=")+update->tag+(shown?" shown=1":" shown=0")+" downloads=0 browserButton=1";
        UpdateLog(message.c_str());
    }catch(...){UpdateLog("UPDATE_NOTICE skipped after internal error; startup unaffected");}
}
} // namespace an
