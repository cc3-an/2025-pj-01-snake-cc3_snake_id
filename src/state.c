#include "state.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "snake_utils.h"

// Definiciones de funciones de ayuda.
static void set_board_at(game_state_t* state, unsigned int row, unsigned int col, char ch);
static bool is_tail(char c);
static bool is_head(char c);
static bool is_snake(char c);
static char body_to_tail(char c);
static char head_to_body(char c);
static unsigned int get_next_row(unsigned int cur_row, char c);
static unsigned int get_next_col(unsigned int cur_col, char c);
static void find_head(game_state_t* state, unsigned int snum);
static char next_square(game_state_t* state, unsigned int snum);
static void update_tail(game_state_t* state, unsigned int snum);
static void update_head(game_state_t* state, unsigned int snum);

/* Tarea 1 */
game_state_t* create_default_state() {
  // Crear el estado del juego en memoria dinámica
    game_state_t* state = malloc(sizeof(game_state_t));
    if (!state) {
        perror("Error al asignar memoria para game_state_t");
        exit(EXIT_FAILURE);
    }

    // Inicializar dimensiones del tablero
    state->num_rows = 18;
    state->board = malloc(state->num_rows * sizeof(char*));
    if (!state->board) {
        perror("Error al asignar memoria para el tablero");
        free(state);
        exit(EXIT_FAILURE);
    }

    // Crear cada fila del tablero
    for (unsigned int i = 0; i < state->num_rows; i++) {
        state->board[i] = malloc(21 * sizeof(char)); // 20 columnas + 1 para '\0'
        if (!state->board[i]) {
            perror("Error al asignar memoria para una fila del tablero");
            for (unsigned int j = 0; j < i; j++) {
                free(state->board[j]);
            }
            free(state->board);
            free(state);
            exit(EXIT_FAILURE);
        }
        // Inicializar filas con espacios o bordes
        if (i == 0 || i == state->num_rows - 1) {
            strcpy(state->board[i], "####################");
        } else {
            strcpy(state->board[i], "#                  #");
        }
    }

    // Colocar la serpiente y la fruta
    strcpy(state->board[2], "# d>D    *         #");

    // Inicializar número de serpientes
    state->num_snakes = 1;
    state->snakes = malloc(state->num_snakes * sizeof(snake_t));
    if (!state->snakes) {
        perror("Error al asignar memoria para las serpientes");
        for (unsigned int i = 0; i < state->num_rows; i++) {
            free(state->board[i]);
        }
        free(state->board);
        free(state);
        exit(EXIT_FAILURE);
    }

    // Inicializar la serpiente
    state->snakes[0].tail_row = 2;
    state->snakes[0].tail_col = 2;
    state->snakes[0].head_row = 2;
    state->snakes[0].head_col = 4;
    state->snakes[0].live = true;

    return state;
}


/* Tarea 2 */
void free_state(game_state_t* state) {
  if (!state) {
        return; // Si el estado es NULL, no hay nada que liberar
    }

    // Liberar cada fila del tablero
    for (unsigned int i = 0; i < state->num_rows; i++) {
        free(state->board[i]);
    }

    // Liberar el arreglo de punteros del tablero
    free(state->board);

    // Liberar el arreglo de serpientes
    free(state->snakes);

    // Liberar la estructura del estado
    free(state);
}


/* Tarea 3 */
void print_board(game_state_t* state, FILE* fp) {
  if (!state || !fp) {
        return; // Si el estado o el archivo son NULL, no hay nada que imprimir
    }

    // Iterar sobre cada fila del tablero
    for (unsigned int i = 0; i < state->num_rows; i++) {
        fprintf(fp, "%s\n", state->board[i]); // Imprimir la fila en el archivo
    }
}


/**
 * Guarda el estado actual a un archivo. No modifica el objeto/struct state.
 * (ya implementada para que la utilicen)
*/
void save_board(game_state_t* state, char* filename) {
  FILE* f = fopen(filename, "w");
  print_board(state, f);
  fclose(f);
}

/* Tarea 4.1 */


/**
 * Funcion de ayuda que obtiene un caracter del tablero dado una fila y columna
 * (ya implementado para ustedes).
*/
char get_board_at(game_state_t* state, unsigned int row, unsigned int col) {
  return state->board[row][col];
}


/**
 * Funcion de ayuda que actualiza un caracter del tablero dado una fila, columna y
 * un caracter.
 * (ya implementado para ustedes).
*/
static void set_board_at(game_state_t* state, unsigned int row, unsigned int col, char ch) {
  state->board[row][col] = ch;
}


/**
 * Retorna true si la variable c es parte de la cola de una snake.
 * La cola de una snake consiste de los caracteres: "wasd"
 * Retorna false de lo contrario.
*/
static bool is_tail(char c) {
  return c == 'w' || c == 'a' || c == 's' || c == 'd';
}


