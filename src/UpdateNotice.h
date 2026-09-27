#pragma once
#include <windows.h>
#include "Localization.h"
namespace an {
void CheckForStartupUpdate(HMODULE module,const Locale& locale) noexcept;
bool StartupNoticeAllowed() noexcept;
void UpdateLog(const char* text) noexcept;
}
