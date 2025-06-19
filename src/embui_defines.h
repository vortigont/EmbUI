// This framework originaly based on JeeUI2 lib used under MIT License Copyright (c) 2019 Marsel Akhkamov
// then re-written and named by (c) 2020 Anton Zolotarev (obliterator) (https://github.com/anton-zolotarev)
// also many thanks to Vortigont (https://github.com/vortigont), kDn (https://github.com/DmytroKorniienko)
// and others people

#pragma once

// Global macro's and framework libs

// STRING Macro
#ifndef __STRINGIFY
 #define __STRINGIFY(a) #a
#endif
#define TOSTRING(x) __STRINGIFY(x)

#define EMBUI_VERSION_MAJOR     4
#define EMBUI_VERSION_MINOR     2
#define EMBUI_VERSION_REVISION  3

// API version for JS frontend
#define EMBUI_JSAPI             8
// loadable UI blocks version requirement (loaded from js/ui_sys.json)
#define EMBUI_UIOBJECTS         6

#define EMBUI_VERSION_VALUE     (MAJ, MIN, REV) ((MAJ) << 16 | (MIN) << 8 | (REV))

/* make version as integer for comparison */
#define EMBUI_VERSION           EMBUI_VERSION_VALUE(EMBUI_VERSION_MAJOR, EMBUI_VERSION_MINOR, EMBUI_VERSION_REVISION)

/* make version as string, i.e. "2.6.1" */
#define EMBUI_VERSION_STRING    TOSTRING(EMBUI_VERSION_MAJOR) "." TOSTRING(EMBUI_VERSION_MINOR) "." TOSTRING(EMBUI_VERSION_REVISION)


#ifndef EMBUI_PUB_PERIOD
#define EMBUI_PUB_PERIOD              10      // Values Publication period, s
#endif

#ifndef EMBUI_AUTOSAVE_TIMEOUT
#define EMBUI_AUTOSAVE_TIMEOUT        30        // configuration autosave timer, sec
#endif

// Default Hostname/AP prefix
#ifndef EMBUI_IDPREFIX
#define EMBUI_IDPREFIX                "EmbUI"
#endif

// maximum number of websocket client connections
#ifndef EMBUI_MAX_WS_CLIENTS
#define EMBUI_MAX_WS_CLIENTS          4
#endif

#define EMBUI_WEBSOCK_URI             "/ws"
