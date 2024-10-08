// This framework originaly based on JeeUI2 lib used under MIT License Copyright (c) 2019 Marsel Akhkamov
// then re-written and named by (c) 2020 Anton Zolotarev (obliterator) (https://github.com/anton-zolotarev)
// also many thanks to Vortigont (https://github.com/vortigont), kDn (https://github.com/DmytroKorniienko)
// and others people

#pragma once

#include "globals.h"
#include "traits.hpp"
#include <list>
#include "AsyncJson.h"


template<typename TString>
using ValidStringRef_t = std::enable_if_t<embui_traits::is_string_obj_v<TString>, void>;

template<typename TChar>
using ValidCharPtr_t = std::enable_if_t<embui_traits::is_string_ptr_v<TChar*>, void>;

template<typename TString>
using ValidString_t = std::enable_if_t<embui_traits::is_string_v<TString>, void>;


// UI Button type enum
enum class button_t:uint8_t {
    generic = (0U),
    submit,
    js,
    href
};

class FrameSend {
    public:
        /**
         * @brief should return 'true' if downlevel protol is availbale
         * i.e. a connection is ready to send data, WebSocket subsribers are available, etc...
         */
        virtual bool available() const = 0;
        virtual ~FrameSend(){ };
        virtual void send(const char* data) = 0;
        virtual void send(const JsonVariantConst& data) = 0;
        void send(const String &data){ send(data.c_str()); };
        //virtual void flush(){};
};

class FrameSendWSServer: public FrameSend {
    protected:
        AsyncWebSocket *ws;
    public:
        FrameSendWSServer(AsyncWebSocket *server) : ws(server){}
        ~FrameSendWSServer() { ws = nullptr; }
        bool available() const override {
            LOGV(P_EmbUI, printf, "WS cnt:%u\n", ws->count());
             return ws->count(); }

        void send(const char* data) override { ws->textAll(data ? data : P_empty_quotes); };
        void send(const JsonVariantConst& data) override;
};

class FrameSendWSClient: public FrameSend {
    protected:
        AsyncWebSocketClient *cl;
    public:
        FrameSendWSClient(AsyncWebSocketClient *client) : cl(client){}
        ~FrameSendWSClient() { cl = nullptr; }
        bool available() const override { return cl->status() == WS_CONNECTED; }

        void send(const char* data) override { cl->text(data ? data : P_empty_quotes); };

        /**
         * @brief - serialize and send json obj directly to the ws buffer
         */
        void send(const JsonVariantConst& data) override;
};

class FrameSendHttp: public FrameSend {
    protected:
        AsyncWebServerRequest *req;

    public:
        FrameSendHttp(AsyncWebServerRequest *request) : req(request) { }
        ~FrameSendHttp() { /* delete stream; */ req = nullptr; }

        void send(const char* data) override;

        /**
         * @brief - serialize and send json obj directly to the ws buffer
         */
        void send(const JsonVariantConst& data) override;

        bool available() const override { return true; }
};

struct HndlrChain {
    int id;
    std::unique_ptr<FrameSend> handler;
    HndlrChain() = delete;
    HndlrChain(const HndlrChain& rhs) = delete;
    explicit HndlrChain(std::unique_ptr<FrameSend>&& rhs) noexcept : id(std::rand()), handler(std::move(rhs)) {};
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
    void send(const char* data) override;

};

class FrameSendAsyncJS: public FrameSend {
    private:
        bool flushed = false;
        AsyncWebServerRequest *req;
        AsyncJsonResponse response{ AsyncJsonResponse(false) };
    public:
        explicit FrameSendAsyncJS(AsyncWebServerRequest *request) : req(request) {}
        ~FrameSendAsyncJS();

        void send(const JsonVariantConst& data) override;

        bool available() const override { return flushed; }

        // not supported
        [[ deprecated( "FrameSendAsyncJS ignores String argument" ) ]] void send(const char* data) override {};
};

class Interface {

    struct section_stack_t{
        int idx{0};
        String name;
        JsonArray block;
        section_stack_t();
        section_stack_t(const char* sect_name, JsonArray&& block) : name(sect_name), block(std::move(block)) {} 
    };

    const bool _delete_handler_on_destruct;
    JsonDocument json;
    std::list<section_stack_t> section_stack;
    FrameSend *send_hndl;


    /**
     * @brief purge json object while keeping section structure
     * used to release mem after _json_frame_send() call
     */
    void _json_frame_next();

    /**
     * @brief - serialize and send Interface object to the WebSocket
     */
    inline void _json_frame_send(){ if (send_hndl) send_hndl->send(json); };

    /**
     * @brief - start UI section
     * A section contains DOM UI elements, this is generic one
     */
    template  <typename TAdaptedString, typename L>
    void _json_section_begin(TAdaptedString name, const L label, bool main, bool hidden, bool line, bool replace, JsonObject obj);


    /* *** PUBLIC METHODS *** */

    public:
        /**
         * @brief Construct a new Interface object
         * it will pick send hanlder from EmbUI object.
         * EmbUI could have it's own chain of send handlers to execute
         * 
         * @param feeder an FrameSender object to use for sending data 
         * @param size desired size of JsonDocument
         */
        Interface (FrameSend *feeder): _delete_handler_on_destruct(false), send_hndl(feeder) {}

        explicit Interface(AsyncWebSocket *server): _delete_handler_on_destruct(true), send_hndl(new FrameSendWSServer(server)) {}
        explicit Interface(AsyncWebSocketClient *client): _delete_handler_on_destruct(true), send_hndl(new FrameSendWSClient(client)) {}
        explicit Interface(AsyncWebServerRequest *request): _delete_handler_on_destruct(true), send_hndl(new FrameSendHttp(request)) {}
        // no copy c-tor
        Interface(const Interface&) = delete;
        Interface & operator=(const Interface&) = delete;
        // d-tor
        ~Interface();


