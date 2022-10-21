// This framework originaly based on JeeUI2 lib used under MIT License Copyright (c) 2019 Marsel Akhkamov
// then re-written and named by (c) 2020 Anton Zolotarev (obliterator) (https://github.com/anton-zolotarev)
// also many thanks to Vortigont (https://github.com/vortigont), kDn (https://github.com/DmytroKorniienko)
// and others people

#pragma once

#include "EmbUI.h"

// static json obj size for tiny ui elements, like checkboxes, number inputs, etc...
#ifndef TINY_JSON_SIZE
#define TINY_JSON_SIZE      256
#endif

// static json obj size for small ui elements
#ifndef SMALL_JSON_SIZE
#define SMALL_JSON_SIZE     512
#endif

// dynamic json for creating websocket frames to be sent to UI
#ifndef IFACE_DYN_JSON_SIZE
#define IFACE_DYN_JSON_SIZE 8192
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

template <size_t desiredCapacity = UI_DEFAULT_JSON_SIZE>
class UIelement {
    ui_element_t _t;

public:
    StaticJsonDocument<desiredCapacity> obj;

    UIelement(ui_element_t t, const String &id);
    UIelement(ui_element_t t) : UIelement(t, (char*)0) {};
    template <typename T>
    UIelement(ui_element_t t, const String &id, const T &value, const String &label) : UIelement(t, id ) {  obj[FPSTR(P_value)] = value; obj[FPSTR(P_label)] = label; };

    template <typename T>
    void param(ui_param_t key, const T &value){ obj[UI_KEY_DICT[(uint8_t)key]] = value; };

    template <typename T>
    void param(const String &key, const T &value){ obj[key] = value; };
    
    /**
     * @brief set 'html' flag for element
     * if set, than value updated for placeholders in html template, otherwise within dynamicaly created elements on page
     * 
     * @param v boolen
     */
    inline void html(bool v){ if (v) obj[FPSTR(P_html)] = true; };

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
            stream = req->beginResponseStream(FPSTR(PGmimejson));
            stream->addHeader(FPSTR(PGhdrcachec), FPSTR(PGnocache));
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

    // UI Button type enum
    enum BType : uint8_t {
        generic = (0U),
        submit,
        js
    };

    EmbUI *embui;
    DynamicJsonDocument json;
    LList<section_stack_t*> section_stack;
    frameSend *send_hndl;


    /**
     * @brief append supplied json data to the current Interface frame
     * this function is unsafe, i.e. when frame is full and can't append
     * current data it returns false and tries to send current frame and open new one
     * use json frame_frame_add() to add data in a safe way
     * 
     * @param obj 
     * @return true 
     * @return false 
     */
    bool json_frame_enqueue(const JsonObject &obj);


    /**
     * @brief - create html button to submit itself's id, value or section form
     * depend on type button can send:
     *  type 0 - it's id with null value
     *  type 0 - it's id + value
     *  type 1 - submit a section's values + it's own id
     *  type 1 - submit a form with section + it's own id + value
     *  type 2 - call a js function with name ~ id
     *  type 2 - call a js function with name ~ id + pass it a value
     */
    template <typename T>
    void button_generic(const String &id, const T &value, const String &label, const String &color, BType type = generic);

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
         * @brief - begin Interface UI secton
         * used to construct WebUI html elements
         */
        inline void json_frame_interface(){ json_frame(FPSTR(P_interface)); };
        void json_frame_interface(const String &name);

        /**
         * @brief - begin Value UI secton
         * used to supply WebUI with data (key:value pairs)
         */
        inline void json_frame_value(){ json_frame(FPSTR(P_value)); };

        /**
         * @brief - add object to current Intreface frame
         * attempts to retry on mem overflow
         */
        void json_frame_add(const JsonObject &obj);
        template <size_t desiredCapacity>
        void json_frame_add( UIelement<desiredCapacity> &ui){ json_frame_add(ui.obj.template as<JsonObject>()); }

        void json_frame_next();
        
        /**
         * @brief purge all current section data
         * 
         */
        void json_frame_clear();
        
        /**
         * @brief finalize and send current sections stack
         * 
         */
        void json_frame_flush();

        /**
         * @brief - serialize and send Interface object to the WebSocket 
         */
        inline void json_frame_send(){ if (send_hndl) send_hndl->send(json.as<JsonObject>()); };

