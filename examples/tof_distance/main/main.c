/*
 * SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: Apache-2.0
 */
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mosaico_module_mgr.h"
#include "mosaico_module_tof.h"
#include "tof_ui.h"

static const char *TAG = "tof_distance";

static bool tof_present(mosaico_module_mgr_slot_t slot)
{
    mosaico_module_mgr_info_t info;
    esp_err_t ret = mosaico_module_mgr_get_info(slot, &info);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Read slot state failed: %s", esp_err_to_name(ret));
        return false;
    }
    return info.presence == MOSAICO_MODULE_PRESENCE_PRESENT &&
           info.descriptor_state == MOSAICO_MODULE_DESCRIPTOR_VALID &&
           info.eeprom.board_type == MOSAICO_BOARD_TYPE_TOF;
}

static void measure(vl53l0x_handle_t sensor, mosaico_module_mgr_slot_t slot)
{
    while (tof_present(slot)) {
        vl53l0x_data_t data = {0};
        esp_err_t ret = vl53l0x_single_measure(sensor, &data);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Measurement interrupted: slot=%s error=%s", mosaico_module_mgr_slot_to_name(slot), esp_err_to_name(ret));
            return;
        }
        /* Discard a sample if removal was detected during the blocking read. */
        if (!tof_present(slot)) {
            return;
        }
        tof_ui_update(&data);
        if (data.valid) {
            ESP_LOGI(TAG, "distance=%u mm signal=%.2f MCPS ambient=%.2f MCPS",
                     (unsigned)data.distance_mm, data.signal_rate_mcps, data.ambient_rate_mcps);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

static void close_module(vl53l0x_handle_t sensor, const mosaico_module_lease_t *lease)
{
    esp_err_t ret;
    /* Retain ownership until all driver resources have been released. */
    while (sensor && (ret = vl53l0x_destroy(sensor)) != ESP_OK) {
        ESP_LOGE(TAG, "Destroy sensor failed: %s", esp_err_to_name(ret));
        tof_ui_status("CLEANUP RETRY", "Releasing the previous sensor");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    sensor = NULL;
    gpio_num_t xshut = bsp_subboard_map_gpio((bsp_subboard_slot_t)lease->slot, GPIO_NUM_48);
    while ((ret = gpio_reset_pin(xshut)) != ESP_OK) {
        ESP_LOGE(TAG, "Release XSHUT failed: %s", esp_err_to_name(ret));
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    while ((ret = mosaico_module_mgr_release(lease)) != ESP_OK) {
        ESP_LOGE(TAG, "Release slot failed: %s", esp_err_to_name(ret));
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK(tof_ui_init());
    ESP_ERROR_CHECK(mosaico_module_mgr_init(NULL));
    /* XSHUT does not reuse the EEPROM control pin, so claimed slots remain monitored. */
    const mosaico_module_mgr_claim_config_t config = {
        .expected_type = MOSAICO_BOARD_TYPE_TOF,
        .slot = MOSAICO_MODULE_MGR_SLOT_AUTO,
        .timeout_ms = 1000,
    };

    while (true) {
        tof_ui_status("WAITING FOR MODULE", "Insert one ToF board into either slot");
        mosaico_module_lease_t lease = {0};
        esp_err_t ret;
        do {
            ret = mosaico_module_mgr_claim(&config, &lease);
            if (ret != ESP_OK && ret != ESP_ERR_TIMEOUT) {
                ESP_LOGE(TAG, "Claim ToF module failed: %s", esp_err_to_name(ret));
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
        } while (ret != ESP_OK);

        vl53l0x_handle_t sensor = NULL;
        tof_ui_status("CALIBRATING", lease.slot == MOSAICO_MODULE_MGR_SLOT_LEFT ? "Left slot  /  VL53L0X" : "Right slot  /  VL53L0X");
        ret = mosaico_tof_new((bsp_subboard_slot_t)lease.slot, &sensor);
        if (ret == ESP_OK) {
            tof_ui_set_slot(lease.slot);
            measure(sensor, lease.slot);
            tof_ui_status("RECONNECTING", "Module removed or communication interrupted");
        } else {
            ESP_LOGW(TAG, "Initialize ToF failed: %s", esp_err_to_name(ret));
            tof_ui_status("RETRYING SENSOR", esp_err_to_name(ret));
        }
        close_module(sensor, &lease);
        /* Let discovery debounce unplugging before attempting another claim. */
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
