//#define FTP_DEBUG
#include <ftpsrv.h>
#include <FTPServer.h>
#include <basicui.h>

FTPServer *ftpsrv = nullptr;

void ftp_start(void){
  if (!ftpsrv) ftpsrv = new FTPServer(LittleFS);
  if (ftpsrv) ftpsrv->begin(embui.paramVariant(FPSTR(P_ftp_usr)) | String(FPSTR(P_ftp)), embui.paramVariant(FPSTR(P_ftp_pwd)) | String(P_ftp));
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

void block_settings_ftp(Interface *interf, JsonObject *data){
    if (!interf) return;
    interf->json_frame_interface();

    // Headline
    interf->json_section_main(FPSTR(T_SET_FTP), FPSTR(T_DICT[lang][TD::D_FTPSRV]));

    interf->checkbox(FPSTR(P_ftp), ftp_status(), F("Enable FTP Server"));     // FTP On/off

    interf->text(FPSTR(P_ftp_usr), embui.paramVariant(FPSTR(P_ftp_usr)) | String(FPSTR(P_ftp)), "FTP login", false);
    interf->password(FPSTR(P_ftp_pwd), embui.paramVariant(FPSTR(P_ftp_pwd)) | String(FPSTR(P_ftp)), FPSTR(T_DICT[lang][TD::D_Password]));
    interf->button_submit(FPSTR(T_SET_FTP), FPSTR(T_DICT[lang][TD::D_SAVE]), FPSTR(P_BLUE));

    // close and send frame
    interf->json_frame_flush(); // main
}

void set_settings_ftp(Interface *interf, JsonObject *data){
    if (!data) return;

    bool newstate = (*data)[FPSTR(P_ftp)];

    embui.var_dropnulls(FPSTR(P_ftp), newstate);                           // ftp on/off

    // set ftp login
    if ((*data)[FPSTR(P_ftp_usr)] == FPSTR(P_ftp))
      embui.var_remove(FPSTR(P_ftp_usr));   // do not save default login
    else
      embui.var_dropnulls(FPSTR(P_ftp_usr), (*data)[FPSTR(P_ftp_usr)]);

    // set ftp passwd
    if ((*data)[FPSTR(P_ftp_pwd)] == FPSTR(P_ftp))
      embui.var_remove(FPSTR(P_ftp_pwd));
    else
      embui.var_dropnulls(FPSTR(P_ftp_pwd), (*data)[FPSTR(P_ftp_pwd)]);    // do not save default pwd

    ftp_stop();
    LOG(println, F("UI: Stopping FTP Server"));

    if ( newstate ){
      ftp_start();
      LOG(println, F("UI: Starting FTP Server"));
    }

    if (interf) basicui::section_settings_frame(interf, nullptr);          // go to "Options" page
}

} // namespace basicui