
#ifndef INCLUDE_WALLABY_TELLO_H
#define INCLUDE_WALLABY_TELLO_H

#ifdef __cplusplus
extern "C" {
#endif

char * find_tellos();

int connect_tello(char * tello);

int wpa_cmd(char const  * cmd, char * buf);

int send_to_tello(char * command);

int connect_wpa_sup();

#ifdef __cplusplus
}
#endif
#endif

