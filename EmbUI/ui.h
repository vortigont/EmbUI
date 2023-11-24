// This framework originaly based on JeeUI2 lib used under MIT License Copyright (c) 2019 Marsel Akhkamov
// then re-written and named by (c) 2020 Anton Zolotarev (obliterator) (https://github.com/anton-zolotarev)
// also many thanks to Vortigont (https://github.com/vortigont), kDn (https://github.com/DmytroKorniienko)
// and others people

#pragma once

#include "globals.h"
#include "traits.hpp"
#include <list>
#include "LList.h"
#include "AsyncJson.h"

// static json obj size for tiny ui elements, like checkboxes, number inputs, etc...
#ifndef TINY_JSON_SIZE
#define TINY_JSON_SIZE      256
#endif

// static json obj size for small ui elements
#ifndef SMALL_JSON_SIZE
#define SMALL_JSON_SIZE     512
#endif

// static json obj size for small ui elements
#ifndef MEDIUM_JSON_SIZE
#define MEDIUM_JSON_SIZE     1024
#endif

// dynamic json for creating websocket frames to be sent to UI
#ifndef IFACE_DYN_JSON_SIZE
#define IFACE_DYN_JSON_SIZE 4096
#endif

// static json doc size
#define UI_DEFAULT_JSON_SIZE SMALL_JSON_SIZE

template<typename TString>
using ValidStringRef_t = std::enable_if_t<embui_traits::is_string_obj_v<TString>, void>;

template<typename TChar>
using ValidCharPtr_t = std::enable_if_t<embui_traits::is_string_ptr_v<TChar*>, void>;

template<typename TString>
using ValidString_t = std::enable_if_t<embui_traits::is_string_v<TString>, void>;

/**
 * @brief a list of available ui elenets
 * changes to this list order MUST be reflected to 'static const char *const UI_T_DICT' in constants.h
 * 
 */
enum class ui_element_t:uint8_t {
    custom = 0,
    button,
    checkbox,
    color,
    comment,
    constant,
    date,
    datetime,
    display,
    div,
    email,      // 10
    file,
    form,
    hidden,
    iframe,
    input,
    option,
    password,
    range,
    select,
    spacer,     // 20
    text,
    textarea,
    time,
    value
};

enum class ui_param_t:uint8_t {
    html = 0,
    id,
    hidden,
    type,
    value
};

// UI Button type enum
enum class button_t:uint8_t {
    generic = (0U),
    submit,
    js,
    href
};

template <size_t capacity = UI_DEFAULT_JSON_SIZE>
class UIelement {
protected:
    ui_element_t _t;

    // set UI html type, if any
    void set_html_type(ui_element_t t){
        // check if element type is within allowed range
        if (static_cast<uint8_t>(t) < UI_T_DICT.size()){
            switch(t){
                case ui_element_t::custom :     // some elements does not need html_type
                case ui_element_t::option :
                case ui_element_t::value :
                    return;
                default :                       // default is to set element type from dict
                    obj[P_html] = UI_T_DICT[static_cast<uint8_t>(t)];
            }
        }
    }

public:
    StaticJsonDocument<capacity> obj;

    // c-tor with ID by value
    template <typename T>
    UIelement(ui_element_t t, const T id, typename std::enable_if<embui_traits::is_string_ptr<T>::value, T>::type* = 0) : _t(t) {
        if (!embui_traits::is_empty_string(id))
            obj[P_id] = id;
        //else
        //    obj[P_id] = P_EMPTY;

        set_html_type(t);
    };

    // c-tor with ID by ref
    template <typename T>
    UIelement(ui_element_t t, const T& id, typename std::enable_if<embui_traits::is_string_obj<T>::value, T>::type* = 0) : _t(t) {
        if (!embui_traits::is_empty_string(id))
            obj[P_id] = id;
        //else
        //    obj[P_id] = P_EMPTY;

        set_html_type(t);
    };

    UIelement(ui_element_t t) : UIelement(t, P_EMPTY) {};

    template <typename ID, typename V>
    UIelement(ui_element_t t, const ID id, const V value) : UIelement(t, id ){
        obj[P_value] = value;
    };

    template <typename T>
        typename std::enable_if<std::is_fundamental_v<T>, void>::type
    param(ui_param_t key, const T value){ obj[UI_KEY_DICT[static_cast<uint8_t>(key)]] = value; };

    template <typename T>
        typename std::enable_if<!std::is_fundamental_v<T>, void>::type
    param(ui_param_t key, const T& value){ obj[UI_KEY_DICT[static_cast<uint8_t>(key)]] = value; };

    template <typename T>
        typename std::enable_if<!std::is_fundamental_v<T>, void>::type
    param(ui_param_t key, T* value){ obj[UI_KEY_DICT[static_cast<uint8_t>(key)]] = value; };

    /**
     * @brief set 'html' flag for element
     * if set, than value updated for {{}} placeholders in html template, otherwise within dynamicaly created elements on page
     * 
     * @param v boolen
     */
    void html(bool v = true){ if (v) obj[P_html] = v; else obj.remove(P_html); }         // tend to null pattern

