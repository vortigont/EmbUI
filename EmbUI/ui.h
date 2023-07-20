// This framework originaly based on JeeUI2 lib used under MIT License Copyright (c) 2019 Marsel Akhkamov
// then re-written and named by (c) 2020 Anton Zolotarev (obliterator) (https://github.com/anton-zolotarev)
// also many thanks to Vortigont (https://github.com/vortigont), kDn (https://github.com/DmytroKorniienko)
// and others people

#pragma once

#include "EmbUI.h"
#include <type_traits>

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
    email,
    file,
    form,
    hidden,
    iframe,
    input,
    option,
    password,
    range,
    select,
    spacer,
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

// type traits we use for various literals
namespace embui_traits{

template<typename T>
struct is_string : public std::disjunction<
        std::is_same<char *, std::decay_t<T>>,
        std::is_same<const char *, std::decay_t<T>>,
        std::is_same<std::string, std::decay_t<T>>,
        std::is_same<std::string_view, std::decay_t<T>>,
        std::is_same<String, std::decay_t<T>>
    > {};

// value helper
template<typename T>
inline constexpr bool is_string_v = is_string<T>::value;

template<typename T>
inline constexpr bool is_string_t = is_string<T>::type;

template<typename T>
typename std::enable_if<is_string_v<T>,bool>::type
is_empty_string(const T &label){
    if constexpr(std::is_same_v<std::string_view, std::decay_t<decltype(label)>>)       // specialisation for std::string_view
        return label.empty();
    if constexpr(std::is_same_v<std::string, std::decay_t<decltype(label)>>)            // specialisation for std::string
        return label.empty();
    if constexpr(std::is_same_v<String, std::decay_t<decltype(label)>>)                 // specialisation for String
        return label.isEmpty();
    if constexpr(std::is_same_v<const char*, std::decay_t<decltype(label)>>)            // specialisation for const char*
        return not (label && *label);
    if constexpr(std::is_same_v<char*, std::decay_t<decltype(label)>>)                  // specialisation for char*
        return not (label && *label);

    return false;   // UB, not a known string type for us
};

}

template <size_t capacity = UI_DEFAULT_JSON_SIZE>
class UIelement {
protected:
    ui_element_t _t;

public:
    StaticJsonDocument<capacity> obj;

    template <typename T>
    UIelement(ui_element_t t, const T& id) : _t(t) {
        if (!embui_traits::is_empty_string(id))
            obj[P_id] = id;

        // check if element type is within allowed range
        if ((uint8_t)t < UI_T_DICT_SIZE){
            switch(t){
                case ui_element_t::custom :     // some elements does not need html_type
                case ui_element_t::option :
                case ui_element_t::value :
                    return;
                default :                       // default is to set element type from dict
                    obj[P_html] = UI_T_DICT[static_cast<uint8_t>(t)];
            }
        }
    };

    UIelement(ui_element_t t) : UIelement(t, (char*)0) {};

    template <typename T, typename V>
    UIelement(ui_element_t t, const T &id, const V &value) : UIelement(t, id ){
        obj[P_value] = value;
    };

    template <typename T>
    void param(ui_param_t key, const T &value){ obj[UI_KEY_DICT[static_cast<uint8_t>(key)]] = value; };

    /**
     * @brief a wrapper method to add key:value pair to json document
     * could be simply replaced with plain obj[key] = value;
     * 
     */
    template <typename K, typename V>
        typename std::enable_if<embui_traits::is_string_v<K>,void>::type
    param(const K &key, const V &value){ obj[key] = value; };
    
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
        typename std::enable_if<embui_traits::is_string_v<T>,void>::type
    label(const T &string){ if (!embui_traits::is_empty_string(string)) obj[P_label] = string; }

    /**
     * @brief add 'color' key to the UI element
     * 
     * @tparam T color literal type
     * @param label 
     */
    template <typename T>
        typename std::enable_if<embui_traits::is_string_v<T>,void>::type
    color(const T &color){ if (!embui_traits::is_empty_string(color)) obj[P_color] = color; }

    /**
     * @brief add 'color' key to the UI element
     * 
     * @tparam T color literal type
     * @param label 
     */
    template <typename T>
    void value(const T &v){ obj[P_value] = v; }

};

