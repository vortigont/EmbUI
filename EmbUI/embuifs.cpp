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

    bool deserializeFile(DynamicJsonDocument& doc, const char* filepath){
        if (!filepath || !*filepath)
            return false;

        //LOG(printf_P, PSTR("Load file: %s\n"), filepath);
        File jfile = LittleFS.open(filepath, "r");

        if (!jfile){
            LOG(printf_P, PSTR("Can't open file: %s"), filepath);
            return false;
        }

        DeserializationError error = deserializeJson(doc, jfile);
        jfile.close();

        if (!error) return true;
        LOG(printf_P, PSTR("File: failed to load json file: %s, deserialize error: "), filepath);
        LOG(println, error.code());
        return false;
    }

    size_t serialize2file(const DynamicJsonDocument& doc, const char* filepath, size_t bufsize){
        File hndlr = LittleFS.open(filepath, "w");
        WriteBufferingStream bufferedFile(hndlr, bufsize);
        size_t len = serializeJson(doc, bufferedFile);
        bufferedFile.flush();
        hndlr.close();
        return len;
    }


}