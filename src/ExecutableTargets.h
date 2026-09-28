#pragma once
#include <cstdint>
#include <string_view>
namespace an {
struct ExecutableTarget {const char* id;std::uint64_t size;const char* sha256;};
// Exact file identities. Redux 3.04 shares Main's mapped .text/.rdata/.data;
// its different resources do not require a second set of engine addresses.
inline constexpr ExecutableTarget executableTargets[]={
 {"nfspatcher-en-1.3",6029312,"80774c2e5d619b4f120b48d4462896fd504c263399d203a238769cffde1d253c"},
 {"nfspatcher-en-1.3-4gb",6029312,"b248271bf8eac8c9b283b8c95e3add672b713bf529b05f1780e58268493b9d06"},
 {"redux-3.04-en-1.3-4gb",5926912,"0c5675a08cd71fd6d31ca87e992a915054bd8b80d268bff0561d7ecc2067e342"}
};
inline bool knownExecutableSize(std::uint64_t size){for(const auto& t:executableTargets)if(t.size==size)return true;return false;}
inline const ExecutableTarget* findExecutable(std::uint64_t size,std::string_view digest){
 for(const auto& t:executableTargets)if(t.size==size&&digest==t.sha256)return &t;return nullptr;
}
}