    /**
     * @brief add 'label' key to the UI element
     * 
     * @tparam T label literal type
     * @param label 
     */
    template <typename T>
        ValidCharPtr_t<T>
    label(const T* string){ if (!embui_traits::is_empty_string(string)) obj[P_label] = string; }

    template <typename T>
        ValidStringRef_t<T>
    label(const T& string){ if (!embui_traits::is_empty_string(string)) obj[P_label] = string; }

    /**
     * @brief add 'color' key to the UI element
     * 
     * @tparam T color literal type
     * @param label 
     */
    template <typename T>
        ValidStringRef_t<T>
    color(const T& color){ if (!embui_traits::is_empty_string(color)) obj[P_color] = color; }

    template <typename T>
        ValidCharPtr_t<T>
    color(const T* color){ if (!embui_traits::is_empty_string(color)) obj[P_color] = color; }

    /**
     * @brief add 'value' key to the UI element
     * 
     */
    template <typename T>
        typename std::enable_if<std::is_fundamental_v<T>, void>::type
    value(const T v){ obj[P_value] = v; }

    template <typename T>
    void value(const T* v){ obj[P_value] = v; }

    template <typename T>
        typename std::enable_if<!std::is_fundamental_v<T>, void>::type
    value(const T& v){ obj[P_value] = v; }

};

template <size_t S = UI_DEFAULT_JSON_SIZE>
class UI_button : public UIelement<S> {
public:
    using UIelement<S>::value;
    using UIelement<S>::color;

    template <typename T, typename L>
    UI_button(button_t btype, const T* id, const L label) : UIelement<S>(ui_element_t::button, id) {
        UIelement<S>::obj[P_type] = static_cast<uint8_t>(btype);
        UIelement<S>::label(label);
    };

    template <typename T, typename L>
    UI_button(button_t btype, const T& id, const L label) : UIelement<S>(ui_element_t::button, id) {
        UIelement<S>::obj[P_type] = static_cast<uint8_t>(btype);
        UIelement<S>::label(label);
    };
};

class FrameSend {
    public:
        /**
         * @brief should return 'true' if downlevel protol is availbale
         * i.e. a connection is ready to send data, WebSocket subsribers are available, etc...
         */
        virtual bool available() const = 0;
        virtual ~FrameSend(){ };
        virtual void send(const String &data) = 0;
        virtual void send(const JsonVariantConst& data) = 0;
        //virtual void flush(){};
};

class FrameSendWSServer: public FrameSend {
    private:
        AsyncWebSocket *ws;
    public:
        FrameSendWSServer(AsyncWebSocket *server) : ws(server){}
        ~FrameSendWSServer() { ws = nullptr; }
        bool available() const override { return ws->count(); }
        void send(const String &data) override { if (!data.isEmpty()) ws->textAll(data); };
        void send(const JsonVariantConst& data) override;
};

class FrameSendWSClient: public FrameSend {
    private:
        AsyncWebSocketClient *cl;
    public:
        FrameSendWSClient(AsyncWebSocketClient *client) : cl(client){}
        ~FrameSendWSClient() { cl = nullptr; }
        bool available() const override { return cl->status() == WS_CONNECTED; }
        void send(const String &data) override { if (!data.isEmpty()) cl->text(data); };
        /**
         * @brief - serialize and send json obj directly to the ws buffer
         */
        void send(const JsonVariantConst& data) override;
};

class FrameSendHttp: public FrameSend {
    private:
        AsyncWebServerRequest *req;
        AsyncResponseStream *stream;
    public:
        FrameSendHttp(AsyncWebServerRequest *request) : req(request) {
            stream = req->beginResponseStream(PGmimejson);
            stream->addHeader(PGhdrcachec, PGnocache);
        }
        ~FrameSendHttp() { /* delete stream; */ req = nullptr; }
        void send(const String &data) override {
            if (!data.length()) return;
            stream->print(data);
        };
        /**
         * @brief - serialize and send json obj directly to the ws buffer
         */
        void send(const JsonVariantConst& data) override {
            serializeJson(data, *stream);
            req->send(stream);
        };
        bool available() const override { return true; }
};

struct HndlrChain {
    int id;
    std::unique_ptr<FrameSend> handler;
    HndlrChain() = delete;
    HndlrChain(const HndlrChain& rhs) = delete;
    HndlrChain(std::unique_ptr<FrameSend>&& rhs) noexcept : id(std::rand()), handler(std::move(rhs)) {};
};

class FrameSendChain : public FrameSend {
    // a list of send handlers
    std::list<HndlrChain> _hndlr_chain;

    public:

    bool available() const override;

    /**
     * @brief add FrameSend object to the chain
     * object WILL be moved and invalidated
     * 
     * @param rhs FrameSend object 
     * @return int unique id for the object in a chain (could be used to remove handler later via remove() )
     */
    int add(std::unique_ptr<FrameSend>&& handler);

    /**
     * @brief removes handler with specified id from the chain, if exist
     * 
     * @param id handler id to remove
     * @return true on success
     * @return false if specified id was not found
     */
    void remove(int id);

