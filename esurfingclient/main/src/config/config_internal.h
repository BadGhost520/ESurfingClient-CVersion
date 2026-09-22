#ifndef ESURFINGCLIENT_CONFIG_INTERNAL_H
#define ESURFINGCLIENT_CONFIG_INTERNAL_H

#include "states/States.h"

#include <cJSON/cJSON.h>

extern bool s_list_only;

uint8_t parse_channel_json(const cJSON* chn, const uint8_t cfg_no);

void apply_channel_ua(login_cfg_t* cfg, uint8_t cfg_no);

bool apply_time_windows(const cJSON* item, login_cfg_t* cfg);

uint8_t complete_cfg(cJSON* cfg_json);

void cfg_halt();

#endif