template <size_t S = UI_DEFAULT_JSON_SIZE>
class UI_button : public UIelement<S> {
public:
    using UIelement<S>::value;
    using UIelement<S>::color;

    template <typename T, typename L>
    UI_button(button_t btype, const T &id, const L &label) : UIelement<S>(ui_element_t::button, id) {
        UIelement<S>::obj[P_type] = static_cast<uint8_t>(btype);
        UIelement<S>::label(label);
    };
};

class frameSend {
    public:
        virtual ~frameSend(){};
        virtual void send(const String &data){};
        virtual void send(const JsonObject& data){};
        virtual void flush(){}
};

class frameSendAll: public frameSend {
    private:
        AsyncWebSocket *ws;
    public:
        frameSendAll(AsyncWebSocket *server) : ws(server){}
        ~frameSendAll() { ws = nullptr; }
        void send(const String &data){ if (!data.isEmpty()) ws->textAll(data); };
        void send(const JsonObject& data);
};

class frameSendClient: public frameSend {
    private:
        AsyncWebSocketClient *cl;
    public:
        frameSendClient(AsyncWebSocketClient *client) : cl(client){}
        ~frameSendClient() { cl = nullptr; }
        void send(const String &data){ if (!data.isEmpty()) cl->text(data); };
        /**
         * @brief - serialize and send json obj directly to the ws buffer
         */
        void send(const JsonObject& data);
};

class frameSendHttp: public frameSend {
    private:
        AsyncWebServerRequest *req;
        AsyncResponseStream *stream;
    public:
        frameSendHttp(AsyncWebServerRequest *request) : req(request) {
            stream = req->beginResponseStream(PGmimejson);
            stream->addHeader(PGhdrcachec, PGnocache);
        }
        ~frameSendHttp() { /* delete stream; */ req = nullptr; }
        void send(const String &data){
            if (!data.length()) return;
            stream->print(data);
        };
        /**
         * @brief - serialize and send json obj directly to the ws buffer
         */
        void send(const JsonObject& data){
            serializeJson(data, *stream);
        };
        void flush(){
            req->send(stream);
        };
};

class Interface {
    typedef struct section_stack_t{
      JsonArray block;
      String name;
      int idx;
    } section_stack_t;

    EmbUI *embui;
    DynamicJsonDocument json;
    LList<section_stack_t*> section_stack;
    frameSend *send_hndl;

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
    bool json_frame_enqueue(const JsonObject &obj, bool shallow = false);

    /**
     * @brief purge json object while keeping section structure
     * used to release mem after json_frame_send() call
     */
    void json_frame_next();

    /**
     * @brief - serialize and send Interface object to the WebSocket
     */
    inline void json_frame_send(){ if (send_hndl) send_hndl->send(json.as<JsonObject>()); };

    /**
     * @brief - start UI section
     * A section contains DOM UI elements, this is generic one
     */
    void json_section_begin(const String &name, const String &label, bool main, bool hidden, bool line, JsonObject obj);


    public:
        Interface(EmbUI *j, AsyncWebSocket *server, size_t size = IFACE_DYN_JSON_SIZE): embui(j), json(size) {
            send_hndl = new frameSendAll(server);
        }
        Interface(EmbUI *j, AsyncWebSocketClient *client, size_t size = IFACE_DYN_JSON_SIZE): embui(j), json(size) {
            send_hndl = new frameSendClient(client);
        }
        Interface(EmbUI *j, AsyncWebServerRequest *request, size_t size = IFACE_DYN_JSON_SIZE): embui(j), json(size) {
            send_hndl = new frameSendHttp(request);
        }
        ~Interface(){
            json_frame_clear();
            delete send_hndl;
            send_hndl = nullptr;
            embui = nullptr;
        }


        /**
         * @brief - begin UI secton of the specified <type>
         * generic frame creation method, used by other calls to create custom-typed frames
         */
        void json_frame(const String &type);

        /**
         * @brief - add object to current Interface frame
         * attempts to retry on mem overflow
         */
        void json_frame_add(const JsonObject &obj);
        template <size_t capacity>
        void json_frame_add( UIelement<capacity> &ui){ json_frame_add(ui.obj.template as<JsonObject>()); }

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
        inline void json_frame_interface(){ json_frame(P_interface); };

