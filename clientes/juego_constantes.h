#ifndef JUEGO_CONSTANTES_H
#define JUEGO_CONSTANTES_H

// Caracteres del mapa
#define CELDA_VACIA ' '
#define CELDA_PARED '#'
#define CELDA_TESORO '$'
#define CELDA_EXPLORADOR 'E'
#define CELDA_GUARDIAN 'G'

// Tipos de jugador
#define JUGADOR_EXPLORADOR 1
#define JUGADOR_GUARDIAN 2

#define HEIGHT 20
#define WIDTH 80

typedef struct posicion
{
    int posX;
    int posY;
}Posicion;

/* int conectar_servidor(const char *nombre_catacumba, int tipo_jugador); */
void desconectar_servidor();/* 
int enviar_movimiento(int x, int y, int tipo_jugador);
int recibir_respuesta(char *mensaje, int *codigo); */

#endif