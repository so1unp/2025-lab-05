/**
 * @file colores.c
 * @brief Inicialización centralizada de pares de colores para ncurses
 *
 * Este módulo define y centraliza la inicialización de todos los pares de colores
 * utilizados en la interfaz gráfica del juego de catacumbas. Su objetivo es evitar
 * la duplicación de código y asegurar la coherencia visual en todos los módulos
 * que utilicen ncurses para mostrar información en pantalla.
 *
 * La función principal, @ref inicializar_colores, debe ser llamada después de cada
 * inicialización de ncurses (initscr()) en cualquier parte del programa donde se
 * vayan a utilizar colores. De esta forma, todos los archivos pueden compartir la
 * misma paleta de colores, facilitando el mantenimiento y la expansión del sistema.
 *
 * El archivo colores.h expone la función @ref inicializar_colores para que pueda
 * ser utilizada desde cualquier módulo del proyecto.
 *
 * @author
 * Equipo de desarrollo
 * @date
 * 2025
 */

#include <ncurses.h>
#include "colores.h"

/**
 * @brief Inicializa todos los pares de colores globales para ncurses.
 *
 * Esta función debe ser llamada después de cada initscr() y antes de usar cualquier
 * COLOR_PAIR en el programa. Define todos los pares de colores necesarios para
 * los distintos elementos visuales del juego (menús, mapas, mensajes, etc.).
 *
 * El uso de esta función permite que todos los módulos del proyecto utilicen los
 * mismos colores, asegurando coherencia visual y evitando la repetición de código.
 */
void inicializar_colores() {
    if (has_colors()) {
        start_color();
        use_default_colors();
        // Pares de colores para todo el juego
        init_pair(1, 235, 235);   // paredes gris oscuro
        init_pair(2, 8, 8);       // fondo/piso gris claro
        init_pair(3, 82, -1);     // Título del mapa e info
        init_pair(4, 11, 8);      // jugador local
        init_pair(5, 2, 8);       // Raider
        init_pair(6, 160, 8);     // Guardián
        init_pair(7, 227, 8);     // Tesoro
        init_pair(8, 88, -1);     // Game Over
        init_pair(9, 122, -1);    // Victoria
        init_pair(10, 68, -1);    // Subtítulos
        init_pair(11, 94, -1);    // Info pantalla
        init_pair(20, 63, 89);    // Menu principal - CATACUMBAS
        init_pair(21, 63, -1);    // Seleccionar opcion
        init_pair(22, 184, -1);   // opciones del menu
        init_pair(23, 17, 135);   // mensajes de opciones del menu
        init_pair(24, 82, -1);    // informacion
        init_pair(25, 53, 60);    // items
        init_pair(110, 16, -1);   // texto negro por defecto
    }
}