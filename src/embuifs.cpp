/*
    This file is part of EmbUI project
    https://github.com/vortigont/EmbUI

    Copyright © 2023 Emil Muratov (Vortigont)   https://github.com/vortigont/

    EmbUI is free software: you can redistribute it and/or modify
    it under the terms of MIT License https://opensource.org/license/mit/
*/

#include "embuifs.hpp"
#include "StreamUtils.h"
#include "globals.h"
//#include "char_const.h"

namespace embuifs {

    bool deserializeFile(JsonDocument& doc, const String& filepath){ return deserializeFile(doc, filepath.c_str()); };

    bool deserializeFile(JsonDocument& doc, const char* filepath){
        if (!filepath || !*filepath)
            return false;

        LOGV(P_EmbUI, printf, "Load file: %s\n", filepath);
        File jfile = LittleFS.open(filepath, "r");

        if (!jfile){
            LOGD(P_EmbUI, printf,"Can't open file: %s\n", filepath);
            return false;
        }

        DeserializationError error = deserializeJson(doc, jfile);
        jfile.close();

        if (!error) return true;
        LOGE(P_EmbUI, printf, "File: failed to load json file: %s, deserialize error: ", filepath);
        LOGE(P_EmbUI, println, error.code());
        return false;
    }

    size_t serialize2file(const JsonDocument& doc, const String& filepath, size_t bufsize){ return serialize2file(doc, filepath.c_str(), bufsize); };

    size_t serialize2file(const JsonDocument& doc, const char* filepath, size_t bufsize){
        File hndlr = LittleFS.open(filepath, "w");
        WriteBufferingStream bufferedFile(hndlr, bufsize);
        size_t len = serializeJson(doc, bufferedFile);
        bufferedFile.flush();
        hndlr.close();
        return len;
    }


}