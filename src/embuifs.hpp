/*
    This file is part of EmbUI project
    https://github.com/vortigont/EmbUI

    Copyright © 2023 Emil Muratov (Vortigont)   https://github.com/vortigont/

    EmbUI is free software: you can redistribute it and/or modify
    it under the terms of MIT License https://opensource.org/license/mit/
*/

#pragma once

#include <ArduinoJson.h>
#include <LittleFS.h>
#include "StreamUtils.h"

#define EMBUIFS_FILE_WRITE_BUFF_SIZE    256

/**
 * @brief A namespace for various functions to help working with files on LittleFS system
 * 
 */
namespace embuifs{
    /**
     *  метод загружает и пробует десериализовать джейсон из файла в предоставленный документ,
     *  возвращает true если загрузка и десериализация прошла успешно
     *  @param doc - JsonDocument куда будет загружен джейсон
     *  @param jsonfile - файл, для загрузки
     */
    template <typename TDestination>
    DeserializationError deserializeFile(TDestination&& dst, const char* filepath, size_t buffsize = EMBUIFS_FILE_WRITE_BUFF_SIZE){
        if (!filepath || !*filepath)
            return DeserializationError::Code::InvalidInput;

        //LOGV(P_EmbUI, printf, T_load_file, filepath);
        File jfile = LittleFS.open(filepath);

        if (!jfile){
            //LOGD(P_EmbUI, printf, T_cant_open_file, filepath);
            return DeserializationError::Code::InvalidInput;
        }

        ReadBufferingStream bufferingStream(jfile, buffsize);
        return deserializeJson(dst, bufferingStream);
        /*
        DeserializationError error = deserializeJson(doc, bufferingStream);
        if (!error) return error;
        LOGE(P_EmbUI, printf, T_deserialize_err, filepath, error.c_str());
        return error;
        */
    }

    /**
     *  метод загружает и пробует десериализовать джейсон из файла в предоставленный документ,
     *  возвращает true если загрузка и десериализация прошла успешно
     *  https://github.com/bblanchon/ArduinoJson/issues/2072
     *  https://arduinojson.org/v7/how-to/deserialize-a-very-large-document/#deserialization-in-chunks
     *  https://github.com/mrfaptastic/json-streaming-parser2
     * 
     *  @param doc - JsonDocument куда будет загружен джейсон
     *  @param jsonfile - файл, для загрузки
     */
    template <typename TDestination>
    DeserializationError deserializeFileWFilter(TDestination&& dst, const char* filepath, JsonDocument& filter, size_t buffsize = EMBUIFS_FILE_WRITE_BUFF_SIZE){
        if (!filepath || !*filepath)
            return DeserializationError::Code::InvalidInput;

        //LOGV(P_EmbUI, printf, T_load_file, filepath);
        File jfile = LittleFS.open(filepath);

        if (!jfile){
            //LOGD(P_EmbUI, printf, T_cant_open_file, filepath);
            return DeserializationError::Code::InvalidInput;
        }

        ReadBufferingStream bufferingStream(jfile, buffsize);
        return deserializeJson(dst, jfile, DeserializationOption::Filter(filter));
        /*
        DeserializationError error = deserializeJson(doc, jfile, DeserializationOption::Filter(filter));
        if (!error) return error;
        LOGE(P_EmbUI, printf, T_deserialize_err, filepath, error.c_str());
        return error;
        */
    };

    /**
     * @brief serialize and write JsonDocument to a file using buffered writes
     * 
     * @param doc to serialize
     * @param filepath to write to
     * @return size_t bytes written
     */
    size_t serialize2file(JsonVariantConst v, const char* filepath, size_t buffsize = EMBUIFS_FILE_WRITE_BUFF_SIZE);

    /**
     * @brief shallow merge objects
     * from https://arduinojson.org/v6/how-to/merge-json-objects/
     * 
     * @param dst 
     * @param src 
     */
    void obj_merge(JsonObject dst, JsonObjectConst src);

    /**
     * @brief deep merge objects
     * from https://arduinojson.org/v6/how-to/merge-json-objects/
     * 
     * @param dst 
     * @param src 
     */
    void obj_deepmerge(JsonVariant dst, JsonVariantConst src);
}