# Mosaico ToF Module

Supports one VL53L0X distance sensor board connected to either the left or right slot. Based on `vl53l0x_tof_test` from the `test/subboard_test_lh` branch, it retains the XSHUT reset, default timing budget, reference calibration, and single-shot measurement configuration.

Before use, initialize the module manager and claim a slot of type `MOSAICO_BOARD_TYPE_TOF` (`0x10`) through `mosaico_module_mgr`, then call:

```c
vl53l0x_handle_t sensor = NULL;
ESP_ERROR_CHECK(mosaico_tof_new((bsp_subboard_slot_t)lease.slot, &sensor));
vl53l0x_data_t data = {0};
ESP_ERROR_CHECK(vl53l0x_single_measure(sensor, &data));
// Use distance_mm only when data.valid is true.
ESP_ERROR_CHECK(vl53l0x_destroy(sensor));
sensor = NULL;
ESP_ERROR_CHECK(mosaico_module_mgr_release(&lease));
```

- The bus comes from `bsp_subboard_get_i2c_bus()`, which accounts for the mainboard revision. XSHUT uses GPIO48 on the left slot and its mapped counterpart on the right.
- Only one ToF board may be connected. Both boards would use the default address `0x29`, so simultaneous use is not supported. INT is unused.
- Initialization includes power settling and reset delays. Measurements are blocking. Serialize all operations and destruction on the same handle; do not open an already claimed sensor again.
- `ESP_OK` indicates successful communication. Check `data.valid` before using the distance; `data.range_status` reports invalid measurement status.
- Destroying the driver does not turn off shared power or delete the shared I²C bus.

The dependency is pinned to [pkolt/vl53l0x 1.0.0](https://components.espressif.com/components/pkolt/vl53l0x/versions/1.0.0). `driver_fix.cmake` patches a source copy in the build directory to fix three issues: an I²C device leak on allocation failure, destruction being rejected after failed initialization, and the measurement state not being cleared after a failed single-shot measurement. It does not modify `managed_components`. Configuration stops if a patch no longer matches; review the patches when upgrading the dependency.

See [tof_distance](../../examples/tof_distance) for a complete example.
