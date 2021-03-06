#ifndef _BBTRACE_CORE_H_
#define _BBTRACE_CORE_H_

#include "bbtrace.h"

#ifdef __cplusplus
extern "C" {
#endif

void dump_symbol_data(buf_symbol_t *buf_item);
void dump_event_data(buf_event_t *buf_item);
void add_dynamic_codes(void* start, void *end);
void lib_entry(void *wrapcxt, INOUT void **user_data);
void lib_exit(void *wrapcxt, INOUT void *user_data);
void WndProc_entry(void *wrapcxt, INOUT void **user_data);

char* set_dump_path(client_id_t id, dr_time_t* start_time);

#ifdef __cplusplus
}
#endif

#endif
