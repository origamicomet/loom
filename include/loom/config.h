//===-- loom/config.h -----------------------------------*- mode: C++11 -*-===//
//
//                            __                  
//                           |  |   ___ ___ _____ 
//                           |  |__| . | . |     |
//                           |_____|___|___|_|_|_|
//
//       This file is distributed under the terms described in LICENSE.
//
//===----------------------------------------------------------------------===//

#ifndef _LOOM_CONFIG_H_
#define _LOOM_CONFIG_H_

// REFACTOR(mtwilliams): Discard concept of debug and development builds,
// outside of setting reasonable defaults that can be overridden.

/// \def LOOM_CONFIGURATION_DEBUG
/// \brief Debug build.
#define LOOM_CONFIGURATION_DEBUG 1

/// \def LOOM_CONFIGURATION_RELEASE
/// \brief Release build.
#define LOOM_CONFIGURATION_RELEASE 2

/// \def LOOM_CONFIGURATION
/// \brief Specifies how "loose and fast" Loom is.
#if defined(DOXYGEN)
  #define LOOM_CONFIGURATION
#else
  #if !defined(LOOM_CONFIGURATION)
    #error ("No configuration specified! Please specify a configuration by defining `LOOM_CONFIGURATION'.")
  #else
    #if (LOOM_CONFIGURATION != LOOM_CONFIGURATION_DEBUG) && \
        (LOOM_CONFIGURATION != LOOM_CONFIGURATION_RELEASE)
      #error ("Unknown configuration specified. See include/loom/config.h for a list of possible configurations.")
    #endif
  #endif
#endif

/// \def LOOM_PLATFORM_WINDOWS
/// \brief Windows
#define LOOM_PLATFORM_WINDOWS 1

/// \def LOOM_PLATFORM_MAC
/// \brief macOS
#define LOOM_PLATFORM_MAC 2

/// \def LOOM_PLATFORM_LINUX
/// \brief Linux
#define LOOM_PLATFORM_LINUX 3

/// \def LOOM_PLATFORM_IOS
/// \brief iOS
#define LOOM_PLATFORM_IOS 4

/// \def LOOM_PLATFORM_ANDROID
/// \brief Android
#define LOOM_PLATFORM_ANDROID 5

/// \def LOOM_PLATFORM
/// \brief Target platform.
#if defined(DOXYGEN)
  #define LOOM_PLATFORM
#else
  #if defined(_WIN32) || defined(_WIN64)
    #define LOOM_PLATFORM LOOM_PLATFORM_WINDOWS
  #elif defined(__APPLE__)
    #include <TargetConditionals.h>
    #if defined(TARGET_OS_IPHONE)
      #define LOOM_PLATFORM LOOM_PLATFORM_IOS
    #else
      #define LOOM_PLATFORM LOOM_PLATFORM_MAC
    #endif
  #elif defined(__linux__)
    #if defined(ANDROID)
      #define LOOM_PLATFORM LOOM_PLATFORM_ANDROID
    #else
      #define LOOM_PLATFORM LOOM_PLATFORM_LINUX
    #endif
  #else
    #error ("Unknown or unsupported platform!")
  #endif
#endif

/// \def LOOM_COMPILER_MSVC
/// \brief Microsoft Visual C/C++.
#define LOOM_COMPILER_MSVC 1

/// \def LOOM_COMPILER_GCC
/// \brief GNU Compiler Collection.
#define LOOM_COMPILER_GCC 2

/// \def LOOM_COMPILER_CLANG
/// \brief LLVM/Clang.
#define LOOM_COMPILER_CLANG 3

/// \def LOOM_COMPILER
/// \brief Host compiler.
#if defined(DOXYGEN)
  #define LOOM_COMPILER
