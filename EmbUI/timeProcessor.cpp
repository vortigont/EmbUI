// This framework originaly based on JeeUI2 lib used under MIT License Copyright (c) 2019 Marsel Akhkamov
// then re-written and named by (c) 2020 Anton Zolotarev (obliterator) (https://github.com/anton-zolotarev)
// also many thanks to Vortigont (https://github.com/vortigont), kDn (https://github.com/DmytroKorniienko)
// and others people

#include "timeProcessor.h"

#include <ctime>
#include <esp_sntp.h>

#ifndef TZONE
#define TZONE "GMT0"         // default Time-Zone
#endif

// stub zone for a  <+-nn> names
static const char P_LOC[] = "LOC";

// static member must be defined outside the class
callback_function_t TimeProcessor::timecb = nullptr;

TimeProcessor::TimeProcessor()
{
    sntp_set_time_sync_notification_cb( [](struct timeval *tv){ timeavailable(tv);} );
#ifdef ESP_ARDUINO_VERSION
    sntp_servermode_dhcp(1);    // enable NTPoDHCP
#endif

    configTzTime(TZONE, ntp1, ntp2, userntp ? userntp->data() : NULL);
    sntp_stop();    // отключаем ntp пока нет подключения к AP

    // hook up WiFi events handler
    WiFi.onEvent(std::bind(&TimeProcessor::_onWiFiEvent, this, std::placeholders::_1, std::placeholders::_2));
}

String TimeProcessor::getFormattedShortTime()
{
    char buffer[6];
    sprintf(buffer, "%02u:%02u", localtime(now())->tm_hour, localtime(now())->tm_min);
    return String(buffer);
}

/**
 * Set current system time from a string "YYYY-MM-DDThh:mm:ss"    [19]
 */
time_t TimeProcessor::setTime(const char *datetimestr){
    if (!datetimestr) return 0;
    //"YYYY-MM-DDThh:mm:ss"    [19]
    LOG(print, "Set datetime to: "); LOG(println, datetimestr);

    struct tm tmStruct;
    memset(&tmStruct, 0, sizeof(tmStruct));
    strptime(datetimestr, strlen(datetimestr) < 19 ? "%Y-%m-%dT%H:%M" : "%Y-%m-%dT%H:%M:%S", &tmStruct);

    time_t time = mktime(&tmStruct);
    timeval tv = { time, 0 };
    settimeofday(&tv, NULL);
    return time;
}


/**
 * установки системной временной зоны/правил сезонного времени.
 * по сути дублирует системную функцию setTZ, но работает сразу
 * со строкой из памяти, а не из PROGMEM
 * Может использоваться для задания настроек зоны/правил налету из
 * браузера/апи вместо статического задания Зоны на этапе компиляции
 * @param tz - указатель на строку в формате TZSET(3)
 * набор отформатированных строк зон под прогмем лежит тут
 * https://github.com/esp8266/Arduino/blob/master/cores/esp8266/TZ.h
 */
void TimeProcessor::tzsetup(const char* tz){
    if (!tz || !*tz)
        return;

    /*
     * newlib has issues with TZ strings with quoted <+-nn> names 
     * this has been fixed in https://github.com/esp8266/Arduino/pull/7702 for esp8266 (still not in stable as of Nov 16 2020)
     * but it also affects ESP32 and who knows when to expect a fix there
     * So let's fix such zones in-place untill core support for both platforms available
     */
    if (tz[0] == 0x3C){     // check if first char is '<'
      String _tz(tz);
      String _tzfix(P_LOC);
      if (_tz.indexOf('<',1) > 0){  // there might be two <> quotes
    	//LOG(print, "2nd pos: "); LOG(println, _tz.indexOf('<',1)); 
        _tzfix += _tz.substring(_tz.indexOf('>')+1, _tz.indexOf('<',1));
        _tzfix += P_LOC;
      }
      _tzfix += _tz.substring(_tz.lastIndexOf('>')+1, _tz.length());
      setenv("TZ", _tzfix.c_str(), 1/*overwrite*/);
      LOG(printf_P, PSTR("TIME: TZ fix applied: %s\n"), _tzfix.c_str());
    } else {
      setenv("TZ", tz, 1/*overwrite*/);
    }

    tzset();
    LOG(printf_P, PSTR("TIME: TZSET rules changed to: %s\n"), tz);
}