        /**
         * @brief - begin UI secton of the specified <type>
         * generic frame creation method, used by other calls to create custom-typed frames
         */
        JsonObject json_frame(const char* type, const char* section_id = P_EMPTY);

        /**
         * @brief purge all frame data
         * 
         */
        void json_frame_clear();
        
        /**
         * @brief finalize, send and clear current sections stack and frame data
         * implies  json_section_end(); _json_frame_send(); json_frame_clear();
         */
        void json_frame_flush();

        /**
         * @brief - begin Interface UI secton
         * used to construct WebUI html elements
         */
        JsonObject json_frame_interface(){ return json_frame(P_interface); };

        template <typename TString>
        JsonObject json_frame_interface(const TString& section_id){ return json_frame(P_interface, section_id); }

        template <typename TChar>
        JsonObject json_frame_interface(const TChar* section_id){ return json_frame(P_interface, section_id); };

        /**
         * @brief frame that executes arbitrary js function on a page providing it a constructed object as an argument
         * opening this frame user can build any object with a structure supported by sectioned engine of EmbUI.
         * On reassembly that object will be passed as an input parameter to executed js function
         * @warning returned JsonObject is invalidated on execution any of json_frame_clear()/json_frame_send()/json_frame_flush() calls
         * @warning a lifetime of the returned JsonObject is limited to a lifetime of Interface object
         * 
         * @tparam TString 
         * @param function - function name to execute on scope of a page
         */
        template <typename TString>
        JsonObject json_frame_jscall(const TString& function);

        /**
         * @copydoc template <typename TString> JsonObject json_frame_jscall(const TString& function);
         */
        template <typename TChar = const char>
        JsonObject json_frame_jscall(const TChar* function);

        /**
         * @brief send accumulated data and clears memory while preserving sections stack
         * this call does NOT closes the frame and implies that a continuation data will follow
         * should be used when sending really large sections by periodicaly dumping the
         * data to network stack and reusing memory
         * @warning invalidates ANY of the previously returned JsonObjects 
         */
        void json_frame_send();

        /**
         * @brief - begin Value UI frame
         * used to supply WebUI with data (key:value pairs)
         */
        JsonObject json_frame_value(){ return json_frame(P_value); };

        /**
         * @brief - begin Value UI secton with supplied json object
         * actualy it is a copy-object method used to echo back the data to the WebSocket in one-to-many scenarios
         * also could be used to update multiple elements at a time with a dict of key:value pairs
         * Might be 
         * 1) an array of objects, i.e. [{"key1": "val1"}, {"key2":42}, {"key3":"val3"}]
         * 2) an array of labeled objects [{"id":"someid", "value":"someval", "html": true}] - used to update template placeholders
         * 
         * @param val json object with supplied data to be copied
         */
        JsonObject json_frame_value(const JsonVariantConst val);

        /* *** Object getters/setters *** */

        /**
         * @brief Get last section's block array
         * i.e. it gives access to an array of current section's objects
         * 
         * @warning returned JsonArray is invalidated on execution any of json_frame_clear()/json_frame_send()/json_frame_flush() calls
         * @warning a lifetime of the returned JsonArray is limited to a lifetime of Interface object
         * 
         * @return JsonArray
         */
        JsonArray json_block_get();

        /**
         * @brief Get last html object in interface stack
         * this method could be used to add/extend html object with arbitrary key:value objects that are
         * not present in existing html element creation methods
         * 
         * @warning returned JsonObject is invalidated on execution any of json_frame_clear()/json_frame_send()/json_frame_flush() calls
         * @warning a lifetime of the returned JsonObject is limited to a lifetime of Interface object
         * 
         * @return JsonObject 
         */
        JsonObject json_object_get();

        /**
         * @brief - add new object to current section block
         * object is added via deep-copy
         * 
         * @warning returned JsonObject is invalidated on execution any of json_frame_clear()/json_frame_send()/json_frame_flush() calls
         * @warning a lifetime of the returned JsonObject is limited to a lifetime of Interface object
         * 
         */
        JsonObject json_object_add(const JsonVariantConst obj);

        /**
         * @brief create a new object at the end of current section's block
         * if the is no section opened so far - returns null JsonObject
         * 
         * @note a frame should be created prior to calling this method
         * @warning returned JsonObject is invalidated on execution any of json_frame_clear()/json_frame_send()/json_frame_flush() calls
         * @warning a lifetime of the returned JsonObject is limited to a lifetime of Interface object
         * 
         * @return empty JsonObject
         */
        JsonObject json_object_create();


        /**
         * @brief start UI section
         * A section contains DOM UI elements, this is generic method
         * @warning returned JsonObject is invalidated on execution any of json_frame_clear()/json_frame_send()/json_frame_flush() calls
         * @warning a lifetime of the returned JsonObject is limited to a lifetime of Interface object
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
         * @param replace - if true, than new frame section will REPLACE existing section on a page, otherwise a new section will be APPENDED to main section
         */
        template  <typename TString, typename L = const char*>
        JsonObject json_section_begin(const TString& name, const L label = P_EMPTY, bool main = false, bool hidden = false, bool line = false, bool replace = false );

        /**
         * @copydoc JsonArrayConst json_section_begin(const TString& name, const L label = P_EMPTY, bool main = false, bool hidden = false, bool line = false, bool replace = false );
         */
        template  <typename TChar, typename L = const char*>
        JsonObject json_section_begin(const TChar* name, const L label = P_EMPTY, bool main = false, bool hidden = false, bool line = false, bool replace = false );

        /**
         * @brief - content section is meant to replace existing data on the page
         * all objects of this section will REPLACE objects on a page with  the same DOM id's
         * 
         * @warning returned JsonObject is invalidated on execution any of json_frame_clear()/json_frame_send()/json_frame_flush() calls
         * @warning a lifetime of the returned JsonObject is limited to a lifetime of Interface object
         */
        JsonObject json_section_content(){ return json_section_begin(P_content); };

