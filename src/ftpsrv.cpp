//#define FTP_DEBUG
#include <ftpsrv.h>
#include <FTPServer.h>
#include <basicui.h>

FTPServer *ftpsrv = nullptr;

void ftp_start(void){
  if (!ftpsrv) ftpsrv = new FTPServer(LittleFS);
  if (ftpsrv) ftpsrv->begin(embui.getConfig()[P_ftp_usr] | P_ftp, embui.getConfig()[P_ftp_pwd] | P_ftp);
}

void ftp_stop(void){
  if (ftpsrv) {
    ftpsrv->stop();
    delete ftpsrv;
    ftpsrv = nullptr;
  }
}
void ftp_loop(void){
  if (ftpsrv) ftpsrv->handleFTP();
}

bool ftp_status(){ return ftpsrv; };

namespace basicui {

void page_settings_ftp(Interface *interf, const JsonObjectConst data, const char* action){
    interf->json_frame_interface();
        interf->json_section_uidata();
        interf->uidata_pick("sys.settings.ftp");
    interf->json_frame_flush();

    interf->json_frame_value();
        interf->value(P_ftp, ftp_status());    // enable FTP checkbox
        interf->value(P_ftp_usr, embui.getConfig()[P_ftp_usr].is<const char*>() ? embui.getConfig()[P_ftp_usr].as<const char*>() : P_ftp );
        interf->value(P_ftp_pwd, embui.getConfig()[P_ftp_pwd].is<const char*>() ? embui.getConfig()[P_ftp_pwd].as<const char*>() : P_ftp );
    interf->json_frame_flush();
}

void set_settings_ftp(Interface *interf, const JsonObjectConst data, const char* action){
    if (!data) return;

    bool newstate = data[P_ftp];

    embui.getConfig()[P_ftp] = newstate;                           // ftp on/off

    // set ftp login
    if (data[P_ftp_usr] == P_ftp)
      embui.getConfig().remove(P_ftp_usr);        // do not save default login
    else
      embui.getConfig()[P_ftp_usr] = data[P_ftp_usr];

    // set ftp passwd
    if (data[P_ftp_pwd] == P_ftp)
      embui.getConfig().remove(P_ftp_pwd);        // do not save default pwd
    else
      embui.getConfig()[P_ftp_pwd] = data[P_ftp_pwd];

    ftp_stop();
    LOGD(P_EmbUI, println, "UI: Stopping FTP Server");

    if ( newstate ){
      ftp_start();
      LOGD(P_EmbUI, println, "UI: Starting FTP Server");
    }

    if (interf) basicui::page_system_settings(interf, {}, NULL);          // go to "Options" page
}

} // namespace basicui