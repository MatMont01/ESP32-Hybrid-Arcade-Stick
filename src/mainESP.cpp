/*
================================================================================
= Proyecto: Palanca Arcade Híbrida de Ultra Baja Latencia (USB/Bluetooth)      =
= Autor: MatMont01                                                             =
= Fecha: 2025-08-07                                                            =
= Versión: 1.0                                                                 =
=                                                                              =
= Descripción:                                                                 =
= Este firmware avanzado convierte un ESP32 en el cerebro de una palanca       =
= arcade de alto rendimiento.                                                  =
=                                                                              =
= Características Clave:                                                       =
= - Modo Dual: Funciona por Bluetooth (inalámbrico) o USB (cableado).          =
= - Latencia Configurable: El bucle principal permite un delay opcional para   =
=   ceder tiempo al procesador, ajustable por el usuario.                      =
= - Eficiencia Energética: Implementa un modo de bajo consumo (Light Sleep)    =
=   cuando está en modo cableado.                                              =
= - Anti-Rebote (Debounce): Utiliza la librería Bounce2 para una lectura de    =
=   botones precisa y fiable.                                                  =
================================================================================
*/

// --- 1. Inclusión de Librerías ---
#include <Arduino.h>
#include <BleGamepad.h>
#include <Bounce2.h>
#include "esp_sleep.h" // Necesario para el modo de bajo consumo

// --- 2. Definiciones y Constantes ---

// --- PINES DE ENTRADA ---
// Botones de acción (8 botones)
const int BOTON_PIN_1 = 13;
const int BOTON_PIN_2 = 12;
const int BOTON_PIN_3 = 14;
const int BOTON_PIN_4 = 27;
const int BOTON_PIN_5 = 26;
const int BOTON_PIN_6 = 25;
const int BOTON_PIN_7 = 33;
const int BOTON_PIN_8 = 32;

// Botones de Start y Select
const int BOTON_START_PIN = 15;
const int BOTON_SELECT_PIN = 4;

// Pines para la palanca (joystick)
const int PALANCA_ARRIBA_PIN = 19;
const int PALANCA_ABAJO_PIN = 18;
const int PALANCA_IZQUIERDA_PIN = 5;
const int PALANCA_DERECHA_PIN = 17;

// Pin para el botón de cambio de modo (cableado/inalámbrico)
const int MODO_PIN = 23;

// Pin para un LED de estado (opcional, pero recomendado)
const int LED_ESTADO_PIN = 2; // El LED integrado en muchas placas ESP32

// --- CONFIGURACIÓN DE RESPUESTA Y DELAY ---
// Intervalo de debounce en milisegundos. 5ms es un buen balance.
const int INTERVALO_DEBOUNCE_MS = 5;

// Delay del bucle principal en milisegundos.
// Aumentar este valor reduce la carga en el CPU pero AUMENTA LA LATENCIA.
// Para juegos de pelea, se recomienda un valor muy bajo (0 o 1).
// 0 = sin delay, máxima velocidad de respuesta.
// 1 = un delay mínimo para ceder algo de tiempo a tareas en segundo plano.
const int DELAY_PRINCIPAL_MS = 1;

// --- 3. Variables Globales y Objetos ---

BleGamepad bleGamepad("PalancaArcadeESP32", "MatMont01", 100);

const int NUM_BOTONES_TOTAL = 10;
Bounce debouncers[NUM_BOTONES_TOTAL];
const int pinesBotones[NUM_BOTONES_TOTAL] = {
    BOTON_PIN_1, BOTON_PIN_2, BOTON_PIN_3, BOTON_PIN_4,
    BOTON_PIN_5, BOTON_PIN_6, BOTON_PIN_7, BOTON_PIN_8,
    BOTON_START_PIN, BOTON_SELECT_PIN
};
const int mapeoBotonesGamepad[NUM_BOTONES_TOTAL] = {
    BUTTON_1, BUTTON_2, BUTTON_3, BUTTON_4,
    BUTTON_5, BUTTON_6, BUTTON_7, BUTTON_8,
    BUTTON_9, BUTTON_10
};

Bounce debouncerPalanca[4];
const int pinesPalanca[4] = {
    PALANCA_ARRIBA_PIN, PALANCA_ABAJO_PIN, PALANCA_IZQUIERDA_PIN, PALANCA_DERECHA_PIN
};

Bounce debouncerModo = Bounce();
volatile bool modoInalambrico = true;

// --- 4. Prototipos de Funciones ---
void inicializarPines();
void gestionarEntradas();
void leerYActualizarBotones();
void leerYActualizarPalanca();
void gestionarCambioDeModo();
void activarModoInalambrico();
void desactivarModoCableado();
void entrarEnModoBajoConsumo();
void gestionarLEDs();

// --- 5. Función de Configuración (setup) ---
void setup() {
    Serial.begin(115200);
    Serial.println("\n\n===============================================");
    Serial.println("=   Palanca Arcade Híbrida - Firmware v1.0    =");
    Serial.println("===============================================");

    inicializarPines();

    pinMode(MODO_PIN, INPUT_PULLUP);
    modoInalambrico = digitalRead(MODO_PIN);

    if (modoInalambrico) {
        activarModoInalambrico();
    } else {
        desactivarModoCableado();
    }
}

