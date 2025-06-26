#ifndef CONFIG_H
#define CONFIG_H
#include "catacumbas.h"  // << Necesario

// ==============================
// Funciones de inicialización
// ==============================
void abrirMemoria(struct Arena *arena, struct Comunicacion *comunicacion);
void abrirMensajeria(struct Comunicacion *comunicacion);
void cargarArchivoConfiguracion(struct Arena *arena, char ruta[]);
void cargarArchivoMapa(struct Arena *arena,char ruta[]);

// ==============================
// Funciones de generacion
// ==============================
void generarTesoros(struct Arena *arena);

// deprecated
void designArena(char mapa[FILAS][COLUMNAS]);

#endif