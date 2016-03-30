#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <stdio.h>
#include <base/inc/platform.h>
#include <base/inc/base_library/base_types_functions.h>

#if defined(SDK_OS_WIN)
# include <windows.h>
#elif defined(SDK_OS_POSIX)
# include <sys/types.h>
# include <unistd.h>
#endif

inline std::wstring Uint64ToWString(SDKUInt64 val)
{
  char str[32];

#ifdef SDK_CC_GCC
  snprintf(str, sizeof(str), "%llu", static_cast<unsigned long long>(val));
#else
  _ultoa_s(static_cast<SDKUInt32>(val), str, 10);
#endif

  std::string s(str);
  return std::wstring(s.begin(), s.end());
}

inline std::wstring Int64ToWString(SDKInt64 val)
{
  char str[32];
#ifdef SDK_CC_GCC
  snprintf(str, sizeof(str), "%lld", static_cast<long long>(val));
#else
  _ltoa_s(static_cast<SDKInt32>(val), str, 10);
#endif
  std::string s(str);
  return std::wstring(s.begin(), s.end());
}

inline std::wstring GetModulePathName()
{
#if defined(SDK_OS_WIN)
  wchar_t module_path[MAX_PATH] = {};
  GetModuleFileNameW(NULL, module_path, sizeof(module_path));
  return module_path;
#elif defined(SDK_OS_LINUX)
  char module_path[1024] = {0};
  char tmp_str[32];
  sprintf(tmp_str, "/proc/%d/exe", getpid());
  int bytes = readlink(tmp_str, module_path, sizeof(module_path) - 1);
  if (bytes >= 0)
    module_path[bytes] = '\0';
  std::string path(module_path);
  return std::wstring(path.begin(), path.end());
#endif
}

inline std::wstring GetBookmarksPath()
{
  std::wstring app_path(GetModulePathName());
  std::wstring::size_type ext_pos = app_path.rfind(L'.');
  if (std::wstring::npos != ext_pos)
    app_path = std::wstring(app_path.begin(), app_path.begin() + ext_pos + 1);
  else
    app_path += L".";

  std::wstring bmk_path = app_path + L"bmk";
  return bmk_path;
}

inline std::wstring GetViewingGroupsPath()
{
  std::wstring app_path(GetModulePathName());
  std::wstring::size_type ext_pos = app_path.rfind(L'.');
  if (std::wstring::npos != ext_pos)
    app_path = std::wstring(app_path.begin(), app_path.begin() + ext_pos + 1);
  else
    app_path += L".";

  std::wstring bmk_path = app_path + L"gvf";
  return bmk_path;
}

#endif // UTILS_H
