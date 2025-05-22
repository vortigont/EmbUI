// This framework originaly based on JeeUI2 lib used under MIT License Copyright (c) 2019 Marsel Akhkamov
// then re-written and named by (c) 2020 Anton Zolotarev (obliterator) (https://github.com/anton-zolotarev)
// also many thanks to Vortigont (https://github.com/vortigont), kDn (https://github.com/DmytroKorniienko)
// and others people

#pragma once

#include "WiFi.h"
#include <string>
#include "embui_log.h"

/*
 * COUNTRY macro allows to select a specific country pool for ntp requests, like ru.pool.ntp.org, eu.pool.ntp.org, etc...
 * otherwise a general pool "pool.ntp.org" is used as a fallback and vniiftri.ru's ntp is used as a primary
 * 
 */
#if !defined EMBUI_NTP_SERVER
    #define EMBUI_NTP_SERVER        "pool.ntp.org"
#endif

using callback_function_t = std::function<void(void)>;

// TimeProcessor class is a Singleton
class TimeProcessor
{
private:
    TimeProcessor();

    std::array<std::string, SNTP_MAX_SERVERS> _ntp_servers;

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
     * @brief apply NTP servers configuration from NVS
     * should be called when IP/DNS has been set already
     */
    void setNTPservers();


    /**
     * Функция установки системного времени, принимает в качестве аргумента указатель на строку в формате
     * "YYYY-MM-DDThh:mm:ss"
     */
    static time_t setTime(const char* datetimestr);

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
     * @brief - retreive NTP server name or IP
     */
    String getserver(uint8_t idx);

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
    static void getDateTimeString(String &buf, const time_t tstamp = 0);

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
};


/*
 * obsolete methods for using http API via worldtimeapi.org
 * Using the API it is not possible to set TZ env var
 * for proper DST/date changes and calculations. So it is deprecated
 * and should NOT be used except for compatibility or some
 * special cases like networks with blocked ntp service
 */
#ifdef EMBUI_WORLDTIMEAPI
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
#endif // end of EMBUI_WORLDTIMEAPI