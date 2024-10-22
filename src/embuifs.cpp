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

static constexpr const char* T_load_file = "Lod file: %s\n";
static constexpr const char* T_cant_open_file = "Can't open file: %s\n";
static constexpr const char* T_deserialize_err = "failed to load json file: %s, deserialize error: %s\n";

namespace embuifs {

    DeserializationError deserializeFile(JsonDocument& doc, const String& filepath, size_t buffsize){ return deserializeFile(doc, filepath.c_str(), buffsize); };

    DeserializationError deserializeFile(JsonDocument& doc, const char* filepath, size_t buffsize){
        if (!filepath || !*filepath)
            return DeserializationError::Code::InvalidInput;

        LOGV(P_EmbUI, printf, T_load_file, filepath);
        File jfile = LittleFS.open(filepath);

        if (!jfile){
            LOGD(P_EmbUI, printf, T_cant_open_file, filepath);
            return DeserializationError::Code::InvalidInput;
        }

        ReadBufferingStream bufferingStream(jfile, buffsize);

        DeserializationError error = deserializeJson(doc, jfile);

        if (!error) return error;
        LOGE(P_EmbUI, printf, T_deserialize_err, filepath, error.c_str());
        return error;
    }

    size_t serialize2file(const JsonDocument& doc, const String& filepath, size_t buffsize){ return serialize2file(doc, filepath.c_str(), buffsize); };

    size_t serialize2file(const JsonDocument& doc, const char* filepath, size_t buffsize){
        File hndlr = LittleFS.open(filepath, "w");
        WriteBufferingStream bufferedFile(hndlr, buffsize);
        size_t len = serializeJson(doc, bufferedFile);
        bufferedFile.flush();
        hndlr.close();
        return len;
    }


    DeserializationError deserializeFileWFilter(JsonDocument& doc, const char* filepath, JsonDocument& filter, size_t buffsize){
        if (!filepath || !*filepath)
            return DeserializationError::Code::InvalidInput;

        LOGV(P_EmbUI, printf, T_load_file, filepath);
        File jfile = LittleFS.open(filepath);

        if (!jfile){
            LOGD(P_EmbUI, printf, T_cant_open_file, filepath);
            return DeserializationError::Code::InvalidInput;
        }

        ReadBufferingStream bufferingStream(jfile, buffsize);

        DeserializationError error = deserializeJson(doc, jfile, DeserializationOption::Filter(filter));

        if (!error) return error;
        LOGE(P_EmbUI, printf, T_deserialize_err, filepath, error.c_str());
        return error;
    }

    DeserializationError deserializeFileWFilter(JsonDocument& doc, const String& filepath, JsonDocument& filter, size_t buffsize){ return deserializeFileWFilter(doc, filepath, filter, buffsize); };

}