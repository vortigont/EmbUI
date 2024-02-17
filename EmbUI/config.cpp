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

void EmbUI::save(const char *_cfg, bool force){
    File configFile;
    if (_cfg == nullptr) {
        LOGD(P_EmbUI, println, "Save config file");
        LittleFS.rename(C_cfgfile,C_cfgfile_bkp);
        embuifs::serialize2file(cfg, C_cfgfile);
    } else {
        LOGD(P_EmbUI, printf, "Save %s main config file\n", _cfg);
        embuifs::serialize2file(cfg, _cfg);
    }
}

void EmbUI::load(const char *cfgfile){

    LOGD(P_EmbUI, print, F("Config file load "));

    if (cfgfile){
        if (embuifs::deserializeFile(cfg, cfgfile))
            return;
    } else {
        String f(C_cfgfile);
        if (!embuifs::deserializeFile(cfg, f.c_str())){
            LOGV(P_EmbUI, println, F("...failed, trying with backup"));
            f = C_cfgfile_bkp;
            if (embuifs::deserializeFile(cfg, f.c_str())){
                LOGV(P_EmbUI, println, F("BackUp load OK!"));
                return;
            }
        } else {
            LOGD(P_EmbUI, println, F("OK!"));
            return;
        }
    }

    // тут выясняется, что оба конфига повреждены, очищаем конфиг, он будет заполнен значениями по-умолчанию
    cfg.clear();
}

void EmbUI::cfgclear(){
    LOGD(P_EmbUI, println, F("!CLEAR SYSTEM CONFIG!"));
    cfg.clear();
    autosave(true);
}
