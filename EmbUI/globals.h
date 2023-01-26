// This framework originaly based on JeeUI2 lib used under MIT License Copyright (c) 2019 Marsel Akhkamov
// then re-written and named by (c) 2020 Anton Zolotarev (obliterator) (https://github.com/anton-zolotarev)
// also many thanks to Vortigont (https://github.com/vortigont), kDn (https://github.com/DmytroKorniienko)
// and others people

#pragma once

// Global macro's and framework libs
#include <Arduino.h>
#include "constants.h"

#ifdef ESP8266
#error "Sorry, esp8266 is no longer supported"
#error "use v2.6 branch for 8266 https://github.com/vortigont/EmbUI/tree/v2.6"
#include "no_esp8266"
#endif

// STRING Macro
#ifndef __STRINGIFY
 #define __STRINGIFY(a) #a
#endif
#define TOSTRING(x) __STRINGIFY(x)

// LOG macro's
#if defined(EMBUI_DEBUG)
  #ifndef EMBUI_DEBUG_PORT
    #define EMBUI_DEBUG_PORT Serial
  #endif

  #define LOG(func, ...) EMBUI_DEBUG_PORT.func(__VA_ARGS__)
  #define LOG_CALL(call...) { call; }
#else
  #define LOG(func, ...) ;
  #define LOG_CALL(call...) ;
#endif


#define EMBUI_VERSION_MAJOR     2
#define EMBUI_VERSION_MINOR     7
#define EMBUI_VERSION_REVISION  1  // '999' here is current dev version

#define EMBUI_VERSION_VALUE     (MAJ, MIN, REV) ((MAJ) << 16 | (MIN) << 8 | (REV))

/* make version as integer for comparison */
#define EMBUI_VERSION           EMBUI_VERSION_VALUE(EMBUI_VERSION_MAJOR, EMBUI_VERSION_MINOR, EMBUI_VERSION_REVISION)

/* make version as string, i.e. "2.6.1" */
#define EMBUI_VERSION_STRING    TOSTRING(EMBUI_VERSION_MAJOR) "." TOSTRING(EMBUI_VERSION_MINOR) "." TOSTRING(EMBUI_VERSION_REVISION)
// compat definiton
#define EMBUIVER                EMBUI_VERSION_STRING

typedef std::function<void(void)> callback_function_t;
