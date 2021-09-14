#pragma once

#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#  define DECL_EXPORT __declspec(dllexport)
#  define DECL_IMPORT __declspec(dllimport)
#else
#  define DECL_EXPORT     __attribute__((visibility("default")))
#  define DECL_IMPORT     __attribute__((visibility("default")))
#endif

#if defined(CPPUTILITIES_LIBRARY)
#  define CPPUTILITIES_EXPORT DECL_EXPORT
#else
#  define CPPUTILITIES_EXPORT DECL_IMPORT
#endif

//THREAD stands for the threading wrapper around std::thread.
//SIGSOT stands for the THREAD secure signal/slot system.

//Dynamic watching about all that can happen at runtime
#define THREAD_TRACKING
#define SIGSOT_TRACKING

//Print when a slot (ftor or xtor) is called from a signal.
//The signal classes have to implement it by their own as the following format, printed BEFORE calling:
/*
    std::cout << "                > [CALLED] [" << ID << "] [" << NAME << "] [" << TYPE << "]" << std::endl;
*/
#define SSCALL_OUTPUTS

//Generate stack trace output and such when fails even if the argument was not passed when running the app.
#define FORCE_DEBUG

//Generates debug data even if the signal have been sent by user (Ctrl+C -> SIGINT)
#define DEBUG_ALL_OUTS

//Easier tracking and meta thinging can be done through objects (threads, slots, signals, their names and signatures)
//SSDSet will store nothing. Though some inherited classes can implement some of its virtual methods and store
//by their own meta data (like get_type() that generates the data at runtime). It additionnaly do the same
//as THREAD_NAME_USE when disabled but for the SIGSOT
#define SIGSOT_META_USE
//If it is not defined, all AbstractThread::name() will return "" and no data will be allocated for their names.
#define THREAD_NAME_USE