    /**
     * @brief clear handlers list
     * 
     */
    void clear(){ _hndlr_chain.clear(); }

    /**
     * @brief send data to all handlers in list
     * 
     * @param data 
     */
    void send(const JsonVariantConst& data) override;
    void send(const String& data) override;

};

class FrameSendAsyncJS: public FrameSend {
    private:
        bool flushed = false;
        AsyncWebServerRequest *req;
        AsyncJsonResponse* response;
    public:
        FrameSendAsyncJS(AsyncWebServerRequest *request) : req(request) {
            response = new AsyncJsonResponse(false, IFACE_DYN_JSON_SIZE);
        }
        ~FrameSendAsyncJS();

        // not supported
        void send(const String &data) override {};

        void send(const JsonVariantConst& data) override;

        bool available() const override { return flushed; }
};

struct section_stack_t{
    JsonArray block;
    String name;
    int idx{0};
};

class Interface {


    DynamicJsonDocument json;
    bool _delete_handler_on_destruct;
    LList<section_stack_t*> section_stack;
    FrameSend *send_hndl;


    /**
     * @brief append supplied json data to the current Interface frame
     * this function is unsafe, i.e. when frame is full and can't append
     * current data it returns false and tries to send current frame and open new one
     * use json_frame_add() to add data in a safe way
     * 
     * @param obj - json object to add
     * @param shallow - add object using shallow copy
     * @return true on success adding obj
     * @return false otherwise
     */
    bool json_frame_enqueue(const JsonVariantConst &obj, bool shallow = false);

    /**
     * @brief purge json object while keeping section structure
     * used to release mem after json_frame_send() call
     */
    void json_frame_next();

    /**
     * @brief - serialize and send Interface object to the WebSocket
     */
    inline void json_frame_send(){ if (send_hndl) send_hndl->send(json); };

    /**
     * @brief - start UI section
     * A section contains DOM UI elements, this is generic one
     */
    template  <typename TAdaptedString, typename L>
    JsonArrayConst json_section_begin(TAdaptedString name, const L label, bool main, bool hidden, bool line, JsonObject &obj);

    template <typename L>
    void _html_input(UIelement<TINY_JSON_SIZE> &ui, const char* type, const L label, bool onChange);


    /* *** PUBLIC METHODS *** */

    public:
        /**
         * @brief Construct a new Interface object
         * it will pick send hanlder from EmbUI object.
         * EmbUI could have it's own chain of send handlers to execute
         * 
         * @param feeder an FrameSender object to use for sending data 
         * @param size desired size of DynamicJsonDocument
         */
        Interface (FrameSend *feeder, size_t size = IFACE_DYN_JSON_SIZE): json(size), _delete_handler_on_destruct(false) {
            send_hndl = feeder;
        }

        Interface(AsyncWebSocket *server, size_t size = IFACE_DYN_JSON_SIZE): json(size), _delete_handler_on_destruct(true) {
            send_hndl = new FrameSendWSServer(server);
        }
        Interface(AsyncWebSocketClient *client, size_t size = IFACE_DYN_JSON_SIZE): json(size), _delete_handler_on_destruct(true) {
            send_hndl = new FrameSendWSClient(client);
        }
        Interface(AsyncWebServerRequest *request, size_t size = IFACE_DYN_JSON_SIZE): json(size), _delete_handler_on_destruct(true) {
            send_hndl = new FrameSendHttp(request);
        }
        ~Interface(){
            json_frame_clear();
            if (_delete_handler_on_destruct){
                delete send_hndl;
                send_hndl = nullptr;
            }
        }


        /**
         * @brief - begin UI secton of the specified <type>
         * generic frame creation method, used by other calls to create custom-typed frames
         */
        template <typename TString>
        void json_frame(const char* type, const TString& section_id);

        template <typename TChar = const char>
        void json_frame(const char* type, const TChar* section_id = P_EMPTY);

        /**
         * @brief - add object to current Interface frame
         * attempts to retry on mem overflow
         */
        void json_frame_add(const JsonVariantConst &obj);
        template <size_t capacity>
        void json_frame_add( UIelement<capacity> &ui){ json_frame_add(ui.obj); }

        /**
         * @brief purge all current section data
         * 
         */
        void json_frame_clear();
        
        /**
         * @brief finalize, send and clear current sections stack
         * implies  json_section_end(); json_frame_send(); json_frame_clear();
         */
        void json_frame_flush();

        /**
         * @brief - begin Interface UI secton
         * used to construct WebUI html elements
         */
        void json_frame_interface(){ json_frame(P_interface); };
        template <typename TString>
        void json_frame_interface(const TString& section_id){ json_frame(P_interface, section_id); }
        template <typename TChar>
        void json_frame_interface(const TChar* section_id){ json_frame(P_interface, section_id); };

        /**
         * @brief - begin Value UI secton
         * used to supply WebUI with data (key:value pairs)
         */
        void json_frame_value(){ json_frame(P_value); };

        /**
         * @brief - begin Value UI secton with supplied json object
         * used to supply WebUI with data (key:value pairs)
         * @param val json object with supplied data to be copied
         * @param shallow use 'shallow' copy, be SURE to keep val object alive intact until frame is fully send
         *                with json_frame_send() or json_frame_flush()
         */
        void json_frame_value(const JsonVariant val, bool shallow = false);