        /**
         * @brief creates nested section for the last html object in current section
         * i.e. extends newly added object with nested elements.
         * Used for UI objects like html 'select'+'select options', 'form'+'inputs', etc...
         * each extended section MUST be closed with json_section_end() prior to
         * adding new objects to a previous section or opening a new section
         * @warning returned JsonObject is invalidated on execution any of json_frame_clear()/json_frame_send()/json_frame_flush() calls
         * @warning a lifetime of the returned JsonObject is limited to a lifetime of Interface object
         * 
         * @param name extended section's name
         */
        template  <typename ID>
            typename std::enable_if<embui_traits::is_string_v<ID>,JsonObject>::type
        json_section_extend(const ID name);

        /**
         * @brief - opens section for UI elements that are aligned in one line on a page
         * @attention each json_section_line() must be closed by a json_section_end() call
         * 
         * @warning returned JsonObject is invalidated on execution any of json_frame_clear()/json_frame_send()/json_frame_flush() calls
         * @warning a lifetime of the returned JsonObject is limited to a lifetime of Interface object
         */
        template  <typename ID = const char*>
            typename std::enable_if<embui_traits::is_string_v<ID>,JsonObject>::type
        json_section_line(const ID name = P_EMPTY){ return json_section_begin(name, P_EMPTY, false, false, true); };

        /**
         * @brief - start a section with a new page content
         * it wipes current page and redraws a new elements from the top of content template, may also contain other nested sections
         * 
         * @warning returned JsonObject is invalidated on execution any of json_frame_clear()/json_frame_send()/json_frame_flush() calls
         * @warning a lifetime of the returned JsonObject is limited to a lifetime of Interface object
         */
        template  <typename ID, typename L>
        JsonObject json_section_main(const ID name, const L label){ return json_section_begin(name, label, true); };

        /**
         * @brief - section with manifest data
         * it contains manifest data for WebUI, like fw name, version, Chip ID
         * and any arbitrary json'ed data that could be processed in user js code
         * could be supplied with additional date via (optional) value() call, otherwise just closed
         * 
         * @warning returned JsonObject is invalidated on execution any of json_frame_clear()/json_frame_send()/json_frame_flush() calls
         * @warning a lifetime of the returned JsonObject is limited to a lifetime of Interface object
         * 
         * @param appname - application name, builds Document title on the page
         * @param devid - unique device ID, usually based on mac address
         * @param appjsapi - application js api version
         * @param appversion - application version, could be of any type suitable for user-code to rely on it
         * if 0 - than no checking required
         */
        template  <typename ID, typename L = const char*>
            typename std::enable_if<embui_traits::is_string_v<ID>,JsonObject>::type
        json_section_manifest(const ID appname, const char* devid, unsigned appjsapi = 0, const L appversion = P_EMPTY);

        /**
         * @brief - start a section with left-side MENU elements
         * 
         * @warning returned JsonObject is invalidated on execution any of json_frame_clear()/json_frame_send()/json_frame_flush() calls
         * @warning a lifetime of the returned JsonObject is limited to a lifetime of Interface object
         */
        JsonObject json_section_menu(){ return json_section_begin(P_menu); };

        /**
         * @brief - start hidden UI section with button to hide/unhide elements
         * 
         * @warning returned JsonObject is invalidated on execution any of json_frame_clear()/json_frame_send()/json_frame_flush() calls
         * @warning a lifetime of the returned JsonObject is limited to a lifetime of Interface object
         */
        template  <typename ID, typename L>
        JsonObject json_section_hidden(const ID name, const L label){ return json_section_begin(name, label, false, true); };

        /**
         * @brief - start hidden UI section with button to hide/unhide elements
         * 
         * @warning returned JsonObject is invalidated on execution any of json_frame_clear()/json_frame_send()/json_frame_flush() calls
         * @warning a lifetime of the returned JsonObject is limited to a lifetime of Interface object
         */
        JsonObject json_section_uidata(){ return json_section_begin(P_uidata); };

        /**
         * @brief section that will side-load external data
         * for any element in section's block where a key 'url' is present
         * a side-load data will be fetched and put into 'block' object of that element
         * 
         * @warning returned JsonObject is invalidated on execution any of json_frame_clear()/json_frame_send()/json_frame_flush() calls
         * @warning a lifetime of the returned JsonObject is limited to a lifetime of Interface object
         */
        JsonObject json_section_xload(){ return json_section_begin(P_xload); };

        /**
         * @brief - close current UI section
         */
        void json_section_end();




        /* *** HTML Elements *** */

        /**
         * @brief - create html button to submit itself's id, section form or else...
         * depend on type button can send:
         *  type generic - sendsit's id with null value
         *  type submit  - submit a section form + it's own id
         * 
         * @warning returned JsonObject is invalidated on execution any of json_frame_clear()/json_frame_send()/json_frame_flush() calls
         * @warning a lifetime of the returned JsonObject is limited to a lifetime of Interface object
         */
        template <typename T>
        JsonObject button(button_t btype, const T id, const T label, const char* color = P_EMPTY);

        /**
         * @brief - create html button to submit itself's id, value, section form or else
         * depend on type button can send:
         *  type 0 - it's id + value
         *  type 1 - submit a form with section + it's own id + value
         *  type 3 href - interpret value as a URI to go to
         * 
         * @warning returned JsonObject is invalidated on execution any of json_frame_clear()/json_frame_send()/json_frame_flush() calls
         * @warning a lifetime of the returned JsonObject is limited to a lifetime of Interface object
         */
        template <typename T, typename V>
        JsonObject button_value(button_t btype, const T id, const V value, const T label, const char* color = P_EMPTY);