        /**
         * @brief - start UI section
         * A section contains html UI elements, this is generic one
         */
        void json_section_begin(const String &name, const String &label = (char*)0, bool main = false, bool hidden = false, bool line = false);
        void json_section_begin(const String &name, const String &label, bool main, bool hidden, bool line, JsonObject obj);

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
         * @brief - start a section with left-side MENU elements
         */
        inline void json_section_menu(){ json_section_begin(FPSTR(P_menu)); };

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
         * @brief - create html button
         * A button can send it's id/value or submit a form with section data
         */
        inline void button(const String &id, const String &label, const String &color = (char*)0){ button_generic(id, nullptr, label, color); };
        template <typename T>
        inline void button_value(const String &id, const T &value, const String &label, const String &color = (char*)0){ button_generic(id, value, label, color); };
        inline void button_submit(const String &section, const String &label, const String &color = (char*)0){ button_generic(section, nullptr, label, color, BType::submit); };
        template <typename T>
        inline void button_submit_value(const String &section, const T &value, const String &label, const String &color = (char*)0){ button_generic(section, value, label, color, BType::submit); };
        // Run user-js function on click, id - acts as a js function selector
        inline void button_js(const String &id, const String &label, const String &color = (char*)0){ button_generic(id, nullptr, label, color, BType::js); };
        // Run user-js function on click, id - acts as a js function selector, value - any additional value or object passed to js function
        template <typename T>
        inline void button_js_value(const String &id, const T &value, const String &label, const String &color = (char*)0){ button_generic(id, value, label, color, BType::js); };

        /**
         * @brief - элемент интерфейса checkbox
         * @param directly - значение чекбокса при изменении сразу передается на сервер без отправки формы
         */
        inline void checkbox(const String &id, const bool value, const String &label, const bool directly = false){ html_input(id, FPSTR(P_chckbox), value, label, directly); };
        inline void checkbox(const String &id, const String &label, const bool directly = false){ html_input(id, FPSTR(P_chckbox), embui->paramVariant(id).as<bool>(), label, directly); };

        void color(const String &id, const String &value, const String &label){ html_input(id, FPSTR(P_color), value, label); };
        inline void color(const String &id, const String &label){ html_input(id, FPSTR(P_color), embui->paramVariant(id), label); };

        /**
         * @brief insert text comment (a simple <p>text</p> block)
         * 
         * @param id comment if (might be usefull when replaced with content() method)
         * @param label - text label
         */
        void comment(const String &id, const String &label);
        inline void comment(const String &label){ comment((char*)0, label); };

        /**
         * @brief Create inactive button with a visible label that does nothing but (possibly) carries value
         *  could be used the same way as 'hidden' element
         */
        template <typename T>
        void constant(const String &id, const T &value, const String &label);
        inline void constant(const String &id, const String &label){ constant(id, embui->paramVariant(id), label); };

        inline void date(const String &id, const String &value, const String &label){ html_input(id, FPSTR(P_date), value, label); };
        inline void date(const String &id, const String &label){ html_input(id, FPSTR(P_date), embui->paramVariant(id), label); };

        inline void datetime(const String &id, const String &value, const String &label){ html_input(id, FPSTR(P_datetime), value, label); };
        inline void datetime(const String &id, const String &label){ html_input(id, FPSTR(P_datetime), embui->paramVariant(id), label); };

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

        inline void email(const String &id, const String &value, const String &label){ html_input(id, FPSTR(P_email), value, label); };
        inline void email(const String &id, const String &label){ html_input(id, FPSTR(P_email), embui->paramVariant(id), label); };

        /**
         * @brief create a file uplaod form
         * based in a template creates html form, an input file upload and optional parametr 'opt'
         * that could be processed by templater
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
        template <typename T>
        void html_input(const String &id, const String &type, const T &value, const String &label, bool direct = false);

        /**
         * @brief cleate an iframe on page
         * 
         * @param id 
         * @param value 
         */
        void iframe(const String &id, const String &value);

        /**
         * @brief - create empty div and call js-function over this div
         * js function receives div.id and params obj as arguments
         * js function must be predefined in front-end's WebUI
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
        inline void number(const String &id, V value, const String &label){ number_constrained(id, value, label, 0); };

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

        inline void password(const String &id, const String &value, const String &label){ html_input(id, FPSTR(P_password), value, label); };
        inline void password(const String &id, const String &label){ html_input(id, FPSTR(P_password), embui->paramVariant(id), label); };

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
        template <typename T>
        void range(const String &id, T value, T min, T max, T step, const String &label, bool directly);

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
        void select(const String &id, const T &value, const String &label = (char*)0, bool directly = false, const String &exturl = (char*)0);
        inline void select(const String &id, const String &label, bool directly = false, const String &exturl = (char*)0){ select(id, embui->paramVariant(id), label, directly, exturl); };

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
        inline void text(const String &id, const String &value, const String &label){ html_input(id, FPSTR(P_text), value, label); };
        inline void text(const String &id, const String &label){ html_input(id, FPSTR(P_text), embui->paramVariant(id), label); };

        /**
         * элемент интерфейса textarea
         * Template accepts types suitable to be added to the ArduinoJson document used as a dictionary
         */
        void textarea(const String &id, const String &value, const String &label);
        inline void textarea(const String &id, const String &label){ textarea(id, embui->paramVariant(id), label); };

