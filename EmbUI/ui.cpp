// This framework originaly based on JeeUI2 lib used under MIT License Copyright (c) 2019 Marsel Akhkamov
// then re-written and named by (c) 2020 Anton Zolotarev (obliterator) (https://github.com/anton-zolotarev)
// also many thanks to Vortigont (https://github.com/vortigont), kDn (https://github.com/DmytroKorniienko)
// and others people

#include "ui.h"

#define FRAME_ADD_RETRY 3

///////////////////////////////////////

/**
 * @brief - add object to frame with mem overflow protection 
 */
void Interface::json_frame_add(const JsonVariantConst &jobj){
    size_t _cnt = FRAME_ADD_RETRY;

    do {
        --_cnt;
        #ifdef EMBUI_DEBUG
            if (!_cnt)
                LOGE(P_EmbUI, println, P_ERR_obj2large);
        #endif
    } while (!json_frame_enqueue(jobj) && _cnt );
};

void Interface::json_frame_clear(){
    while (section_stack.size())
        delete section_stack.shift();

    json.clear();
}

bool Interface::json_frame_enqueue(const JsonVariantConst &obj, bool shallow){
    if(shallow){
        LOGD(P_EmbUI, printf, "Frame add shallow obj %u b, mem:%d/%d\n", obj.memoryUsage(), json.memoryUsage(), json.capacity());
        JsonVariant nested = section_stack.tail()->block.createNestedObject();
        nested.shallowCopy(obj);
        return true;
    }

    LOGV(P_EmbUI, printf, "Frame add obj %u b, mem:%d/%d\n", obj.memoryUsage(), json.memoryUsage(), json.capacity());

    if ( ( json.capacity() - json.memoryUsage() > obj.memoryUsage() + 16 ) && section_stack.tail()->block.add(obj)) {
        LOGV(P_EmbUI, printf, "...OK idx:%u\theap free: %u\n", section_stack.tail()->idx, ESP.getFreeHeap());
        section_stack.tail()->idx++;        // incr idx for next obj
        return true;
    }
    LOGW(P_EmbUI, printf, " - Frame full! Heap free: %u\n", ESP.getFreeHeap());

    json_frame_send();
    json_frame_next();
    return false;
}

void Interface::json_frame_flush(){
    if (!section_stack.size()) return;
    json[P_final] = true;
    json_section_end();
    LOGD(P_EmbUI, println, "json_frame_flush");
    json_frame_send();
    json_frame_clear();
}

void Interface::json_frame_next(){
    json.clear();
    JsonObject obj = json.to<JsonObject>();
    for (unsigned i = 0; i < section_stack.size(); i++) {
        if (i) obj = section_stack[i - 1]->block.createNestedObject();
        obj[P_section] = section_stack[i]->name;
        obj["idx"] = section_stack[i]->idx;
        section_stack[i]->block = obj.createNestedArray(P_block);
        //LOG(printf, "nesting section:'%s' [#%u] idx:%u\n", section_stack[i]->name.isEmpty() ? "-" : section_stack[i]->name.c_str(), i, section_stack[i]->idx);
    }
    LOGI(P_EmbUI, printf, "json_frame_next: [#%u], mem:%u/%u\n", section_stack.size()-1, obj.memoryUsage(), json.capacity());   // section index counts from 0
}

void Interface::json_frame_value(const JsonVariant val, bool shallow){
    json_frame(P_value);
    if (shallow)
        json_frame_enqueue(val, shallow);
    else
        json_frame_add(val);
}

void Interface::json_section_end(){
    if (!section_stack.size()) return;

    section_stack_t *section = section_stack.pop();
    if (section_stack.size()) {
        section_stack.tail()->idx++;
    }
    LOGD(P_EmbUI, printf, "section end #%u '%s'\n", section_stack.size(), section->name.isEmpty() ? "-" : section->name.c_str(), ESP.getFreeHeap());        // size() before pop()
    delete section;
}

JsonObject Interface::get_last_object(){
    return JsonObject (section_stack.tail()->block[section_stack.tail()->block.size()-1]);    // find last array element and return it as an Jobject
}

void Interface::uidata_xload(const char* key, const char* url, bool merge, unsigned version){
    StaticJsonDocument<UI_DEFAULT_JSON_SIZE> obj;
    obj[P_action] = P_xload;
    obj[P_key] = key;
    obj[P_url] = url;
    if (merge) obj[P_merge] = true;
    if (version) obj[P_version] = version;
    json_frame_add(obj);
}

void Interface::uidata_pick(const char* key, const char* prefix, const char* suffix){
    StaticJsonDocument<UI_DEFAULT_JSON_SIZE> obj;
    obj[P_action] = P_pick;
    obj[P_key] = key;
    if (!embui_traits::is_empty_string(prefix))
        obj[P_prefix] = const_cast<char*>(prefix);
    if (!embui_traits::is_empty_string(suffix))
        obj[P_suffix] = const_cast<char*>(suffix);
    json_frame_add(obj);
}

/**
 * @brief - serialize and send json obj directly to the ws buffer
 */
void FrameSendWSServer::send(const JsonVariantConst& data){
    if (!available()) { LOGW(P_EmbUI, println, "FrameSendWSServer::send - not available!"); return; }   // no need to do anything if there is no clients connected

    size_t length = measureJson(data);
    auto buffer = ws->makeBuffer(length);
    if (!buffer){
        LOGW(P_EmbUI, println, "FrameSendWSServer::send - no buffer!");
        return;
    }

#ifndef YUBOXMOD
    serializeJson(data, (char*)buffer->get(), length);
#else
    serializeJson(data, (char*)buffer->data() , length);
#endif

    ws->textAll(buffer);
};

/**
 * @brief - serialize and send json obj directly to the ws buffer
 */
void FrameSendWSClient::send(const JsonVariantConst& data){
    if (!available()) return;   // no need to do anything if there is no clients connected

    size_t length = measureJson(data);
    auto buffer = cl->server()->makeBuffer(length);
    if (!buffer)
        return;

#ifndef YUBOXMOD
    serializeJson(data, (char*)buffer->get(), length);
#else
    serializeJson(data, (char*)buffer->data(), length);
#endif
    cl->text(buffer);
};

void FrameSendChain::remove(int id){
    _hndlr_chain.remove_if([id](HndlrChain &c){ return id == c.id; });
};

bool FrameSendChain::available() const {
    for (const auto &i : _hndlr_chain)
        if (i.handler->available()) return true;

    return false;
};


int FrameSendChain::add(std::unique_ptr<FrameSend>&& handler){
    _hndlr_chain.emplace_back(std::forward<std::unique_ptr<FrameSend>>(handler));
    return _hndlr_chain.back().id;
}

void FrameSendChain::send(const JsonVariantConst& data){
    for (auto &i : _hndlr_chain)
        i.handler->send(data);
}

void FrameSendChain::send(const String& data){
    for (auto &i : _hndlr_chain)
        i.handler->send(data);
}

void FrameSendAsyncJS::send(const JsonVariantConst& data){
    if (flushed) return;    // we can send only ONCE!

    if (data[P_pkg] == P_value){
        JsonVariant& root = response->getRoot();
        root.shallowCopy(data[P_block]);
    } else
        return;     // won't reply with non-value packets

    response->setLength();
    req->send(response);
    flushed = true;
};

FrameSendAsyncJS::~FrameSendAsyncJS() {
    if (!flushed){
        // there were no usefull data, let's reply with empty obj
        response->setLength();
        req->send(response);        
    }
    req = nullptr;
}
