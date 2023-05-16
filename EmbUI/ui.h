// This framework originaly based on JeeUI2 lib used under MIT License Copyright (c) 2019 Marsel Akhkamov
// then re-written and named by (c) 2020 Anton Zolotarev (obliterator) (https://github.com/anton-zolotarev)
// also many thanks to Vortigont (https://github.com/vortigont), kDn (https://github.com/DmytroKorniienko)
// and others people

#ifndef ui_h
#define ui_h

#include "EmbUI.h"

#ifdef ESP8266
#ifndef IFACE_DYN_JSON_SIZE
  #define IFACE_DYN_JSON_SIZE 2048
#endif
#ifndef SMALL_JSON_SIZE
  #define SMALL_JSON_SIZE  768
#endif
#elif defined ESP32
#ifndef IFACE_DYN_JSON_SIZE
  #define IFACE_DYN_JSON_SIZE 8192
#endif
#ifndef SMALL_JSON_SIZE
  #define SMALL_JSON_SIZE  1024
#endif
#endif

// static json doc size
#define IFACE_STA_JSON_SIZE SMALL_JSON_SIZE
#define FRAME_ADD_RETRY 5

// UI Button type enums
enum BType : uint8_t {
    generic = (0U),
    submit,
    js
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
        frameSendAll(AsyncWebSocket *server){ ws = server; }
        ~frameSendAll() { ws = nullptr; }
        void send(const String &data){ if (data.length()) ws->textAll(data); };
        void send(const JsonObject& data);
};