        /**
         * @brief - begin Value UI secton
         * used to supply WebUI with data (key:value pairs)
         */
        inline void json_frame_value(){ json_frame(P_value); };

        /**
         * @brief - begin Value UI secton with supplied json object
         * used to supply WebUI with data (key:value pairs)
         * @param val json object with supplied data to be copied
         * @param shallow use 'shallow' copy, be SURE to keep val object alive intact until frame is fully send
         *                with json_frame_send() or json_frame_flush()
         */
        void json_frame_value(JsonVariant &val, bool shallow = false);

        /**
         * @brief start UI section
         * A section contains DOM UI elements, this is generic method
         * 
         * @param name - name identifies a group of DOM objects, could be arbitrary name.
         *               Some names are reserved:
         *                  'menu' - contains left-side menu elements
         *                  'content' - updates values for exiting DOM elements  
         * @param label - a headliner for a group of section elements
         * @param main  - main section starts a blank page, may contain other nested sections
         * @param hidden - creates section hidden under 'spoiler' button, user needs to press the button to unfold it
         * @param line  -  all elements of the section will be alligned in a line
         */
        void json_section_begin(const String &name, const String &label = (char*)0, bool main = false, bool hidden = false, bool line = false);

        /**
         * @brief - content section is meant to replace existing data on the page
         */
        inline void json_section_content(){ json_section_begin(F("content")); };

        /**
         * @brief - opens section for UI elements that are aligned in one line on a page
         * each json_section_line() must be closed by a json_section_end() call
         */
        inline void json_section_line(const String &name = (char*)0){ json_section_begin(name, (char*)0, false, false, true); };

        /**
         * @brief - section with manifest data
         * it contains manifest data for WebUI, like fw name, version, Chip ID
         * and any arbitrary json'ed data that could be processed in user js code
         * could be supplied with additional date via (optional) value() call, otherwise just closed
         * @param appname - application name, builds Document title on the page
         * @param fwversion - numeric firmware version, this is for compatibility checking between fw and user JS code,
         * if 0 - than no checking required
         */
        void json_section_manifest(const String &appname, unsigned appjsapi = 0, const String &appversion = (char*)0);

        /**
         * @brief - start a section with left-side MENU elements
         */
        inline void json_section_menu(){ json_section_begin(P_menu); };

        /**
         * @brief - start a section with a new page content
         * it replaces current page from scratch, may contain other nested sections
         */
        inline void json_section_main(const String &name, const String &label){ json_section_begin(name, label, true); };

        /**
         * @brief - start hidden UI section with button to hide/unhide elements
         */
        inline void json_section_hidden(const String &name, const String &label){ json_section_begin(name, label, false, true); };

        /**
         * @brief opens nested json_section using previous section's index
         * i.e. extend previous object with nested elements.
         * used for elements like html 'select'+'option', 'form'+'inputs', etc...
         * each extended section MUST be closed with json_section_end() prior to opening new section
         * @param name extended section name
         */
        void json_section_extend(const String &name);

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
         *  type js - call a js function with name ~ id
         */
        template <typename T>
        void button(button_t btype, const T &id, const T &label, const char* color = reinterpret_cast<char*>(0) );

        /**
         * @brief - create html button to submit itself's id, value, section form or else
         * depend on type button can send:
         *  type 0 - it's id + value
         *  type 1 - submit a form with section + it's own id + value
         *  type 2 - call a js function with name ~ id + pass it a value
         */
        template <typename T, typename V>
        void button_value(button_t btype, const T &id, const V &value, const T &label, const char* color = reinterpret_cast<char*>(0));

        /**
         * @brief - элемент интерфейса checkbox
         * @param onChange - значение чекбокса при изменении сразу передается на сервер без отправки формы
         */
        template <typename ID, typename L>
        void checkbox(const ID &id, bool value, const L &label, bool onChange = false){ html_input(id, P_chckbox, value, label, onChange); };