        inline void time(const String &id, const String &value, const String &label){ html_input(id, FPSTR(P_time), value, label); };
        inline void time(const String &id, const String &label){ html_input(id, FPSTR(P_time), embui->paramVariant(id), label); };

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

template <size_t desiredCapacity>
UIelement<desiredCapacity>::UIelement(ui_element_t t, const String &id) : _t(t) {
    if (!id.isEmpty())
        obj[FPSTR(P_id)] = id;

    // check if element type is withing allowed range
    if ((uint8_t)t < UI_T_DICT_SIZE){
        switch(t){
            case ui_element_t::custom :     // some elements does not need html_type
            case ui_element_t::option :
            case ui_element_t::value :
                return;
            default :                       // default is to set elent type from dict
                obj[FPSTR(P_html)] = FPSTR(UI_T_DICT[(uint8_t)t]);
        }
    }
}

template <typename T>
void Interface::button_generic(const String &id, const T &value, const String &label, const String &color, BType type){
    UIelement<TINY_JSON_SIZE> ui(ui_element_t::button, id, value, label);
    ui.obj[FPSTR(P_type)] = (uint8_t)type;
    if (!color.isEmpty())
        ui.obj[FPSTR(P_color)] = color;

    json_frame_add(ui);
};

template <typename T>
void Interface::constant(const String &id, const T &value, const String &label){
    UIelement<UI_DEFAULT_JSON_SIZE> ui(ui_element_t::constant, id, value, label);
    json_frame_add(ui);
};

template <typename T>
void Interface::display(const String &id, const T &value, const String &label, const String &css, const JsonObject &params ){
    String cssclass(css);   // make css selector like "class id", id used as a secondary distinguisher 
    if (!css.length())
        cssclass += P_display;   // "display is the default css selector"
    cssclass += F(" ");
    cssclass += id;
    div(id, P_html, value, label, cssclass, params);
};

template <typename T>
void Interface::div(const String &id, const String &type, const T &value, const String &label, const String &css, const JsonObject &params){
    UIelement<UI_DEFAULT_JSON_SIZE> ui(ui_element_t::div, id, value, label);
    ui.obj[FPSTR(P_type)] = type;
    if (!css.isEmpty())
        ui.obj[P_class] = css;
    if (!params.isNull()){
        JsonObject nobj = ui.obj.createNestedObject(FPSTR(P_params));
        nobj.set(params);
    }
    json_frame_add(ui);
};

template <typename T>
void Interface::hidden(const String &id, const T &value){
    UIelement<UI_DEFAULT_JSON_SIZE> ui(ui_element_t::hidden, id);
    ui.obj[FPSTR(P_value)] = value;
    json_frame_add(ui);
};

template <typename T>
void Interface::number_constrained(const String &id, T value, const String &label, T step, T min, T max){
    UIelement<TINY_JSON_SIZE> ui(ui_element_t::input, id, value, label);
    ui.obj[FPSTR(P_type)] = FPSTR(P_number);
    if (min) ui.obj[FPSTR(P_min)] = min;
    if (max) ui.obj[FPSTR(P_max)] = max;
    if (step) ui.obj[FPSTR(P_step)] = step;
    json_frame_add(ui);
};

template <typename T>
void Interface::option(const T &value, const String &label){
    UIelement<TINY_JSON_SIZE> ui(ui_element_t::option, (char*)0, value, label);

    json_frame_add(ui);
}

template <typename T>
void Interface::range(const String &id, T value, T min, T max, T step, const String &label, bool directly){
    UIelement<TINY_JSON_SIZE> ui(ui_element_t::input, id, value, label);
    ui.obj[FPSTR(P_type)] = F("range");
    ui.obj[FPSTR(P_min)] = min;
    ui.obj[FPSTR(P_max)] = max;
    ui.obj[FPSTR(P_step)] = step;
    if (directly) ui.obj[FPSTR(P_directly)] = true;
    json_frame_add(ui);
};

template <typename T>
void Interface::select(const String &id, const T &value, const String &label, bool directly, const String &exturl){
    UIelement<UI_DEFAULT_JSON_SIZE> ui(ui_element_t::select, id, value, label);
    if (directly) ui.obj[FPSTR(P_directly)] = true;
    if (!exturl.isEmpty())
        ui.obj[FPSTR(P_url)] = exturl;
    json_frame_add(ui);
    // open new nested section for 'option' elements
    json_section_extend(FPSTR(P_options));
};


template <typename T>
void Interface::value(const String &id, const T& val, bool html){
    UIelement<TINY_JSON_SIZE> ui(ui_element_t::value, id);
    ui.obj[FPSTR(P_value)] = val;
    ui.html(html);
    json_frame_add(ui);
};

template <typename T>
void Interface::html_input(const String &id, const String &type, const T &value, const String &label, bool direct){
    UIelement<TINY_JSON_SIZE> ui(ui_element_t::input, id, value, label);
    ui.obj[FPSTR(P_type)] = type;
    if (direct) ui.obj[FPSTR(P_directly)] = true;
    json_frame_add(ui);
};