        /**
         * @brief start UI section
         * A section contains DOM UI elements, this is generic method
         * 
         * @param name - name identifies a group of DOM objects, could be arbitrary name.
         *               Some names are reserved:
         *                  'menu' - contains left-side menu elements
         *                  'content' - updates values for exiting DOM elements
         *                  'xload' - load section object from external json via ajax request
         * @param label - a headliner for a group of section elements
         * @param main  - main section starts a blank page, may contain other nested sections
         * @param hidden - creates section hidden under 'spoiler' button, user needs to press the button to unfold it
         * @param line  -  all elements of the section will be alligned in a line
         */
        template  <typename TString, typename L = const char*>
        JsonArrayConst json_section_begin(const TString& name, const L label = P_EMPTY, bool main = false, bool hidden = false, bool line = false);
        template  <typename TChar, typename L = const char*>
        JsonArrayConst json_section_begin(const TChar* name, const L label = P_EMPTY, bool main = false, bool hidden = false, bool line = false);

        /**
         * @brief - content section is meant to replace existing data on the page
         */
        void json_section_content(){ json_section_begin("content"); };

        /**
         * @brief opens nested json_section using previous section's index
         * i.e. extend previous object with nested elements.
         * used for elements like html 'select'+'option', 'form'+'inputs', etc...
         * each extended section MUST be closed with json_section_end() prior to opening new section
         * @param name extended section name
         */
        template  <typename ID>
            typename std::enable_if<embui_traits::is_string_v<ID>,void>::type
        json_section_extend(const ID name);

        /**
         * @brief - opens section for UI elements that are aligned in one line on a page
         * each json_section_line() must be closed by a json_section_end() call
         */
        template  <typename ID = const char*>
            typename std::enable_if<embui_traits::is_string_v<ID>,void>::type
        json_section_line(const ID name = P_EMPTY){ json_section_begin(name, P_EMPTY, false, false, true); };

        /**
         * @brief - start a section with a new page content
         * it replaces current page from scratch, may contain other nested sections
         */
        template  <typename ID, typename L>
        void json_section_main(const ID name, const L label){ json_section_begin(name, label, true); };

        /**
         * @brief - section with manifest data
         * it contains manifest data for WebUI, like fw name, version, Chip ID
         * and any arbitrary json'ed data that could be processed in user js code
         * could be supplied with additional date via (optional) value() call, otherwise just closed
         * @param appname - application name, builds Document title on the page
         * @param devid - unique device ID, usually based on mac address
         * @param appjsapi - application js api version
         * @param appversion - application version, could be of any type suitable for user-code to rely on it
         * if 0 - than no checking required
         */
        template  <typename ID, typename L = const char*>
            typename std::enable_if<embui_traits::is_string_v<ID>,void>::type
        json_section_manifest(const ID appname, const char* devid, unsigned appjsapi = 0, const L appversion = P_EMPTY);

        /**
         * @brief - start a section with left-side MENU elements
         */
        void json_section_menu(){ json_section_begin(P_menu); };

        /**
         * @brief - start hidden UI section with button to hide/unhide elements
         */
        template  <typename ID, typename L>
        void json_section_hidden(const ID name, const L label){ json_section_begin(name, label, false, true); };

        void json_section_uidata(){ json_section_begin("uidata"); };

        /**
         * @brief section that will side-load external data
         * for any element in section's block where a key 'url' is present
         * a side-load data will be fetched and put into 'block' object of that element
         * 
         */
        void json_section_xload(){ json_section_begin(P_xload); };

        /**
         * @brief - close current UI section
         */
        void json_section_end();


        /* *** Object getters *** */

        /**
         * @brief Get the last section object, i.e. it gives RO access to current section array
         * 
         * @return const section_stack_t 
         */
        const section_stack_t* get_last_section(){ return section_stack.tail(); };

        /**
         * @brief Get last html object in interface stack
         * this method could be used to add/extend html object with arbitrary key:value objects that are
         * not present in existing html element creation methods
         * NOTE: returned object might get INVALIDATED on next object addition when frame is flushed.
         * A care should be taken not to overflow memory pool of Interface object
         * 
         * @return JsonObject 
         */
        JsonObject get_last_object();


        /* *** HTML Elements *** */

        /**
         * @brief - create html button to submit itself's id, section form or else...
         * depend on type button can send:
         *  type generic - sendsit's id with null value
         *  type submit  - submit a section form + it's own id
         *  type js - call a js function with name ~ id
         */
        template <typename T>
        void button(button_t btype, const T id, const T label, const char* color = P_EMPTY);

        /**
         * @brief - create html button to submit itself's id, value, section form or else
         * depend on type button can send:
         *  type 0 - it's id + value
         *  type 1 - submit a form with section + it's own id + value
         *  type 2 - call a js function with name ~ id + pass it a value
         */
        template <typename T, typename V>
        void button_value(button_t btype, const T id, const V value, const T label, const char* color = P_EMPTY);

