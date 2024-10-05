// This framework originaly based on JeeUI2 lib used under MIT License Copyright (c) 2019 Marsel Akhkamov
// then re-written and named by (c) 2020 Anton Zolotarev (obliterator) (https://github.com/anton-zolotarev)
// also many thanks to Vortigont (https://github.com/vortigont), kDn (https://github.com/DmytroKorniienko)
// and others people

#include "ui.h"

void Interface::json_frame(const char* type, const char* section_id){
    json[P_pkg] = type;
    json[P_final] = false;
    json_section_begin(section_id);
};

void Interface::json_frame_clear(){
    section_stack.clear();
    json.clear();
}

void Interface::json_frame_add(const JsonVariantConst obj){
    LOGV(P_EmbUI, printf, "Frame add obj %u items\n", obj.size());

    //(section_stack.size() ? section_stack.back().block.add<JsonObject>() : json.as<JsonObject>())
    if (!section_stack.size()) { LOGW(P_EmbUI, println, "Empty section stack!"); return; }
    if ( section_stack.back().block.add(obj) ){
        LOGV(P_EmbUI, printf, "...OK idx:%u\theap free: %u\n", section_stack.back().idx, ESP.getFreeHeap());
        section_stack.back().idx++;        // incr idx for next obj
        return;
    }

    // this is no longer valid, but I do not know why it might probably return false, so let's just send and make next frame section
    //LOGW(P_EmbUI, printf, " - Frame full! Heap free: %u\n", ESP.getFreeHeap());

    _json_frame_send();
    _json_frame_next();
}

void Interface::json_frame_flush(){
    if (!section_stack.size()) return;
    json[P_final] = true;
    json_section_end();
    LOGD(P_EmbUI, println, "json_frame_flush");
    _json_frame_send();
    json_frame_clear();
}

void Interface::_json_frame_next(){
    json.clear();
    JsonObject obj = json.to<JsonObject>();
    for ( auto i = std::next(section_stack.begin()); i != section_stack.end(); ++i ){
        obj = (*std::prev(i)).block.add<JsonObject>();
        obj[P_section] = (*i).name;
        obj[P_idx] = (*i).idx;
        (*i).block = obj[P_block].to<JsonArray>();
        //LOG(printf, "nesting section:'%s' [#%u] idx:%u\n", section_stack[i]->name.isEmpty() ? "-" : section_stack[i]->name.c_str(), i, section_stack[i]->idx);
    }
    LOGI(P_EmbUI, printf, "json_frame_next: [#%u]\n", section_stack.size()-1);   // section index counts from 0
}

void Interface::json_frame_value(const JsonVariantConst val){
    json_frame(P_value);
    json_frame_add(val);
}

void Interface::json_section_end(){
    if (!section_stack.size()) return;

    section_stack.erase(std::prev( section_stack.end() ));
    if (section_stack.size()) {
        section_stack.back().idx++;
        LOGD(P_EmbUI, printf, "section end #%u '%s'\n", section_stack.size(), section_stack.back().name.isEmpty() ? "-" : section_stack.back().name.c_str());
    }
}

void Interface::uidata_xload(const char* key, const char* url, bool merge, unsigned version){
    JsonDocument obj;
    obj[P_action] = P_xload;
    obj[P_key] = key;
    obj[P_url] = url;
    if (merge) obj[P_merge] = true;
    if (version) obj[P_version] = version;
    json_frame_add(obj);
}

void Interface::uidata_pick(const char* key, const char* prefix, const char* suffix){
    JsonDocument obj;
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
        response.getRoot()[P_block] = data[P_block];
    } else
        return;     // won't reply with non-value packets

    response.setLength();
    req->send(&response);
    flushed = true;
};

FrameSendAsyncJS::~FrameSendAsyncJS() {
    if (!flushed){
        // there were no usefull data, let's reply with empty obj
        response.setLength();
        req->send(&response);
    }
    req = nullptr;
}
