/*
================================================================================
= Project: Ultra-Low Latency Hybrid Arcade Stick (USB/Bluetooth)               =
= Author: MatMont01                                                            =
= Date: 2025-08-07                                                             =
= Version: 1.0                                                                 =
=                                                                              =
= Description:                                                                 =
= This advanced firmware turns an ESP32 into the brain of a high-performance   =
= arcade stick.                                                                =
=                                                                              =
= Key Features:                                                                =
= - Dual Mode: Operates via Bluetooth (wireless) or USB (wired).               =
= - Configurable Latency: The main loop includes an optional, user-adjustable  =
=   delay to yield processor time.                                             =
= - Power Efficiency: Implements a low-power mode (Light Sleep) when in        =
=   wired mode, waking up instantly when the mode switch is toggled.           =
= - Debouncing: Uses the Bounce2 library for precise and reliable button       =
=   reading.                                                                   =
================================================================================
*/

// --- 1. Library Includes ---
#include <Arduino.h>
#include <BleGamepad.h>
#include <Bounce2.h>
#include "esp_sleep.h" // Required for low-power sleep mode

// --- 2. Definitions and Constants ---

// --- INPUT PINS ---
// Action Buttons (8 buttons)
const int ACTION_BUTTON_PIN_1 = 13;
const int ACTION_BUTTON_PIN_2 = 12;
const int ACTION_BUTTON_PIN_3 = 14;
const int ACTION_BUTTON_PIN_4 = 27;
const int ACTION_BUTTON_PIN_5 = 26;
const int ACTION_BUTTON_PIN_6 = 25;
const int ACTION_BUTTON_PIN_7 = 33;
const int ACTION_BUTTON_PIN_8 = 32;

// Start and Select Buttons
const int START_BUTTON_PIN = 15;
const int SELECT_BUTTON_PIN = 4;

// Pins for the joystick
const int JOYSTICK_UP_PIN = 19;
const int JOYSTICK_DOWN_PIN = 18;
const int JOYSTICK_LEFT_PIN = 5;
const int JOYSTICK_RIGHT_PIN = 17;

// Pin for the mode switch button (wired/wireless)
const int MODE_SWITCH_PIN = 23;

// Pin for a status LED (optional, but recommended)
const int STATUS_LED_PIN = 2; // The built-in LED on many ESP32 boards

// --- RESPONSE AND DELAY CONFIGURATION ---
// Debounce interval in milliseconds. 5ms is a good balance.
const int DEBOUNCE_INTERVAL_MS = 5;

// Main loop delay in milliseconds.
// Increasing this value reduces CPU load but INCREASES LATENCY.
// For fighting games, a very low value (0 or 1) is recommended.
// 0 = no delay, maximum responsiveness.
// 1 = a minimal delay to yield some time to background tasks.
const int MAIN_LOOP_DELAY_MS = 1;

// --- 3. Global Variables and Objects ---

// Bluetooth Gamepad Object
// The name that will appear when scanning for Bluetooth devices.
// The 100 represents the initial battery level.
BleGamepad bleGamepad("ArcadeStickESP32", "MatMont01", 100);

// --- Button Debouncing ---
const int TOTAL_BUTTONS = 10; // 8 action + Start + Select
Bounce buttonDebouncers[TOTAL_BUTTONS];
const int buttonPins[TOTAL_BUTTONS] = {
    ACTION_BUTTON_PIN_1, ACTION_BUTTON_PIN_2, ACTION_BUTTON_PIN_3, ACTION_BUTTON_PIN_4,
    ACTION_BUTTON_PIN_5, ACTION_BUTTON_PIN_6, ACTION_BUTTON_PIN_7, ACTION_BUTTON_PIN_8,
    START_BUTTON_PIN, SELECT_BUTTON_PIN
};
// Map our physical pins to the buttons the OS will understand.
const int gamepadButtonMap[TOTAL_BUTTONS] = {
    BUTTON_1, BUTTON_2, BUTTON_3, BUTTON_4,
    BUTTON_5, BUTTON_6, BUTTON_7, BUTTON_8,
    BUTTON_9,  // Mapped to Start
    BUTTON_10 // Mapped to Select
};