        /**
         * @brief - элемент интерфейса checkbox
         * @param onChange - значение чекбокса при изменении сразу передается на сервер без отправки формы
         */
        template <typename TChar, typename L>
            ValidCharPtr_t<TChar>
        checkbox(const TChar* id, bool value, const L label, bool onChange = false){ html_input(id, P_chckbox, value, label, onChange); };

        template <typename TString, typename L>
            ValidStringRef_t<TString>
        checkbox(const TString& id, bool value, const L label, bool onChange = false){ html_input(id, P_chckbox, value, label, onChange); };

        /**
         * @brief - элемент интерфейса "color selector"
         */
        template <typename ID, typename V, typename L>
        void color(const ID id, const V value, const L label){ html_input(id, P_color, value, label); };

        /**
         * @brief insert text comment (a simple <p>text</p> block)
         * 
         * @param id comment id (might be used to replace value with content() method)
         * @param label - text label
         */
        template <typename ID, typename L>
        void comment(const ID id, const L label);

        template <typename L>
        void comment(const L label){ comment(P_EMPTY, label); };

        /**
         * @brief Create inactive button with a visible label that does nothing but (possibly) carries a value
         *  could be used same way as 'hidden' element
         * @param id - element/div DOM id
         * @param label  - element button will carry a label as text
         * @param value  - element value (hidden value, if any)
         *  
         */
        template <typename ID, typename L, typename V=int>
        void constant(const ID id, const L label, const V value=0);

        template <typename L>
        void constant(const L label){ constant(P_EMPTY, label); };

        template <typename ID, typename L, typename V>
            typename std::enable_if<embui_traits::is_string_v<V>,void>::type
        date(const ID id, const V value, const L label){ html_input(id, P_date, value, label); };

        template <typename ID, typename L, typename V>
            typename std::enable_if<embui_traits::is_string_v<V>,void>::type
        datetime(const ID id, const V value, const L label){ html_input(id, P_datetime, value, label); };

        /**
         * @brief - create "display" div with custom css selector
         * It's value is passed "as-is" in plain html, could be used for making all kinds of "sensor" outputs on the page
         * with live-updated values without the need to redraw interface element
         * @param id - element/div DOM id
         * @param value  - element value (treated as text)
         * @param class - base css class for Display div, css selector value created as "${class} ${id}" to allow many sensors
         *                inherit from the base class, default is "display ${id}"
         * @param params - additional parameters (reserved for future use to be used in template processor)
         */
        template <typename ID, typename V, typename L = const char*>
        void display(const ID id, V&& value, const L label = P_EMPTY, String css = P_EMPTY, const JsonObject params = JsonObject() );

        /**
         * @brief - Creates html div element based on a templated configuration and arbitrary parameters
         * used to create user-defined interface elements with custom css/js handlers, etc...
         * 
         * @param id - becomes div ID
         * @param type - type specificator for templater
         * @param value - value usage depends on template
         * @param label - value usage depends on template
         * @param params - dict with arbitrary params, usage depends on template
         */
        template <typename ID, typename V, typename L = const char*, typename CSS = const char*>
        void div(const ID id, const ID type, const V value, const L label = P_EMPTY, const CSS css = P_EMPTY, const JsonVariantConst params = JsonVariantConst());

        template <typename ID, typename V, typename L>
            typename std::enable_if<embui_traits::is_string_v<V>,void>::type
        email(const ID id, const V value, const L label){ html_input(id, P_email, value, label); };

        /**
         * @brief create a file upload form
         * based on a template it will create an html form containing an input "file upload"
         * an optional parameter 'opt' could be used to set additional values via templater
         * 
         * @param id - file upload id
         * @param action - URI for upload action
         * @param label - label for file upload
         * @param opt - optional parametr, passes "opt"=opt_value
         */
        template <typename ID, typename V, typename L = const char*>
            typename std::enable_if<embui_traits::is_string_v<V>,void>::type
        file_form(const ID id, const V action, const L label, const L opt = P_EMPTY);

        /**
         * @brief Create hidden html field
         * could be used to pass some data between different sections
         */
        template <typename ID, typename V>
        void hidden(const ID id, const V value);

        /**
         * @brief - create generic html input element
         * @param id - element id
         * @param type - html element type, ex. 'text'
         * @param value - element value
         * @param label - element label
         * @param onChange - if true, element value in send via ws on-change 
         */
        template <typename TChar, typename V, typename L>
            typename std::enable_if<embui_traits::is_string_ptr_v<TChar*>, void>::type
        html_input(const TChar* id, const char* type, const V value, const L label, bool onChange = false);

        template <typename TString, typename V, typename L>
            typename std::enable_if<embui_traits::is_string_obj_v<TString>, void>::type
        html_input(const TString& id, const char* type, const V value, const L label, bool onChange = false);

        /**
         * @brief create an iframe on page
         * 
         * @param id 
         * @param value 
         */
        template <typename ID, typename V>
        void iframe(const ID id, const V value){ html_input(id, P_iframe, value, P_EMPTY); };

