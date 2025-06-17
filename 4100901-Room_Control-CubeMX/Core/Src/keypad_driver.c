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
 * @brief Escanea el teclado para identificar la tecla presionada.
 *        Esta versión está optimizada para velocidad y eficiencia.
 * @param keypad: Puntero al manejador del teclado.
 * @param col_pin: Pin de la columna que generó la interrupción.
 * @return La tecla presionada o '\0' si no se detecta ninguna (ej. rebote).
 */
char keypad_scan(keypad_handle_t* keypad, uint16_t col_pin) {
    char key_pressed = '\0';
    int col_index = -1;

    // 1. Identificar el índice de la columna que causó la interrupción.
    // Este paso es rápido y necesario.
    for (int i = 0; i < KEYPAD_COLS; i++) {
        if (col_pin == keypad->col_pins[i]) {
            col_index = i;
            break;
        }
    }

    if (col_index == -1) {
        return '\0'; // No es un pin de columna de nuestro keypad.
    }

    // --- Inicio del escaneo optimizado ---

    // 2. Poner TODAS las filas en ALTO. Esto "desactiva" todas las conexiones.
    // Se hace una sola vez al principio del escaneo.
    for (int i = 0; i < KEYPAD_ROWS; i++) {
        HAL_GPIO_WritePin(keypad->row_ports[i], keypad->row_pins[i], GPIO_PIN_SET);
    }

    // 3. Escanear cada fila individualmente.
    for (int row = 0; row < KEYPAD_ROWS; row++) {
        // Poner solo la fila actual en BAJO para probarla.
        HAL_GPIO_WritePin(keypad->row_ports[row], keypad->row_pins[row], GPIO_PIN_RESET);

        // Se lee el estado de la columna. Si está en BAJO, hemos encontrado la fila
        // que completa el circuito, ya que es la única que está conduciendo.
        // Nota: No se usa HAL_Delay(). Un pequeño retardo es inherente a la ejecución
        // de las instrucciones, lo que suele ser suficiente para la estabilización.
        if (HAL_GPIO_ReadPin(keypad->col_ports[col_index], keypad->col_pins[col_index]) == GPIO_PIN_RESET) {
            key_pressed = keypad_map[row][col_index];
            // Una vez encontrada la tecla, no necesitamos seguir buscando.
            // Salimos del bucle para restaurar el estado y devolver el resultado.
            break; 
        }

        // Restaurar la fila actual a ALTO antes de probar la siguiente.
        // Esto aísla la prueba para cada fila.
        HAL_GPIO_WritePin(keypad->row_ports[row], keypad->row_pins[row], GPIO_PIN_SET);
    }

    // 4. Restaurar el estado inicial: todas las filas en BAJO.
    // Esto es crucial para que el sistema de interrupciones de las columnas
    // pueda detectar la próxima pulsación.
    for (int i = 0; i < KEYPAD_ROWS; i++) {
        HAL_GPIO_WritePin(keypad->row_ports[i], keypad->row_pins[i], GPIO_PIN_RESET);
    }

    // NOTA SOBRE ANTI-REBOTE (DEBOUNCE):
    // La tecla encontrada debe ser procesada en el bucle principal (main loop),
    // no aquí. El bucle principal debe implementar una lógica de anti-rebote
    // usando HAL_GetTick() para ignorar pulsaciones rápidas y falsas.
    // Esta función solo debe identificar la tecla, no validarla.

    return key_pressed;
}