/**
 *  WiFi events callback to start/stop ntp sync
 */
void TimeProcessor::_onWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info){
    switch (event){
    case SYSTEM_EVENT_STA_GOT_IP:
        sntp_setservername(1, (char*)ntp2);
        if (userntp)
            sntp_setservername(CUSTOM_NTP_INDEX, userntp->data());
        sntp_init();
        LOGI(P_EmbUI_time, println, "Starting sntp sync");
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        sntp_stop();
        break;
    default:
        break;
    }
}

void TimeProcessor::timeavailable(struct timeval *t){
    LOG(println, F("UI TIME: Event - Time adjusted"));
    if(timecb)
        timecb();
}

/**
 * функция допечатывает в переданную строку передайнный таймстамп даты/времени в формате "9999-99-99T99:99"
 * @param _tstamp - преобразовать заданный таймстамп, если не задан используется текущее локальное время
 */
void TimeProcessor::getDateTimeString(String &buf, const time_t tstamp){
  char buff[std::size("yyyy-mm-ddThh:mm:ss")];
  std::time_t time = std::time({});
  std::strftime(std::data(buff), std::size(buff), "%FT%T", std::localtime(tstamp ? &tstamp : &time));
  buf.concat(buff);
}

/**
 * установка текущего смещения от UTC в секундах
 */
void TimeProcessor::setOffset(const int val){
    LOGI(P_EmbUI_time, printf, "TZ offset: %d\n", val);

    //setTimeZone((long)val, 0);    // this breaks linker in some weird way
    configTime((long)val, 0, ntp1, ntp2);
}

/**
 * Возвращает текущее смещение локального системного времени от UTC в секундах
 * с учетом часовой зоны и правил смены сезонного времени (если эти параметры были
 * корректно установленно ранее каким-либо методом)
 */
long int TimeProcessor::getOffset(){
    const tm* tm = localtime(now());
    auto tz = __gettzinfo();
    return *(tm->tm_isdst == 1 ? &tz->__tzrule[1].offset : &tz->__tzrule[0].offset);
}

void TimeProcessor::setcustomntp(const char* ntp){
    if (!ntp) return;
    if (!userntp)
        userntp = new std::string(ntp);
    else
        *userntp = ntp;
    sntp_setservername(CUSTOM_NTP_INDEX, userntp->data());
    LOGI(P_EmbUI_time, printf, "Set custom NTP to: %s\n", userntp->data());
}

/**
 * @brief - retreive NTP server name or IP
 */
String TimeProcessor::getserver(uint8_t idx){
    if (sntp_getservername(idx)){
        return String(sntp_getservername(idx));
    } else {
        const ip_addr_t * _ip = sntp_getserver(idx);
        IPAddress addr(_ip->u_addr.ip4.addr);
        return addr.toString();
    }
};

/**
 * Attach user-defined call-back function that would be called on time-set event
 * 
 */
void TimeProcessor::attach_callback(callback_function_t callback){
    timecb = std::move(callback);
}


void TimeProcessor::ntpodhcp(bool enable){
    sntp_servermode_dhcp(enable);

    if (!enable){
        LOGI(P_EmbUI_time, println, "TIME: Disabling NTP over DHCP");
        sntp_setservername(0, (char*)ntp1);
        sntp_setservername(1, (char*)ntp2);
        if (userntp)
            sntp_setservername(CUSTOM_NTP_INDEX, userntp->data());
    }
};


#ifdef EMBUI_WORLDTIMEAPI

#include <ArduinoJson.h>
#ifdef ESP32
#include <HTTPClient.h>
#endif

