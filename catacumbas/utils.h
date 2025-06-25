#ifndef UTILS_H
#define UTILS_H
#include "catacumbas.h"  // << Necesario

#define ANSI_RESET "\x1b[0m"
#define ANSI_RED "\x1b[31m"
#define ANSI_YELLOW "\x1b[33m"
#define ANSI_BLUE "\x1b[34m"

void imprimirTesoros(struct Tesoro *tesoros, struct Estado *estado);
void imprimirEstado(struct Estado *estado);
void imprimirTituloSolicitud(long pid, const char *accion);
void mostrarMapa(char mapa[FILAS][COLUMNAS]);

#endif