#else // !defined(DOXYGEN)
  #if defined(_MSC_VER)
    #define LOOM_COMPILER LOOM_COMPILER_MSVC
    #define LOOM_MICROSOFT_VISUAL_STUDIO_2013 1800
    #define LOOM_MICROSOFT_VISUAL_STUDIO_2012 1700
    #define LOOM_MICROSOFT_VISUAL_STUDIO_2010 1600
    #define LOOM_MICROSOFT_VISUAL_STUDIO_2008 1500
    #define LOOM_MICROSOFT_VISUAL_STUDIO_2005 1400
    #define LOOM_MICROSOFT_VISUAL_STUDIO_2003 1310
    #define LOOM_MICROSOFT_VISUAL_STUDIO_2002 1300
    #define LOOM_MICROSOFT_VISUAL_C_6 1200
    #if (_MSC_VER >= MICROSOFT_VISUAL_STUDIO_2005)
      // Only support Microsoft Visual Studio 2005 or newer.
    #elif (_MSC_VER == MICROSOFT_VISUAL_STUDIO_2003)
      #error ("Microsoft Visual C/C++ .NET 2003 is unsupported! Please upgrade to Microsoft Visual C/C++ 2005 or newer.")
    #elif (_MSC_VER == MICROSOFT_VISUAL_STUDIO_2002)
      // Visual Studio .NET 2002 had major bugs so annoying that Microsoft
      // provided complementary upgrades to Microsoft Visual C/C++ .NET 2003.
      #error ("Microsoft Visual C/C++ .NET 2002 is unsupported! Please upgrade to Microsoft Visual C/C++ 2005 or newer.")
    #elif (_MSC_VER == LOOM_MICROSOFT_VISUAL_C_6)
      // If we did ever want to support Visual Studio 6, we should check for
      // Service Pack 6 by checking if _MSC_FULL_VER is less than 12008804.
      #error ("Microsoft Visual C/C++ 6 is unsupported! Please upgrade to Microsoft Visual C/C++ 2005 or newer.")
    #else
      #error ("Your version of Microsoft Visual C/C++ is unsupported! Please upgrade to Microsoft Visual C/C++ 2005 or newer.")
    #endif
  #elif defined(__GNUC__)
    #if defined(__clang__)
      #define LOOM_COMPILER LOOM_COMPILER_CLANG
      #define __CLANG_VERSION__ (__clang_major__ * 10000 \
                                 + __clang_minor__ * 100 \
                                 + __clang_patchlevel__)
    #else
      // HACK(mtwilliams): Assume we're being compiled with GCC.
      #define LOOM_COMPILER LOOM_COMPILER_GCC
      #if defined(__GNUC_PATCHLEVEL__)
        #define __GNUC_VERSION__ (__GNUC__ * 10000 \
                                  + __GNUC_MINOR__ * 100 \
                                  + __GNUC_PATCHLEVEL__)
      #else
        #define __GNUC_VERSION__ (__GNUC__ * 10000 \
                                  + __GNUC_MINOR__ * 100)
      #endif
    #endif
  #else
    #error ("Unknown or unsupported compiler!")
  #endif
#endif

/// \def LOOM_ARCHITECTURE_X86
/// \brief Intel/AMD x86.
#define LOOM_ARCHITECTURE_X86 1

/// \def LOOM_ARCHITECTURE_X86_64
/// \brief Intel/AMD x86_64.
#define LOOM_ARCHITECTURE_X86_64 2

/// \def LOOM_ARCHITECTURE
/// \brief Target architecture.
#if defined(DOXYGEN)
  #define LOOM_COMPILER
#else
  #if defined(_M_IX86) || defined(__i386__)
    #define LOOM_ARCHITECTURE LOOM_ARCHITECTURE_X86
  #elif defined(_M_X64) || defined(_M_AMD64) || defined(__x86_64) || defined(__x86_64__) || defined(__amd64) || defined(__amd64__)
    #define LOOM_ARCHITECTURE LOOM_ARCHITECTURE_X86_64
  #else
    #error ("Unknown or unsupported architecture!")
  #endif
#endif

#endif // _LOOM_CONFIG_H_
