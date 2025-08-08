# ESP32 Hybrid Arcade Stick

![Arcade Stick](https://img.icons8.com/ios-filled/100/000000/joystick.png)

Firmware for a high-performance, dual-mode (Bluetooth/USB) arcade stick using an ESP32. This project is designed for competitive gaming, featuring ultra-low latency, power-saving modes, and reliable input processing.

**Author:** MatMont01  
**Version:** 1.0 
**Date:** 2025-08-07

## Key Features

-   **Dual Mode Operation**: Seamlessly switch between a wireless Bluetooth connection and a traditional wired USB connection.
-   **Ultra-Low Latency**: The main loop is optimized to be free of blocking `delay()` calls, ensuring the fastest possible response time for competitive gameplay.
-   **Configurable Latency**: A single, well-documented variable (`MAIN_LOOP_DELAY_MS`) allows you to introduce a minimal, non-blocking delay if needed, giving you full control over the performance/stability balance.
-   **Power Efficiency**: When switched to wired mode, the ESP32 enters a `Light Sleep` low-power state to conserve battery, waking up instantly when the mode is changed back to wireless.
-   **Reliable Inputs**: Implements the `Bounce2` library for robust button and joystick debouncing, preventing accidental double-presses or ghost inputs.
-   **Visual Feedback**: A status LED provides clear visual cues for connection status (connected, waiting, or in wired mode).

## Hardware Requirements

To build this project, you will need:
-   An **ESP32 Development Board** (e.g., ESP32-WROOM-32 DevKitC). The code is generic and should work on any standard ESP32 board.
-   An existing **USB Joystick Board** to source the inputs from.
-   **Arcade Stick Components**: 8 action buttons, Start/Select buttons, and a digital joystick.
-   A **DPDT (Double Pole, Double Throw) Switch**: This is crucial for switching the USB data lines (`D+`/`D-`) between the USB Joystick Board (for wired mode) and disconnecting them (for wireless mode).
-   **Power System** (for wireless mode):
    -   A LiPo Battery.
    -   A **TP4056 Charging Module** to safely charge and manage the battery.
-   A **Status LED** (optional, but highly recommended). The code is pre-configured to use the built-in LED on pin 2.
-   Jumper wires, soldering equipment, and a suitable enclosure for the arcade stick.

## Software & Library Requirements

This project is built using **PlatformIO** within **Visual Studio Code**.

The following libraries are required and will be installed automatically by PlatformIO thanks to the `platformio.ini` file:

-   [**ESP32-BLE-Gamepad**](https://github.com/lemmingDev/ESP32-BLE-Gamepad) by lemmingDev: The core library for emulating a Bluetooth Low Energy gamepad.
-   [**Bounce2**](https://github.com/thomasfredericks/Bounce2) by thomasfredericks: A fantastic library for debouncing button inputs.

## Setup and Installation

1.  **Clone the Repository**:
    ```bash
    git clone https://github.com/MatMont01/ESP32-Hybrid-Arcade-Stick
    ```
2.  **Open in VS Code**: Open the cloned folder in Visual Studio Code with the PlatformIO extension installed.
3.  **Install Dependencies**: PlatformIO should automatically detect the `platformio.ini` file and prompt you to install the required libraries. If not, the build process will install them.
4.  **Configure Pins**: Open the `src/main.cpp` file and adjust the pin constants at the top of the file to match your physical wiring.
5.  **Upload**: Connect your ESP32 board via USB and click the "Upload" button (the right-arrow icon) in the PlatformIO status bar at the bottom of VS Code.

## Pin Configuration

You must define your pinout in `src/main.cpp`. The default configuration is:

| Function              | Pin Constant              | Default ESP32 Pin |
| --------------------- | ------------------------- | ----------------- |
| **Action Buttons**    | `ACTION_BUTTON_PIN_1-8`   | 13, 12, 14, 27, 26, 25, 33, 32 |
| **Start Button**      | `START_BUTTON_PIN`        | 15                |
| **Select Button**     | `SELECT_BUTTON_PIN`       | 4                 |
| **Joystick Up**       | `JOYSTICK_UP_PIN`         | 19                |
| **Joystick Down**     | `JOYSTICK_DOWN_PIN`       | 18                |
| **Joystick Left**     | `JOYSTICK_LEFT_PIN`       | 5                 |
| **Joystick Right**    | `JOYSTICK_RIGHT_PIN`      | 17                |
| **Mode Switch**       | `MODE_SWITCH_PIN`         | 23                |
| **Status LED**        | `STATUS_LED_PIN`          | 2 (Built-in LED)  |

## How to Use

The arcade stick operates in two distinct modes, controlled by the physical DPDT switch.

#### Wireless Mode
-   The ESP32 will broadcast as a Bluetooth device named **"ArcadeStickESP32"**.
-   The status LED will **blink slowly** while waiting for a connection.
-   Once connected to a PC or console, the LED will turn **solid ON**.
-   All inputs are read by the ESP32 and sent wirelessly.

#### Wired Mode
-   The DPDT switch routes the USB data lines directly from your original USB Joystick Board to the computer.
-   The ESP32 detects this state, terminates any Bluetooth connection, and enters a **low-power sleep mode**.
-   The status LED will be **OFF**.
-   To switch back to wireless mode, toggle the DPDT switch. The ESP32 will wake up instantly and start broadcasting again.

## Advanced Configuration

You can fine-tune the performance by modifying these constants in `src/main.cpp`:

-   `DEBOUNCE_INTERVAL_MS`: The time in milliseconds to ignore rapid signal changes on a button. The default of `5` is ideal for most arcade buttons.
-   `MAIN_LOOP_DELAY_MS`: A small delay added to each loop iteration in wireless mode.
    -   `0`: Maximum responsiveness, zero artificial delay. Best for competitive play.
    -   `1` (Default): Adds a 1ms delay, which is imperceptible but gives the ESP32's background tasks (like the Bluetooth stack) more processing time, potentially increasing stability.

## Credits and Acknowledgements

This project would not be possible without the incredible work of the open-source community. Special thanks to:

-   **lemmingDev** for creating and maintaining the [ESP32-BLE-Gamepad](https://github.com/lemmingDev/ESP32-BLE-Gamepad) library.
-   **thomasfredericks** for the simple and effective [Bounce2](https://github.com/thomasfredericks/Bounce2) library.
