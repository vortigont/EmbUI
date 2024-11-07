// LOG macro's
#ifndef EMBUI_DEBUG_PORT
  #define EMBUI_DEBUG_PORT Serial
#endif

// undef possible LOG macros
#ifdef LOG
  #undef LOG
#endif
#ifdef LOGV
  #undef LOGV
#endif
#ifdef LOGD
  #undef LOGD
#endif
#ifdef LOGI
  #undef LOGI
#endif
#ifdef LOGW
  #undef LOGW
#endif
#ifdef LOGE
  #undef LOGE
#endif



#if defined(EMBUI_DEBUG_LEVEL) && EMBUI_DEBUG_LEVEL == 5
	#define LOGV(tag, func, ...) EMBUI_DEBUG_PORT.print(tag); EMBUI_DEBUG_PORT.print(" V: "); EMBUI_DEBUG_PORT.func(__VA_ARGS__)
#else
	#define LOGV(...)
#endif

#if defined(EMBUI_DEBUG_LEVEL) && EMBUI_DEBUG_LEVEL > 3
	#define LOGD(tag, func, ...) EMBUI_DEBUG_PORT.print(tag); EMBUI_DEBUG_PORT.print(" D: "); EMBUI_DEBUG_PORT.func(__VA_ARGS__)
#else
	#define LOGD(...)
#endif

#if defined(EMBUI_DEBUG_LEVEL) && EMBUI_DEBUG_LEVEL > 2
	#define LOGI(tag, func, ...) EMBUI_DEBUG_PORT.print(tag); EMBUI_DEBUG_PORT.print(" I: "); EMBUI_DEBUG_PORT.func(__VA_ARGS__)
	// compat macro
	#define LOG(func, ...) EMBUI_DEBUG_PORT.func(__VA_ARGS__)
	#define LOG_CALL(call...) { call; }
#else
	#define LOGI(...)
	// compat macro
	#define LOG(...)
	#define LOG_CALL(call...) ;
#endif

#if defined(EMBUI_DEBUG_LEVEL) && EMBUI_DEBUG_LEVEL > 1
	#define LOGW(tag, func, ...) EMBUI_DEBUG_PORT.print(tag); EMBUI_DEBUG_PORT.print(" W: "); EMBUI_DEBUG_PORT.func(__VA_ARGS__)
#else
	#define LOGW(...)
#endif

#if defined(EMBUI_DEBUG_LEVEL) && EMBUI_DEBUG_LEVEL > 0
	#define LOGE(tag, func, ...) EMBUI_DEBUG_PORT.print(tag); EMBUI_DEBUG_PORT.print(" E: "); EMBUI_DEBUG_PORT.func(__VA_ARGS__)
#else
	#define LOGE(...)
#endif
