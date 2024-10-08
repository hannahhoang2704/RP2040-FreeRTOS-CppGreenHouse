//
// Created by Hanh Hoang on 6.10.2024.
//

#ifndef FREERTOS_GREENHOUSE_TLS_COMMON_H
#define FREERTOS_GREENHOUSE_TLS_COMMON_H

#include "RTOS_infrastructure.h"
#include "queue.h"

typedef struct TLS_CLIENT_T_ {
    struct altcp_pcb *pcb;
    bool complete;
    int error;
    const char *http_request;
    int timeout;
} TLS_CLIENT_T;

void set_iRTOS(struct RTOS_infrastructure RTOSs);
//QueueHandle_t co2_command_queue;
//bool init_tls(TLS_CLIENT_T * tls_client, const char * request, int timeout, const char * server);
bool run_tls_client_test(const uint8_t *cert, size_t cert_len, const char *server, const char *request, int timeout);
//err_t send(TLS_CLIENT_T * tls_client, const char * server);

#endif //FREERTOS_GREENHOUSE_TLS_COMMON_H
