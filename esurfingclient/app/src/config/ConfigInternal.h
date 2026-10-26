#ifndef ESURFINGCLIENT_CONFIGINTERNAL_H
#define ESURFINGCLIENT_CONFIGINTERNAL_H

#include "core/States.h"

#include <cJSON/cJSON.h>

uint8_t parse_channel_json(const cJSON* chn, const uint8_t cfg_no);

void apply_channel_ua(cfg_t* cfg, uint8_t cfg_no);

bool apply_time_windows(const cJSON* item, cfg_t* cfg);

uint8_t complete_cfg(cJSON* cfg_json);

#endif
