/******************************************************************************
    Minimal Signal Callback Definitions for OBS Plugin Development
    This file provides signal callback stubs for standalone compilation
******************************************************************************/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Signal types (stubs) */
typedef void* signal_handler_t;

#define signal_handler_connect(handler, signal, callback, priv)
#define signal_handler_disconnect(handler, callback, priv)

#ifdef __cplusplus
}
#endif
