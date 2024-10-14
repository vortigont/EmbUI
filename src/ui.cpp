// This framework originaly based on JeeUI2 lib used under MIT License Copyright (c) 2019 Marsel Akhkamov
// then re-written and named by (c) 2020 Anton Zolotarev (obliterator) (https://github.com/anton-zolotarev)
// also many thanks to Vortigont (https://github.com/vortigont), kDn (https://github.com/DmytroKorniienko)
// and others people

#include "ui.h"

static constexpr const char* MGS_empty_stack =  "no opened section for an object!";

Interface::~Interface(){
    json_frame_clear();
    if (_delete_handler_on_destruct){
        delete send_hndl;
        send_hndl = nullptr;
    }
}

JsonArray Interface::json_block_get(){
    return section_stack.size() ? section_stack.back().block : JsonArray();
};

JsonObject Interface::json_object_get(){
    return section_stack.size() ? JsonObject (section_stack.back().block[section_stack.back().block.size()-1]) : JsonObject();
};

JsonObject Interface::json_object_add(const JsonVariantConst obj){
    LOGV(P_EmbUI, printf, "Frame obj add %u items\n", obj.size());

    //(section_stack.size() ? section_stack.back().block.add<JsonObject>() : json.as<JsonObject>())
    if (!section_stack.size()) { LOGW(P_EmbUI, println, MGS_empty_stack); return {}; }
    if ( section_stack.back().block.add(obj) ){
        LOGV(P_EmbUI, printf, "...OK idx:%u\theap free: %u\n", section_stack.back().idx, ESP.getFreeHeap());
        section_stack.back().idx++;        // incr idx for next obj
    }
    // return newly added object reference
    return json_object_get();
}

JsonObject Interface::json_frame(const char* type, const char* section_id){
    json_frame_flush();         // ensure to start a new frame purging any existing data
    json[P_pkg] = type;
    json[P_final] = false;
    json_section_begin(section_id);
    return json.as<JsonObject>();
};

void Interface::json_frame_clear(){
    section_stack.clear();
    json.clear();
}

void Interface::json_frame_flush(){
    if (!section_stack.size()) return;
    json[P_final] = true;
    json_section_end();
    LOGD(P_EmbUI, println, "json_frame_flush");
    _json_frame_send();
    json_frame_clear();
}

void Interface::json_frame_send(){
    _json_frame_send();
    _json_frame_next();
}

void Interface::_json_frame_next(){
    if (!section_stack.size()) return;
    json.clear();
    JsonObject obj = json.to<JsonObject>();
    if (section_stack.size() > 1){
        size_t idx{0};
        for ( auto i = section_stack.begin(); i != section_stack.end(); ++i ){
            if (idx++)
                obj = (*std::prev(i)).block.add<JsonObject>();
            obj[P_section] = (*i).name;
            obj[P_idx] = (*i).idx;
            (*i).block = obj[P_block].to<JsonArray>();
            //LOG(printf, "nesting section:'%s' [#%u] idx:%u\n", section_stack[i]->name.isEmpty() ? "-" : section_stack[i]->name.c_str(), i, section_stack[i]->idx);
        }
    }
    LOGI(P_EmbUI, printf, "json_frame_next: [#%d]\n", section_stack.size()-1);   // section index counts from 0
}

JsonObject Interface::json_frame_value(const JsonVariantConst val){
    json_frame_flush();     // ensure this will purge existing frame
    json_frame(P_value);
    return json_object_add(val);
}

void Interface::json_section_end(){
    if (!section_stack.size()) return;

    section_stack.erase(std::prev( section_stack.end() ));
    if (section_stack.size()) {
        section_stack.back().idx++;
        LOGD(P_EmbUI, printf, "section end #%u '%s'\n", section_stack.size(), section_stack.back().name.isEmpty() ? "-" : section_stack.back().name.c_str());
    }
}

JsonObject Interface::json_object_create(){
    if (!section_stack.size()) { LOGW(P_EmbUI, println, MGS_empty_stack); return JsonObject(); }
    section_stack.back().idx++;        // incr idx for next obj
    return section_stack.back().block.add<JsonObject>();
}


JsonObject Interface::uidata_xload(const char* key, const char* url, bool merge, unsigned version){
    JsonObject obj(json_object_create());
    obj[P_action] = P_xload;
    obj[P_key] = key;
    obj[P_url] = url;
    if (merge) obj[P_merge] = true;
    if (version) obj[P_version] = version;
    return obj;
}

JsonObject Interface::uidata_xmerge(const char* url, const char* key, const char* source){
    JsonObject obj(json_object_create());
    obj[P_action] = P_xmerge;
    obj[P_url] = url;
    obj[P_key] = key;
    obj[P_src] = source;
    return obj;
}


JsonObject Interface::uidata_pick(const char* key, const char* prefix, const char* suffix){
    JsonObject obj(json_object_create());
    obj[P_action] = P_pick;
    obj[P_key] = key;
    if (!embui_traits::is_empty_string(prefix))
        obj[P_prefix] = const_cast<char*>(prefix);
    if (!embui_traits::is_empty_string(suffix))
        obj[P_suffix] = const_cast<char*>(suffix);
    return obj;
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

    serializeJson(data, (char*)buffer->get(), length);
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

    serializeJson(data, (char*)buffer->get(), length);
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

void FrameSendChain::send(const char* data){
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

void FrameSendHttp::send(const char* data){
    AsyncWebServerResponse* r = req->beginResponse(200, asyncsrv::T_application_json, data);
    r->addHeader(asyncsrv::T_Cache_Control, asyncsrv::T_no_cache);
    req->send(r);
}

void FrameSendHttp::send(const JsonVariantConst& data) {
    AsyncResponseStream* stream = req->beginResponseStream(asyncsrv::T_application_json);
    stream->addHeader(asyncsrv::T_Cache_Control, asyncsrv::T_no_cache);
    serializeJson(data, *stream);
    req->send(stream);
};
