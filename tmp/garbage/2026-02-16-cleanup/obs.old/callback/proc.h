/******************************************************************************
    Minimal Procedure Callback Definitions for OBS Plugin Development
    This file provides procedure callback stubs for standalone compilation
******************************************************************************/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Procedure types (stubs) */
typedef void* proc_handler_t;

#define proc_handler_add(handler, name, callback, priv)
#define proc_handler_call(handler, name, calldata)

#ifdef __cplusplus
}
#endif
