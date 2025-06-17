#include "keypad_driver.h"
#include "main.h" // Necesario para HAL_Delay, HAL_GetTick y GPIO

static const char keypad_map[KEYPAD_ROWS][KEYPAD_COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

/**
 * @brief Inicializa el controlador del teclado, asegurando que las filas estén en BAJO.
 * @param keypad: Puntero al manejador del teclado.
 */
void keypad_init(keypad_handle_t* keypad) {
    // MODIFICADO: Asegura que todas las filas se configuren como salidas
    // y se pongan en estado BAJO para permitir la detección de interrupciones
    // por flanco de bajada en las columnas.
    for (int i = 0; i < KEYPAD_ROWS; i++) {
        HAL_GPIO_WritePin(keypad->row_ports[i], keypad->row_pins[i], GPIO_PIN_RESET);
    }
}

/**
 * @brief Escanea el teclado para identificar la tecla presionada después de una interrupción.
 * @param keypad: Puntero al manejador del teclado.
 * @param col_pin: Pin de la columna que generó la interrupción.
 * @return La tecla presionada o '\0' si no se detecta ninguna.
 */
char keypad_scan(keypad_handle_t* keypad, uint16_t col_pin) {
    char key_pressed = '\0';
    int col_index = -1;

    // MODIFICADO: Se elimina el anti-rebote de la ISR.

    // Identificar el índice de la columna que causó la interrupción
    for (int i = 0; i < KEYPAD_COLS; i++) {
        if (col_pin == keypad->col_pins[i]) {
            col_index = i;
            break;
        }
    }

    if (col_index == -1) {
        return '\0'; // No es un pin de columna de nuestro keypad
    }
    
    // Breve retraso para estabilizar las señales eléctricas antes de escanear.
    // Usamos un delay corto aquí porque estamos en una ISR y queremos ser rápidos.
    HAL_Delay(2);

    // MODIFICADO: Lógica de escaneo simplificada y robusta.
    // Escanea las filas para encontrar cuál está causando la conexión.
    for (int row = 0; row < KEYPAD_ROWS; row++) {
        // Poner todas las filas en ALTO
        for (int i = 0; i < KEYPAD_ROWS; i++) {
            HAL_GPIO_WritePin(keypad->row_ports[i], keypad->row_pins[i], GPIO_PIN_SET);
        }

        // Poner la fila actual en BAJO
        HAL_GPIO_WritePin(keypad->row_ports[row], keypad->row_pins[row], GPIO_PIN_RESET);

        // Si la columna que generó la interrupción sigue en BAJO, encontramos la tecla
        if (HAL_GPIO_ReadPin(keypad->col_ports[col_index], keypad->col_pins[col_index]) == GPIO_PIN_RESET) {
            key_pressed = keypad_map[row][col_index];
            break; // Salir del bucle una vez encontrada la tecla
        }
    }

    // Restaurar el estado: todas las filas en BAJO para la próxima interrupción.
    for (int i = 0; i < KEYPAD_ROWS; i++) {
        HAL_GPIO_WritePin(keypad->row_ports[i], keypad->row_pins[i], GPIO_PIN_RESET);
    }
    
    return key_pressed;
}