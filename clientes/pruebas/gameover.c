#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void play_melody(const char* melody_type) {
    if (strcmp(melody_type, "game_over") == 0) {
        // Melodía de game over (escala descendente menor)
        system("(speaker-test -t sine -f 523 -l 1 -s 1 > /dev/null 2>&1 &)"); // Do
        usleep(400000);
        system("(speaker-test -t sine -f 466 -l 1 -s 1 > /dev/null 2>&1 &)"); // Sib
        usleep(400000);
        system("(speaker-test -t sine -f 415 -l 1 -s 1 > /dev/null 2>&1 &)"); // Lab
        usleep(400000);
        system("(speaker-test -t sine -f 349 -l 1 -s 1 > /dev/null 2>&1 &)"); // Fa
        usleep(600000);
        system("(speaker-test -t sine -f 311 -l 1 -s 1 > /dev/null 2>&1 &)"); // Mib
        usleep(800000);
    } else if (strcmp(melody_type, "victory") == 0) {
        // Melodía de victoria (fanfarria ascendente)
        system("(speaker-test -t sine -f 262 -l 1 -s 1 > /dev/null 2>&1 &)"); // Do
        usleep(200000);
        system("(speaker-test -t sine -f 330 -l 1 -s 1 > /dev/null 2>&1 &)"); // Mi
        usleep(200000);
        system("(speaker-test -t sine -f 392 -l 1 -s 1 > /dev/null 2>&1 &)"); // Sol
        usleep(200000);
        system("(speaker-test -t sine -f 523 -l 1 -s 1 > /dev/null 2>&1 &)"); // Do agudo
        usleep(600000);
    } else if (strcmp(melody_type, "intro") == 0) {
        // Melodía de introducción suave
        system("(speaker-test -t sine -f 440 -l 1 -s 1 > /dev/null 2>&1 &)"); // La
        usleep(300000);
        system("(speaker-test -t sine -f 523 -l 1 -s 1 > /dev/null 2>&1 &)"); // Do
        usleep(300000);
        system("(speaker-test -t sine -f 659 -l 1 -s 1 > /dev/null 2>&1 &)"); // Mi
        usleep(500000);
    } else if (strcmp(melody_type, "ambient") == 0) {
        // Melodía ambiente suave
        system("(speaker-test -t sine -f 349 -l 1 -s 1 > /dev/null 2>&1 &)"); // Fa
        usleep(250000);
        system("(speaker-test -t sine -f 392 -l 1 -s 1 > /dev/null 2>&1 &)"); // Sol
        usleep(250000);
        system("(speaker-test -t sine -f 440 -l 1 -s 1 > /dev/null 2>&1 &)"); // La
        usleep(300000);
    }
}

// Función para tocar acordes más complejos
void play_chord_progression() {
    // Progresión Am - F - C - G (muy común y agradable)
    
    // Acorde Am (La menor)
    system("(speaker-test -t sine -f 220 -l 1 -s 1 > /dev/null 2>&1 &)"); // La
    usleep(50000);
    system("(speaker-test -t sine -f 262 -l 1 -s 1 > /dev/null 2>&1 &)"); // Do
    usleep(50000);
    system("(speaker-test -t sine -f 330 -l 1 -s 1 > /dev/null 2>&1 &)"); // Mi
    usleep(700000);
    
    // Acorde F (Fa mayor)
    system("(speaker-test -t sine -f 175 -l 1 -s 1 > /dev/null 2>&1 &)"); // Fa
    usleep(50000);
    system("(speaker-test -t sine -f 220 -l 1 -s 1 > /dev/null 2>&1 &)"); // La
    usleep(50000);
    system("(speaker-test -t sine -f 262 -l 1 -s 1 > /dev/null 2>&1 &)"); // Do
    usleep(700000);
}

// Función para reproducir sonido usando beep del terminal
void play_terminal_beep() {
    printf("\a");
    fflush(stdout);
}

