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

#define FILE_WRITE_BUFF_SIZE    512

/**
 * @brief A namespace for various functions to help working with files on LittleFS system
 * 
 */
namespace embuifs{
    /**
     *  метод загружает и пробует десериализовать джейсон из файла в предоставленный документ,
     *  возвращает true если загрузка и десериализация прошла успешно
     *  @param doc - DynamicJsonDocument куда будет загружен джейсон
     *  @param jsonfile - файл, для загрузки
     */
    bool deserializeFile(DynamicJsonDocument& doc, const char* filepath);
    bool deserializeFile(DynamicJsonDocument& doc, const String& filepath){ return deserializeFile(doc, filepath.c_str()); };

    /**
     * @brief serialize and write JsonDocument to a file using buffered writes
     * 
     * @param doc to serialize
     * @param filepath to write to
     * @return size_t bytes written
     */
    size_t serialize2file(DynamicJsonDocument& doc, const char* filepath, size_t bufsize = FILE_WRITE_BUFF_SIZE);
    size_t serialize2file(DynamicJsonDocument& doc, const String& filepath, size_t bufsize = FILE_WRITE_BUFF_SIZE){ return serialize2file(doc, filepath.c_str(), bufsize); };

}