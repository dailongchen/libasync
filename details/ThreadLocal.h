#pragma once

#ifndef THREAD_LOCAL
    #ifdef __GNUC__
        #define THREAD_LOCAL __thread
    #elif (defined(_MSC_VER) && _MSC_VER <= 1800)
        #define THREAD_LOCAL __declspec(thread)
    #else
        #define THREAD_LOCAL thread_local
    #endif
#endif