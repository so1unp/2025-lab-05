#include <stdio.h>
#include "utils.h"
#include "catacumbas.h" 

void imprimirEstado(struct Estado *estado) {
    printf("\n===== ESTADO DEL SERVIDOR =====\n");
    printf("- Máx. jugadores permitidos : %d\n", estado->max_jugadores);
    printf("- Cantidad total de jugadores: %d\n", estado->cant_jugadores);
    printf("- Cantidad de Raiders        : %d\n", estado->cant_raiders);
    printf("- Cantidad de Guardianes     : %d\n", estado->cant_guardianes);
    printf("- Cantidad de Tesoros        : %d\n", estado->cant_tesoros);
    printf("================================\n\n");
}

// deprecated
void imprimirTesoros(struct Tesoro *tesoros, struct Estado *estado) {
    printf("\n===== Tesoros Posiciones =====\n");
    int i;
    for (i = 0; i < estado->cant_tesoros; i++) {
        printf("tesoro %d con pos [%d, %d]\n",
            tesoros[i].id,
            tesoros[i].posicion.fila,
            tesoros[i].posicion.columna);
    }
}

void imprimirTituloSolicitud(long pid, const char *accion) {
    printf("\n═══════════════════════════════════════════════════════════════\n");
    printf("\tJugador (%ld) %s\n", pid, accion);
    printf("═══════════════════════════════════════════════════════════════\n\n");
}

void mostrarMapa(char mapa[FILAS][COLUMNAS]) {
    printf("\n=== MAPA DE LA CATACUMBA ===\n");
    int i, j;
    for (i = 0; i < FILAS; i++) {
        for (j = 0; j < COLUMNAS; j++) {
            const char *color = ANSI_RESET;

            if (mapa[i][j] == PARED) {
                color = ANSI_BLUE;
            } else if (mapa[i][j] == TESORO) {
                color = ANSI_YELLOW;
            } else {
                color = ANSI_RESET;
            }
            printf("%s%c%s", color, mapa[i][j], ANSI_RESET);
        }
        putchar('\n');
    }
    printf("===========================\n");
}