        /**
         * @brief - create empty div and call js-function over this div
         * js function receives div.id and params obj as arguments
         * js function must be predefined in front-end's WebUI .js files
         * 
         * @param id - element/div DOM id
         * @param value - js function name
         * @param class - css class for div
         * @param params - additional parameters (reserved to be used in template processor)
         */
        template <typename ID, typename V, typename L = const char*>
        void jscall(const ID id, const V value, const L label = P_EMPTY, const L css = P_EMPTY, JsonVariantConst params = JsonVariantConst() ){ div(id, P_js, value, label, css, params); };

        /**
         * @brief - create "number" html field with optional step, min, max constraints
         * implies constrains on number input field.
         * Template accepts types suitable to be added to the ArduinoJson document used as a dictionary
         * front-end converts numeric values to integers or floats
         */
        template <typename ID, typename T, typename L>
            typename std::enable_if<std::is_arithmetic_v<T>, void>::type
        number_constrained(const ID id, T value, const L label, T step = 0, T min = 0, T max = 0);

        /**
         * @brief - create "number" html field with optional step, min, max constraints
         * Template accepts types suitable to be added to the ArduinoJson document used as a dictionary
         * front-end converts numeric values to integers or floats
         */
        template <typename ID, typename T, typename L>
            typename std::enable_if<std::is_arithmetic_v<T>, void>::type
        number(const ID id, T value, const L label){ number_constrained(id, value, label); };

        /**
         * @brief - create an option element for "select" drop-down list
         */
        template <typename T, typename L>
        void option(const T value, const L label);

        template <typename ID, typename L>
            typename std::enable_if<embui_traits::is_string_v<ID>,void>::type
        password(const ID id, const L value, const L label){ html_input(id, P_password, value, label); };

        /**
         * @brief create live progressbar based on div+css
         * progress value should be sent via 'value' frame (json_frame_value)
         * with (id, value) pair
         * 
         * @param id 
         * @param label 
         */
        template <typename ID, typename L>
        void progressbar(const ID id, const L label){ div(id, P_progressbar, 0, label); };

        /**
         * @brief - create "range" html field with step, min, max constraints
         * Template accepts types suitable to be added to the ArduinoJson document used as a dictionary
         */
        template <typename ID, typename T, typename L>
            typename std::enable_if<embui_traits::is_string_v<ID>,void>::type
        range(const ID id, T value, T min, T max, T step, const L label, bool onChange = false);

        /**
         * @brief - create drop-down selection list
         * after select element is opened, the 'option' elements must follow with list items.
         * After last item, 'select' section must be closed with json_section_end()
         * 
         * content of a large lists could be loaded with ajax from the client's side
         * @param exturl - an url for xhr request to fetch list content, it must be a valid json object
         * with label/value pairs arranged in assoc array
         */
        template <typename ID, typename T, typename L = const char*>
        void select(const ID id, const T value, const L label = P_EMPTY, bool onChange = false, const L exturl = P_EMPTY);

        /**
         * @brief create spacer line with optional text label
         * 
         * @param label 
         */
        template <typename L = const char*>
            typename std::enable_if<embui_traits::is_string_v<L>,void>::type
        spacer(const L label = P_EMPTY);

        /**
         * @brief html text-input field
         * 
         * 
         * @param id 
         * @param value 
         * @param label 
         */
        template <typename ID, typename V, typename L>
            typename std::enable_if<embui_traits::is_string_v<L>,void>::type
        text(const ID id, const V value, const L label){ html_input(id, P_text, value, label); };

        /**
         * элемент интерфейса textarea
         * Template accepts literal types as textarea value
         */
        template <typename ID, typename V, typename L>
            typename std::enable_if<embui_traits::is_string_v<V>,void>::type
        textarea(const ID id, const V value, const L label);

        /**
         * элемент интерфейса time selector
         * Template accepts literal types as time value
         */
        template <typename ID, typename V, typename L>
            typename std::enable_if<embui_traits::is_string_v<V>,void>::type
        time(const ID id, const V value, const L label){ html_input(id, P_time, value, label); };

        /**
         * @brief load uidata on the front-end side
         * this medothod generates object with instruction to load uidata object structure from specified url
         * and save it under specified key
         * this method should be wrapped in json_section_uidata()
         * 
         * @param key - a key to load data to, a dot sepparated notation (i.e. "app.page.controls")
         * @param url - url to fetch json from, could be relative to /
         * @param merge - if 'true', then try to merge/update data under existing key, otherwise replace it
         */
        void uidata_xload(const char* key, const char* url, bool merge = false);

        /**
         * @brief pick and implace UI structured data objects from front-end side-storage
         * 
         * @param key - a key to load data from, a dot sepparated notation (i.e. "app.page.controls")
         */
        void uidata_pick(const char* key);

        /**
         * @brief - Add 'value' object to the Interface frame
         * used to replace values of existing ui elements on the page
         * Template accepts types suitable to be added to the ArduinoJson document used as a dictionary
         */
        template <typename ID, typename T>
        void value(const ID id, const T val, bool html = false);

