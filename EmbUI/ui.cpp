// This framework originaly based on JeeUI2 lib used under MIT License Copyright (c) 2019 Marsel Akhkamov
// then re-written and named by (c) 2020 Anton Zolotarev (obliterator) (https://github.com/anton-zolotarev)
// also many thanks to Vortigont (https://github.com/vortigont), kDn (https://github.com/DmytroKorniienko)
// and others people

#include "ui.h"

void Interface::frame(const String &id, const String &value){
    StaticJsonDocument<IFACE_STA_JSON_SIZE> obj;
    obj[FPSTR(P_html)] = F("iframe");
    obj[FPSTR(P_type)] = F("frame");
    obj[FPSTR(P_id)] = id;
    obj[FPSTR(P_value)] = value;

    frame_add_safe(obj.as<JsonObject>());
}

void Interface::frame2(const String &id, const String &value){
    StaticJsonDocument<IFACE_STA_JSON_SIZE> obj;
    obj[FPSTR(P_html)] = F("iframe2");;
    obj[FPSTR(P_type)] = F("frame");
    obj[FPSTR(P_id)] = id;
    obj[FPSTR(P_value)] = value;

    frame_add_safe(obj.as<JsonObject>());
}

void Interface::constant(const String &id, const String &value, const String &label){
    StaticJsonDocument<IFACE_STA_JSON_SIZE> obj;
    obj[FPSTR(P_html)] = F("const");
    obj[FPSTR(P_id)] = id;
    obj[FPSTR(P_value)] = value;
    obj[FPSTR(P_label)] = label;

    frame_add_safe(obj.as<JsonObject>());
}

void Interface::file(const String &name, const String &action, const String &label){
    StaticJsonDocument<IFACE_STA_JSON_SIZE> obj;
    obj[FPSTR(P_html)] = FPSTR(P_file);
    obj[F("name")] = name;
    obj[F("action")] = action;
    obj[FPSTR(P_label)] = label;

    frame_add_safe(obj.as<JsonObject>());
}

void Interface::spacer(const String &label){
    StaticJsonDocument<IFACE_STA_JSON_SIZE> obj;
    obj[FPSTR(P_html)] = F("spacer");
    if (label.length()) obj[FPSTR(P_label)] = label;

    frame_add_safe(obj.as<JsonObject>());
}

void Interface::comment(const String &label){
    StaticJsonDocument<IFACE_STA_JSON_SIZE * 2> obj;
    obj[FPSTR(P_html)] = F("comment");
    if (label.length()) obj[FPSTR(P_label)] = label;

    frame_add_safe(obj.as<JsonObject>());
}


///////////////////////////////////////

/**
 * @brief - begin UI secton of the specified <type>
 * generic frame creation method, used by other calls to create pther custom-typed frames 
 */
void Interface::json_frame(const String &type){
    json[F("pkg")] = type;
    json[FPSTR(P_final)] = false;

    json_section_begin("root" + String(micros()));
}


/**
 * @brief - begin Interface UI secton
 * used to construc WebUI html elements
 */
void Interface::json_frame_interface(const String &name){
    json[F("app")] = name;
    json[F("mc")] = embui->mc;
    json[F("ver")] = F(EMBUI_VERSION_STRING);

    json_frame_interface();
}


bool Interface::json_frame_add(const JsonObject &obj) {
    if (!obj.memoryUsage()) // пустышки не передаем
        return false;

    LOG(printf_P, PSTR("UI: Frame add obj %u b, storage %d/%d"), obj.memoryUsage(), json.memoryUsage(), json.capacity());

    if (json.capacity() - json.memoryUsage() > obj.memoryUsage() + 16 && section_stack.end()->block.add(obj)) {
        section_stack.end()->idx++;
        LOG(printf_P, PSTR("...OK [%u]\tMEM: %u\n"), section_stack.end()->idx, ESP.getFreeHeap());
        return true;
    }
    LOG(printf_P, PSTR(" - Frame full! Heap: %u\n"), ESP.getFreeHeap());

    json_frame_send();
    json_frame_next();
    return false;
}

