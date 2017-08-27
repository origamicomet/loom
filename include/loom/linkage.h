//===-- loom/linkage.h ----------------------------------*- mode: C++11 -*-===//
//
//                            __                  
//                           |  |   ___ ___ _____ 
//                           |  |__| . | . |     |
//                           |_____|___|___|_|_|_|
//
//       This file is distributed under the terms described in LICENSE.
//
//===----------------------------------------------------------------------===//

#ifndef _LOOM_LINKAGE_H_
#define _LOOM_LINKAGE_H_

/// \def LOOM_LINKAGE_INTERNAL
/// \brief Export symbols for inclusion via objects.
#define LOOM_LINKAGE_INTERNAL 0

/// \def LOOM_LINKAGE_STATIC
/// \brief Export symbols for inclusion via static library.
#define LOOM_LINKAGE_STATIC 1

/// \def LOOM_LINKAGE_DYNAMIC
/// \brief Export symbols for inclusion via dynamic library.
#define LOOM_LINKAGE_DYNAMIC 2

/// \def LOOM_LINKAGE
/// \brief Specifies how you intend to link to Loom.
#ifndef LOOM_LINKAGE
  #error ("Please specify how you intend on linking to Loom by defining `LOOM_LINKAGE'.")
#endif

/// \def LOOM_LOCAL
/// \brief Marks a symbol for internal usage.
#if defined(DOXYGEN)
  #define LOOM_LOCAL
#else
  #if LOOM_LINKAGE == LOOM_LINKAGE_STATIC
    #define LOOM_LOCAL
  #elif LOOM_LINKAGE == LOOM_LINKAGE_DYNAMIC
    #if defined(__LOOM_IS_BEING_COMPILED__)
      #if defined(__GNUC__)
        #if __GNUC__ >= 4
          #define LOOM_LOCAL __attribute__ ((visibility ("hidden")))
        #else
          #define LOOM_LOCAL
        #endif
      #elif defined(__clang__)
        #define LOOM_LOCAL __attribute__ ((visibility ("hidden")))
      #elif defined(_MSC_VER) || defined(__CYGWIN__)
        #define LOOM_LOCAL
      #else
        #error ("Unknown or unsupported toolchain!")
      #endif
    #else
      #define LOOM_LOCAL
    #endif
  #endif
#endif

/// \def LOOM_PUBLIC
/// \brief Marks a symbol for public usage.
#if defined(DOXYGEN)
  #define LOOM_PUBLIC
#else
  #if LOOM_LINKAGE == LOOM_LINKAGE_STATIC
    #define LOOM_PUBLIC
  #elif LOOM_LINKAGE == LOOM_LINKAGE_DYNAMIC
    #if defined(__LOOM_IS_BEING_COMPILED__)
      #if defined(__GNUC__)
        #if __GNUC__ >= 4
          #define LOOM_PUBLIC __attribute__ ((visibility ("default")))
        #else
          #define LOOM_PUBLIC
        #endif
      #elif defined(__clang__)
        #define LOOM_PUBLIC __attribute__ ((visibility ("default")))
      #elif defined(_MSC_VER) || defined(__CYGWIN__)
        #define LOOM_PUBLIC __declspec(dllexport)
      #else
        #error ("Unknown or unsupported toolchain!")
      #endif
    #else // #if !defined(__LOOM_IS_BEING_COMPILED__)
      #if (defined(__GNUC__) && (__GNUC__ >= 4))
        #define LOOM_PUBLIC
      #elif defined(__clang__)
        #define LOOM_PUBLIC
      #elif defined(_MSC_VER) || defined(__CYGWIN__)
        #define LOOM_PUBLIC __declspec(dllimport)
      #else
        #error ("Unknown or unsupported toolchain!")
      #endif
    #endif
  #endif
#endif

/// \def LOOM_BEGIN_EXTERN_C
/// \internal
/// \def LOOM_END_EXTERN_C
/// \internal
#if defined(DOXYGEN)
  #define LOOM_BEGIN_EXTERN_C
  #define LOOM_END_EXTERN_C
#else
  #if defined(__cplusplus)
    #define LOOM_BEGIN_EXTERN_C extern "C" {
    #define LOOM_END_EXTERN_C }
  #else
    #define LOOM_BEGIN_EXTERN_C
    #define LOOM_END_EXTERN_C
  #endif
#endif

#endif // _LOOM_LINKAGE_H_
