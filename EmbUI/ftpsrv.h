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
void block_settings_ftp(Interface *interf, JsonObject *data);

void set_settings_ftp(Interface *interf, JsonObject *data);

} // namespace basicui