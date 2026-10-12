# Camera Photo App

A full-screen camera example for ESP-Mosaico with live OV3640 preview, a touch UI, and photo storage on SPI NAND. Optional **USB High-Speed MSC** lets a PC browse photos on a removable drive (**disabled by default**).

## Features

| Feature | Description |
|---------|-------------|
| Live preview | Full-screen 480×480 preview; the PPA dynamically center-crops/scales YUV422 frames based on the camera resolution |
| Photo storage | JPEG images (quality 80, maximum 256 KB each) stored on SPI NAND as `0001.jpg`, `0002.jpg`, etc. |
| Gallery | Full-screen browsing, horizontal swipes, `filename(sizeKB)` labels, and deletion confirmation |
| Flash | Onboard flash on GPIO34; when enabled, lights for 200 ms before and after capture, with exposure temporarily increased by ×1.5 |
| Preview flip | Bottom flip button toggles preview mirroring and the top-left `Front` / `Rear` label; the setting is saved in NVS |
| Camera power | Long-press the top board button (GPIO7) to enter or leave PWDN sleep; the screen turns black and shows a message while off |
| USB drive (optional) | `CONFIG_CAMERA_PHOTO_USB_MSC`, disabled by default; exports `DCIM/` over HS OTG when enabled |

## Hardware requirements

### Required

- **ESP-Mosaico** mainboard with 16 MB Flash, PSRAM, and a CO5300 480×480 display
- **Camera subboard** (OV3640) in the **left slot** (EEPROM `0x50`)

### Interfaces and constraints

| Item | Description |
|------|-------------|
| Debug port | UART console; USB Serial/JTAG is disabled because GPIO33 is shared with camera D2 |
| Top button | GPIO7; **long-press** to toggle camera power; short presses are ignored to prevent accidental activation |
| Flash | GPIO34, active low |
| USB export | USB 2.0 HS OTG, D+ Pin 44 / D- Pin 45; requires `CONFIG_CAMERA_PHOTO_USB_MSC` |
| Photo NAND | SPI NAND; see the storage details below for the catalog page and JPEG data area |

## Project structure

```text
examples/camera_photo_app/
├── partitions.csv          # Custom partitions: app + FAT storage (USB MSC)
├── sdkconfig.defaults
└── main/
    ├── main.c              # Preview, capture, and gallery logic
    ├── camera_ui.c/h       # LVGL camera and gallery UI
    ├── camera_settings.c/h # NVS persistence for preview mirroring
    ├── photo_store.c/h     # SPI NAND photo catalog and I/O
    ├── photo_usb_msc.c/h   # USB MSC and NAND-to-FAT synchronization
    └── ui_icons.c/h        # UI icon assets
```

Dependencies include `esp-mosaico-bsp`, `mosaico_module_camera`, `mosaico_module_mgr`, `esp_new_jpeg`, and `esp_tinyusb`.

## Build and flash

```bash
cd examples/camera_photo_app
idf.py set-target esp32s31
idf.py build
idf.py -p PORT flash
idf.py -p PORT monitor
```

> Perform a full `flash`, including `partition_table`, when enabling USB export for the first time or after changing the partition table.

Defaults in `sdkconfig.defaults` include Octal PSRAM at 250 MHz, the sensor's Kconfig default camera resolution, and **USB mass storage disabled** with the `SINGLE_APP_LARGE` partition table.

> **Note:** `sdkconfig.defaults` applies when `sdkconfig` is first generated or regenerated after deletion. If USB was enabled in a previous build, the local `sdkconfig` retains that setting. Follow “Disable USB mass storage” below before flashing again.

## Quick start

1. Insert the camera subboard into the left slot and power on. Wait for `Camera photo app started` in the serial log and the live preview on the display.
2. Tap the bottom **shutter** button to take a photo. The bottom-left thumbnail updates to the latest photo.
3. Tap the bottom-left **gallery** button to browse photos. Swipe left or right to switch photos, and tap **back** at the top left to return to the camera.
4. Optionally, enable USB mass storage and connect the HS OTG port to a PC. Open the drive's `DCIM` folder to view photos.

## Camera touch interface

```text
┌─────────────────────────────────────┐
│ Front              [Flash] [i]      │  ← Top bar
│                                     │
│ Live preview / black when off       │
│                                     │
│ [Gallery]     (Shutter)      [Flip]  │  ← Bottom bar
└─────────────────────────────────────┘
```

| Control | Position | Action |
|---------|----------|--------|
| Orientation label | Top left | Shows `Front` or `Rear` according to the flip setting |
| Flash | Right side of top bar | Toggles flash; the icon indicates on/off |
| Help (i) | Far right of top bar | Shows button controls and top-button long-press instructions |
| Gallery | Bottom left | Opens the gallery and shows the latest thumbnail |
| Shutter | Bottom center | Takes a photo |
| Flip | Bottom right | Toggles front/rear preview mirroring and saves the setting in NVS |

### Turn off the camera with a long press

- **Action:** Long-press the **top board button (GPIO7)**.
- **While off:** A black screen displays `Camera is off.` / `Long press the top button to turn on again.`
- **Turn on again:** Long-press the top button again to restore the preview.

The sensor enters sleep through PWDN (GPIO48). Capture stops, but the subboard remains registered.

## Gallery mode

