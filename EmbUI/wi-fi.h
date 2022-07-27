// This framework originaly based on JeeUI2 lib used under MIT License Copyright (c) 2019 Marsel Akhkamov
// then re-written and named by (c) 2020 Anton Zolotarev (obliterator) (https://github.com/anton-zolotarev)
// also many thanks to Vortigont (https://github.com/vortigont), kDn (https://github.com/DmytroKorniienko)
// and others people

#pragma once

#define __EMBUI_WIFI_H

#ifdef ESP32
 #include <WiFi.h>
 #include <ESPmDNS.h>
#ifdef USE_SSDP
 #include <ESP32SSDP.h>
#endif
#endif

#define WIFI_CONNECT_TIMEOUT    10                      // timer for WiFi STA connection attempt 
#define WIFI_SET_AP_AFTER_DISCONNECT_TIMEOUT    15      // time after WiFi client disconnects and before internal AP is brought up
#define WIFI_RECONNECT_TIMER    30                      // timer for STA connect retry
#define WIFI_BEGIN_DELAY        3                       // scheduled delay for STA begin() connection

#define WIFI_PSK_MIN_LENGTH     8