        /**
         * @brief - Add the whole JsonObject to the Interface frame
         * actualy it is a copy-object method used to echo back the data to the WebSocket in one-to-many scenarios
         * also could be used to update multiple elements at a time with a dict of key:value pairs
         */
        void value(JsonVariant data){ json_frame_add(data); }

};


/* *** TEMPLATED CLASSES implementation follows *** */


template <typename T>
void Interface::button(button_t btype, const T id, const T label, const char* color){
    UI_button<TINY_JSON_SIZE> ui(btype, id, label);
    ui.color(color);
    json_frame_add(ui);
};

template <typename T, typename V>
void Interface::button_value(button_t btype, const T id, const V value, const T label, const char* color){
    UI_button<TINY_JSON_SIZE> ui(btype, id, label);
    ui.obj[P_value] = value;
    ui.color(color);

    json_frame_add(ui);
};

template <typename ID, typename L>
void Interface::comment(const ID id, const L label){
    UIelement<UI_DEFAULT_JSON_SIZE * 2> ui(ui_element_t::comment, id);     // use a bit larger buffer for long texts
    ui.label(label);
    json_frame_add(ui);
}

template <typename ID, typename L, typename V>
void Interface::constant(const ID id, const L label, const V value){
    UIelement<UI_DEFAULT_JSON_SIZE> ui(ui_element_t::constant, id, value);
    ui.obj[P_label] = label;    // implicitly set label to a supplied parameter (could be non-literal)
    json_frame_add(ui);
};

template <typename ID, typename V, typename L = const char*>
void Interface::display(const ID id, V&& value, const L label, String cssclass, const JsonObject params ){
    if (cssclass.isEmpty())	// make css selector like 'class "css" "id"', id used as a secondary distinguisher 
        cssclass = P_display;   // "display is the default css selector"
    cssclass += (char)0x20;
    cssclass += id;
    div(id, P_html, std::forward<V>(value), label, cssclass, params);
};

template <typename ID, typename V, typename L = const char*, typename CSS = const char*>
void Interface::div(const ID id, const ID type, const V value, const L label, const CSS css, const JsonVariantConst params){
    UIelement<UI_DEFAULT_JSON_SIZE> ui(ui_element_t::div, id, value);
    ui.obj[P_type] = type;
    ui.label(label);
    if (!embui_traits::is_empty_string(css)) ui.obj[P_class] = css;

    if (!params.isNull()){
        JsonVariant nobj = ui.obj.createNestedObject(P_params);
        nobj.shallowCopy(params);
    }
    json_frame_add(ui);
};

template <typename ID, typename V, typename L>
    typename std::enable_if<embui_traits::is_string_v<V>,void>::type
Interface::file_form(const ID id, const V action, const L label, const L opt){
    UIelement<TINY_JSON_SIZE> ui(ui_element_t::file, id);
    ui.obj[P_type] = P_file;
    ui.obj[P_action] = action;
    ui.label(label);
    if (!embui_traits::is_empty_string(opt)) ui.obj["opt"] = opt;
    json_frame_add(ui);
}

template <typename ID, typename V>
void Interface::hidden(const ID id, const V value){
    UIelement<UI_DEFAULT_JSON_SIZE> ui(ui_element_t::hidden, id);
    ui.value(value);
    json_frame_add(ui);
};

template <typename TString>
void Interface::json_frame(const char* type, const TString& section_id){
    json[P_pkg] = type;
    json[P_final] = false;
    json_section_begin(section_id);
};

template <typename TChar>
void Interface::json_frame(const char* type, const TChar* section_id){
    json[P_pkg] = type;
    json[P_final] = false;
    json_section_begin(section_id);
};

template  <typename TString, typename L>
JsonArrayConst Interface::json_section_begin(const TString& name, const L label, bool main, bool hidden, bool line){
    JsonObject obj(section_stack.size() ? section_stack.tail()->block.createNestedObject() : json.as<JsonObject>());
    return json_section_begin(detail::adaptString(name), label, main, hidden, line, obj);
}
template  <typename TChar, typename L>
JsonArrayConst Interface::json_section_begin(const TChar* name, const L label, bool main, bool hidden, bool line){
    JsonObject obj(section_stack.size() ? section_stack.tail()->block.createNestedObject() : json.as<JsonObject>());
    return json_section_begin(detail::adaptString(name), label, main, hidden, line, obj);
}

template  <typename TAdaptedString, typename L>
JsonArrayConst Interface::json_section_begin(TAdaptedString name, const L label, bool main, bool hidden, bool line, JsonObject &obj){
    if (embui_traits::is_empty_string(name))
        obj[P_section] = String (std::rand()); // need a deep-copy
    else
        obj[P_section] = name;

    if (!embui_traits::is_empty_string(label)) obj[P_label] = label;
    if (main) obj["main"] = true;
    if (hidden) obj[P_hidden] = true;
    if (line) obj["line"] = true;

    section_stack_t *section = new section_stack_t;
    section->name = obj[P_section].as<const char*>();
    section->block = obj.createNestedArray(P_block);

    LOG(printf, "UI: section begin #%u '%s', %ub free\n", section_stack.size(), section->name.isEmpty() ? "-" : section->name.c_str(), json.capacity() - json.memoryUsage());   // section index counts from 0, so I print in fo BEFORE adding section to stack
    section_stack.add(section);
    return JsonArrayConst(section->block);
}