// --- Joystick Debouncing ---
Bounce joystickDebouncers[4];
const int joystickPins[4] = {
    JOYSTICK_UP_PIN, JOYSTICK_DOWN_PIN, JOYSTICK_LEFT_PIN, JOYSTICK_RIGHT_PIN
};

// --- Operation Mode Handling ---
Bounce modeDebouncer = Bounce();
// We use 'volatile' because this variable can be changed by an interrupt
// when waking up from sleep mode.
volatile bool isWirelessMode = true;

// --- 4. Function Prototypes ---
void initializePins();
void manageInputs();
void readAndProcessButtons();
void readAndProcessJoystick();
void manageModeSwitch();
void activateWirelessMode();
void deactivateForWiredMode();
void enterLightSleepMode();
void manageStatusLED();

// --- 5. Setup Function ---
void setup() {
    Serial.begin(115200);
    Serial.println("\n\n===============================================");
    Serial.println("=   Hybrid Arcade Stick - Firmware v1.0       =");
    Serial.println("===============================================");

    initializePins();

    // Determine the initial mode based on the pin reading.
    // This is useful if the mode switch is already in a position at power-on.
    pinMode(MODE_SWITCH_PIN, INPUT_PULLUP);
    isWirelessMode = digitalRead(MODE_SWITCH_PIN); // Assumes HIGH = Wireless, LOW = Wired

    if (isWirelessMode) {
        activateWirelessMode();
    } else {
        deactivateForWiredMode();
    }
}

// --- 6. Main Loop ---
void loop() {
    // The main loop is now much simpler and faster.
    // It delegates logic to specific functions based on the current mode.
    if (isWirelessMode) {
        manageModeSwitch(); // Check if we need to switch to wired mode
        if (bleGamepad.isConnected()) {
            manageInputs(); // Process buttons and joystick
        }
        manageStatusLED(); // Update the status LED
        
        // Add a configurable delay at the end of the wireless mode loop.
        // This yields processing time to other ESP32 tasks (like the Bluetooth stack).
        // It's not strictly necessary for latency but can improve stability.
        if (MAIN_LOOP_DELAY_MS > 0) {
            delay(MAIN_LOOP_DELAY_MS);
        }

    } else {
        // If in wired mode, do nothing but enter low-power mode.
        // The ESP32 will "sleep" in the next function until the mode button wakes it up.
        enterLightSleepMode();
    }
}

// --- 7. Function Implementations ---

/**
 * @brief Initializes all input pins, debouncers, and the LED.
 */
void initializePins() {
    for (int i = 0; i < TOTAL_BUTTONS; i++) {
        buttonDebouncers[i].attach(buttonPins[i], INPUT_PULLUP);
        buttonDebouncers[i].interval(DEBOUNCE_INTERVAL_MS);
    }

    for (int i = 0; i < 4; i++) {
        joystickDebouncers[i].attach(joystickPins[i], INPUT_PULLUP);
        joystickDebouncers[i].interval(DEBOUNCE_INTERVAL_MS);
    }

    modeDebouncer.attach(MODE_SWITCH_PIN, INPUT_PULLUP);
    modeDebouncer.interval(25);

    pinMode(STATUS_LED_PIN, OUTPUT);
}

/**
 * @brief Main function to process all player inputs.
 * Called continuously only in wireless and connected mode.
 */
void manageInputs() {
    readAndProcessButtons();
    readAndProcessJoystick();
}

/**
 * @brief Reads the state of all buttons and updates the gamepad state.
 * The report sending is handled automatically by the BleGamepad library.
 */
void readAndProcessButtons() {
    for (int i = 0; i < TOTAL_BUTTONS; i++) {
        buttonDebouncers[i].update();
        if (buttonDebouncers[i].fell()) { // If the button was pressed
            bleGamepad.press(gamepadButtonMap[i]);
        } else if (buttonDebouncers[i].rose()) { // If the button was released
            bleGamepad.release(gamepadButtonMap[i]);
        }
    }
}

