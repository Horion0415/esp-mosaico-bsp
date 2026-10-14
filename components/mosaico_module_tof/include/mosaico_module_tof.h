/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include "bsp/subboard.h"
#include "vl53l0x.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Reset and initialize one VL53L0X module for single measurements.
 *
 * Initialize and claim the slot through mosaico_module_mgr before calling.
 * Only one ToF board may be connected: both slots share address 0x29.
 * Serialize access to the handle; read with vl53l0x_single_measure() and
 * destroy with vl53l0x_destroy() before releasing the slot.
 * On failure, out_sensor is NULL.
 */
esp_err_t mosaico_tof_new(bsp_subboard_slot_t slot, vl53l0x_handle_t *out_sensor);

#ifdef __cplusplus
}
#endif
