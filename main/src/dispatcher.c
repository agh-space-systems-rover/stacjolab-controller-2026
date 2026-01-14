#include "dispatcher.h"

#include "freertos/FreeRTOS.h" // IWYU pragma: keep
#include "freertos/portmacro.h"
#include <string.h>



typedef struct {
    msg_handler_t handler;
    void *user_ctx;
} handler_entry_t;

static handler_entry_t s_handlers[256];


static portMUX_TYPE s_dispatcher_mux = portMUX_INITIALIZER_UNLOCKED;
#define DISP_CRIT_ENTER() vPortEnterCritical(&s_dispatcher_mux)
#define DISP_CRIT_EXIT()  vPortExitCritical(&s_dispatcher_mux)


void dispatcher_init() {
    DISP_CRIT_ENTER();
    memset(s_handlers, 0, sizeof(s_handlers));
    DISP_CRIT_EXIT();
}

dispatcher_err_t dispatcher_register_handler(uint8_t msg_type, msg_handler_t handler, void *user_ctx) {
    if (!handler) return DISPATCHER_ERR_NULL;

    DISP_CRIT_ENTER();
    if(s_handlers[msg_type].handler != NULL) {
        DISP_CRIT_EXIT();
        return DISPATCHER_ERR_ALREADY_REGISTERED;
    }

    s_handlers[msg_type].handler = handler;
    s_handlers[msg_type].user_ctx = user_ctx;
    DISP_CRIT_EXIT();
    return DISPATCHER_OK;
}

dispatcher_err_t dispatcher_unregister_handler(uint8_t msg_type) {
    DISP_CRIT_ENTER();
    if(s_handlers[msg_type].handler == NULL) {
        DISP_CRIT_EXIT();
        return DISPATCHER_ERR_NOT_FOUND;
    }

    s_handlers[msg_type].handler = NULL;
    s_handlers[msg_type].user_ctx = NULL;
    DISP_CRIT_EXIT();
    return DISPATCHER_OK;
}

dispatcher_err_t dispatcher_dispatch(const msg_t *msg) {
    if (!msg) return DISPATCHER_ERR_NULL;

    msg_handler_t handler = NULL;
    void *user_ctx = NULL;

    DISP_CRIT_ENTER();
    handler = s_handlers[msg->type].handler;
    user_ctx = s_handlers[msg->type].user_ctx;
    DISP_CRIT_EXIT();

    if (handler) {
        handler(msg, user_ctx);
        return DISPATCHER_OK;
    } else {
        return DISPATCHER_ERR_NOT_FOUND;
    }
}