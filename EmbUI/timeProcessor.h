// This framework originaly based on JeeUI2 lib used under MIT License Copyright (c) 2019 Marsel Akhkamov
// then re-written and named by (c) 2020 Anton Zolotarev (obliterator) (https://github.com/anton-zolotarev)
// also many thanks to Vortigont (https://github.com/vortigont), kDn (https://github.com/DmytroKorniienko)
// and others people

#pragma once

#include "globals.h"
#include <WiFi.h>

/*
 * COUNTRY macro allows to select a specific country pool for ntp requests, like ru.pool.ntp.org, eu.pool.ntp.org, etc...
 * otherwise a general pool "pool.ntp.org" is used as a fallback and vniiftri.ru's ntp is used as a primary
 * 
 */
#if !defined NTP1ADDRESS && !defined NTP2ADDRESS
#ifdef CONTRY
    #define NTP1ADDRESS        COUNTRY ".pool.ntp.org"      // пул серверов времени для NTP
    #define NTP2ADDRESS        "ntp3.vniiftri.ru"           // https://vniiftri.ru/catalog/services/sinkhronizatsiya-vremeni-cherez-ntp-servera/
#else
    #define NTP1ADDRESS        "ntp3.vniiftri.ru"
    #define NTP2ADDRESS        ("pool.ntp.org")
#endif
#endif

#if defined ESP_ARDUINO_VERSION
    #define CUSTOM_NTP_INDEX    2
#else       // older Arduino core <2.0
    #define CUSTOM_NTP_INDEX    0
#endif

#define TM_BASE_YEAR        1900
#define DAYSECONDS          (86400U)
#define DATETIME_STRLEN     (20U)   // ISO data/time string "YYYY-MM-DDThh:mm:ss", seconds optional


// TimeProcessor class is a Singleton
class TimeProcessor
{
private:
    TimeProcessor();

    const char* ntp1 = NTP1ADDRESS;
    const char* ntp2 = NTP2ADDRESS;
    std::unique_ptr<char[]> ntpCustom;   // pointer for custom ntp hostname

    /**
     * обратный вызов при подключении к WiFi точке доступа
     * запускает синхронизацию времени
     */
    void _onWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info);

protected:
    static callback_function_t timecb;

    /**
     * Timesync callback
     */
    static void timeavailable(struct timeval *t);


public:
    // this is a singleton
    TimeProcessor(TimeProcessor const&) = delete;
    void operator=(TimeProcessor const&) = delete;


    /**
     * obtain a pointer to singleton instance
     */
    static TimeProcessor& getInstance(){
        static TimeProcessor inst;
        return inst;
    }

    /**
     * Функция установки системного времени, принимает в качестве аргумента указатель на строку в формате
     * "YYYY-MM-DDThh:mm:ss"
     */
    static void setTime(const String &timestr);
    static inline void setTime(const char *timestr){ setTime(String (timestr)); };

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
    void tzsetup(const char* tz);

    /**
     * установка пользовательского ntp-сервера
     * сервер имеет низший приоритет
     * @param ntp - сервер в виде ip или hostname
     */
    void setcustomntp(const char* ntp);

    /**
     * @brief - retreive NTP server name or IP
     */
    String getserver(uint8_t idx);

    /**
     *  установка смещения текущего системного времени от UTC в секундах
     *  можно применять если не получается выставить нормально зону
     *  и правила перехода сезонного времени каким-либо другим способом.
     *  При уставке правила перехода сезонного времени (если были) сбрасываются!
     */
    void setOffset(const int val);

    /**
     * Attach user-defined call-back function that would be called on time-set event
     * 
     */
    void attach_callback(callback_function_t callback);

    void dettach_callback(){
        timecb = nullptr;
    }

    /**
     * Возвращает текущее смещение локального системного времени от UTC в секундах
     * с учетом часовой зоны и правил смены сезонного времени (если эти параметры были
     * корректно установленны ранее каким-либо методом)
     */
    static long int getOffset();

    /**
     *  возвращает текуший unixtime
     */
    static time_t getUnixTime() {return time(nullptr); }

    /**
     * возвращает строку с временем в формате "00:00"
     */
    String getFormattedShortTime();

    int getHours()
    {
        return localtime(now())->tm_hour;
    }

    int getMinutes()
    {
        return localtime(now())->tm_min;
    }

    /**
     * функция допечатывает в переданную строку заданный таймстамп в дату/время в формате "9999-99-99T99:99"
     * @param _tstamp - преобразовать заданный таймстамп, если не задан используется текущее локальное время
     */
    static void getDateTimeString(String &buf, const time_t _tstamp = 0);

    /**
     * returns pointer to current unixtime
     * (удобно использовать для передачи в localtime())
     */
    static const time_t* now(){
        static time_t _now;
        time(&_now);
        return &_now;
    }

    /**
     * возвращает true если текущее время :00 секунд 
     */
    static bool seconds00(){
        if ((localtime(now())->tm_sec))
          return false;
        else
          return true;
    }

    /**
     * @brief enable/disable NTP over DHCP
     */
    void ntpodhcp(bool enable);

};


/*
 * obsolete methods for using http API via worldtimeapi.org
 * Using the API it is not possible to set TZ env var
 * for proper DST/date changes and calculations. So it is deprecated
 * and should NOT be used except for compatibility or some
 * special cases like networks with blocked ntp service
 */
#ifdef USE_WORLDTIMEAPI
#include "ts.h"

#define TIMEAPI_BUFSIZE     600
#define HTTPSYNC_DELAY      5
#define HTTP_REFRESH_HRS    3     // время суток для обновления часовой зоны
#define HTTP_REFRESH_MIN    3

// TaskScheduler - Let the runner object be a global, single instance shared between object files.
extern Scheduler ts;

class WorldTimeAPI
{
private:
    String tzone;            // строка зоны для http-сервиса как она задана в https://raw.githubusercontent.com/nayarsystems/posix_tz_db/master/zones.csv

    Task _wrk;              // scheduler for periodic updates

    unsigned int getHttpData(String &payload, const String &url);

    /**
     * Функция обращается к внешнему http-сервису, получает временную зону/летнее время
     * на основании либо установленной переменной tzone, либо на основе IP-адреса
     * в случае если временная зона содержит правила перехода на летнее время, функция
     * запускает планировщик для автокоррекции временной зоны ежесуточно в 3 часа ночи
     */
    void getTimeHTTP();

public:
    WorldTimeAPI(){ ts.addTask(_wrk); };

    ~WorldTimeAPI(){ ts.deleteTask(_wrk); };

    /**
     * установка строки с текущей временной зоной в текстовом виде,
     * влияет, на запрос через http-api за временем в конкретной зоне,
     * вместо автоопределения по ip
     * !ВНИМАНИЕ! Никакого отношения к текущей системной часовой зоне эта функция не имеет!!! 
     */
    void httpTimezone(const char *var);

    /**
     * функция установки планировщика обновления временной зоны
     * при вызове без параметра выставляет отложенный запуск на HTTP_REFRESH_HRS:HTTP_REFRESH_MIN
     */
    void httprefreshtimer(const uint32_t delay=0);

};
#endif // end of USE_WORLDTIMEAPI