        /**
         * @brief - create html button that calls user defined js function on click 
         * 
         * @warning returned JsonObject is invalidated on execution any of json_frame_clear()/json_frame_send()/json_frame_flush() calls
         * @warning a lifetime of the returned JsonObject is limited to a lifetime of Interface object
         */
        template <typename T, typename V>
        JsonObject button_jscallback(const T id, const T label, const char* callback, const V value = 0, const char* color = P_EMPTY);

        /**
         * @brief - create a checkbox element
         * 
         * @warning returned JsonObject is invalidated on execution any of json_frame_clear()/json_frame_send()/json_frame_flush() calls
         * @warning a lifetime of the returned JsonObject is limited to a lifetime of Interface object
         * 
         * @param onChange - значение чекбокса при изменении сразу передается на сервер без отправки формы
         */
        template <typename TChar, typename L>
            std::enable_if_t<embui_traits::is_string_ptr_v<TChar*>, JsonObject>     //  ValidCharPtr_t<TChar>
        checkbox(const TChar* id, bool value, const L label, bool onChange = false){ return html_input(id, P_chckbox, value, label, onChange); };

        /**
         * @copydoc checkbox(const TChar* id, bool value, const L label, bool onChange = false)
         */
        template <typename TString, typename L>
            std::enable_if_t<embui_traits::is_string_obj_v<TString>, JsonObject>    //  ValidStringRef_t<TString>
        checkbox(const TString& id, bool value, const L label, bool onChange = false){ return html_input(id, P_chckbox, value, label, onChange); };

        /**
         * @brief - элемент интерфейса "color selector"
         * 
         * @warning returned JsonObject is invalidated on execution any of json_frame_clear()/json_frame_send()/json_frame_flush() calls
         * @warning a lifetime of the returned JsonObject is limited to a lifetime of Interface object
         */
        template <typename ID, typename V, typename L>
        JsonObject color(const ID id, const V value, const L label){ return html_input(id, P_color, value, label); };

        /**
         * @brief insert text comment (a simple <p>text</p> block)
         * 
         * @warning returned JsonObject is invalidated on execution any of json_frame_clear()/json_frame_send()/json_frame_flush() calls
         * @warning a lifetime of the returned JsonObject is limited to a lifetime of Interface object
         * 
         * @param id comment id (might be used to replace value with content() method)
         * @param label - text label
         */
        template <typename ID, typename L>
        JsonObject comment(const ID id, const L label);

        template <typename L>
        JsonObject comment(const L label){ return comment(P_EMPTY, label); };

        /**
         * @brief Create inactive button with a visible label that does nothing but (possibly) carries a value
         *  could be used same way as 'hidden' element
         * 
         * @warning returned JsonObject is invalidated on execution any of json_frame_clear()/json_frame_send()/json_frame_flush() calls
         * @warning a lifetime of the returned JsonObject is limited to a lifetime of Interface object
         * 
         * @param id - element/div DOM id
         * @param label  - element button will carry a label as text
         * @param value  - element value (hidden value, if any)
         */
        template <typename ID, typename L, typename V=int>
        JsonObject constant(const ID id, const L label, const V value=0);

        template <typename L>
        JsonObject constant(const L label){ return constant(P_EMPTY, label); };

        /**
         * @brief date selector element
         * 
         * @warning returned JsonObject is invalidated on execution any of json_frame_clear()/json_frame_send()/json_frame_flush() calls
         * @warning a lifetime of the returned JsonObject is limited to a lifetime of Interface object
         * 
         * @tparam ID 
         * @tparam L 
         * @tparam V 
         * @param id 
         * @param value 
         * @param label 
         * @return JsonObject
         */
        template <typename ID, typename L, typename V>
            typename std::enable_if<embui_traits::is_string_v<V>,JsonObject>::type
        date(const ID id, const V value, const L label){ return html_input(id, P_date, value, label); };

        /**
         * @brief date-time selector element
         * 
         * @warning returned JsonObject is invalidated on execution any of json_frame_clear()/json_frame_send()/json_frame_flush() calls
         * @warning a lifetime of the returned JsonObject is limited to a lifetime of Interface object
         * 
         * @tparam ID 
         * @tparam L 
         * @tparam V 
         * @param id 
         * @param value 
         * @param label 
         * @return JsonObject
         */
        template <typename ID, typename L, typename V>
            typename std::enable_if<embui_traits::is_string_v<V>,JsonObject>::type
        datetime(const ID id, const V value, const L label){ return html_input(id, P_datetime, value, label); };

        /**
         * @brief - create "display" div with custom css selector
         * It's value is passed "as-is" in plain html, could be used for making all kinds of "sensor" outputs on the page
         * with live-updated values without the need to redraw interface element
         * 
         * @warning returned JsonObject is invalidated on execution any of json_frame_clear()/json_frame_send()/json_frame_flush() calls
         * @warning a lifetime of the returned JsonObject is limited to a lifetime of Interface object
         * 
         * @param id - element/div DOM id
         * @param value  - element value (treated as text)
         * @param class - base css class for Display div, css selector value created as "${class} ${id}" to allow many sensors
         *                inherit from the base class, default is "display ${id}"
         * @param params - additional parameters (reserved for future use to be used in template processor)
         */
        template <typename ID, typename V, typename L = const char*>
        JsonObject display(const ID id, V&& value, const L label = P_EMPTY, String css = P_EMPTY, const JsonObject params = JsonObject() );

        /**
         * @brief - Creates html div element based on a templated configuration and arbitrary parameters
         * used to create user-defined interface elements with custom css/js handlers, etc...
         * 
         * @warning returned JsonObject is invalidated on execution any of json_frame_clear()/json_frame_send()/json_frame_flush() calls
         * @warning a lifetime of the returned JsonObject is limited to a lifetime of Interface object
         * 
         * @param id - becomes div ID
         * @param type - type specificator for templater
         * @param value - value usage depends on template
         * @param label - value usage depends on template
         * @param params - dict with arbitrary params, usage depends on template
         */
        template <typename ID, typename V, typename L = const char*, typename CSS = const char*>
        JsonObject div(const ID id, const ID type, const V value, const L label = P_EMPTY, const CSS css = P_EMPTY, const JsonVariantConst params = JsonVariantConst());

