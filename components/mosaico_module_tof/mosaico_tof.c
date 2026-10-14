/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: Apache-2.0
 */
#include "mosaico_module_tof.h"
#include "esp_check.h"
#include "bsp/power.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "vl53l0x_def.h"

static const char *TAG = "mosaico_tof";

esp_err_t mosaico_tof_new(bsp_subboard_slot_t slot, vl53l0x_handle_t *out_sensor)
{
    ESP_RETURN_ON_FALSE(out_sensor, ESP_ERR_INVALID_ARG, TAG, "null output handle");
    *out_sensor = NULL;
    ESP_RETURN_ON_FALSE(slot >= BSP_SUBBOARD_SLOT_LEFT && slot < BSP_SUBBOARD_SLOT_COUNT,
                        ESP_ERR_INVALID_ARG, TAG, "invalid slot");
    i2c_master_bus_handle_t bus = bsp_subboard_get_i2c_bus();
    ESP_RETURN_ON_FALSE(bus, ESP_ERR_INVALID_STATE, TAG, "subboard bus not initialized");

    ESP_RETURN_ON_ERROR(bsp_power_set_codec_3v3(true), TAG, "enable module rail failed");
    vTaskDelay(pdMS_TO_TICKS(200) + 1);

    gpio_num_t xshut = bsp_subboard_map_gpio(slot, GPIO_NUM_48);
    ESP_RETURN_ON_ERROR(gpio_reset_pin(xshut), TAG, "reset XSHUT GPIO failed");
    ESP_RETURN_ON_ERROR(gpio_hold_dis(xshut), TAG, "release XSHUT hold failed");
    ESP_RETURN_ON_ERROR(gpio_sleep_sel_dis(xshut), TAG, "disable XSHUT sleep selection failed");
    const gpio_config_t gpio = {
        .pin_bit_mask = 1ULL << xshut,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&gpio), TAG, "configure XSHUT failed");
    ESP_RETURN_ON_ERROR(gpio_set_level(xshut, 0), TAG, "assert XSHUT failed");
    /* Round up so the reset pulse survives low FreeRTOS tick rates. */
    vTaskDelay(pdMS_TO_TICKS(2) + 1);
    ESP_RETURN_ON_ERROR(gpio_set_level(xshut, 1), TAG, "release XSHUT failed");
    vTaskDelay(pdMS_TO_TICKS(50) + 1);

    esp_err_t ret;
    vl53l0x_handle_t sensor = NULL;
    vl53l0x_ref_calibration_t calibration = {0};
    ESP_GOTO_ON_ERROR(vl53l0x_create(&sensor, bus), fail, TAG, "create sensor failed");
    ESP_GOTO_ON_ERROR(vl53l0x_init(sensor), fail, TAG, "initialize sensor failed");
    ESP_GOTO_ON_ERROR(vl53l0x_set_timing_budget(sensor, VL53L0X_DEFAULT_TIMING_BUDGET_US), fail, TAG, "set timing budget failed");
    ESP_GOTO_ON_ERROR(vl53l0x_perform_ref_calibration(sensor, &calibration), fail, TAG, "reference calibration failed");
    ESP_GOTO_ON_ERROR(vl53l0x_set_profile(sensor, VL53L0X_PROFILE_DEFAULT), fail, TAG, "set profile failed");
    ESP_GOTO_ON_ERROR(vl53l0x_set_mode(sensor, VL53L0X_MODE_SINGLE), fail, TAG, "set single mode failed");
    *out_sensor = sensor;
    ESP_LOGI(TAG, "VL53L0X ready: slot=%s XSHUT=%d", slot == BSP_SUBBOARD_SLOT_LEFT ? "left" : "right", xshut);
    return ESP_OK;

fail:
    if (sensor) {
        ESP_ERROR_CHECK_WITHOUT_ABORT(vl53l0x_destroy(sensor));
    }
    ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_set_level(xshut, 0));
    return ret;
}
