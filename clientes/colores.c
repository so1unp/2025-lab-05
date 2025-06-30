/**
 * @file colores.c
 * @brief Inicialización centralizada de pares de colores para ncurses
 */

#include <ncurses.h>
#include "colores.h"

void inicializar_colores() {
    if (has_colors()) {
        start_color();
        use_default_colors();
        
        // Elementos del mapa
        init_pair(COLOR_PARED, 235, 235);
        init_pair(COLOR_PISO, 8, 8);
        init_pair(COLOR_TITULO_MAPA, 82, -1);
        init_pair(COLOR_JUGADOR_LOCAL, 11, 8);
        init_pair(COLOR_RAIDER, 2, 8);
        init_pair(COLOR_GUARDIAN, 160, 8);
        init_pair(COLOR_TESORO, 227, 8);
        
        // Mensajes del juego
        init_pair(COLOR_GAME_OVER, 88, -1);
        init_pair(COLOR_VICTORIA, 122, -1);
        init_pair(COLOR_SUBTITULOS, 68, -1);
        init_pair(COLOR_INFO_PANTALLA, 94, -1);
        
        // Menú principal
        init_pair(COLOR_MENU_TITULO, 63, 89);
        init_pair(COLOR_MENU_TEXTO, 63, -1);
        init_pair(COLOR_MENU_OPCIONES, 184, -1);
        init_pair(COLOR_MENU_SELECCIONADO, 17, 135);
        init_pair(COLOR_MENU_INFORMACION, 82, -1);
        init_pair(COLOR_MENU_ITEMS, 53, 60);
        
        // General
        init_pair(COLOR_TEXTO_NEGRO, 16, -1);
    }
}