#pragma once

#include "tlv.h"

typedef void (*msg_handler_t)(const msg_t *msg, void *user_ctx);

typedef enum {
    DISPATCHER_OK,
    DISPATCHER_ERR_NULL,
    DISPATCHER_ERR_ALREADY_REGISTERED,
    DISPATCHER_ERR_NOT_FOUND,
} dispatcher_err_t;

void dispatcher_init(void);

dispatcher_err_t dispatcher_register_handler(uint8_t msg_type, msg_handler_t handler, void *user_ctx);
dispatcher_err_t dispatcher_unregister_handler(uint8_t msg_type);
dispatcher_err_t dispatcher_dispatch(const msg_t *msg);