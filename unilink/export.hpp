#pragma once

#if defined(_WIN32)
#if defined(UNILINK_BUILD)
#define UNILINK_API __declspec(dllexport)
#else
#define UNILINK_API __declspec(dllimport)
#endif
#else
#define UNILINK_API __attribute__((visibility("default")))
#endif