#ifndef CONFIG_H
#define CONFIG_H
#include "catacumbas.h"  // << Necesario

// ==============================
// Funciones de inicializacion
// ==============================

/// @brief Abre y mapea la memoria compartida que contiene la arena del juego.
/// @param arena Puntero a la arena del juego.
/// @param comunicacion Puntero a la estructura con los nombres de espacio de memoria compartida
void abrirMemoria(struct Arena *arena, struct Comunicacion *comunicacion);

/// @brief Abre o crea las colas de mensajes necesarias para la comunicacion
/// @param comunicacion 
void abrirMensajeria(struct Comunicacion *comunicacion);

/// @brief Carga los parametros de configuración del juego desde un archivo.
/// @param arena Puntero a la arena del juego.
/// @param ruta ruta al archivo de configuracion.
void cargarArchivoConfiguracion(struct Arena *arena, char ruta[]);

/// @brief Carga la estructura del mapa desde un archivo de texto.
/// @param arena Puntero a la arena del juego.
/// @param ruta ruta al archivo del mapa.
void cargarArchivoMapa(struct Arena *arena,char ruta[]);

// ==============================
// Funciones de generacion
// ==============================

/// @brief Genera los tesoros en el mapa de forma pseudo aleatoria
/// @param arena Puntero a la arena del juego.
void generarTesoros(struct Arena *arena);

// deprecated
void designArena(char mapa[FILAS][COLUMNAS]);

#endif