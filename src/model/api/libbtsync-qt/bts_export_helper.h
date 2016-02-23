#pragma once

#ifdef _WIN32
# ifdef libbtsync_qt_EXPORTS
#  define  __declspec(dllexport)
# else
#  define  __declspec(dllimport)
# endif
#else
# define 
#endif

#ifdef _MSC_VER
#define noexcept
#endif