class frameSendClient: public frameSend {
    private:
        AsyncWebSocketClient *cl;
    public:
        frameSendClient(AsyncWebSocketClient *client){ cl = client; }
        ~frameSendClient() { cl = nullptr; }
        void send(const String &data){ if (data.length()) cl->text(data); };
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
        frameSendHttp(AsyncWebServerRequest *request){
            req = request;
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

    EmbUI *embui;
    DynamicJsonDocument json;
    LList<section_stack_t*> section_stack;
    frameSend *send_hndl;

    /**
     * @brief - add object to frame with mem overflow protection 
     */
    void frame_add_safe(const JsonObject &jobj);

    /**
     * @brief - create html button to submit itself, value or section form
     * on press button could send:
     *  - it's id with null value
     *  - id + value
     *  - submit a form with section + it's own id
     *  - submit a form with section + it's own id + value
     */
    template <typename T>
    void button_generic(const String &id, const T &value, const String &label, const String &color, BType type = generic){
        StaticJsonDocument<IFACE_STA_JSON_SIZE> obj;
        obj[FPSTR(P_html)] = FPSTR(P_button);
        obj[FPSTR(P_id)] = id;
        obj[FPSTR(P_type)] = (uint8_t)type;
        obj[FPSTR(P_label)] = label;
        obj[FPSTR(P_value)] = value;
        if (!color.isEmpty())
            obj[FPSTR(P_color)] = color;

        frame_add_safe(obj.as<JsonObject>());
    };


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
        inline void json_frame_interface(){ json_frame(F("interface")); };
        void json_frame_interface(const String &name);

        /**
         * @brief - begin Value UI secton
         * used to supply WebUI with data (key:value pairs)
         */
        inline void json_frame_value(){ json_frame(FPSTR(P_value)); }


        bool json_frame_add(const JsonObject &obj);
        void json_frame_next();
        void json_frame_clear();
        void json_frame_flush();

        /**
         * @brief - serialize and send Interface object to the WebSocket 
         */
        void json_frame_send(){ if (send_hndl) send_hndl->send(json.as<JsonObject>()); };

        /**
         * @brief - begin custom UI secton
         * for backward-compatibility only
         */
        inline void json_frame_custom(const String &type){ json_frame(type); };

        /**
         * @brief - start UI section
         * A section contains html UI elements, this is generic one
         */
        void json_section_begin(const String &name, const String &label = "", bool main = false, bool hidden = false, bool line = false);
        void json_section_begin(const String &name, const String &label, bool main, bool hidden, bool line, JsonObject obj);

        /**
         * @brief - content section is meant to replace existing data on the page
         */
        inline void json_section_content(){ json_section_begin(F("content")); };

        /**
         * @brief - opens section for UI elements that are aligned in one line on a page
         * each json_section_line() must be closed by a json_section_end() call
         */
        void json_section_line(const String &name = "");

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
         * @brief - close current UI section
         */
        void json_section_end();

        /**
         * @brief insert iFrame
         * 
         * @param id 
         * @param value 
         */
        void iframe(const String &id, const String &value);

        /**
         * @brief - Add 'value' object to the Interface frame
         * Template accepts types suitable to be added to the ArduinoJson document used as a dictionary
         */
        template <typename T> void value(const String &id, const T& val, bool html = false){
            StaticJsonDocument<IFACE_STA_JSON_SIZE> obj;
            obj[FPSTR(P_id)] = id;
            obj[FPSTR(P_value)] = val;
            if (html) obj[FPSTR(P_html)] = true;

//            frame_add_safe(obj.as<JsonObject>()); // crashes in sys ctx
            if (!json_frame_add(obj.as<JsonObject>())) {
                value(id, val, html);
            }
        };

        /**
         * @brief - Add embui's config param id as a 'value' to the Interface frame
         */
        inline void value(const String &id, bool html = false){ value(id, embui->paramVariant(id), html); };

        /**
         * @brief - Add the whole JsonObject to the Interface frame
         * actualy it is a copy-object method used to echo back the data to the WebSocket in one-to-many scenarios
         */
        inline void value(const JsonObject &data){
            frame_add_safe(data);
        }

        /**
         * @brief Create hidden html field
         *  could be used to pass some data between different sections
         */
        inline void hidden(const String &id){ hidden(id, embui->paramVariant(id)); };
        template <typename T>
        void hidden(const String &id, const T &value){
            StaticJsonDocument<IFACE_STA_JSON_SIZE> obj;
            obj[FPSTR(P_html)] = FPSTR(P_hidden);
            obj[FPSTR(P_id)] = id;
            obj[FPSTR(P_value)] = value;

            frame_add_safe(obj.as<JsonObject>());
        }

        /**
         * @brief Create inactive button with a visible label that does nothing but (possibly) carries value
         *  could be used the same way as 'hidden' element
         */
        template <typename T>
        void constant(const String &id, const T &value, const String &label){
            StaticJsonDocument<IFACE_STA_JSON_SIZE> obj;
            obj[FPSTR(P_html)] = F("const");
            obj[FPSTR(P_id)] = id;
            obj[FPSTR(P_value)] = value;
            obj[FPSTR(P_label)] = label;

            frame_add_safe(obj.as<JsonObject>());
        };
        inline void constant(const String &id, const String &label){ constant(id, embui->paramVariant(id), label); };

        // 4-й параметр обязателен, т.к. компилятор умудряется привести F() к булевому виду и использует неверный оверлоад (под esp32)
        void text(const String &id, const String &value, const String &label, bool directly){ html_input(id, F("text"), value, label, directly); };
        void text(const String &id, const String &label, bool directly = false){ text(id, embui->param(id), label, directly); };

        void password(const String &id, const String &value, const String &label){ html_input(id, FPSTR(P_password), value, label); };
        void password(const String &id, const String &label){ password(id, embui->param(id), label); };

        /**
         * @brief - create "number" html field with optional step, min, max constraints
         * Template accepts types suitable to be added to the ArduinoJson document used as a dictionary
         */
        template <typename V, typename T>
        void number(const String &id, V value, const String &label, T step, T min = 0, T max = 0){
            StaticJsonDocument<IFACE_STA_JSON_SIZE> obj;
            obj[FPSTR(P_html)] = FPSTR(P_input);
            obj[FPSTR(P_type)] = FPSTR(P_number);
            obj[FPSTR(P_id)] = id;
            obj[FPSTR(P_value)] = value;
            obj[FPSTR(P_label)] = label;
            if (min) obj[FPSTR(P_min)] = min;
            if (max) obj[FPSTR(P_max)] = max;
            if (step) obj[FPSTR(P_step)] = step;

            frame_add_safe(obj.as<JsonObject>());
        };

        inline void number(const String &id, const String &label){ number(id, embui->paramVariant(id), label, 0); };

        template <typename T>
        inline void number(const String &id, const String &label, T step = 0, T min = 0, T max = 0){
            number(id, embui->paramVariant(id), label, step, min, max);
        };

        template <typename V>
        inline void number(const String &id, V value, const String &label){
            number(id, value, label, 0);
        };

        template <typename T>
        void time(const String &id, const T &value, const String &label){ html_input(id, FPSTR(P_time), value, label); };
        inline void time(const String &id, const String &label){ time(id, embui->paramVariant(id), label); };

        template <typename T>
        void date(const String &id, const T &value, const String &label){ html_input(id, FPSTR(P_date), value, label); };
        inline void date(const String &id, const String &label){ date(id, embui->paramVariant(id), label); };

        template <typename T>
        void datetime(const String &id, const T &value, const String &label){ html_input(id, F("datetime-local"), value, label); };
        inline void datetime(const String &id, const String &label){ datetime(id, embui->paramVariant(id), label); };

        void email(const String &id, const String &value, const String &label){ html_input(id, F("email"), value, label); };
        void email(const String &id, const String &label){ email(id, embui->paramVariant(id), label); };

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
        void range(const String &id, V value, T min, T max, T step, const String &label, bool directly){
            StaticJsonDocument<IFACE_STA_JSON_SIZE> obj;
            obj[FPSTR(P_html)] = FPSTR(P_input);
            obj[FPSTR(P_type)] = F("range");
            obj[FPSTR(P_id)] = id;
            obj[FPSTR(P_value)] = value;
            obj[FPSTR(P_label)] = label;
            if (directly) obj[FPSTR(P_directly)] = true;

            obj[FPSTR(P_min)] = min;
            obj[FPSTR(P_max)] = max;
            obj[FPSTR(P_step)] = step;

            frame_add_safe(obj.as<JsonObject>());
        };

        /**
         * @brief - create drop-down selection list
         * content of a large lists could be loaded with ajax from the client side
         * @param exturl - an url for xhr request to fetch list content, it must be a valid json object
         * with label/value pairs arranged in assoc array
         */
        template <typename T>
        void select(const String &id, const T &value, const String &label, bool directly = false, bool skiplabel = false, const String &exturl = (char*)0){
            StaticJsonDocument<IFACE_STA_JSON_SIZE> obj;
            obj[FPSTR(P_html)] = F("select");
            obj[FPSTR(P_id)] = id;
            obj[FPSTR(P_value)] = value;
            obj[FPSTR(P_label)] = skiplabel ? "" : label;
            if (directly) obj[FPSTR(P_directly)] = true;

            if (!exturl.isEmpty())
                obj[F("url")] = exturl;

            frame_add_safe(obj.as<JsonObject>());
            section_stack.tail()->idx--;
            // open new section for 'option' elements
            json_section_begin(FPSTR(P_options), "", false, false, false, section_stack.tail()->block[section_stack.tail()->block.size()-1]);    // find last array element
        };
        inline void select(const String &id, const String &label, bool directly = false, bool skiplabel = false){ select(id, embui->paramVariant(id), label, directly, skiplabel); };

        /**
         * @brief - create an option element for select drop-down list
         */
        template <typename T>
        void option(const T &value, const String &label){
            StaticJsonDocument<IFACE_STA_JSON_SIZE> obj;
            obj[FPSTR(P_label)] = label;
            obj[FPSTR(P_value)] = value;

            frame_add_safe(obj.as<JsonObject>());
        }

        /**
         * @brief - элемент интерфейса checkbox
         * @param directly - значение чекбокса при изменении сразу передается на сервер без отправки формы
         */
        void checkbox(const String &id, const bool value, const String &label, const bool directly = false){ html_input(id, F("checkbox"), value, label, directly); };
        inline void checkbox(const String &id, const String &label, const bool directly = false){ checkbox(id, embui->paramVariant(id).as<bool>(), label, directly); };

        void color(const String &id, const String &value, const String &label){ html_input(id, FPSTR(P_color), value, label); };
        inline void color(const String &id, const String &label){ color(id, embui->paramVariant(id), label); };

        /**
         * элемент интерфейса textarea
         * Template accepts types suitable to be added to the ArduinoJson document used as a dictionary
         */
        template <typename T>
        void textarea(const String &id, const T &value, const String &label){ html_input(id, F("textarea"), value, label); };
        inline void textarea(const String &id, const String &label){ textarea(id, embui->paramVariant(id), label); };

        void file(const String &name, const String &action, const String &label, const String &opt = "");

        /**
         * @brief - create html button
         * A button can send it's id/value or submit a form with section data
         */
        inline void button(const String &id, const String &label, const String &color = ""){ button_generic(id, nullptr, label, color); };
        template <typename T>
        inline void button_value(const String &id, const T &value, const String &label, const String &color = ""){ button_generic(id, value, label, color); };
        inline void button_submit(const String &section, const String &label, const String &color = ""){ button_generic(section, nullptr, label, color, BType::submit); };
        template <typename T>
        inline void button_submit_value(const String &section, const T &value, const String &label, const String &color = ""){ button_generic(section, value, label, color, BType::submit); };
        // Run user-js function on click, id - acts as a js function selector
        inline void button_js(const String &id, const String &label, const String &color = ""){ button_generic(id, nullptr, label, color, BType::js); };
        // Run user-js function on click, id - acts as a js function selector, value - any additional value or object passed to js function
        template <typename T>
        inline void button_js_value(const String &id, const T &value, const String &label, const String &color = ""){ button_generic(id, value, label, color, BType::js); };


        void spacer(const String &label = "");

        /**
         * @brief insert text comment (a simple <p>text</p> block)
         * 
         * @param id comment if (might be usefull when replaced with content() method)
         * @param label - text label
         */
        void comment(const String &id, const String &label);
        void comment(const String &label){ comment("", label); };

        /**
         * @brief - create generic html input element
         * @param id - element id
         * @param type - html element type, ex. 'text'
         * @param value - element value
         * @param label - element label
         * @param direct - if true, element value in send via ws on-change 
         */
        template <typename T>
        void html_input(const String &id, const String &type, const T &value, const String &label, bool direct = false){
            StaticJsonDocument<IFACE_STA_JSON_SIZE> obj;
            obj[FPSTR(P_html)] = FPSTR(P_input);
            obj[FPSTR(P_type)] = type;
            obj[FPSTR(P_id)] = id;
            obj[FPSTR(P_value)] = value;
            obj[FPSTR(P_label)] = label;
            if (direct) obj[FPSTR(P_directly)] = true;

            frame_add_safe(obj.as<JsonObject>());
        };

        /**
         * @brief - create "display" div with custom css selector
         * could be used for making all kinds of "sensor" outputs on the page with live-updated values without the need to redraw interface element
         * @param id - element/div DOM id
         * @param value  - element value (treated as text)
         * @param class - base css class for Display, css selector value created as "class id" to allow many sensors inherit from the base class
         * @param params - additional parameters (reserved for future use)
         */
        template <typename T>
        void display(const String &id, const T &value, const String &css = "", const JsonObject &params = JsonObject() ){
            String cssclass(css);   // make css selector like "class id", id used as a secondary distinguisher 
            if (!css.length())
                cssclass += F("display");   // "display is the default css selector"
            cssclass += F(" ");
            cssclass += id;
            custom(id, F("txt"), value, cssclass, params);
        };

        /**
         * @brief - Creates html element with cutomized type and arbitrary parameters
         * used to create user-defined interface elements with custom css/js handlers
         */
        template <typename T>
        void custom(const String &id, const String &type, const T &value, const String &label, const JsonObject &param = JsonObject()){
            StaticJsonDocument<IFACE_STA_JSON_SIZE*2> obj; // по этот контрол выделяем IFACE_STA_JSON_SIZE*2 т.к. он может быть большой...
            obj[FPSTR(P_html)] = F("custom");;
            obj[FPSTR(P_type)] = type;
            obj[FPSTR(P_id)] = id;
            obj[FPSTR(P_value)] = value;
            obj[FPSTR(P_label)] = label;
            JsonObject nobj = obj.createNestedObject(String(F("param")));
            nobj.set(param);
            frame_add_safe(obj.as<JsonObject>());
        }

};

#endif