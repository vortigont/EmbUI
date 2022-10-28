// This framework originaly based on JeeUI2 lib used under MIT License Copyright (c) 2019 Marsel Akhkamov
// then re-written and named by (c) 2020 Anton Zolotarev (obliterator) (https://github.com/anton-zolotarev)
// also many thanks to Vortigont (https://github.com/vortigont), kDn (https://github.com/DmytroKorniienko)
// and others people

#include "ui.h"

#define FRAME_ADD_RETRY 3


void Interface::comment(const String &id, const String &label){
    UIelement<UI_DEFAULT_JSON_SIZE * 2> ui(ui_element_t::comment, id);     // use a bit larger buffer for long texts
    ui.obj[FPSTR(P_label)] = label;

    json_frame_add(ui);
}

void Interface::file_form(const String &id, const String &action, const String &label, const String &opt){
    UIelement<TINY_JSON_SIZE> ui(ui_element_t::file, id);
    ui.obj[FPSTR(P_type)] = FPSTR(P_file);
    ui.obj[FPSTR(P_action)] = action;
    ui.obj[FPSTR(P_label)] = label;
    if (opt.length()) ui.obj["opt"] = opt;
    json_frame_add(ui);
}

void Interface::iframe(const String &id, const String &value){
    UIelement<UI_DEFAULT_JSON_SIZE> ui(ui_element_t::iframe, id);
    ui.obj[FPSTR(P_type)] = FPSTR(P_frame);
    ui.obj[FPSTR(P_value)] = value;
    json_frame_add(ui);
}

/*
void Interface::frame2(const String &id, const String &value){
    StaticJsonDocument<UI_DEFAULT_JSON_SIZE> obj;
    obj[FPSTR(P_html)] = F("iframe2");;
    obj[FPSTR(P_type)] = F("frame");
    obj[FPSTR(P_id)] = id;
    obj[FPSTR(P_value)] = value;

    json_frame_add(obj.as<JsonObject>());
}
*/


void Interface::spacer(const String &label){
    UIelement<TINY_JSON_SIZE> ui(ui_element_t::spacer);
    if (label.length()) ui.obj[FPSTR(P_label)] = label;
    json_frame_add(ui);
}

void Interface::textarea(const String &id, const String &value, const String &label){
    UIelement<UI_DEFAULT_JSON_SIZE> ui(ui_element_t::textarea, id, value, label);
    json_frame_add(ui);
};

///////////////////////////////////////

/**
 * @brief - begin UI secton of the specified <type>
 * generic frame creation method, used by other calls to create pther custom-typed frames 
 */
void Interface::json_frame(const String &type){
    json[F("pkg")] = type;
    json[FPSTR(P_final)] = false;

    json_section_begin(String(micros()));
}


/**
 * @brief - begin Interface UI secton
 * used to construc WebUI html elements
 */
void Interface::json_frame_interface(const String &name){
    json[F("app")] = name;
    json[F("mc")] = embui->macid();
    json[F("ver")] = F(EMBUI_VERSION_STRING);

    json_frame_interface();
}


bool Interface::json_frame_enqueue(const JsonObject &obj) {
    if (!obj.memoryUsage()) // пустышки не передаем
        return false;

    LOG(printf_P, PSTR("UI: Frame add obj %u b, mem:%d/%d"), obj.memoryUsage(), json.memoryUsage(), json.capacity());

    if (json.capacity() - json.memoryUsage() > obj.memoryUsage() + 16 && section_stack.tail()->block.add(obj)) {
        LOG(printf_P, PSTR("...OK idx:%u\tMEM: %u\n"), section_stack.tail()->idx, ESP.getFreeHeap());
        section_stack.tail()->idx++;        // incr idx for next obj
        return true;
    }
    LOG(printf_P, PSTR(" - Frame full! Heap: %u\n"), ESP.getFreeHeap());

    json_frame_send();
    json_frame_next();
    return false;
}

/**
 * @brief - add object to frame with mem overflow protection 
 */
void Interface::json_frame_add(const JsonObject &jobj){
    size_t _cnt = FRAME_ADD_RETRY;

    do {
        --_cnt;
        #ifdef EMBUI_DEBUG
            if (!_cnt)
                LOG(println, FPSTR(P_ERR_obj2large));
        #endif
    } while (!json_frame_enqueue(jobj) && _cnt );
};

void Interface::json_frame_next(){
    json.clear();
    JsonObject obj = json.to<JsonObject>();
    for (unsigned i = 0; i < section_stack.size(); i++) {
        if (i) obj = section_stack[i - 1]->block.createNestedObject();
        obj[FPSTR(P_section)] = section_stack[i]->name;
        obj[F("idx")] = section_stack[i]->idx;
        LOG(printf_P, PSTR("UI: section:'%s' [#%u] idx:%u\n"), section_stack[i]->name.c_str(), i, section_stack[i]->idx);
        section_stack[i]->block = obj.createNestedArray(FPSTR(P_block));
    }
    LOG(printf_P, PSTR("json_frame_next: [#%u], mem:%u/%u\n"), section_stack.size()-1, obj.memoryUsage(), json.capacity());   // section index counts from 0
}

void Interface::json_frame_clear(){
    while (section_stack.size())
        delete section_stack.shift();

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

void Interface::json_section_begin(const String &name, const String &label, bool main, bool hidden, bool line){
    JsonObject obj;
    if (section_stack.size()) {
        obj = section_stack.tail()->block.createNestedObject();
    } else {
        obj = json.as<JsonObject>();
    }
    json_section_begin(name, label, main, hidden, line, obj);
}

void Interface::json_section_begin(const String &name, const String &label, bool main, bool hidden, bool line, JsonObject obj){
    obj[FPSTR(P_section)] = name;
    if (!label.isEmpty()) obj[FPSTR(P_label)] = label;
    if (main) obj[F("main")] = true;
    if (hidden) obj[FPSTR(P_hidden)] = true;
    if (line) obj[F("line")] = true;

    section_stack_t *section = new section_stack_t;
    section->name = name;
    section->block = obj.createNestedArray(FPSTR(P_block));
    section->idx = 0;
    section_stack.add(section);
    LOG(printf_P, PSTR("UI: section begin:'%s' [#%u] %u free\n"), name.isEmpty() ? "" : name.c_str(), section_stack.size()-1, json.capacity() - json.memoryUsage());   // section index counts from 0
}

void Interface::json_section_end(){
    if (!section_stack.size()) return;

    section_stack_t *section = section_stack.pop();
    if (section_stack.size()) {
        section_stack.tail()->idx++;
    }
    LOG(printf_P, PSTR("UI: section end:'%s' [#%u] MEM: %u\n"), section->name.isEmpty() ? "" : section->name.c_str(), section_stack.size(), ESP.getFreeHeap());        // size() before pop()
    delete section;
}

void Interface::json_section_extend(const String &name){
    section_stack.tail()->idx--;
    // open new nested section
    json_section_begin(name, (char*)0, false, false, false, section_stack.tail()->block.getElement(section_stack.tail()->block.size()-1));    // find last array element
};

/**
 * @brief - serialize and send json obj directly to the ws buffer
 */
void frameSendAll::send(const JsonObject& data){
    size_t length = measureJson(data);
    AsyncWebSocketMessageBuffer * buffer = ws->makeBuffer(length);
    if (!buffer)
        return;

    serializeJson(data, (char*)buffer->get(), length);
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

    serializeJson(data, (char*)buffer->get(), length);
    cl->text(buffer);
};