        /**
         * @brief - элемент интерфейса checkbox, значение чекбокса выбирается из одноменного параметра системного конфига
         * @param onChange - значение чекбокса при изменении сразу передается на сервер без отправки формы
         */
        template <typename ID, typename L>
        void checkbox_cfg(const ID &id, const L &label, bool onChange = false){ html_input(id, P_chckbox, embui->paramVariant(id), label, onChange); };

        /**
         * @brief - элемент интерфейса "color selector"
         */
        template <typename ID, typename V, typename L>
        void color(const ID &id, const V &value, const L &label){ html_input(id, P_color, value, label); };

        /**
         * @brief insert text comment (a simple <p>text</p> block)
         * 
         * @param id comment id (might be used to replace value with content() method)
         * @param label - text label
         */
        template <typename ID, typename L>
        void comment(const ID &id, const L &label);

        /**
         * @brief Create inactive button with a visible label that does nothing but (possibly) carries value
         *  could be used same way as 'hidden' element
         */
        template <typename ID, typename L, typename V=int>
        void constant(const ID &id, const L &label, const V &value=0);

        template <typename L>
        void constant(const L &label){ constant((char*)0, label); };

        inline void date(const String &id, const String &value, const String &label){ html_input(id, P_date, value, label); };
        inline void date(const String &id, const String &label){ html_input(id, P_date, embui->paramVariant(id), label); };

        inline void datetime(const String &id, const String &value, const String &label){ html_input(id, P_datetime, value, label); };
        inline void datetime(const String &id, const String &label){ html_input(id, P_datetime, embui->paramVariant(id), label); };

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
        template <typename T>
        void display(const String &id, const T &value, const String &label = (char*)0, const String &css = (char*)0, const JsonObject &params = JsonObject() );

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
        template <typename T>
        void div(const String &id, const String &type, const T &value, const String &label = (char*)0, const String &css = (char*)0, const JsonObject &params = JsonObject());

        inline void email(const String &id, const String &value, const String &label){ html_input(id, P_email, value, label); };
        inline void email(const String &id, const String &label){ html_input(id, P_email, embui->param(id), label); };

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
        void file_form(const String &id, const String &action, const String &label, const String &opt = (char*)0);

        /**
         * @brief Create hidden html field
         * could be used to pass some data between different sections
         */
        inline void hidden(const String &id){ hidden(id, embui->paramVariant(id)); };
        template <typename T>
        void hidden(const String &id, const T &value);

        /**
         * @brief - create generic html input element
         * @param id - element id
         * @param type - html element type, ex. 'text'
         * @param value - element value
         * @param label - element label
         * @param direct - if true, element value in send via ws on-change 
         */
        template <typename ID, typename V, typename L>
        void html_input(const ID &id, const char* type, const V &value, const L &label, bool onChange = false);

        /**
         * @brief create an iframe on page
         * 
         * @param id 
         * @param value 
         */
        void iframe(const String &id, const String &value);

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
        inline void jscall(const String &id, const String &value, const String &label = (char*)0, const String &css = (char*)0, const JsonObject &params = JsonObject() ){ div(id, P_js, value, label, css, params); };

        /**
         * @brief - create "number" html field with optional step, min, max constraints
         * implies constrains on number input field.
         * Template accepts types suitable to be added to the ArduinoJson document used as a dictionary
         * front-end converts numeric values to integers or floats
         */
        template <typename T>
        void number_constrained(const String &id, T value, const String &label, T step, T min = 0, T max = 0);

        /**
         * @brief constrained number filled from system config
         * it is limited to Int's only due type in config can't be deducted in a template
         * for any other types than Int's use overload with value
         * 
         */
        inline void number_constrained(const String &id, const String &label, int step, int min = 0, int max = 0){ number_constrained(id, embui->paramVariant(id).as<int>(), label, step, min, max); };

        /**
         * @brief - create "number" html field with optional step, min, max constraints
         * Template accepts types suitable to be added to the ArduinoJson document used as a dictionary
         * front-end converts numeric values to integers or floats
         */
        template <typename V>
        inline void number(const String &id, V value, const String &label){ number_constrained(id, value, label, static_cast<V>(0)); };

