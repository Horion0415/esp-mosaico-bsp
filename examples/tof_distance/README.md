# ToF Distance Example

Uses one VL53L0X board and the current BSP to display a live ranging dashboard on the 480 × 480 screen. The application waits 100 ms after each measurement and also logs results over the serial console.

The interface uses a dark blue background with mint green accents: a large central distance reading, a 0–2 m arc gauge, a moving scale marker, and cards for return signal and ambient light levels (MCPS). Invalid measurements clear the previous distance, hide the target marker, and show the status code. Initialization or communication failures leave a visible error message. English labels avoid the need for a large Chinese font.

Display initialization, brightness, LVGL locking, power, and slot mapping use the current BSP APIs. The BSP selects the configuration for the hardware revision; the example does not duplicate revision-specific GPIO definitions. Supported revisions follow the BSP: v1.0 and v1.2, with v1.1 using the v1.2 mapping. Touch is disabled.

## Hardware and Usage

- ESP-Mosaico / ESP32-S31 with ESP-IDF 6.2 or newer.
- One ToF board in either slot, with a valid ToF EEPROM descriptor (type `0x10`). Use the `module_eeprom_programmer` example to program the descriptor if needed.
- Connect only one ToF board to avoid an address conflict at `0x29` on the shared bus.

After activating the ESP-IDF environment:

```bash
cd examples/tof_distance
idf.py build
idf.py -p PORT flash monitor
```

The module manager runs continuously. The application waits for a valid ToF descriptor in either slot, claims the slot, initializes the sensor, and displays the active slot above the gauge. No module at boot is a normal waiting state.

Removing the board or encountering a measurement communication error clears the displayed values, destroys the sensor handle, resets the mapped XSHUT GPIO, and releases the slot. The application then resumes discovery automatically. Reinsert the board into either slot to resume measurements without resetting the mainboard. Initialization failures are retried with a one-second delay. Cleanup failures retain ownership and are retried before another sensor is opened.

Invalid range results do not indicate removal: the screen shows the range status while measurement continues. Valid distances are also logged as `distance=... mm`. Slot presence is checked through the module manager before and after each blocking measurement. Its normal EEPROM debounce remains active while the slot is claimed.

This supports moving one ToF board between the two slots, not simultaneous operation of two boards sharing address `0x29`.

## Validation Scope

The serial version was tested with the left slot of the v1.1 mainboard connected through ACM1. The display version was built and flashed to the same board, and logs confirmed successful display, LVGL, and ToF initialization. During observation, measurements consistently returned invalid target status `4`, with no crashes. The rendered interface has not been visually verified from a screen capture or photo. Other mainboard revisions, the right slot, and removal behavior still require separate hardware validation.

The hot-plug lifecycle passed an ESP32-S31 build and host fault-injection checks covering discovery in both slots, removal during a read, initialization and communication failures, cleanup retries, and continued sampling after invalid ranges. The updated firmware was flashed to ACM1. Hardware logs confirmed right-slot discovery, valid measurements, and successful lease release after communication loss followed by an absent-slot event. A complete unplug/replug cycle in each slot still requires a continuous hardware capture.
