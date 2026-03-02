#pragma once

#if defined(_MSC_VER)
    #define CSICS_DISABLE_WARNING_PUSH __pragma(warning(push))
    #define CSICS_DISABLE_WARNING_POP __pragma(warning(pop))
    #define CSICS_DISABLE_WARNING_MSVC(w) __pragma(warning(disable: w))
    #define CSICS_DISABLE_WARNING_GCC(w)
    #define CSICS_DISABLE_WARNING_CLANG(w)
#elif defined(__clang__)
    #define CSICS_DISABLE_WARNING_PUSH _Pragma("clang diagnostic push")
    #define CSICS_DISABLE_WARNING_POP _Pragma("clang diagnostic pop")
    #define CSICS_DISABLE_WARNING_MSVC(w)
    #define CSICS_DISABLE_WARNING_GCC(w)
    #define CSICS_DISABLE_WARNING_CLANG(w) _Pragma(CSICS_STRINGIFY(clang diagnostic ignored w))
#elif defined(__GNUC__)
    #define CSICS_DISABLE_WARNING_PUSH _Pragma("GCC diagnostic push")
    #define CSICS_DISABLE_WARNING_POP _Pragma("GCC diagnostic pop")
    #define CSICS_DISABLE_WARNING_MSVC(w)
    #define CSICS_DISABLE_WARNING_GCC(w) _Pragma(CSICS_STRINGIFY(GCC diagnostic ignored w))
    #define CSICS_DISABLE_WARNING_CLANG(w)
#endif

#define CSICS_STRINGIFY(x) #x