void Interface::json_frame_next(){
    json.clear();
    JsonObject obj = json.to<JsonObject>();
    for (int i = 0; i < section_stack.size(); i++) {
        if (i) obj = section_stack[i - 1]->block.createNestedObject();
        obj[FPSTR(P_section)] = section_stack[i]->name;
        obj[F("idx")] = section_stack[i]->idx;
        LOG(printf_P, PSTR("UI: section %u %s %u\n"), i, section_stack[i]->name.c_str(), section_stack[i]->idx);
        section_stack[i]->block = obj.createNestedArray(FPSTR(P_block));
    }
    LOG(printf_P, PSTR("json_frame_next: [%u], used %u, free %u\n"), section_stack.size(), obj.memoryUsage(), json.capacity() - json.memoryUsage());
}

void Interface::json_frame_clear(){
    while(section_stack.size()) {
        section_stack_t *section = section_stack.shift();
        delete section;
    }
    json.clear();
}

void Interface::json_frame_flush(){
    if (!section_stack.size()) return;
    LOG(println, F("UI: json_frame_flush"));
    json[FPSTR(P_final)] = true;
    json_section_end();
    json_frame_send();
    json_frame_clear();
}

void Interface::json_section_line(const String &name){
    json_section_begin(name, "", false, false, true);
}

void Interface::json_section_begin(const String &name, const String &label, bool main, bool hidden, bool line){
    JsonObject obj;
    if (section_stack.size()) {
        obj = section_stack.end()->block.createNestedObject();
    } else {
        obj = json.as<JsonObject>();
    }
    json_section_begin(name, label, main, hidden, line, obj);
}

void Interface::json_section_begin(const String &name, const String &label, bool main, bool hidden, bool line, JsonObject obj){
    obj[FPSTR(P_section)] = name;
    if (label != "") obj[FPSTR(P_label)] = label;
    if (main) obj[F("main")] = true;
    if (hidden) obj[FPSTR(P_hidden)] = true;
    if (line) obj[F("line")] = true;

    section_stack_t *section = new section_stack_t;
    section->name = name;
    section->block = obj.createNestedArray(FPSTR(P_block));
    section->idx = 0;
    section_stack.add(section);
    LOG(printf_P, PSTR("UI: section begin %s [%u] %u free\n"), name.c_str(), section_stack.size(), json.capacity() - json.memoryUsage());
}

void Interface::json_section_end(){
    if (!section_stack.size()) return;

    section_stack_t *section = section_stack.pop();
    if (section_stack.size()) {
        section_stack.end()->idx++;
    }
    LOG(printf_P, PSTR("UI: section end %s [%u] MEM: %u\n"), section->name.c_str(), section_stack.size(), ESP.getFreeHeap());
    delete section;
}

/**
 * @brief - serialize and send json obj directly to the ws buffer
 */
void frameSendAll::send(const JsonObject& data){
    size_t length = measureJson(data);
    AsyncWebSocketMessageBuffer * buffer = ws->makeBuffer(length);
    if (!buffer)
        return;

    serializeJson(data, (char*)buffer->get(), ++length);
    ws->textAll(buffer);
};

/**
 * @brief - serialize and send json obj directly to the ws buffer
 */
void frameSendClient::send(const JsonObject& data){
    size_t length = measureJson(data);
    AsyncWebSocketMessageBuffer * buffer = cl->server()->makeBuffer(length);
    if (!buffer)
        return;

    serializeJson(data, (char*)buffer->get(), ++length);
    cl->text(buffer);
};


/**
 * @brief - add object to frame with mem overflow protection 
 */
void Interface::frame_add_safe(const JsonObject &jobj){
    size_t _cnt = FRAME_ADD_RETRY;

    do {
        --_cnt;
        #ifdef EMBUI_DEBUG
            if (!_cnt)
                LOG(println, FPSTR(P_ERR_obj2large));
        #endif
    } while (!json_frame_add(jobj) && _cnt );
};