void draw_game_over() {
    clear();
    
    // Reproducir melodía suave de introducción
    play_melody("intro");
    
    // GAME OVER en estilo ASCII con asteriscos
    attron(COLOR_PAIR(1));
    
    // Letra G
    mvprintw(2, 2, "***********");
    mvprintw(3, 2, "***********");
    mvprintw(4, 2, "***        ");
    mvprintw(5, 2, "***   *****");
    mvprintw(6, 2, "***   *****");
    mvprintw(7, 2, "***     ***");
    mvprintw(8, 2, "***********");
    mvprintw(9, 2, "***********");
    
    // Letra A
    mvprintw(2, 15, "   *****   ");
    mvprintw(3, 15, "  *******  ");
    mvprintw(4, 15, " ***   *** ");
    mvprintw(5, 15, " ********* ");
    mvprintw(6, 15, " ********* ");
    mvprintw(7, 15, "***     ***");
    mvprintw(8, 15, "***     ***");
    mvprintw(9, 15, "***     ***");
    
    // Letra M
    mvprintw(2, 28, "***     ***");
    mvprintw(3, 28, "****   ****");
    mvprintw(4, 28, "*** *** ***");
    mvprintw(5, 28, "***  *  ***");
    mvprintw(6, 28, "***     ***");
    mvprintw(7, 28, "***     ***");
    mvprintw(8, 28, "***     ***");
    mvprintw(9, 28, "***     ***");
    
    // Letra E
    mvprintw(2, 41, "***********");
    mvprintw(3, 41, "***********");
    mvprintw(4, 41, "***        ");
    mvprintw(5, 41, "*******    ");
    mvprintw(6, 41, "*******    ");
    mvprintw(7, 41, "***        ");
    mvprintw(8, 41, "***********");
    mvprintw(9, 41, "***********");
    
    // Espacio entre GAME y OVER
    
    // Letra O
    mvprintw(12, 2, " ********* ");
    mvprintw(13, 2, "***********");
    mvprintw(14, 2, "***     ***");
    mvprintw(15, 2, "***     ***");
    mvprintw(16, 2, "***     ***");
    mvprintw(17, 2, "***     ***");
    mvprintw(18, 2, "***********");
    mvprintw(19, 2, " ********* ");
    
    // Letra V
    mvprintw(12, 15, "***     ***");
    mvprintw(13, 15, "***     ***");
    mvprintw(14, 15, "***     ***");
    mvprintw(15, 15, " ***   *** ");
    mvprintw(16, 15, " ***   *** ");
    mvprintw(17, 15, "  *** ***  ");
    mvprintw(18, 15, "   *****   ");
    mvprintw(19, 15, "    ***    ");
    
    // Letra E
    mvprintw(12, 28, "***********");
    mvprintw(13, 28, "***********");
    mvprintw(14, 28, "***        ");
    mvprintw(15, 28, "*******    ");
    mvprintw(16, 28, "*******    ");
    mvprintw(17, 28, "***        ");
    mvprintw(18, 28, "***********");
    mvprintw(19, 28, "***********");
    
    // Letra R
    mvprintw(12, 41, "********** ");
    mvprintw(13, 41, "***********");
    mvprintw(14, 41, "***     ***");
    mvprintw(15, 41, "********** ");
    mvprintw(16, 41, "***  ***   ");
    mvprintw(17, 41, "***   ***  ");
    mvprintw(18, 41, "***    *** ");
    mvprintw(19, 41, "***     ***");
    
    attroff(COLOR_PAIR(1));
    
    // Mensaje parpadeante más suave
    attron(COLOR_PAIR(2));
    mvprintw(25, 15, "♪ Presiona cualquier tecla para continuar ♪");
    attroff(COLOR_PAIR(2));
    
    // Sonido ambiente suave
    play_melody("ambient");

    refresh();
}

// Función para animación más suave con melodías
void animate_game_over() {
    for (int i = 0; i < 2; i++) { // Reducido a 2 iteraciones
        // Melodía ambiente antes de mostrar
        play_melody("ambient");
        
        draw_game_over();
        usleep(1200000); // Espera más larga (1.2 segundos)
        
        if (i < 1) { // No limpiar en la última iteración
            clear();
            refresh();
            usleep(400000); // Pausa más corta
        }
    }
}

int mostrar_game_over() {
    initscr();
    start_color();
    
    // Inicializar pares de colores más suaves
    init_pair(1, COLOR_MAGENTA, COLOR_BLACK);  // Magenta en lugar de rojo agresivo
    init_pair(2, COLOR_CYAN, COLOR_BLACK);     // Cyan para instrucciones
    init_pair(3, COLOR_WHITE, COLOR_BLUE);     // Blanco sobre azul para marco
    
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    
    // Reproducir animación con melodías suaves
    animate_game_over();
    
    // Mostrar pantalla final
    draw_game_over();
    
    // Melodía final melancólica pero no agresiva
    usleep(500000);
    play_melody("game_over");
    
    getch();
    
    endwin();
    return 0;
}

// Función adicional para victoria (si la necesitas)
int mostrar_victoria() {
    initscr();
    start_color();
    
    init_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);
    
    clear();
    
    attron(COLOR_PAIR(1));
    mvprintw(10, 20, "¡¡¡ V I C T O R I A !!!");
    attroff(COLOR_PAIR(1));
    
    attron(COLOR_PAIR(2));
    mvprintw(12, 15, "¡Felicidades! Has completado el juego");
    attroff(COLOR_PAIR(2));
    
    refresh();
    
    // Melodía alegre de victoria
    play_melody("victory");
    
    getch();
    endwin();
    return 0;
}