/**
 * Retorna true si la variable c es parte de la cabeza de una snake.
 * La cabeza de una snake consiste de los caracteres: "WASDx"
 * Retorna false de lo contrario.
*/
static bool is_head(char c) {
  return c == 'W' || c == 'A' || c == 'S' || c == 'D' || c == 'x';
}


/**
 * Retorna true si la variable c es parte de una snake.
 * Una snake consiste de los siguientes caracteres: "wasd^<v>WASDx"
*/
static bool is_snake(char c) {
  return is_tail(c) || is_head(c) || c == '^' || c == '<' || c == 'v' || c == '>';
}


/**
 * Convierte un caracter del cuerpo de una snake ("^<v>")
 * al caracter que correspondiente de la cola de una
 * snake ("wasd").
*/
static char body_to_tail(char c) {
  switch (c) {
    case '^': return 'w';
    case '<': return 'a';
    case 'v': return 's';
    case '>': return 'd';
    default: return '?'; // Indefinido para caracteres no válidos
  }
}


/**
 * Convierte un caracter de la cabeza de una snake ("WASD")
 * al caracter correspondiente del cuerpo de una snake
 * ("^<v>").
*/
static char head_to_body(char c) {
  switch (c) {
        case 'W': return '^';
        case 'A': return '<';
        case 'S': return 'v';
        case 'D': return '>';
        default: return '?'; // Indefinido para caracteres no válidos
    }
}


/**
 * Retorna cur_row + 1 si la variable c es 'v', 's' o 'S'.
 * Retorna cur_row - 1 si la variable c es '^', 'w' o 'W'.
 * Retorna cur_row de lo contrario
*/
static unsigned int get_next_row(unsigned int cur_row, char c) {
  if (c == 'v' || c == 's' || c == 'S') {
     return cur_row + 1;
  } else if (c == '^' || c == 'w' || c == 'W') {
     return cur_row - 1;
  }
  return cur_row;
}


/**
 * Retorna cur_col + 1 si la variable c es '>' or 'd' or 'D'.
 * Retorna cur_col - 1 si la variable c es '<' or 'a' or 'A'.
 * Retorna cur_col de lo contrario
*/
static unsigned int get_next_col(unsigned int cur_col, char c) {
  if (c == '>' || c == 'd' || c == 'D') {
        return cur_col + 1;
    } else if (c == '<' || c == 'a' || c == 'A') {
        return cur_col - 1;
    }
    return cur_col;
}


/**
 * Tarea 4.2
 *
 * Funcion de ayuda para update_state. Retorna el caracter de la celda
 * en donde la snake se va a mover (en el siguiente paso).
 *
 * Esta funcion no deberia modificar nada de state.
*/
static char next_square(game_state_t* state, unsigned int snum) {
  // TODO: Implementar esta funcion.
  // Obtener la posición actual de la cabeza de la serpiente
    unsigned int head_row = state->snakes[snum].head_row;
    unsigned int head_col = state->snakes[snum].head_col;

    // Obtener el carácter de la cabeza de la serpiente
    char head_char = get_board_at(state, head_row, head_col);

    // Calcular la siguiente posición de la cabeza
    unsigned int next_row = get_next_row(head_row, head_char);
    unsigned int next_col = get_next_col(head_col, head_char);

    // Retornar el carácter en la siguiente posición
    return get_board_at(state, next_row, next_col);
}


/**
 * Tarea 4.3
 *
 * Funcion de ayuda para update_state. Actualiza la cabeza de la snake...
 *
 * ... en el tablero: agregar un caracter donde la snake se va a mover (¿que caracter?)
 *
 * ... en la estructura del snake: actualizar el row y col de la cabeza
 *
 * Nota: esta funcion ignora la comida, paredes, y cuerpos de otras snakes
 * cuando se mueve la cabeza.
*/
static void update_head(game_state_t* state, unsigned int snum) {
  // Obtener la posición actual de la cabeza de la serpiente
    unsigned int head_row = state->snakes[snum].head_row;
    unsigned int head_col = state->snakes[snum].head_col;

    // Obtener el carácter de la cabeza de la serpiente
    char head_char = get_board_at(state, head_row, head_col);

    // Calcular la siguiente posición de la cabeza
    unsigned int next_row = get_next_row(head_row, head_char);
    unsigned int next_col = get_next_col(head_col, head_char);

    // Actualizar el tablero con el nuevo carácter de la cabeza
    char new_body_char = head_to_body(head_char); // Convertir la cabeza actual en cuerpo
    set_board_at(state, head_row, head_col, new_body_char);

    // Colocar la nueva cabeza en el tablero
    set_board_at(state, next_row, next_col, head_char);

    // Actualizar la posición de la cabeza en la estructura de la serpiente
    state->snakes[snum].head_row = next_row;
    state->snakes[snum].head_col = next_col;
}


