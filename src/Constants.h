#pragma once

#include <QtGlobal>

namespace Constants {

inline constexpr char kAppName[] = "BigA";
inline constexpr char kAppDisplayName[] = "BigA";
inline constexpr char kAppVersion[] = "1.0.0";

// Session-local named mutex. A second process in the same Windows session exits
// immediately, without creating a QApplication or any window.
inline constexpr wchar_t kSingleInstanceMutexName[] =
    L"Local\\BigA.SingleInstance.4A0FB9E4-2E0E-4AB8-B725-31E8AF90E80B";

inline constexpr int kDefaultDotSize = 3;
inline constexpr int kMinCapacity = 0;
inline constexpr int kMaxCapacity = 9;
inline constexpr int kCapacityDebounceMs = 3000;

}  // namespace Constants