        /**
         * @brief number filled from system config
         * it is limited to Int's only due type in config can't be deducted in a template
         * for any other types than Int's use overload with value
         * 
         */
        inline void number(const String &id, const String &label){ number_constrained(id, embui->paramVariant(id).as<int>(), label, 0); };

        /**
         * @brief - create an option element for "select" drop-down list
         */
        template <typename T>
        void option(const T &value, const String &label);

        inline void password(const String &id, const String &value, const String &label){ html_input(id, P_password, value, label); };
        inline void password(const String &id, const String &label){ html_input(id, P_password, embui->param(id), label); };

        /**
         * @brief create live progressbar based on div+css
         * progress value should be sent via 'value' frame (json_frame_value)
         * with (id, value) pair
         * 
         * @param id 
         * @param label 
         */
        inline void progressbar(const String &id, const String &label){ div(id, P_progressbar, 0, label); };

        /**
         * @brief - create "range" html field with step, min, max constraints
         * Template accepts types suitable to be added to the ArduinoJson document used as a dictionary
         */
        template <typename T>
        inline void range(const String &id, T min, T max, T step, const String &label, bool directly = false){ range(id, embui->paramVariant(id), min, max, step, label, directly); };

        /**
         * @brief - create "range" html field with step, min, max constraints
         * Template accepts types suitable to be added to the ArduinoJson document used as a dictionary
         */
        template <typename V, typename T>
        void range(const String &id, V value, T min, T max, T step, const String &label, bool directly);

        /**
         * @brief - create drop-down selection list
         * after select element is opened, the 'option' elements must follow with list items.
         * After last item, 'select' section must be closed with json_section_end()
         * 
         * content of a large lists could be loaded with ajax from the client's side
         * @param exturl - an url for xhr request to fetch list content, it must be a valid json object
         * with label/value pairs arranged in assoc array
         */
        template <typename T>
        void select(const String &id, const T &value, const String &label = (char*)0, bool onChange = false, const String &exturl = (char*)0);
        inline void select(const String &id, const String &label, bool onChange = false, const String &exturl = (char*)0){ select(id, embui->paramVariant(id), label, onChange, exturl); };

        /**
         * @brief create spacer line with optional text label
         * 
         * @param label 
         */
        void spacer(const String &label = (char*)0);

        /**
         * @brief html text-input field
         * 
         * 
         * @param id 
         * @param value 
         * @param label 
         * @param directly 
         */
        inline void text(const String &id, const String &value, const String &label){ html_input(id, P_text, value, label); };
        inline void text(const String &id, const String &label){ html_input(id, P_text, embui->param(id), label); };

        /**
         * элемент интерфейса textarea
         * Template accepts types suitable to be added to the ArduinoJson document used as a dictionary
         */
        void textarea(const String &id, const String &value, const String &label);
        inline void textarea(const String &id, const String &label){ textarea(id, embui->param(id), label); };

        inline void time(const String &id, const String &value, const String &label){ html_input(id, P_time, value, label); };
        inline void time(const String &id, const String &label){ html_input(id, P_time, embui->paramVariant(id), label); };

        /**
         * @brief - Add 'value' object to the Interface frame
         * used to replace values of existing ui elements on the page
         * Template accepts types suitable to be added to the ArduinoJson document used as a dictionary
         */
        template <typename T>
        void value(const String &id, const T& val, bool html = false);

        /**
         * @brief - Add embui's config param id as a 'value' to the Interface frame
         */
        //inline void value(const String &id, bool html = false){ value(id, embui->paramVariant(id), html); };

        /**
         * @brief - Add the whole JsonObject to the Interface frame
         * actualy it is a copy-object method used to echo back the data to the WebSocket in one-to-many scenarios
         * also could be used to update multiple elements at a time with a dict of key:value pairs
         */
        inline void value(const JsonObject &data){ json_frame_add(data); }


};


/* *** TEMPLATED CLASSES implementation follows *** */


template <typename T>
void Interface::button(button_t btype, const T &id, const T &label, const char* color){
    UI_button<TINY_JSON_SIZE> ui(btype, id, label);
    ui.color(color);

    json_frame_add(ui);
};

