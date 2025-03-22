/*
    This file is part of EmbUI project
    https://github.com/vortigont/EmbUI

    Copyright © 2023 Emil Muratov (Vortigont)   https://github.com/vortigont/

    EmbUI is free software: you can redistribute it and/or modify
    it under the terms of MIT License https://opensource.org/license/mit/
*/

#include "embuifs.hpp"
#include "embui_constants.h"
#include "embui_log.h"

static constexpr const char* T_load_file = "Lod file: %s\n";
static constexpr const char* T_cant_open_file = "Can't open file: %s\n";
static constexpr const char* T_deserialize_err = "failed to load json file: %s, deserialize error: %s\n";

namespace embuifs {

    size_t serialize2file(JsonVariantConst v, const String& filepath, size_t buffsize){ return serialize2file(v, filepath.c_str(), buffsize); };

    size_t serialize2file(JsonVariantConst v, const char* filepath, size_t buffsize){
        File hndlr = LittleFS.open(filepath, "w");
        WriteBufferingStream bufferedFile(hndlr, buffsize);
        size_t len = serializeJson(v, bufferedFile);
        bufferedFile.flush();
        hndlr.close();
        return len;
    }

    void obj_merge(JsonObject dst, JsonObjectConst src){
        for (JsonPairConst kvp : src){
            dst[kvp.key()] = kvp.value();
        }
    }

    void obj_deepmerge(JsonVariant dst, JsonVariantConst src){
        if (src.is<JsonObjectConst>()){
            for (JsonPairConst kvp : src.as<JsonObjectConst>()){
            if (dst[kvp.key()])
                obj_deepmerge(dst[kvp.key()], kvp.value());
            else
                dst[kvp.key()] = kvp.value();
            }
        } else
            dst.set(src);
    }

}