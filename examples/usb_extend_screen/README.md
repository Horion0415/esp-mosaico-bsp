# USB extend screen (ESP-Mosaico)

Based on the [esp-iot-solution USB extend screen example](https://github.com/espressif/esp-iot-solution/tree/master/examples/usb/device/usb_extend_screen), this example uses ESP-Mosaico as a Windows secondary display. The PC sends frames over USB High Speed, and the board displays them on the CO5300 480×480 panel. It also supports HID touch reporting and UAC audio. JPEG frames are decoded by the hardware JPEG decoder.

## Features

- Native 480×480 RGB565 lossless transfer by default, avoiding JPEG ringing around text and mouse-pointer edges
- Up to five touch points reported to the PC over HID
- ES8311 audio input/output at 48 kHz stereo

## Hardware connections

1. Connect the Mosaico **USB High Speed (OTG)** port to the PC.
2. Ensure that the display and ES8311 audio circuit are powered. The example automatically enables VCC_3V3 and the backlight.

## Build and flash

This application owns TinyUSB. Flash and monitor through the UART port:

```bash
cd examples/usb_extend_screen
idf.py --preview set-target esp32s31
idf.py build
idf.py -p UART_PORT flash monitor
```

The first build downloads `tinyusb` and `usb_device_uac` from the component registry, so network access is required.

The example uses the USB-OTG port for the display device and disables the BSP USB CDC console by default. Use UART for debug logs.

## Windows driver

Windows 10/11 requires the Espressif indirect display driver (IDD). See the esp-iot-solution documentation for downloads and installation:

- [Windows driver instructions](https://github.com/espressif/esp-iot-solution/tree/master/examples/usb/device/usb_extend_screen/windows_driver)
- [Driver installer](https://dl.espressif.com/AE/esp-iot-solution/xfz1986_usb_graphic_250224_rc_sign.exe)

The composite device (display, touch, and audio) uses `USB\VID_303A&PID_2986`. For display-only operation, disable touch and audio in `menuconfig` and change the PID to `0x2987`.

## Configuration

| Option | Description |
|--------|-------------|
| `CONFIG_USB_EXTEND_SCREEN_WIDTH` / `HEIGHT` | Logical resolution on the PC, default 480×480; lower values make content larger but reduce sharpness |
| `CONFIG_USB_EXTEND_SCREEN_MAX_FPS` | Maximum frame rate reported to the PC driver |
| `CONFIG_USB_EXTEND_SCREEN_JPEG_QUALITY` | JPEG quality hint in the vendor string |
| `CONFIG_USB_EXTEND_SCREEN_FRAME_LIMIT_B` | Maximum JPEG frame size in bytes |
| `CONFIG_HID_TOUCH_ENABLE` | Enables HID touch |
| `CONFIG_UAC_AUDIO_ENABLE` | Enables UAC audio |

## Troubleshooting

### Touch controls the wrong display

Run the setup wizard again in Windows **Tablet PC Settings** and select the Mosaico secondary display.

### Audio stutters

Reduce `CONFIG_USB_EXTEND_SCREEN_MAX_FPS` to leave more USB bandwidth for UAC audio.

### Flashing and monitoring

Use UART for flashing and serial monitoring. The extended-display protocol owns USB-OTG while this example is running.
