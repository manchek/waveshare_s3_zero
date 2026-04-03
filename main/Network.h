#ifndef NETWORK_H
#define NETWORK_H

#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t networkInit();

esp_err_t networkStartHttpd();

#ifdef __cplusplus
}
#endif

#endif /*NETWORK_H*/

