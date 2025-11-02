#pragma once

#ifdef _WIN32
  #if defined(RATS_EXPORT_DLL)
    #define RATS_API __declspec(dllexport)
  #elif defined(RATS_IMPORT_DLL)
    #define RATS_API __declspec(dllimport)
  #else
    #define RATS_API
  #endif
#else
  #if __GNUC__ >= 4
    #define RATS_API __attribute__((visibility("default")))
  #else
    #define RATS_API
  #endif
#endif
