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

#define EMBUI_VERSION_MAJOR     4
#define EMBUI_VERSION_MINOR     0
#define EMBUI_VERSION_REVISION  0

// API version for JS frontend
#define EMBUI_JSAPI             6
// loadable UI blocks version requirement (loaded from js/ui_sys.json)
#define EMBUI_UIOBJECTS         3

#define EMBUI_VERSION_VALUE     (MAJ, MIN, REV) ((MAJ) << 16 | (MIN) << 8 | (REV))

/* make version as integer for comparison */
#define EMBUI_VERSION           EMBUI_VERSION_VALUE(EMBUI_VERSION_MAJOR, EMBUI_VERSION_MINOR, EMBUI_VERSION_REVISION)

/* make version as string, i.e. "2.6.1" */
#define EMBUI_VERSION_STRING    TOSTRING(EMBUI_VERSION_MAJOR) "." TOSTRING(EMBUI_VERSION_MINOR) "." TOSTRING(EMBUI_VERSION_REVISION)
// compat definiton
#define EMBUIVER                EMBUI_VERSION_STRING


// LOG macro's
#ifndef EMBUI_DEBUG_PORT
  #define EMBUI_DEBUG_PORT Serial
#endif

// undef possible LOG macros
#ifdef LOG
  #undef LOG
#endif
#ifdef LOGV
  #undef LOGV
#endif
#ifdef LOGD
  #undef LOGD
#endif
#ifdef LOGI
  #undef LOGI
#endif
#ifdef LOGW
  #undef LOGW
#endif
#ifdef LOGE
  #undef LOGE
#endif



#if defined(EMBUI_DEBUG_LEVEL) && EMBUI_DEBUG_LEVEL == 5
	#define LOGV(tag, func, ...) EMBUI_DEBUG_PORT.print(tag); EMBUI_DEBUG_PORT.print(" V: "); EMBUI_DEBUG_PORT.func(__VA_ARGS__)
#else
	#define LOGV(...)
#endif

#if defined(EMBUI_DEBUG_LEVEL) && EMBUI_DEBUG_LEVEL > 3
	#define LOGD(tag, func, ...) EMBUI_DEBUG_PORT.print(tag); EMBUI_DEBUG_PORT.print(" D: "); EMBUI_DEBUG_PORT.func(__VA_ARGS__)
#else
	#define LOGD(...)
#endif

#if defined(EMBUI_DEBUG_LEVEL) && EMBUI_DEBUG_LEVEL > 2
	#define LOGI(tag, func, ...) EMBUI_DEBUG_PORT.print(tag); EMBUI_DEBUG_PORT.print(" I: "); EMBUI_DEBUG_PORT.func(__VA_ARGS__)
	// compat macro
	#define LOG(func, ...) EMBUI_DEBUG_PORT.func(__VA_ARGS__)
	#define LOG_CALL(call...) { call; }
#else
	#define LOGI(...)
	// compat macro
	#define LOG(...)
	#define LOG_CALL(call...) ;
#endif

#if defined(EMBUI_DEBUG_LEVEL) && EMBUI_DEBUG_LEVEL > 1
	#define LOGW(tag, func, ...) EMBUI_DEBUG_PORT.print(tag); EMBUI_DEBUG_PORT.print(" W: "); EMBUI_DEBUG_PORT.func(__VA_ARGS__)
#else
	#define LOGW(...)
#endif

#if defined(EMBUI_DEBUG_LEVEL) && EMBUI_DEBUG_LEVEL > 0
	#define LOGE(tag, func, ...) EMBUI_DEBUG_PORT.print(tag); EMBUI_DEBUG_PORT.print(" E: "); EMBUI_DEBUG_PORT.func(__VA_ARGS__)
#else
	#define LOGE(...)
#endif