        template <typename ID, typename V, typename L>
            typename std::enable_if<embui_traits::is_string_v<V>,JsonObject>::type
        email(const ID id, const V value, const L label){ return html_input(id, P_email, value, label); };

        /**
         * @brief create a file upload form
         * based on a template it will create an html form containing an input "file upload"
         * an optional parameter 'opt' could be used to set additional values via templater
         * 
         * @warning returned JsonObject is invalidated on execution any of json_frame_clear()/json_frame_send()/json_frame_flush() calls
         * @warning a lifetime of the returned JsonObject is limited to a lifetime of Interface object
         * 
         * @param id - file upload id
         * @param action - URI for upload action
         * @param label - label for file upload
         * @param opt - optional parametr, passes "opt"=opt_value
         */
        template <typename ID, typename V, typename L = const char*>
            typename std::enable_if<embui_traits::is_string_v<V>,JsonObject>::type
        file_form(const ID id, const V action, const L label, const L opt = P_EMPTY);

        /**
         * @brief Create hidden html field
         * could be used to pass some data between different sections
         * 
         * @warning returned JsonObject is invalidated on execution any of json_frame_clear()/json_frame_send()/json_frame_flush() calls
         * @warning a lifetime of the returned JsonObject is limited to a lifetime of Interface object
         */
        template <typename ID, typename V>
        JsonObject hidden(const ID id, const V value);

        /**
         * @brief - create generic html input element
         * 
         * @warning returned JsonObject is invalidated on execution any of json_frame_clear()/json_frame_send()/json_frame_flush() calls
         * @warning a lifetime of the returned JsonObject is limited to a lifetime of Interface object
         * 
         * @param id - element id
         * @param type - html element type, ex. 'text'
         * @param value - element value
         * @param label - element label
         * @param onChange - if true, element value in send via ws on-change 
         */
        template <typename TChar, typename V, typename L>
            typename std::enable_if<embui_traits::is_string_ptr_v<TChar*>, JsonObject>::type
        html_input(const TChar* id, const char* type, const V value, const L label, bool onChange = false);

        template <typename TString, typename V, typename L>
            typename std::enable_if<embui_traits::is_string_obj_v<TString>, JsonObject>::type
        html_input(const TString& id, const char* type, const V value, const L label, bool onChange = false);

        /**
         * @brief create an iframe on page
         * 
         * @warning returned JsonObject is invalidated on execution any of json_frame_clear()/json_frame_send()/json_frame_flush() calls
         * @warning a lifetime of the returned JsonObject is limited to a lifetime of Interface object
         * 
         * @param id 
         * @param value 
         */
        template <typename ID, typename V>
        JsonObject iframe(const ID id, const V value){ return html_input(id, P_iframe, value, P_EMPTY); };

        /**
         * @brief - create empty div and call js-function over this div
         * js function receives DOM id and params obj as arguments
         * 
         * @note function must be predefined in front-end's WebUI .js files
         * 
         * @warning returned JsonObject is invalidated on execution any of json_frame_clear()/json_frame_send()/json_frame_flush() calls
         * @warning a lifetime of the returned JsonObject is limited to a lifetime of Interface object
         * 
         * @param id - element/div DOM id
         * @param value - js function name
         * @param class - css class for div
         * @param params - additional parameters (reserved to be used in template processor)
         */
        template <typename ID, typename V, typename L = const char*>
        JsonObject jscall(const ID id, const V value, const L label = P_EMPTY, const L css = P_EMPTY, JsonVariantConst params = JsonVariantConst() ){ return div(id, P_js, value, label, css, params); };

        /**
         * @brief - create "number" html field with optional step, min, max constraints
         * implies constrains on number input field.
         * Template accepts types suitable to be added to the ArduinoJson document used as a dictionary
         * front-end converts numeric values to integers or floats
         * 
         * @warning returned JsonObject is invalidated on execution any of json_frame_clear()/json_frame_send()/json_frame_flush() calls
         * @warning a lifetime of the returned JsonObject is limited to a lifetime of Interface object
         */
        template <typename ID, typename T, typename L>
            typename std::enable_if<std::is_arithmetic_v<T>, JsonObject>::type
        number_constrained(const ID id, T value, const L label, T step = 0, T min = 0, T max = 0);

        /**
         * @brief - create "number" html field with optional step, min, max constraints
         * Template accepts types suitable to be added to the ArduinoJson document used as a dictionary
         * front-end converts numeric values to integers or floats
         * 
         * @warning returned JsonObject is invalidated on execution any of json_frame_clear()/json_frame_send()/json_frame_flush() calls
         * @warning a lifetime of the returned JsonObject is limited to a lifetime of Interface object
         */
        template <typename ID, typename T, typename L>
            typename std::enable_if<std::is_arithmetic_v<T>, JsonObject>::type
        number(const ID id, T value, const L label){ return number_constrained(id, value, label); };

        /**
         * @brief - create an option element for "select" drop-down list
         * 
         * @warning returned JsonObject is invalidated on execution any of json_frame_clear()/json_frame_send()/json_frame_flush() calls
         * @warning a lifetime of the returned JsonObject is limited to a lifetime of Interface object
         */
        template <typename T, typename L>
        JsonObject option(const T value, const L label);

        /**
         * @brief password input field
         * value of this field is always interpreted as text string
         * 
         * @warning returned JsonObject is invalidated on execution any of json_frame_clear()/json_frame_send()/json_frame_flush() calls
         * @warning a lifetime of the returned JsonObject is limited to a lifetime of Interface object
         * 
         * @tparam ID 
         * @tparam L 
         * @param id 
         * @param value 
         * @param label 
         * @return std::enable_if<embui_traits::is_string_v<ID>,JsonObject>::type 
         */
        template <typename ID, typename L>
            typename std::enable_if<embui_traits::is_string_v<ID>,JsonObject>::type
        password(const ID id, const L value, const L label){ return html_input(id, P_password, value, label); };

