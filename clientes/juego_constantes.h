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

// Función para saber si una celda es caminable para un tipo de jugador
static inline int es_caminable(int tipo_jugador, char celda) {
    if (tipo_jugador == JUGADOR_EXPLORADOR) {
        // El explorador puede caminar por celdas vacías y tesoros
        return celda == CELDA_VACIA || celda == CELDA_TESORO;
    } else if (tipo_jugador == JUGADOR_GUARDIAN) {
        // El guardián solo puede caminar por celdas vacías y sobre el explorador
        return celda == CELDA_VACIA || celda == CELDA_EXPLORADOR;
    }
    return 0;
}

// Función para saber si el guardián captura al explorador
static inline int captura_guardian(char celda) {
    return celda == CELDA_EXPLORADOR;
}

// Función para saber si el explorador agarra un tesoro
static inline int captura_tesoro(int tipo_jugador, char celda) {
    // Solo el explorador puede capturar tesoros
    return tipo_jugador == JUGADOR_EXPLORADOR && celda == CELDA_TESORO;
}

#endif