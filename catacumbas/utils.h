#ifndef UTILS_H
#define UTILS_H
#include "catacumbas.h"  // << Necesario

// =====================================
//   UTILIDADES
// =====================================

/** @brief Color normal para caracteres en la terminal */
#define ANSI_RESET "\x1b[0m"

/** @brief Color rojo para caracteres en la terminal */
#define ANSI_RED "\x1b[31m"

/** @brief Color amarillo para caracteres en la terminal */
#define ANSI_YELLOW "\x1b[33m"

/** @brief Color azul para caracteres en la terminal */
#define ANSI_BLUE "\x1b[34m"

/** 
 * @brief Mostrar en la salida estándar el uso y parámetros para ejecutar el servidor
 * @param argv comando para ejecutar el programa
 */
void usage(char *argv[]);

/**
 * @brief Mostrar en la salida de errores un mensaje de error y terminar la ejecución
 * @param msg mensaje a mostrar
 */
void fatal(char msg[]);

/** 
 * @brief Mostrar en la salida estándar el estado de la Arena
 * @param estado el struct Estado a mostrar
 */
void imprimirEstado(struct Estado *estado);

/** 
 * @brief Mostrar en la salida estándar un título con una operación que un jugador pretende ejecutar
 * @param pid el PID del jugador
 * @param accion el nombre de la operación
 */
void imprimirTituloSolicitud(long pid, const char *accion);

/** 
 * @brief Mostrar en la salida estándar el mapa de la Arena
 * @param mapa una matriz de caracteres
 */
void mostrarMapa(char mapa[FILAS][COLUMNAS]);

#endif