        /**
         * @brief create live progressbar based on div+css
         * progress value should be sent via 'value' frame (json_frame_value)
         * with (id, value) pair
         * 
         * @warning returned JsonObject is invalidated on execution any of json_frame_clear()/json_frame_send()/json_frame_flush() calls
         * @warning a lifetime of the returned JsonObject is limited to a lifetime of Interface object
         * 
         * @param id 
         * @param label 
         */
        template <typename ID, typename L>
        JsonObject progressbar(const ID id, const L label){ return div(id, P_progressbar, 0, label); };

        /**
         * @brief - create "range" (slider) html field with step, min, max constraints
         * Template accepts types suitable to be added to the ArduinoJson document used as a dictionary
         * 
         * @warning returned JsonObject is invalidated on execution any of json_frame_clear()/json_frame_send()/json_frame_flush() calls
         * @warning a lifetime of the returned JsonObject is limited to a lifetime of Interface object
         */
        template <typename ID, typename T, typename L>
            typename std::enable_if<embui_traits::is_string_v<ID>,JsonObject>::type
        range(const ID id, T value, T min, T max, T step, const L label, bool onChange = false);

        /**
         * @brief - create drop-down selection list
         * after select element is opened, the 'option' elements must follow with list items.
         * After last item, 'select' section must be closed with json_section_end()
         * @note content of a large lists could be loaded with ajax from the client's side
         * 
         * @warning returned JsonObject is invalidated on execution any of json_frame_clear()/json_frame_send()/json_frame_flush() calls
         * @warning a lifetime of the returned JsonObject is limited to a lifetime of Interface object
         * 
         * @param exturl - an url for xhr request to fetch list content, it must be a valid json object
         * with label/value pairs arranged in assoc array
         */
        template <typename ID, typename T, typename L = const char*>
        JsonObject select(const ID id, const T value, const L label = P_EMPTY, bool onChange = false, const L exturl = P_EMPTY);

        /**
         * @brief create spacer line with optional text label
         * 
         * @warning returned JsonObject is invalidated on execution any of json_frame_clear()/json_frame_send()/json_frame_flush() calls
         * @warning a lifetime of the returned JsonObject is limited to a lifetime of Interface object
         * 
         * @param label 
         */
        template <typename L = const char*>
            typename std::enable_if<embui_traits::is_string_v<L>,JsonObject>::type
        spacer(const L label = P_EMPTY);

        /**
         * @brief html text-input field
         * 
         * @warning returned JsonObject is invalidated on execution any of json_frame_clear()/json_frame_send()/json_frame_flush() calls
         * @warning a lifetime of the returned JsonObject is limited to a lifetime of Interface object
         * 
         * @param id 
         * @param value 
         * @param label 
         */
        template <typename ID, typename V, typename L>
            typename std::enable_if<embui_traits::is_string_v<L>,JsonObject>::type
        text(const ID id, const V value, const L label){ return html_input(id, P_text, value, label); };

        /**
         * @brief html textarea field
         * Template accepts literal types as textarea value
         * 
         * @warning returned JsonObject is invalidated on execution any of json_frame_clear()/json_frame_send()/json_frame_flush() calls
         * @warning a lifetime of the returned JsonObject is limited to a lifetime of Interface object
         */
        template <typename ID, typename V, typename L>
            typename std::enable_if<embui_traits::is_string_v<V>,JsonObject>::type
        textarea(const ID id, const V value, const L label);

        /**
         * @brief html time selector
         * Template accepts literal types as time value
         * 
         * @warning returned JsonObject is invalidated on execution any of json_frame_clear()/json_frame_send()/json_frame_flush() calls
         * @warning a lifetime of the returned JsonObject is limited to a lifetime of Interface object
         */
        template <typename ID, typename V, typename L>
            typename std::enable_if<embui_traits::is_string_v<V>,JsonObject>::type
        time(const ID id, const V value, const L label){ return html_input(id, P_time, value, label); };

        /**
         * @brief load uidata on the front-end side
         * this method generates object with instruction to load uidata object structure from specified url
         * and save it under specified key
         * this method should be wrapped in json_section_uidata()
         * 
         * @warning returned JsonObject is invalidated on execution any of json_frame_clear()/json_frame_send()/json_frame_flush() calls
         * @warning a lifetime of the returned JsonObject is limited to a lifetime of Interface object
         * 
         * @param key - a key to load data to, a dot sepparated notation (i.e. "app.page.controls")
         * @param url - url to fetch json from, could be relative to /
         * @param merge - if 'true', then try to merge/update data under existing key, otherwise replace it
         */
        JsonObject uidata_xload(const char* key, const char* url, bool merge = false, unsigned version = 0);

        /**
         * @brief merge data into uidata storage
         * generates object with instruction to merge into uidata an object structure from specified url
         * and save it under specified key 
         * this method should be wrapped in json_section_uidata()
         * 
         * @param url - url to load json from
         * @param key - a key to load data to, a dot sepparated notation (i.e. "app.page.controls")
         * @param source - a source key to load a subset object data from, a dot sepparated notation (i.e. "app.page.controls")
         */
        void uidata_xmerge(const char* url, const char* key, const char* source = NULL );

        /**
         * @brief pick and implace UI structured data objects from front-end side-storage
         * 
         * @warning returned JsonObject is invalidated on execution any of json_frame_clear()/json_frame_send()/json_frame_flush() calls
         * @warning a lifetime of the returned JsonObject is limited to a lifetime of Interface object
         * 
         * @param key - a key to load data from, a dot sepparated notation (i.e. "app.page.controls")
         * @param prefix - if not null, then all ID's in loaded objects will be prepended with provided prefix
         * @param suffix - if not null, then to all ID's in loaded objects will be concatenated with provided suffix
         */
        JsonObject uidata_pick(const char* key, const char* prefix = NULL, const char* suffix = NULL);

