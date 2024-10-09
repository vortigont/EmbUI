#pragma once
#include <EmbUI.h>

void ftp_start(void);
void ftp_stop(void);
void ftp_loop(void);
inline bool ftp_status();

namespace basicui {

/**
 *  BasicUI ftp server setup UI block
 */
void page_settings_ftp(Interface *interf, const JsonObjectConst data, const char* action = NULL);

void set_settings_ftp(Interface *interf, const JsonObjectConst data, const char* action = NULL);

} // namespace basicui