- **Open:** Tap the bottom-left gallery button in the camera view.
- **Navigate:** Swipe **left or right** across the image.
- **Title:** The top bar shows a label such as `0039.jpg(320KB)`.
- **Delete:** Tap the top-right delete icon, then confirm with **Delete**.
- **Return:** Tap the back icon at the top left.

Deleting a photo updates the NAND catalog. If USB MSC is enabled, it also removes the exported copy when the PC is not mounting the drive. Deleting all photos returns to the camera view automatically.

## Flash behavior

When flash is enabled, each capture:

1. Temporarily increases exposure to approximately **1.5 times** its current value.
2. Turns the flash **on for 200 ms**, captures a frame, and encodes and saves it to NAND and the USB export area if enabled.
3. Lights the flash for another **200 ms** after saving, then turns it off and restores exposure.

The flash remains off throughout capture when disabled.

## USB photo export (optional mass storage)

> **Disabled by default.** Under `menuconfig -> Camera Photo App`, enable `Enable USB High-Speed MSC photo export (U-disk)` (`CONFIG_CAMERA_PHOTO_USB_MSC`).

Also complete these steps:

1. Merge the settings from `sdkconfig.defaults.usb_msc` into `sdkconfig.defaults`, or configure the partition table and TinyUSB MSC manually in menuconfig.
2. Use the **`partitions.csv`** in this directory, which includes the FAT `storage` partition. **Delete `sdkconfig`**, then rebuild and perform a full flash.

### Disable USB mass storage

`sdkconfig.defaults` does not automatically override an existing `sdkconfig`. If the PC still detects a drive after flashing, the local configuration still contains the old settings:

```bash
cd examples/camera_photo_app
rm -f sdkconfig
idf.py build
# Enter ROM download mode with the board controls, then:
idf.py -p ROM_PORT flash
```

Alternatively, disable `Enable USB High-Speed MSC photo export` in `idf.py menuconfig` and restore the `Single factory app, large` partition table.

Verify these settings in `sdkconfig`:

- `# CONFIG_CAMERA_PHOTO_USB_MSC is not set`
- `# CONFIG_TINYUSB_MSC_ENABLED is not set`
- `CONFIG_PARTITION_TABLE_SINGLE_APP_LARGE=y`

### How it works

- The primary photo store is **SPI NAND**, using the custom `photo_store` catalog.
- A separate **FAT partition** in internal Flash (`storage` in `partitions.csv`) serves USB MSC.
- JPEG files in NAND are automatically synchronized to `/usb/DCIM/` for photo capture, deletion, and USB connection.

### Usage

1. Take photos on the device.
2. Connect the **HS OTG port (Pin 44/45)** to a PC with a data-capable USB cable.
3. The PC detects a **removable drive**. Open **`DCIM`** to view files such as `0001.jpg`.
4. Safely eject or disconnect USB to continue taking photos. The device synchronizes again before the next connection.

### Notes

- The drive contains **exported copies** of the device gallery. Deleting files on the PC **does not** delete photos from NAND; they may reappear on the next synchronization.
- The device can still take photos and save them to NAND while the PC mounts the drive. USB synchronization waits until the drive is released.
- This example **does not enable** the BSP USB CDC console. Use UART for debugging.

### Partition table (16 MB Flash)

| Partition | Type | Approximate size | Purpose |
|-----------|------|------------------|---------|
| `factory` | app | 2.5 MB | Application |
| `storage` | fat | Remaining ~13.4 MB | USB MSC / DCIM export |

## Configuration

### menuconfig → Camera Photo App

| Option | Default | Description |
|--------|---------|-------------|
| Enable USB High-Speed MSC photo export | **Disabled** | Enables mass-storage export as described above |
| First logical NAND page used for photo storage | 64 | First NAND page for the photo catalog and data area |
| Maximum number of photos stored on NAND | 64 | Maximum photo count |

Changing the NAND start page **overwrites** the photo storage area at that page and subsequent pages. Adjust it carefully.

### Other settings

- Preview flip: NVS namespace `camera_app`, key `prev_flip`
- JPEG: `JPEG_QUALITY=80`, `MAX_JPEG_SIZE=256 KB`

## Storage details (SPI NAND)

- Page `CONFIG_CAMERA_PHOTO_NAND_START_PAGE`: catalog containing magic `PHOT`, filenames, sizes, and page numbers.
- Subsequent pages: JPEG files appended sequentially. Deletion updates only the catalog and does not reclaim written pages.
- Filenames: incrementing `%04u.jpg`, matching the USB drive's `DCIM` contents.

## Troubleshooting

**No USB drive appears on the PC**

Enable `CONFIG_CAMERA_PHOTO_USB_MSC` in menuconfig and perform a full flash with `partitions.csv`. Connect the USB cable to the HS OTG port (Pin 44/45).

**Corrupted preview or camera disconnects**

Check the camera subboard connection in the left slot. Capture failures reported in the serial log trigger an automatic attempt to restart streaming.

**The last frame remains visible after turning off the camera**

Flash firmware containing the latest UI fix. Turning off the camera should show a black screen and the status message.

## References

- Preview and display: `camera_lcd_preview` (PPA + LVGL canvas)
- NAND access: BSP `bsp_nand_flash_*` APIs
- USB MSC: ESP-IDF `tusb_msc` / `esp_tinyusb` component