        /**
         * @brief - Add 'value' object to the Interface frame
         * used to replace values of existing ui elements on the page
         * Template accepts types suitable to be added to the ArduinoJson document used as a dictionary
         * 
         * @warning returned JsonObject is invalidated on execution any of json_frame_clear()/json_frame_send()/json_frame_flush() calls
         * @warning a lifetime of the returned JsonObject is limited to a lifetime of Interface object
         * @note this call should be wrapped in json_frame_value() frame
         */
        template <typename ID, typename T>
        JsonObject value(const ID id, const T value, bool html = false);

        /**
         * @brief - Add the whole JsonObject to the Interface frame
         * actualy it is a copy-object method used to echo back the data to the WebSocket in one-to-many scenarios
         * also could be used to update multiple elements at a time with a dict of key:value pairs
         * Might be 
         * 1) an array of objects, i.e. [{"key1": "val1"}, {"key2":42}, {"key3":"val3"}]
         * 2) an array of labeled objects [{"id":"someid", "value":"someval", "html": true}] - used to update template placeholders
         * @note this call should be wrapped in json_frame_value() frame
         * 
         * @param data - object to add as values. Might be an arrray 
         */
        JsonObject value(const JsonVariantConst data){ return json_object_add(data); }

};


/* *** TEMPLATED CLASSES implementation follows *** */


template <typename T>
JsonObject Interface::button(button_t btype, const T id, const T label, const char* color){
    JsonObject o(json_object_create());
    o[P_html] = P_button;
    o[P_type] = static_cast<uint8_t>(btype);
    o[P_id] = id;
    o[P_label] = label;
    if (!embui_traits::is_empty_string(color)) o[P_color] =color;
    return o;
};

template <typename T, typename V>
JsonObject Interface::button_value(button_t btype, const T id, const V value, const T label, const char* color){
    JsonObject o = button(btype, id, label, color);
    o[P_value] = value;
    return o;
};

template <typename T, typename V>
JsonObject Interface::button_jscallback(const T id, const T label, const char* callback, const V value, const char* color){
    JsonObject o = button(button_t::js, id, label, color);
    o[P_value] = value;
    o[P_function] = callback;
    return o;
};

template <typename ID, typename L>
JsonObject Interface::comment(const ID id, const L label){
    JsonObject o(json_object_create());
    o[P_html] = P_comment;
    o[P_id] = id;
    o[P_label] = label;

    return o;
}

template <typename ID, typename L, typename V>
JsonObject Interface::constant(const ID id, const L label, const V value){
    JsonObject o(json_object_create());
    o[P_html] = P_comment;
    o[P_id] = id;
    o[P_label] = label;
    o[P_value] = value;

    return o;
};

template <typename ID, typename V, typename L>
JsonObject Interface::display(const ID id, V&& value, const L label, String cssclass, const JsonObject params ){
    if (cssclass.isEmpty())	// make css selector like 'class "css" "id"', id used as a secondary distinguisher 
        cssclass = P_display;   // "display is the default css selector"
    cssclass += (char)0x20;
    cssclass += id;
    return div(id, P_html, std::forward<V>(value), label, cssclass, params);
};

template <typename ID, typename V, typename L, typename CSS>
JsonObject Interface::div(const ID id, const ID type, const V value, const L label, const CSS css, const JsonVariantConst params){
    JsonObject o(json_object_create());
    o[P_html] = P_div;
    o[P_id] = id;
    o[P_label] = label;
    o[P_value] = value;
    o[P_type] = type;
    if (!embui_traits::is_empty_string(css)) o[P_class] = css;

    if (!params.isNull()){
        o[P_params] = params;
    }

    return o;
};

template <typename ID, typename V, typename L>
    typename std::enable_if<embui_traits::is_string_v<V>,JsonObject>::type
Interface::file_form(const ID id, const V action, const L label, const L opt){
    JsonObject o(json_object_create());
    o[P_html] = P_file;
    o[P_id] = id;
    o[P_label] = label;
    o[P_action] = action;
    if (!embui_traits::is_empty_string(opt)) o[P_opt] = opt;
    return o;
}

template <typename ID, typename V>
JsonObject Interface::hidden(const ID id, const V value){
    JsonObject o(json_object_create());
    o[P_html] = P_hidden;
    o[P_id] = id;
    o[P_value] = value;
    return o;
};

template <typename TString>
JsonObject Interface::json_frame_jscall(const TString& function){
    json_frame_flush();         // ensure to start a new frame
    json[P_pkg] = P_jscall;
    json[P_jsfunc] = function;
    json[P_final] = false;
    return json_section_begin(P_EMPTY);
};

template <typename TChar>
JsonObject Interface::json_frame_jscall(const TChar* function){
    json_frame_flush();         // ensure to start a new frame
    json[P_pkg] = P_jscall;
    json[P_jsfunc] = function;
    json[P_final] = false;
    return json_section_begin(P_EMPTY);
};

template  <typename TAdaptedString, typename L>
void Interface::_json_section_begin(TAdaptedString name, const L label, bool main, bool hidden, bool line, bool replace, JsonObject obj){
    if (embui_traits::is_empty_string(name))
        obj[P_section] = String (std::rand()); // need a deep-copy
    else
        obj[P_section] = name;

    if (!embui_traits::is_empty_string(label)) obj[P_label] = label;
    if (main) obj[P_main] = true;
    if (hidden) obj[P_hidden] = true;
    if (line) obj[P_line] = true;
    if (replace) obj[P_replace] = true;

    // add a new section to the stack
    section_stack.emplace_back(obj[P_section].as<const char*>(), obj[P_block].to<JsonArray>());
    LOGD(P_EmbUI, printf, "section begin #%u '%s'\n", section_stack.size(), section_stack.back().name.isEmpty() ? "-" : section_stack.back().name.c_str());   // section index counts from 0, so I print in fo BEFORE adding section to stack
    //return JsonArrayConst(section_stack.back().block);
}

