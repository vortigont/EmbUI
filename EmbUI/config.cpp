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

    if ( sysData.fsDirty && !force ){
        LOG(println, F("UI: FS corrupt flag is set, won't write, u may try to reboot/reflash"));
        return;
    }


    File configFile;
    if (_cfg == nullptr) {
        LOG(println, F("UI: Save default main config file"));
        LittleFS.rename(P_cfgfile,P_cfgfile_bkp);
        embuifs::serialize2file(cfg, P_cfgfile);
    } else {
        LOG(printf_P, PSTR("UI: Save %s main config file\n"), _cfg);
        embuifs::serialize2file(cfg, _cfg);
    }
}

void EmbUI::load(const char *cfgfile){

    LOG(print, F("UI: Config file load "));

    if (cfgfile){
        if (embuifs::deserializeFile(cfg, cfgfile))
            return;
    } else {
        String f(P_cfgfile);
        if (!embuifs::deserializeFile(cfg, f.c_str())){
            LOG(println, F("...failed, trying with backup"));
            f = P_cfgfile_bkp;
            if (embuifs::deserializeFile(cfg, f.c_str())){
                LOG(println, F("BackUp load OK!"));
                return;
            }
        } else {
            LOG(println, F("OK!"));
            return;
        }
    }

    // тут выясняется, что оба конфига повреждены, очищаем конфиг, он будет заполнен значениями по-умолчанию
    cfg.clear();
}

void EmbUI::cfgclear(){
    LOG(println, F("UI: !CLEAR SYSTEM CONFIG!"));
    cfg.clear();
    autosave(true);
}