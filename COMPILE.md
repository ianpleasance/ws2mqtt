# Compiling and flashing ws2mqtt firmware

This guide explains how to build and install both firmware components of [ws2mqtt](https://github.com/Tho85/ws2mqtt) on **Windows** and **Linux**.

> **Safety:** ws2mqtt is an unofficial hobby project, **not a safety-critical alarm system**. Never rely on it as your only means of smoke, heat or carbon-monoxide detection or notification. Keep the certified WiSafe2 alarms working independently.

## 1. What you are building

The repository contains two separate [PlatformIO](https://platformio.org/) projects:

| Directory | Target | Role |
| --- | --- | --- |
| `Firmware-Atmega/` | Arduino Pro Mini / ATmega328P | Interface to the WiSafe2 radio module |
| `Firmware-ESP/` | ESP32 development board | Wi-Fi, MQTT and Home Assistant integration |

**Build and flash both projects separately.** The ESP32 cannot upload firmware to the Arduino simply by flashing the ESP32 project.

The checked repository configures:

- Arduino: `platform = atmelavr`, `board = pro8MHzatmega328`, `framework = arduino`.
- ESP32: `platform = espressif32`, `board = esp32dev`, `framework = arduino`.
- ESP32 dependencies: `ArduinoJson`, `PubSubClient`, and `Sqlite3Esp32`; PlatformIO installs these automatically.

The Arduino's configured target is an **8 MHz ATmega328P Pro Mini**. Check the actual board before proceeding. If yours is a **5 V / 16 MHz** Pro Mini, change the Arduino project's `board` setting to `pro16MHzatmega328` before compiling and uploading. The voltage, clock speed, bootloader and serial adapter must match your actual hardware.

## 2. Obtain the source code (Windows and Linux)

Install [Git](https://git-scm.com/downloads), or download the repository as a ZIP from GitHub and extract it.

```sh
git clone https://github.com/Tho85/ws2mqtt.git
cd ws2mqtt
```

If you are working from your own fork, clone **your fork's URL** instead. All commands below assume you are in the repository root, which contains `Firmware-Atmega` and `Firmware-ESP`.

## 3. Install PlatformIO

### Recommended: Visual Studio Code (Windows or Linux)

1. Install [Visual Studio Code](https://code.visualstudio.com/).
2. Open **Extensions** (`Ctrl+Shift+X`).
3. Search for **PlatformIO IDE** and install the extension.
4. Restart VS Code if prompted, and allow PlatformIO to install its toolchains.
5. Open **one firmware folder at a time** using **File → Open Folder**: first `Firmware-Atmega`, then `Firmware-ESP`.

Each firmware folder contains its own `platformio.ini`. Opening the specific project folder makes it straightforward to build and upload the correct target.

### Finding the Build and Upload buttons in VS Code

With the **PlatformIO IDE** extension installed and a firmware project open, look at the **bottom status bar**, usually towards the left. PlatformIO displays small icons:

| Icon (approximate) | Action | What it does |
| --- | --- | --- |
| **✓** (tick/check mark) | **Build** | Compiles the selected firmware project without flashing the board. |
| **→** (right-pointing arrow) | **Upload** | Builds if necessary, then flashes the firmware to the connected board. |
| **Plug** icon | **Serial Monitor** | Opens serial output from the board for debugging. |

The exact icon appearance and placement can vary slightly with the PlatformIO and VS Code versions. Hover your mouse over an icon to see its tooltip before clicking.

**Alternative route (especially useful when working with both firmware projects):**

1. Click the **PlatformIO alien-head icon** in the left-hand Activity Bar.
2. Expand **PROJECT TASKS** and select the relevant environment (normally `default` for the Arduino project, or the configured ESP32 environment).
3. Expand **General**, then click **Build**, **Upload**, or **Monitor**.

**If you cannot see the buttons:** Check that the **PlatformIO IDE** extension is installed and enabled, and open the *specific firmware directory* containing `platformio.ini` using **File → Open Folder**. Wait for PlatformIO to finish initialising. You can always use **PROJECT TASKS** instead of the status-bar shortcuts.


### Alternative: PlatformIO command line (Windows or Linux)

Install Python 3 and pip, then install [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation/index.html):

```sh
python -m pip install --user platformio
```

On some Linux distributions, `python3` is the command instead of `python`; a Python virtual environment or `pipx` may be preferable if system Python is externally managed. On Windows, the Python launcher may be available as `py -m pip ...`.

Check the installation:

```sh
pio --version
```

If `pio` is not found, use the **PlatformIO Core CLI terminal** inside VS Code, or add the Python user scripts directory to your `PATH`. On Windows the executable may be in `%APPDATA%\Python\Python3xx\Scripts`; on Linux it is often in `~/.local/bin`.

The commands below work in PowerShell, Command Prompt and Linux shells unless otherwise indicated.

## 4. Compile and upload the Arduino firmware

### 4.1 Hardware and connection

The Arduino Pro Mini usually has **no USB connector**. For normal serial bootloader uploads you need a **USB-to-TTL serial adapter** with a suitable logic voltage and, preferably, a **DTR** output. You can also use an **ISP programmer** if the bootloader is missing.

For a typical six-pin serial programming header:

| USB-to-TTL adapter | Arduino Pro Mini |
| --- | --- |
| GND | GND |
| VCC | VCC (only if the adapter supply voltage matches the board) |
| TX | RXI |
| RX | TXO |
| DTR | DTR / GRN / reset circuitry, if provided |

**Check your board's pin labels and wiring before applying power.** TX/RX are crossed. Never connect 5 V logic to a 3.3 V-only board. Disconnect other power sources when unsure; avoid powering a board simultaneously from multiple supplies. For an assembled ws2mqtt PCB, programming the Arduino separately may avoid electrical interference from the connected ESP32 and radio module.

### 4.2 Check the board configuration

Open `Firmware-Atmega/platformio.ini`. The original project's default environment uses:

```ini
[env]
platform = atmelavr
board = pro8MHzatmega328
framework = arduino
```

For a **16 MHz** ATmega328P Pro Mini, change only the board setting to:

```ini
board = pro16MHzatmega328
```

Do not change this merely to make an upload succeed: first confirm the actual board clock, supply voltage and bootloader. If you are using a nonstandard bootloader, further upload settings may be needed.

### 4.3 Build with VS Code

1. Open `Firmware-Atmega` as the VS Code project folder.
2. Select the `default` PlatformIO environment.
3. Click **Build** (check mark).
4. Wait for **SUCCESS** in the terminal.

Building produces firmware under `.pio/build/default/`, normally including `firmware.hex`.

### 4.4 Build and upload from the command line

From the repository root:

```sh
cd Firmware-Atmega
pio run -e default
```

Connect the USB-to-TTL adapter and identify its serial port:

- **Windows:** Device Manager → **Ports (COM & LPT)**, for example `COM5`.
- **Linux:** typically `/dev/ttyUSB0` or `/dev/ttyACM0`. You can run `pio device list` or inspect `dmesg` after connecting the adapter.

Upload through the Arduino bootloader, replacing the port as appropriate:

**Windows:**

```powershell
pio run -e default -t upload --upload-port COM5
```

**Linux:**

```sh
pio run -e default -t upload --upload-port /dev/ttyUSB0
```

Alternatively, select **Upload** in VS Code after choosing the correct serial port.

> **Important:** The repository also defines an environment named `programmer`. It uses a **custom `avrdude` command with `stk500v2`**, rather than the normal serial bootloader uploader. Do **not** select this environment for a basic USB-to-TTL adapter. Use `default` for bootloader uploads. The `programmer` environment is only suitable if your attached programmer supports its configured protocol; adjust the settings for other ISP programmers.

If the adapter lacks DTR, you may need to press the Pro Mini's **RESET** button just as uploading starts. If the Arduino has no working bootloader, use a compatible ISP programmer and an appropriate `avrdude`/PlatformIO configuration instead.

## 5. Configure, compile and upload the ESP32 firmware

### 5.1 Configure Wi-Fi and MQTT credentials

Before building, copy the supplied template:

**Windows PowerShell:**

```powershell
Copy-Item Firmware-ESP/include/credentials.h.example Firmware-ESP/include/credentials.h
```

**Linux:**

```sh
cp Firmware-ESP/include/credentials.h.example Firmware-ESP/include/credentials.h
```

Edit `Firmware-ESP/include/credentials.h` and enter your values:

```c
#ifndef CREDENTIALS_H
#define CREDENTIALS_H 1

#define WIFI_SSID "YOUR_WIFI_NAME"
#define WIFI_PSK "YOUR_WIFI_PASSWORD"

#define MQTT_HOST "192.168.1.10"
#define MQTT_USERNAME "YOUR_MQTT_USERNAME"
#define MQTT_PASSWORD "YOUR_MQTT_PASSWORD"

#endif
```

Replace the example IP address and credentials with those for **your** network and MQTT broker. The ESP32 must be able to reach the broker. Avoid committing your real `credentials.h` to Git; check `git status` before committing.

### 5.2 Check the ESP32 configuration

Open `Firmware-ESP/platformio.ini`. The checked upstream configuration uses:

```ini
[env]
platform = espressif32
board = esp32dev
framework = arduino
monitor_filters = esp32_exception_decoder
upload_speed = 921600
```

This targets a typical ESP32-WROOM-based development board. If your board is different, select the corresponding PlatformIO board ID. If uploads fail at `921600`, try lowering `upload_speed` to `115200`.

PlatformIO automatically downloads the dependencies declared in the same file. You do not need to install them manually through Arduino IDE.

### 5.3 Build with VS Code

1. Open `Firmware-ESP` as the VS Code project folder.
2. Check that `include/credentials.h` contains the correct settings.
3. Select the `default` environment.
4. Click **Build** and wait for **SUCCESS**.
5. Connect the ESP32 to your computer using a **data-capable** USB cable.
6. Select the ESP32 serial port and click **Upload**.

The resulting binary is normally `.pio/build/default/firmware.bin`.

### 5.4 Build and upload from the command line

From the repository root:

```sh
cd Firmware-ESP
pio run -e default
```

Find the ESP32's serial port with `pio device list` or your operating system's device manager.

**Windows:**

```powershell
pio run -e default -t upload --upload-port COM6
```

**Linux:**

```sh
pio run -e default -t upload --upload-port /dev/ttyUSB0
```

If the uploader stays at `Connecting...`, hold the board's **BOOT** button until flashing begins, then release it. Some boards also require pressing **EN/RESET** while BOOT is held.

### 5.5 View the ESP32 serial output

The ESP32 source configures serial output at **19200 baud**. Use:

**Windows:**

```powershell
pio device monitor -p COM6 -b 19200
```

**Linux:**

```sh
pio device monitor -p /dev/ttyUSB0 -b 19200
```

Press the ESP32 **EN/RESET** button if you need to see startup messages. Close the serial monitor before trying to upload again if the port is busy.

## 6. Windows-specific notes

- Install drivers for your board's USB interface if the port does not appear. Common chips include **CP210x**, **CH340/CH341**, and **FTDI FT232**; use drivers from the chip manufacturer or trusted board vendor.
- Check the assigned port in **Device Manager → Ports (COM & LPT)**. The Arduino adapter and ESP32 can have **different COM ports**.
- Close Arduino IDE, other serial monitors and terminal programs before uploading; only one application can normally own a serial port at a time.
- PowerShell commands use the same `pio run` syntax as Linux, with a Windows COM port.

## 7. Linux-specific notes

USB serial devices commonly appear as `/dev/ttyUSB0` or `/dev/ttyACM0`.

List devices:

```sh
pio device list
ls -l /dev/serial/by-id/  # if available
```

Using a stable `/dev/serial/by-id/...` path is preferable if you have multiple USB serial devices attached.

On Ubuntu/Debian, serial port access is often granted through the `dialout` group:

```sh
sudo usermod -aG dialout "$USER"
```

**Log out and back in** (or start a new login session) for the group change to take effect. Check the actual device's group with `ls -l /dev/ttyUSB0`; other distributions may use different groups or udev rules.

If a device is busy or disappears, inspect recent kernel messages:

```sh
sudo dmesg | tail -n 30
```

Some Linux installations run services such as ModemManager that may probe serial adapters. Check for port conflicts before changing system services. Do not use `chmod 777` on serial devices as a permanent workaround.

## 8. Flashing order and initial setup

1. Confirm the Arduino board variant and compile **`Firmware-Atmega`**.
2. Flash the Arduino via a serial bootloader or correctly configured ISP programmer.
3. Create and edit **`Firmware-ESP/include/credentials.h`**.
4. Compile and flash **`Firmware-ESP`**.
5. Disconnect programming cables and assemble/connect the Arduino, ESP32 and WiSafe2 radio module according to the repository's PCB and wiring documentation.
6. Power the gateway through the **ESP32 USB connector**, as described by the repository.
7. Check ESP32 serial output and confirm the gateway joins Wi-Fi and connects to MQTT.
8. Pair the WiSafe2 radio module with the existing alarm network using the instructions in the repository's `README.md`.
9. Confirm that the gateway and devices appear in Home Assistant through MQTT discovery.

Always keep the certified detectors' normal alarm functionality independent of ws2mqtt.

## 9. Troubleshooting

| Symptom | Checks |
| --- | --- |
| `pio: command not found` | Open the PlatformIO Core terminal, or add the Python scripts directory to `PATH`. |
| Arduino build fails | Confirm `Firmware-Atmega` is the active project, dependencies have installed, and the configured board matches your hardware. |
| Arduino upload times out | Check TX/RX crossing, GND, DTR/reset, serial port, board clock and bootloader. Use `default`, not `programmer`, with a USB-to-TTL adapter. |
| Arduino has no bootloader | Flash using an appropriate ISP programmer/configuration or install the correct bootloader first. |
| ESP32 upload stuck at `Connecting...` | Use a data USB cable, check the port, press BOOT, and try `upload_speed = 115200`. |
| `Permission denied` on Linux | Check device ownership and group membership; log in again after adding yourself to `dialout`. |
| Serial monitor shows garbage | Set the monitor to `19200` baud for ESP32 application logs; early ROM boot messages may use a different rate. |
| ESP32 builds but cannot connect to Wi-Fi | Verify SSID/password and 2.4 GHz Wi-Fi coverage. |
| ESP32 connects to Wi-Fi but not MQTT | Check `MQTT_HOST`, credentials, broker availability, firewall rules and network connectivity. |
| Home Assistant devices do not appear | Check MQTT connectivity and discovery configuration, and verify that the WiSafe2 module has been paired. |

For more detailed errors, run a verbose build:

```sh
pio run -e default -v
```

## 10. Quick command reference

Run these from the **repository root** (replace `COM5`, `COM6` and `/dev/ttyUSB0` with your real ports):

| Action | Command |
| --- | --- |
| Build Arduino | `pio run -d Firmware-Atmega -e default` |
| Flash Arduino (Windows) | `pio run -d Firmware-Atmega -e default -t upload --upload-port COM5` |
| Flash Arduino (Linux) | `pio run -d Firmware-Atmega -e default -t upload --upload-port /dev/ttyUSB0` |
| Build ESP32 | `pio run -d Firmware-ESP -e default` |
| Flash ESP32 (Windows) | `pio run -d Firmware-ESP -e default -t upload --upload-port COM6` |
| Flash ESP32 (Linux) | `pio run -d Firmware-ESP -e default -t upload --upload-port /dev/ttyUSB0` |
| List connected serial devices | `pio device list` |

## References

- [ws2mqtt repository and setup instructions](https://github.com/Tho85/ws2mqtt)
- [Arduino PlatformIO configuration](https://github.com/Tho85/ws2mqtt/blob/main/Firmware-Atmega/platformio.ini)
- [ESP32 PlatformIO configuration](https://github.com/Tho85/ws2mqtt/blob/main/Firmware-ESP/platformio.ini)
- [ESP32 credentials template](https://github.com/Tho85/ws2mqtt/blob/main/Firmware-ESP/include/credentials.h.example)
- [PlatformIO documentation](https://docs.platformio.org/)