template  <typename TString, typename L>
JsonObject Interface::json_section_begin(const TString& name, const L label, bool main, bool hidden, bool line, bool replace){
    JsonObject obj(section_stack.size() ? section_stack.back().block.add<JsonObject>() : json.as<JsonObject>());
    _json_section_begin(detail::adaptString(name), label, main, hidden, line, replace, obj);
    return obj;
}
template  <typename TChar, typename L>
JsonObject Interface::json_section_begin(const TChar* name, const L label, bool main, bool hidden, bool line, bool replace){
    JsonObject obj(section_stack.size() ? section_stack.back().block.add<JsonObject>() : json.as<JsonObject>());
    _json_section_begin(detail::adaptString(name), label, main, hidden, line, replace, obj);
    return obj;
}

template  <typename ID>
    typename std::enable_if<embui_traits::is_string_v<ID>,JsonObject>::type
Interface::json_section_extend(const ID name){
    section_stack.back().idx--;                                   // decrement section index
    JsonObject o(section_stack.back().block[section_stack.back().block.size()-1]);    // find last array element
    _json_section_begin(name, P_EMPTY, false, false, false, false, o);
    return o;
};

template  <typename ID, typename L>
    typename std::enable_if<embui_traits::is_string_v<ID>,JsonObject>::type
Interface::json_section_manifest(const ID appname, const char* devid, unsigned appjsapi, const L appversion){
    JsonObject obj = json_section_begin("manifest");
    //JsonObject obj( section_stack.back().block.add<JsonObject>() );
    obj[P_uijsapi] = EMBUI_JSAPI;
    obj[P_uiver] = EMBUI_VERSION_STRING;
    obj["uiobjects"] = EMBUI_UIOBJECTS;
    obj[P_app] = appname;
    obj[P_appjsapi] = appjsapi;
    if (!embui_traits::is_empty_string(appversion)) obj["appver"] = appversion;
    obj["mc"] = devid;
    return obj;
}

template <typename ID, typename T, typename L>
    typename std::enable_if<std::is_arithmetic_v<T>, JsonObject>::type
Interface::number_constrained(const ID id, T value, const L label, T step, T min, T max){
    JsonObject o(json_object_create());
    o[P_html] = P_input;
    o[P_id] = id;
    o[P_label] = label;
    o[P_value] = value;
    if (min) o[P_min] = min;
    if (max) o[P_max] = max;
    if (step) o[P_step] = step;

    return o;
};

template <typename T, typename L>
JsonObject Interface::option(const T value, const L label){
    JsonObject o(json_object_create());
    o[P_label] = label;
    o[P_value] = value;
    return o;
}

template <typename ID, typename T, typename L>
    typename std::enable_if<embui_traits::is_string_v<ID>,JsonObject>::type
Interface::range(const ID id, T value, T min, T max, T step, const L label, bool onChange){
    JsonObject o(json_object_create());
    o[P_html] = P_input;
    o[P_id] = id;
    o[P_type] = "range";
    o[P_label] = label;
    o[P_value] = value;
    o[P_min] = min;
    o[P_max] = max;
    o[P_step] = step;
    if (onChange) o[P_onChange] = true;

    return o;
};

template <typename ID, typename T, typename L>
JsonObject Interface::select(const ID id, const T value, const L label, bool onChange, const L exturl){
    JsonObject o(json_object_create());
    o[P_html] = P_select;
    o[P_id] = id;
    o[P_label] = label;
    o[P_value] = value;
    if (onChange) o[P_onChange] = true;
    if (!embui_traits::is_empty_string(exturl)) o[P_url] = exturl;

    // open new nested section for 'option' elements
    json_section_extend(P_options);
    return o;
};

template <typename L>
    typename std::enable_if<embui_traits::is_string_v<L>,JsonObject>::type
Interface::spacer(const L label){
    JsonObject o(json_object_create());
    o[P_html] = P_spacer;
    o[P_label] = label;
    return o;
}

template <typename ID, typename V, typename L>
    typename std::enable_if<embui_traits::is_string_v<V>,JsonObject>::type
Interface::textarea(const ID id, const V value, const L label){
    JsonObject o(json_object_create());
    o[P_html] = P_textarea;
    o[P_id] = id;
    o[P_label] = label;
    o[P_value] = value;
    return o;
};

template <typename ID, typename T>
JsonObject Interface::value(const ID id, const T value, bool html){
    JsonObject o(json_object_create());
    if (html){
        o[P_id] = id;
        o[P_value] = value;
        if (html) o[P_html] = true;
    } else {
        o[id] = value;
    }
    return o;
};

template <typename TChar, typename V, typename L>
    std::enable_if_t<embui_traits::is_string_ptr_v<TChar*>, JsonObject>           //  ValidCharPtr_t<TChar>
Interface::html_input(const TChar* id, const char* type, const V value, const L label, bool onChange){
    JsonObject o(json_object_create());
    o[P_html] = P_input;
    o[P_id] = id;
    o[P_type] = type;
    o[P_label] = label;
    o[P_value] = value;
    if (onChange) o[P_onChange] = true;
    return o;
};

template <typename TString, typename V, typename L>
    std::enable_if_t<embui_traits::is_string_obj_v<TString>, JsonObject>     //  ValidStringRef_t<TString>
Interface::html_input(const TString& id, const char* type, const V value, const L label, bool onChange){
    JsonObject o(json_object_create());
    o[P_html] = P_input;
    o[P_id] = id;
    o[P_type] = type;
    o[P_label] = label;
    o[P_value] = value;
    if (onChange) o[P_onChange] = true;
    return o;
};
