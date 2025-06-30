#ifndef COLORES_H
#define COLORES_H

// ============================================================================
// CONSTANTES DE COLORES - ELEMENTOS DEL MAPA
// ============================================================================
#define COLOR_PARED                1
#define COLOR_PISO                 2
#define COLOR_TITULO_MAPA         3
#define COLOR_JUGADOR_LOCAL       4
#define COLOR_RAIDER              5
#define COLOR_GUARDIAN            6
#define COLOR_TESORO              7

// ============================================================================
// CONSTANTES DE COLORES - MENSAJES DEL JUEGO
// ============================================================================
#define COLOR_GAME_OVER           8
#define COLOR_VICTORIA            9
#define COLOR_SUBTITULOS         10
#define COLOR_INFO_PANTALLA      11

// ============================================================================
// CONSTANTES DE COLORES - MENÚ PRINCIPAL
// ============================================================================
#define COLOR_MENU_TITULO        20
#define COLOR_MENU_TEXTO         21
#define COLOR_MENU_OPCIONES      22
#define COLOR_MENU_SELECCIONADO  23
#define COLOR_MENU_INFORMACION   24
#define COLOR_MENU_ITEMS         25

// ============================================================================
// CONSTANTES DE COLORES - GENERAL
// ============================================================================
#define COLOR_TEXTO_NEGRO       110

void inicializar_colores();

#endif

/*  podriamos usar colores.h (cambiando el nombre claro)
para que los archivos llamen a los metodos a traves del .h,
sea llamar alguna pantalla o cualquier metodo que quiera ser llamado por main o otro archivo () */