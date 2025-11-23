# Firmware Installation

This folder contains the pre-compiled binary file (`.bin`) for the **ESP32 Ham Combo**. You can use this file to flash your device without needing to install the Arduino IDE or compile the code yourself.

## ⚠️ Hardware Compatibility
This firmware is compiled specifically for the **ESP32-2432S028R** (Cheap Yellow Display) board.

## How to Flash

We recommend using **[ESPHome-Flasher](https://github.com/esphome/esphome-flasher/releases)** as it is the easiest tool to use and requires no configuration.

### Steps:

1.  **Download** the latest `.bin` file from this folder (e.g., `ESP32_ham_combo_v1.0.0.bin`) or from the [Releases](../../releases) page.
2.  Download and open **ESPHome-Flasher**.
3.  Connect your ESP32 board to your computer via USB.
4.  In ESPHome-Flasher:
    *   **Serial Port:** Select the COM port of your device.
    *   **Firmware:** Click "Browse" and select the downloaded `.bin` file.
5.  Click **Flash ESP**.

The tool will automatically erase the necessary memory sectors, upload the firmware, and reset the board. You can watch the "Console" output to see the device booting up.

### 🔧 Troubleshooting: Stuck on "Connecting..."

If the flasher gets stuck at the `Connecting........_____.....` stage and eventually shows an error, it means the automatic boot mode failed. You need to enable it manually:

1.  Hold down the **BOOT** button on the back of the device.
2.  While holding BOOT, press and release the **RST** (Reset) button.
3.  Release the **BOOT** button.
4.  Click **Flash ESP** again.


---
**License & Copyright**
© 2025 Leszek (HF7A). This firmware is part of the [ESP32 Ham Combo](../../) project and is licensed under [CC BY-NC-SA 4.0](../../LICENSE).