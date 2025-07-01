#include <stdio.h>
#include <stdlib.h>
#include "utils.h"
#include "catacumbas.h" 

void usage(char *argv[]) {
    printf("Argumentos inválidos, por favor utilicelo de la siguiente forma:\n");
    printf("\n");
    printf("%s ruta/a/mapa.txt  ruta/a/config.properties\n",argv[0]);
    printf("\n");
    printf("    mapa.txt:\n");
    printf("        El archivo del que se cargará el mapa.\n");
    printf("        Debe ser de 80x25 sin espacios ni líneas extra\n");        
    printf("\n");
    printf("    config.properties:\n");
    printf("        El archivo de configuraciones de mapa.\n");
    printf("        Debe especificar:\n");
    printf("        - max_tesoros=<maximo de tesoros>\n");
    printf("        - max_raiders=<maximo de raiders>\n");
    printf("        - max_guardianes=<maximo de guardianes>\n");

}

void fatal(char msg[]) {
    perror(msg);
    exit(EXIT_FAILURE);
}

void imprimirEstado(struct Estado *estado) {
    printf("\n===== ESTADO DEL SERVIDOR =====\n");
    printf("- Máx. jugadores permitidos : %d\n", estado->max_jugadores);
    printf("- Cantidad total de jugadores: %d\n", estado->cant_jugadores);
    printf("- Cantidad de Raiders        : %d\n", estado->cant_raiders);
    printf("- Cantidad de Guardianes     : %d\n", estado->cant_guardianes);
    printf("- Cantidad de Tesoros        : %d\n", estado->cant_tesoros);
    printf("================================\n\n");
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