// worldtimeapi.org service URLs
static const char PG_timeapi_tz_url[] PROGMEM  = "http://worldtimeapi.org/api/timezone/";
static const char PG_timeapi_ip_url[] PROGMEM  = "http://worldtimeapi.org/api/ip";

/**
 * берем урл и записываем ответ в переданную строку
 * в случае если в коде ответа ошибка, обнуляем строку
 */ 
unsigned int WorldTimeAPI::getHttpData(String &payload, const String &url)
{
  WiFiClient client;
  HTTPClient http;
  LOGI(P_EmbUI_time, println, F("TimeZone updating via HTTP..."));
  http.begin(client, url);

  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK){
    payload = http.getString(); 
  } else {
    LOGD(P_EmbUI_time, printf, "Time HTTPCode=%d\n", httpCode);
  }
  http.end();
  return payload.length();
}

void WorldTimeAPI::getTimeHTTP()
{
    String result((char *)0);
    result.reserve(TIMEAPI_BUFSIZE);
    if(tzone.length()){
        String url(PG_timeapi_tz_url);
        url+=tzone;
        getHttpData(result, url);
    }

    if(!result.length()){
        String url(PG_timeapi_ip_url);
        if(!getHttpData(result, url))
            return;
    }

    LOGV(P_EmbUI_time, println, result);
    DynamicJsonDocument doc(TIMEAPI_BUFSIZE);
    DeserializationError error = deserializeJson(doc, result);
    result="";

    if (error) {
        LOGE(P_EmbUI_time, print, F("Time deserializeJson error: "));
        LOGE(P_EmbUI_time, println, error.code());
        return;
    }

    int raw_offset, dst_offset = 0;

    raw_offset=doc[F("raw_offset")];    // по сути ничего кроме текущего смещения от UTC от сервиса не нужно
                                        // правила перехода сезонного времени в формате, воспринимаемом системной
                                        // либой он не выдает, нужно писать внешний парсер. Мнемонические определения
                                        // слишком объемные для контроллера чтобы держать и обрабатывать их на лету.
                                        // Вероятно проще будет их запихать в js веб-интерфейса
    dst_offset=doc[F("dst_offset")];

    // Save mnemonic time-zone (do not know why :) )
    if (!tzone.length()) {
        const char *tz = doc[F("timezone")];
        tzone+=tz;
    }
    LOGI(P_EmbUI_time, printf, "HTTP TimeZone: %s, offset: %d, dst offset: %d\n", tzone.c_str(), raw_offset, dst_offset);

    TimeProcessor::getInstance().setOffset(raw_offset+dst_offset);

    TimeProcessor::getInstance().setTime(doc[F("datetime")].as<String>());

    if (doc[F("dst_from")]!=nullptr){
        LOGD(P_EmbUI_time, println, F("Zone has DST, rescheduling refresh"));
        httprefreshtimer();
    }
}

void WorldTimeAPI::httprefreshtimer(const uint32_t delay){
    time_t timer;

    if (delay){
        timer = delay;
    } else {
        struct tm t;
        tm *tm=&t;
        localtime_r(TimeProcessor::now(), tm);

        tm->tm_mday++;                  // выставляем "завтра"
        tm->tm_hour= HTTP_REFRESH_HRS;
        tm->tm_min = HTTP_REFRESH_MIN;

        timer = (mktime(tm) - TimeProcessor::getUnixTime())% DAYSECONDS;

        LOGD(P_EmbUI_time, printf_P, PSTR("Schedule TZ refresh in %ld\n"), timer);
    }

    _wrk.set(timer * TASK_SECOND, TASK_ONCE, [this](){getTimeHTTP();});
    _wrk.restartDelayed();
}

/**
 * установка строки с текущей временной зоной в текстовом виде
 * влияет, на запрос через http-api за временем в конкретной зоне,
 * вместо автоопределения по ip
 */
void WorldTimeAPI::httpTimezone(const char *var){
  if (!var)
    return;
  tzone = var;
}
#endif // end of EMBUI_WORLDTIMEAPI