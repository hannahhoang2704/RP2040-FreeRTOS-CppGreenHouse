//
// Created by Hanh Hoang on 6.10.2024.
//

#ifndef FREERTOS_GREENHOUSE_TLS_COMMON_H
#define FREERTOS_GREENHOUSE_TLS_COMMON_H

typedef struct TLS_CLIENT_T_ {
    struct altcp_pcb *pcb;
    bool complete;
    int error;
    const char *http_request;
    int timeout;
} TLS_CLIENT_T;

#endif //FREERTOS_GREENHOUSE_TLS_COMMON_H