template  <typename ID>
    typename std::enable_if<embui_traits::is_string_v<ID>,void>::type
Interface::json_section_extend(const ID name){
    section_stack.tail()->idx--;
    JsonObject o(section_stack.tail()->block[section_stack.tail()->block.size()-1]);    // find last array element
    json_section_begin(name, P_EMPTY, false, false, false, o);
};

template  <typename ID, typename L = const char*>
    typename std::enable_if<embui_traits::is_string_v<ID>,void>::type
Interface::json_section_manifest(const ID appname, const char* devid, unsigned appjsapi, const L appversion){
    json_section_begin("manifest", P_EMPTY, false, false, false);
    JsonObject obj = section_stack.tail()->block.createNestedObject();
    obj[P_uijsapi] = EMBUI_JSAPI;
    obj[P_uiver] = EMBUI_VERSION_STRING;
    obj["uiobjects"] = EMBUI_UIOBJECTS;
    obj[P_app] = appname;
    obj[P_appjsapi] = appjsapi;
    if (!embui_traits::is_empty_string(appversion)) obj["appver"] = appversion;
    obj["mc"] = devid;
}

template <typename ID, typename T, typename L>
    typename std::enable_if<std::is_arithmetic_v<T>, void>::type
Interface::number_constrained(const ID id, T value, const L label, T step, T min, T max){
    UIelement<TINY_JSON_SIZE> ui(ui_element_t::input, id, value);
    ui.obj[P_type] = P_number;
    ui.label(label);
    if (min) ui.obj[P_min] = min;
    if (max) ui.obj[P_max] = max;
    if (step) ui.obj[P_step] = step;
    json_frame_add(ui);
};

template <typename T, typename L>
void Interface::option(const T value, const L label){
    UIelement<TINY_JSON_SIZE> ui(ui_element_t::option, P_EMPTY, value);
    ui.label(label);
    json_frame_add(ui);
}

template <typename ID, typename T, typename L>
    typename std::enable_if<embui_traits::is_string_v<ID>,void>::type
Interface::range(const ID id, T value, T min, T max, T step, const L label, bool onChange){
    UIelement<TINY_JSON_SIZE> ui(ui_element_t::input, id, value);
    ui.obj[P_type] = "range";
    ui.obj[P_min] = min;
    ui.obj[P_max] = max;
    ui.obj[P_step] = step;
    ui.label(label);
    if (onChange) ui.obj[P_onChange] = true;
    json_frame_add(ui);
};

template <typename ID, typename T, typename L = const char*>
void Interface::select(const ID id, const T value, const L label, bool onChange, const L exturl){
    UIelement<UI_DEFAULT_JSON_SIZE> ui(ui_element_t::select, id, value);
    ui.label(label);
    if (onChange) ui.obj[P_onChange] = true;
    if (!embui_traits::is_empty_string(exturl)) ui.obj[P_url] = exturl;
    json_frame_add(ui);
    // open new nested section for 'option' elements
    json_section_extend(P_options);
};

template <typename L = const char*>
    typename std::enable_if<embui_traits::is_string_v<L>,void>::type
Interface::spacer(const L label){
    UIelement<TINY_JSON_SIZE> ui(ui_element_t::spacer);
    ui.label(label);
    json_frame_add(ui);
}

template <typename ID, typename V, typename L>
    typename std::enable_if<embui_traits::is_string_v<V>,void>::type
Interface::textarea(const ID id, const V value, const L label){
    UIelement<UI_DEFAULT_JSON_SIZE> ui(ui_element_t::textarea, id, value);
    ui.label(label);
    json_frame_add(ui);
};

template <typename ID, typename T>
void Interface::value(const ID id, const T val, bool html){
    if (html){
        UIelement<TINY_JSON_SIZE> ui(ui_element_t::value, id);
        ui.obj[P_value] = val;
        ui.html(html);
        json_frame_add(ui);
    } else {
        StaticJsonDocument<TINY_JSON_SIZE> jdoc;
        jdoc[id] = val;
        json_frame_add(jdoc);
    }
};

template <typename TChar, typename V, typename L>
    ValidCharPtr_t<TChar>
Interface::html_input(const TChar* id, const char* type, const V value, const L label, bool onChange){
    UIelement<TINY_JSON_SIZE> ui(ui_element_t::input, id, value);
    _html_input(ui, type, label, onChange);
};

template <typename TString, typename V, typename L>
    ValidStringRef_t<TString>
Interface::html_input(const TString& id, const char* type, const V value, const L label, bool onChange){
    UIelement<TINY_JSON_SIZE> ui(ui_element_t::input, id, value);
    _html_input(ui, type, label, onChange);
};

template <typename L>
void Interface::_html_input(UIelement<TINY_JSON_SIZE> &ui, const char* type, const L label, bool onChange){
    ui.label(label);
    ui.obj[P_type] = type;
    if (onChange) ui.obj[P_onChange] = true;
    json_frame_add(ui);
};
