// Copyright (c) 2026 odinesn1ck
// SPDX-License-Identifier: Apache-2.0
// NeuroCube portable adapter API for the Espressif ES8311 derivative.
#pragma once
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
enum { ES8311_OK = 0, ES8311_IO_ERROR = -1, ES8311_INVALID_ARGUMENT = -2 };
typedef struct {
    void *context;
    bool (*write)(void *, uint8_t reg, uint8_t value);
    bool (*read)(void *, uint8_t reg, uint8_t *value);
    void (*delay_ms)(void *, uint32_t milliseconds);
} es8311_bus_t;
typedef struct {
    es8311_bus_t bus;
} es8311_t;
int es8311_init_24k(es8311_t *dev);
int es8311_read_id(es8311_t *dev, uint16_t *id);
int es8311_voice_volume_set(es8311_t *dev, int volume, int *volume_set);
int es8311_voice_mute(es8311_t *dev, bool mute);
#ifdef __cplusplus
}
#endif