/**
 * @brief Reads the joystick state and maps it to the Hat Switch (DPAD).
 */
void readAndProcessJoystick() {
    for (int i = 0; i < 4; i++) {
        joystickDebouncers[i].update();
    }

    bool up = !joystickDebouncers[0].read();
    bool down = !joystickDebouncers[1].read();
    bool left = !joystickDebouncers[2].read();
    bool right = !joystickDebouncers[3].read();

    if (up) {
        if (left) bleGamepad.setHat(HAT_UP_LEFT);
        else if (right) bleGamepad.setHat(HAT_UP_RIGHT);
        else bleGamepad.setHat(HAT_UP);
    } else if (down) {
        if (left) bleGamepad.setHat(HAT_DOWN_LEFT);
        else if (right) bleGamepad.setHat(HAT_DOWN_RIGHT);
        else bleGamepad.setHat(HAT_DOWN);
    } else if (left) {
        bleGamepad.setHat(HAT_LEFT);
    } else if (right) {
        bleGamepad.setHat(HAT_RIGHT);
    } else {
        bleGamepad.setHat(HAT_CENTERED);
    }
}

/**
 * @brief Checks if the mode button has been pressed to switch to wired mode.
 */
void manageModeSwitch() {
    modeDebouncer.update();
    if (modeDebouncer.fell()) {
        isWirelessMode = false;
        deactivateForWiredMode();
    }
}

/**
 * @brief Configures the system to operate in wireless (Bluetooth) mode.
 */
void activateWirelessMode() {
    Serial.println("Current mode: WIRELESS.");
    Serial.println("Starting Bluetooth services. Waiting for connection...");

    BleGamepadConfiguration bleGamepadConfig;
    bleGamepadConfig.setButtonCount(10);
    bleGamepadConfig.setHatSwitchCount(1);
    bleGamepadConfig.setWhichAxes(false, false, false, false, false, false, false, false);
    bleGamepad.begin(&bleGamepadConfig);
}

/**
 * @brief Prepares the system to switch to wired mode and enter low-power sleep.
 */
void deactivateForWiredMode() {
    Serial.println("Current mode: WIRED.");
    Serial.println("Stopping Bluetooth services.");
    if (bleGamepad.isConnected()) {
        bleGamepad.end();
    }
}

/**
 * @brief Puts the ESP32 into Light Sleep mode.
 * Battery consumption will be minimal. It will wake up when the mode switch
 * button is pressed.
 */
void enterLightSleepMode() {
    Serial.println("Entering low-power mode. Press the mode button to wake up.");
    digitalWrite(STATUS_LED_PIN, LOW); // Turn off LED to save power
    Serial.flush(); // Ensure all serial messages are sent before sleeping

    // Configure the mode pin as the wakeup interrupt source
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_23, 0); // Wake up when MODE_SWITCH_PIN (23) is LOW (pressed)

    // Enter Light Sleep mode
    esp_light_sleep_start();

    // --- Code execution resumes here after waking up ---
    Serial.println("Waking up from low-power mode!");
    isWirelessMode = true; // When waking up, switch to wireless mode
    activateWirelessMode();
}

/**
 * @brief Manages a status LED to provide visual feedback.
 * - Slow blink: Waiting for Bluetooth connection.
 * - Solid ON: Connected via Bluetooth.
 * - Solid OFF: Wired mode / Low-power sleep.
 */
void manageStatusLED() {
    static uint32_t previousTime = 0;
    const int blinkInterval = 500; // ms

    if (bleGamepad.isConnected()) {
        digitalWrite(STATUS_LED_PIN, HIGH); // Solid LED when connected
    } else {
        // Slow blink if not connected
        if (millis() - previousTime > blinkInterval) {
            previousTime = millis();
            digitalWrite(STATUS_LED_PIN, !digitalRead(STATUS_LED_PIN));
        }
    }
}