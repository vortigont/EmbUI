// This framework originaly based on JeeUI2 lib used under MIT License Copyright (c) 2019 Marsel Akhkamov
// then re-written and named by (c) 2020 Anton Zolotarev (obliterator) (https://github.com/anton-zolotarev)
// also many thanks to Vortigont (https://github.com/vortigont), kDn (https://github.com/DmytroKorniienko)
// and others people

#include "EmbUI.h"

#ifdef ESP32
// remove esp8266's macros
 #ifdef FPSTR
  #undef FPSTR
  #define FPSTR(pstr_pointer) (pstr_pointer)
 #endif
 #ifdef F
  #undef F
  #define F(string_literal) (string_literal)
 #endif
#endif

void EmbUI::save(const char *_cfg){
    embuifs::serialize2file(cfg, _cfg ? _cfg : C_cfgfile);
    LOGD(P_EmbUI, println, "Save config file");
}

void EmbUI::load(const char *cfgfile){
    LOGD(P_EmbUI, print, F("Config file load "));
    embuifs::deserializeFile(cfg, cfgfile ? cfgfile : C_cfgfile);
}

void EmbUI::cfgclear(){
    LOGI(P_EmbUI, println, "!CLEAR SYSTEM CONFIG!");
    cfg.clear();
    autosave(true);
}
