/*
 * dllexport.h - dllexport
 *
 * Date   : 2021/03/16
 */
#ifndef __DLLEXPORT_H__
#define __DLLEXPORT_H__

#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
    #define DLL_EXPORT_API __declspec(dllexport)
    #define DLL_IMPORT_API __declspec(dllimport)
#else
    #if __GNUC__ >= 4
        #define DLL_EXPORT_API __attribute__((visibility("default")))
        #define DLL_IMPORT_API
    #else
        #define DLL_EXPORT_API
        #define DLL_IMPORT_API
    #endif
#endif


#endif