// --- 6. Bucle Principal (loop) ---
void loop() {
    if (modoInalambrico) {
        gestionarCambioDeModo();
        if (bleGamepad.isConnected()) {
            gestionarEntradas();
        }
        gestionarLEDs();
        
        // Añadimos un delay configurable al final del bucle del modo inalámbrico.
        // Esto cede tiempo de procesamiento a otras tareas del ESP32 (como el Bluetooth).
        // No es estrictamente necesario para la latencia, pero puede mejorar la estabilidad.
        if (DELAY_PRINCIPAL_MS > 0) {
            delay(DELAY_PRINCIPAL_MS);
        }

    } else {
        entrarEnModoBajoConsumo();
    }
}

// --- 7. Implementación de Funciones ---

/**
 * @brief Inicializa todos los pines de entrada, los debouncers y el LED.
 */
void inicializarPines() {
    for (int i = 0; i < NUM_BOTONES_TOTAL; i++) {
        debouncers[i].attach(pinesBotones[i], INPUT_PULLUP);
        debouncers[i].interval(INTERVALO_DEBOUNCE_MS);
    }

    for (int i = 0; i < 4; i++) {
        debouncerPalanca[i].attach(pinesPalanca[i], INPUT_PULLUP);
        debouncerPalanca[i].interval(INTERVALO_DEBOUNCE_MS);
    }

    debouncerModo.attach(MODO_PIN, INPUT_PULLUP);
    debouncerModo.interval(25);

    pinMode(LED_ESTADO_PIN, OUTPUT);
}

/**
 * @brief Función principal para procesar todas las entradas del jugador.
 */
void gestionarEntradas() {
    leerYActualizarBotones();
    leerYActualizarPalanca();
}

/**
 * @brief Lee el estado de todos los botones y actualiza el estado del gamepad.
 */
void leerYActualizarBotones() {
    for (int i = 0; i < NUM_BOTONES_TOTAL; i++) {
        debouncers[i].update();
        if (debouncers[i].fell()) {
            bleGamepad.press(mapeoBotonesGamepad[i]);
        } else if (debouncers[i].rose()) {
            bleGamepad.release(mapeoBotonesGamepad[i]);
        }
    }
}

/**
 * @brief Lee el estado de la palanca y lo mapea al Hat Switch (DPAD).
 */
void leerYActualizarPalanca() {
    for (int i = 0; i < 4; i++) {
        debouncerPalanca[i].update();
    }

    bool arriba = !debouncerPalanca[0].read();
    bool abajo = !debouncerPalanca[1].read();
    bool izquierda = !debouncerPalanca[2].read();
    bool derecha = !debouncerPalanca[3].read();

    if (arriba) {
        if (izquierda) bleGamepad.setHat(HAT_UP_LEFT);
        else if (derecha) bleGamepad.setHat(HAT_UP_RIGHT);
        else bleGamepad.setHat(HAT_UP);
    } else if (abajo) {
        if (izquierda) bleGamepad.setHat(HAT_DOWN_LEFT);
        else if (derecha) bleGamepad.setHat(HAT_DOWN_RIGHT);
        else bleGamepad.setHat(HAT_DOWN);
    } else if (izquierda) {
        bleGamepad.setHat(HAT_LEFT);
    } else if (derecha) {
        bleGamepad.setHat(HAT_RIGHT);
    } else {
        bleGamepad.setHat(HAT_CENTERED);
    }
}

/**
 * @brief Revisa si el botón de modo ha sido presionado para cambiar a modo cableado.
 */
void gestionarCambioDeModo() {
    debouncerModo.update();
    if (debouncerModo.fell()) {
        modoInalambrico = false;
        desactivarModoCableado();
    }
}

/**
 * @brief Configura el sistema para operar en modo inalámbrico (Bluetooth).
 */
void activarModoInalambrico() {
    Serial.println("Modo actual: INALÁMBRICO.");
    Serial.println("Iniciando servicios Bluetooth. Buscando conexión...");

    BleGamepadConfiguration bleGamepadConfig;
    bleGamepadConfig.setButtonCount(10);
    bleGamepadConfig.setHatSwitchCount(1);
    bleGamepadConfig.setWhichAxes(false, false, false, false, false, false, false, false);
    bleGamepad.begin(&bleGamepadConfig);
}

/**
 * @brief Prepara el sistema para cambiar a modo cableado y entrar en bajo consumo.
 */
void desactivarModoCableado() {
    Serial.println("Modo actual: CABLEADO.");
    Serial.println("Deteniendo servicios Bluetooth.");
    if (bleGamepad.isConnected()) {
        bleGamepad.end();
    }
}

/**
 * @brief Pone el ESP32 en modo de bajo consumo (Light Sleep).
 */
void entrarEnModoBajoConsumo() {
    Serial.println("Entrando en modo de bajo consumo. Presione el botón de modo para despertar.");
    digitalWrite(LED_ESTADO_PIN, LOW);
    Serial.flush();

    esp_sleep_enable_ext0_wakeup(GPIO_NUM_23, 0);
    esp_light_sleep_start();

    Serial.println("¡Despertando del modo de bajo consumo!");
    modoInalambrico = true;
    activarModoInalambrico();
}

/**
 * @brief Gestiona un LED de estado para dar feedback visual.
 */
void gestionarLEDs() {
    static uint32_t tiempoAnterior = 0;
    const int intervaloParpadeo = 500;

    if (bleGamepad.isConnected()) {
        digitalWrite(LED_ESTADO_PIN, HIGH);
    } else {
        if (millis() - tiempoAnterior > intervaloParpadeo) {
            tiempoAnterior = millis();
            digitalWrite(LED_ESTADO_PIN, !digitalRead(LED_ESTADO_PIN));
        }
    }
}