/**
 * Tarea 4.4
 *
 * Funcion de ayuda para update_state. Actualiza la cola de la snake...
 *
 * ... en el tablero: colocar un caracter blanco (spacio) donde se encuentra
 * la cola actualmente, y cambiar la nueva cola de un caracter de cuerpo (^<v>)
 * a un caracter de cola (wasd)
 *
 * ...en la estructura snake: actualizar el row y col de la cola
*/
static void update_tail(game_state_t* state, unsigned int snum) {
  // Obtener la posición actual de la cola de la serpiente
    unsigned int tail_row = state->snakes[snum].tail_row;
    unsigned int tail_col = state->snakes[snum].tail_col;

    // Obtener el carácter actual de la cola
    char tail_char = get_board_at(state, tail_row, tail_col);

    // Borrar el carácter actual de la cola (colocar un espacio)
    set_board_at(state, tail_row, tail_col, ' ');

    // Calcular la nueva posición de la cola
    unsigned int next_row = get_next_row(tail_row, tail_char);
    unsigned int next_col = get_next_col(tail_col, tail_char);

    // Obtener el carácter en la nueva posición de la cola
    char new_tail_char = get_board_at(state, next_row, next_col);

    // Cambiar el carácter de cuerpo a un carácter de cola
    char updated_tail_char = body_to_tail(new_tail_char);
    set_board_at(state, next_row, next_col, updated_tail_char);

    // Actualizar la posición de la cola en la estructura de la serpiente
    state->snakes[snum].tail_row = next_row;
    state->snakes[snum].tail_col = next_col;
}

/* Tarea 4.5 */
void update_state(game_state_t* state, int (*add_food)(game_state_t* state)) {
  // Iterar sobre todas las serpientes
  for (unsigned int snum = 0; snum < state->num_snakes; snum++) {
    // Verificar si la serpiente está viva
    if (!state->snakes[snum].live) {
        continue; // Saltar serpientes muertas
    }

    // Obtener el carácter de la celda a la que se moverá la cabeza
    char next_char = next_square(state, snum);

    // Manejar colisiones con paredes o cuerpos
    if (next_char == '#' || is_snake(next_char)) {
        // La serpiente muere
        state->snakes[snum].live = false;
        unsigned int head_row = state->snakes[snum].head_row;
        unsigned int head_col = state->snakes[snum].head_col;
        set_board_at(state, head_row, head_col, 'x'); // Reemplazar la cabeza con 'x'
        continue; // Pasar a la siguiente serpiente
    }

    // Manejar si la serpiente come una fruta
    if (next_char == '*') {
        // Mover la cabeza sin mover la cola (crecimiento)
        update_head(state, snum);
        // Generar una nueva fruta
        add_food(state);
        continue; // Pasar a la siguiente serpiente
    }

    // Si no hay colisión ni fruta, mover la cabeza y la cola
    update_head(state, snum);
    update_tail(state, snum);
}
}

/* Tarea 5 */
game_state_t* load_board(char* filename) {
  // Abrir el archivo en modo lectura
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Error al abrir el archivo");
        return NULL;
    }

    // Crear la estructura game_state_t
    game_state_t* state = malloc(sizeof(game_state_t));
    if (!state) {
        perror("Error al asignar memoria para game_state_t");
        fclose(file);
        return NULL;
    }

    // Inicializar valores iniciales
    state->num_rows = 0;
    state->board = NULL;
    state->num_snakes = 0;
    state->snakes = NULL;

    // Leer el archivo línea por línea
    char buffer[1024]; // Buffer temporal para leer líneas
    while (fgets(buffer, sizeof(buffer), file)) {
        // Remover el salto de línea al final de la línea
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
            len--;
        }

        // Asignar memoria para la nueva fila
        char* row = malloc((len + 1) * sizeof(char));
        if (!row) {
            perror("Error al asignar memoria para una fila del tablero");
            fclose(file);
            free_state(state);
            return NULL;
        }

        // Copiar la línea al tablero
        strcpy(row, buffer);

        // Reasignar memoria para agregar la nueva fila al tablero
        char** new_board = realloc(state->board, (state->num_rows + 1) * sizeof(char*));
        if (!new_board) {
            perror("Error al reasignar memoria para el tablero");
            free(row);
            fclose(file);
            free_state(state);
            return NULL;
        }

        state->board = new_board;
        state->board[state->num_rows] = row;
        state->num_rows++;
    }

    // Cerrar el archivo
    fclose(file);

    return state;
}


/**
 * Tarea 6.1
 *
 * Funcion de ayuda para initialize_snakes.
 * Dada una structura de snake con los datos de cola row y col ya colocados,
 * atravezar el tablero para encontrar el row y col de la cabeza de la snake,
 * y colocar esta informacion en la estructura de la snake correspondiente
 * dada por la variable (snum)
*/
static void find_head(game_state_t* state, unsigned int snum) {
  // TODO: Implementar esta funcion.
  return;
}

/* Tarea 6.2 */
game_state_t* initialize_snakes(game_state_t* state) {
  // TODO: Implementar esta funcion.
  return NULL;
}