template <typename T, typename V>
void Interface::button_value(button_t btype, const T &id, const V &value, const T &label, const char* color){
    UI_button<TINY_JSON_SIZE> ui(btype, id, label);
    ui.obj[P_value] = value;
    ui.color(color);

    json_frame_add(ui);
};

template <typename ID, typename L>
void Interface::comment(const ID &id, const L &label){
    UIelement<UI_DEFAULT_JSON_SIZE * 2> ui(ui_element_t::comment, id);     // use a bit larger buffer for long texts
    ui.label(label);
    json_frame_add(ui);
}

template <typename ID, typename L, typename V>
void Interface::constant(const ID &id, const L &label, const V &value){
    UIelement<UI_DEFAULT_JSON_SIZE> ui(ui_element_t::constant, id, value);
    ui.label(label);
    json_frame_add(ui);
};


template <typename T>
void Interface::display(const String &id, const T &value, const String &label, const String &css, const JsonObject &params ){
    String cssclass(css);   // make css selector like 'class "css" "id"', id used as a secondary distinguisher 
    if (css.isEmpty())
        cssclass = P_display;   // "display is the default css selector"
    cssclass += (char)0x20;
    cssclass += id;
    div(id, P_html, value, label, cssclass, params);
};

template <typename T>
void Interface::div(const String &id, const String &type, const T &value, const String &label, const String &css, const JsonObject &params){
    UIelement<UI_DEFAULT_JSON_SIZE> ui(ui_element_t::div, id, value);
    ui.label(label);
    ui.obj[P_type] = type;
    if (!css.isEmpty())
        ui.obj[P_class] = css;
    if (!params.isNull()){
        JsonVariant nobj = ui.obj.createNestedObject(P_params);
        nobj.shallowCopy(params);
    }
    json_frame_add(ui);
};

template <typename T>
void Interface::hidden(const String &id, const T &value){
    UIelement<UI_DEFAULT_JSON_SIZE> ui(ui_element_t::hidden, id);
    ui.value(value);
    json_frame_add(ui);
};

template <typename T>
void Interface::number_constrained(const String &id, T value, const String &label, T step, T min, T max){
    UIelement<TINY_JSON_SIZE> ui(ui_element_t::input, id, value);
    ui.obj[P_type] = P_number;
    ui.label(label);
    if (min) ui.obj[P_min] = min;
    if (max) ui.obj[P_max] = max;
    if (step) ui.obj[P_step] = step;
    json_frame_add(ui);
};

template <typename T>
void Interface::option(const T &value, const String &label){
    UIelement<TINY_JSON_SIZE> ui(ui_element_t::option, static_cast<const char*>(0), value, label.c_str());
    json_frame_add(ui);
}

template <typename V, typename T>
void Interface::range(const String &id, V value, T min, T max, T step, const String &label, bool directly){
    UIelement<TINY_JSON_SIZE> ui(ui_element_t::input, id, value, label);
    ui.obj[P_type] = F("range");
    ui.obj[P_min] = min;
    ui.obj[P_max] = max;
    ui.obj[P_step] = step;
    if (directly) ui.obj[P_directly] = true;
    json_frame_add(ui);
};

template <typename T>
void Interface::select(const String &id, const T &value, const String &label, bool onChange, const String &exturl){
    UIelement<UI_DEFAULT_JSON_SIZE> ui(ui_element_t::select, id, value);
    ui.label(label);
    if (onChange) ui.obj[P_directly] = true;
    if (!exturl.isEmpty())
        ui.obj[P_url] = exturl;
    json_frame_add(ui);
    // open new nested section for 'option' elements
    json_section_extend(P_options);
};


template <typename T>
void Interface::value(const String &id, const T& val, bool html){
    UIelement<TINY_JSON_SIZE> ui(ui_element_t::value, id);
    ui.obj[P_value] = val;
    ui.html(html);
    json_frame_add(ui);
};

template <typename ID, typename V, typename L>
void Interface::html_input(const ID &id, const char* type, const V &value, const L &label, bool onChange){
    UIelement<TINY_JSON_SIZE> ui(ui_element_t::input, id, value);
    ui.label(label);
    ui.obj[P_type] = type;
    if (onChange) ui.obj[P_directly] = true;
    json_frame_add(ui);
};
