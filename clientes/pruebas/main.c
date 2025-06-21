#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <unistd.h>
#include <string.h>
#include "../juego_constantes.h"
#include "../../directorio/directorio.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define MENU_PRINCIPAL_ITEMS 5
#define MENU_WIDTH 50

// Variables globales para rol y mapa seleccionados
char selected_role[50] = "NO SELECCIONADO";
char selected_map[50] = "NO SELECCIONADO";
char player_character = 'J'; // Valor por defecto para el personaje del jugador

// Funciones para establecer rol y mapa
void set_game_role(const char *role)
{
    if (role != NULL)
    {
        strncpy(selected_role, role, 49);
        selected_role[49] = '\0';
    }
}

void set_game_map(const char *map)
{
    if (map != NULL)
    {
        strncpy(selected_map, map, 49);
        selected_map[49] = '\0';
    }
}

// Declaraciones de funciones externas
extern int mostrar_base(char player_character);
extern int mostrar_menu_rol();
extern int mostrar_seleccion_mapa();

// Declaraciones de funciones internas
int ejecutar_base();
int ejecutar_seleccion_mapa();
int ejecutar_seleccion_rol();

void mostrar_catacumbas_formateadas(char *datos);

typedef struct
{
    char *texto;
    char *descripcion;
    int (*funcion)();
} OpcionMenuPrincipal;

// Función para buscar catacumbas disponibles
int buscar_catacumbas_disponibles()
{
    // Cerrar ncurses temporalmente para mostrar mensajes
    endwin();
    
    int mailbox_solicitudes = msgget(MAILBOX_KEY, 0666);
    int mailbox_respuestas = msgget(MAILBOX_RESPUESTA_KEY, 0666);

    if (mailbox_solicitudes == -1 || mailbox_respuestas == -1)
    {
        printf("❌ Directorio no disponible\n");
        printf("   ℹ️  Asegúrate de que el servidor de directorio esté ejecutándose:\n");
        printf("   📁 cd ../../directorio\n");
        printf("   🚀 ./server &\n\n");
        printf("Presiona Enter para regresar al menú principal...");
        getchar();
        
        // Reinstalar ncurses antes de regresar
        initscr();
        cbreak();
        noecho();
        keypad(stdscr, TRUE);
        curs_set(0);
        if (has_colors()) {
            start_color();
            init_pair(1, COLOR_BLACK, COLOR_WHITE);
            init_pair(2, COLOR_WHITE, COLOR_BLACK);
            init_pair(3, COLOR_YELLOW, COLOR_BLACK);
            init_pair(4, COLOR_CYAN, COLOR_BLACK);
            init_pair(5, COLOR_GREEN, COLOR_BLACK);
        }
        
        return 0; // <-- CAMBIAR DE -1 A 0 para no cerrar el programa
    }

    struct solicitud msg;
    struct respuesta resp;
    pid_t mi_pid = getpid();

    // Solicitar lista de catacumbas
    msg.mtype = mi_pid;
    msg.tipo = OP_LISTAR;
    msg.texto[0] = '\0';

    if (msgsnd(mailbox_solicitudes, &msg, sizeof(msg) - sizeof(long), 0) == -1)
    {
        perror("Error enviando solicitud");
        printf("Presiona Enter para regresar al menú principal...");
        getchar();
        
        // Reinstalar ncurses antes de regresar
        initscr();
        cbreak();
        noecho();
        keypad(stdscr, TRUE);
        curs_set(0);
        if (has_colors()) {
            start_color();
            init_pair(1, COLOR_BLACK, COLOR_WHITE);
            init_pair(2, COLOR_WHITE, COLOR_BLACK);
            init_pair(3, COLOR_YELLOW, COLOR_BLACK);
            init_pair(4, COLOR_CYAN, COLOR_BLACK);
            init_pair(5, COLOR_GREEN, COLOR_BLACK);
        }
        
        return 0; // <-- CAMBIAR DE -1 A 0
    }

    // Recibir respuesta
    if (msgrcv(mailbox_respuestas, &resp, sizeof(resp) - sizeof(long), mi_pid, 0) == -1)
    {
        perror("Error recibiendo respuesta");
        printf("Presiona Enter para regresar al menú principal...");
        getchar();
        
        // Reinstalar ncurses antes de regresar
        initscr();
        cbreak();
        noecho();
        keypad(stdscr, TRUE);
        curs_set(0);
        if (has_colors()) {
            start_color();
            init_pair(1, COLOR_BLACK, COLOR_WHITE);
            init_pair(2, COLOR_WHITE, COLOR_BLACK);
            init_pair(3, COLOR_YELLOW, COLOR_BLACK);
            init_pair(4, COLOR_CYAN, COLOR_BLACK);
            init_pair(5, COLOR_GREEN, COLOR_BLACK);
        }
        
        return 0; // <-- CAMBIAR DE -1 A 0
    }

    if (resp.codigo == RESP_OK)
    {
        // Parsear y mostrar catacumbas disponibles
        mostrar_catacumbas_formateadas(resp.datos);
        return 0;
    }
    else
    {
        printf("❌ Error: %s\n", resp.datos);
        printf("Presiona Enter para regresar al menú principal...");
        getchar();
        
        // Reinstalar ncurses antes de regresar
        initscr();
        cbreak();
        noecho();
        keypad(stdscr, TRUE);
        curs_set(0);
        if (has_colors()) {
            start_color();
            init_pair(1, COLOR_BLACK, COLOR_WHITE);
            init_pair(2, COLOR_WHITE, COLOR_BLACK);
            init_pair(3, COLOR_YELLOW, COLOR_BLACK);
            init_pair(4, COLOR_CYAN, COLOR_BLACK);
            init_pair(5, COLOR_GREEN, COLOR_BLACK);
        }
        
        return 0; // <-- CAMBIAR DE -1 A 0
    }
}

// Función para mostrar catacumbas formateadas
void mostrar_catacumbas_formateadas(char *datos)
{
    // Cerrar ncurses temporalmente para mostrar en consola
    endwin();

    if (strlen(datos) == 0)
    {
        printf("ℹ️  No hay catacumbas disponibles.\n");
        printf("Presiona Enter para continuar...");
        getchar();
        return;
    }

    // Si es un mensaje simple, mostrarlo directamente
    if (strchr(datos, '|') == NULL)
    {
        printf("📝 %s\n", datos);
        printf("Presiona Enter para continuar...");
        getchar();
        return;
    }

    printf("\n--------------------------------------------------------------\n");
    printf("|          🏰 CATACUMBAS DISPONIBLES PARA JUGAR                |\n");
    printf("----------------------------------------------------------------\n");

    // Crear una copia para strtok
    char datos_copia[MAX_DAT_RESP];
    strncpy(datos_copia, datos, MAX_DAT_RESP - 1);
    datos_copia[MAX_DAT_RESP - 1] = '\0';

    // Procesar cada catacumba
    char *catacumba = strtok(datos_copia, ";");
    int index = 1;

    while (catacumba != NULL)
    {
        char *nombre = strtok(catacumba, "|");
        char *direccion = strtok(NULL, "|");
        char *mailbox = strtok(NULL, "|");
        char *cantJug_str = strtok(NULL, "|");
        char *maxJug_str = strtok(NULL, "|");

        if (nombre && direccion && mailbox && cantJug_str && maxJug_str)
        {
            int cantJug = atoi(cantJug_str);
            int maxJug = atoi(maxJug_str);

            printf("\n🏛️  Catacumba #%d:\n", index++);
            printf("   ├─ 📝 Nombre:     \"%s\"\n", nombre);
            printf("   ├─ 📍 Dirección:  \"%s\"\n", direccion);
            printf("   ├─ 📬 Mailbox:    \"%s\"\n", mailbox);
            printf("   └─ 👥 Jugadores:  %d/%d", cantJug, maxJug);

            // Indicador visual del estado
            if (cantJug == 0)
            {
                printf(" 🟢 (Vacía)\n");
            }
            else if (cantJug == maxJug)
            {
                printf(" 🔴 (Llena)\n");
            }
            else
            {
                printf(" 🟡 (Disponible)\n");
            }
        }

        catacumba = strtok(NULL, ";");
    }

    printf("\nPresiona Enter para continuar...");
    getchar();

    // Reinicializar ncurses
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    if (has_colors())
    {
        start_color();
        init_pair(1, COLOR_BLACK, COLOR_WHITE);
        init_pair(2, COLOR_WHITE, COLOR_BLACK);
        init_pair(3, COLOR_YELLOW, COLOR_BLACK);
        init_pair(4, COLOR_CYAN, COLOR_BLACK);
        init_pair(5, COLOR_GREEN, COLOR_BLACK);
    }
}

int mostrar_menu_principal()
{
    OpcionMenuPrincipal opciones[MENU_PRINCIPAL_ITEMS] = {
        {"Juego (demo)", "Demostración básica del juego con mapa fijo", ejecutar_base},
        {"Buscar Partidas reales", "Conectar a catacumabs disponibles", buscar_catacumbas_disponibles},
        {"Selección de Rol", "Elegir entre Explorador y Guardián", ejecutar_seleccion_rol},
        {"Selección de Mapa", "Interfaz para seleccionar mapas disponibles", ejecutar_seleccion_mapa},
        {"Salir", "Cerrar la aplicación", NULL}};

    int seleccion = 0;
    int ch;
    int max_y, max_x;

    // Inicializar ncurses
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    // Habilitar colores
    if (has_colors())
    {
        start_color();
        init_pair(1, COLOR_BLACK, COLOR_WHITE);
        init_pair(2, COLOR_WHITE, COLOR_BLACK);
        init_pair(3, COLOR_YELLOW, COLOR_BLACK);
        init_pair(4, COLOR_CYAN, COLOR_BLACK);
        init_pair(5, COLOR_GREEN, COLOR_BLACK);
    }

    while (1)
    {
        getmaxyx(stdscr, max_y, max_x);
        clear();

        // Título principal
        attron(COLOR_PAIR(3));
        mvprintw(max_y / 2 - 8, (max_x - strlen("=== MENU PRINCIPAL - CATACUMBAS ===")) / 2,
                 "=== MENU PRINCIPAL - CATACUMBAS ===");
        attroff(COLOR_PAIR(3));

        // Subtítulo
        attron(COLOR_PAIR(5));
        mvprintw(max_y / 2 - 6, (max_x - strlen("Selecciona una opción")) / 2,
                 "Selecciona una opción");
        attroff(COLOR_PAIR(5));

        // Mostrar opciones del menú
        for (int i = 0; i < MENU_PRINCIPAL_ITEMS; i++)
        {
            int y_pos = max_y / 2 - 3 + i * 2;
            int x_pos = max_x / 2 - MENU_WIDTH / 2;

            if (i == seleccion)
            {
                attron(COLOR_PAIR(1));
                mvprintw(y_pos, x_pos, " > %-30s < ", opciones[i].texto);
                attroff(COLOR_PAIR(1));

                // Mostrar descripción del item seleccionado
                if (i < MENU_PRINCIPAL_ITEMS - 1)
                {
                    attron(COLOR_PAIR(4));
                    mvprintw(y_pos + 1, x_pos + 3, "%-45s", opciones[i].descripcion);
                    attroff(COLOR_PAIR(4));
                }
            }
            else
            {
                attron(COLOR_PAIR(2));
                mvprintw(y_pos, x_pos, "   %-30s   ", opciones[i].texto);
                attroff(COLOR_PAIR(2));
            }
        }

        // Instrucciones
        mvprintw(max_y - 4, (max_x - strlen("Usa flechas para navegar, ENTER para seleccionar, ESC para salir")) / 2,
                 "Usa flechas para navegar, ENTER para seleccionar, ESC para salir");

        // Info adicional
        refresh();

        ch = getch();

        switch (ch)
        {
        case KEY_UP:
            seleccion = (seleccion - 1 + MENU_PRINCIPAL_ITEMS) % MENU_PRINCIPAL_ITEMS;
            break;
        case KEY_DOWN:
            seleccion = (seleccion + 1) % MENU_PRINCIPAL_ITEMS;
            break;
        case '\n':
        case '\r':
        case KEY_ENTER:
            if (opciones[seleccion].funcion)
            {
                int resultado = opciones[seleccion].funcion();
                if (resultado == -1)
                {
                    endwin();
                    return -1; // Salir completamente
                }

                // Si regresamos del juego base, reinicializar ncurses
                if (seleccion == 0)
                { // Juego Base
                    // Reinicializar ncurses completamente
                    initscr();
                    cbreak();
                    noecho();
                    keypad(stdscr, TRUE);
                    curs_set(0);
                    if (has_colors())
                    {
                        start_color();
                        init_pair(1, COLOR_BLACK, COLOR_WHITE);
                        init_pair(2, COLOR_WHITE, COLOR_BLACK);
                        init_pair(3, COLOR_YELLOW, COLOR_BLACK);
                        init_pair(4, COLOR_CYAN, COLOR_BLACK);
                        init_pair(5, COLOR_GREEN, COLOR_BLACK);
                    }
                }
            }
            else
            {
                // Opción "Salir"
                endwin();
                return 0;
            }
            break;
        case 'q':
        case 'Q':
        case 27: // ESC
            endwin();
            return 0;
        }
    }
}

// Función principal
int main()
{
    printf("Iniciando Menu Principal de Catacumbas...\n");
    sleep(1);

    int resultado = mostrar_menu_principal();

    if (resultado == 0)
    {
        printf("¡Gracias por jugar!\n");
    }

    return 0;
}

// Implementaciones que llaman a las funciones externas
int ejecutar_seleccion_rol()
{

    int resultado = mostrar_menu_rol();

    // Establecer el rol basado en el resultado
    switch (resultado)
    {
    case 'E': // JUGADOR_EXPLORADOR
        set_game_role("EXPLORADOR");
        break;
    case 'G': // JUGADOR_GUARDIAN
        set_game_role("GUARDIAN");
        break;
    default:
        set_game_role("NO SELECCIONADO");
        break;
    }

    return resultado;
}

int ejecutar_seleccion_mapa()
{
    int mapa_id = mostrar_seleccion_mapa();

    // Establecer el mapa basado en el resultado
    switch (mapa_id)
    {
    case 0:
        set_game_map("CASTILLO OSCURO");
        break;
    case 1:
        set_game_map("BOSQUE ENCANTADO");
        break;
    case 2:
        set_game_map("CIUDAD PERDIDA");
        break;
    default:
        set_game_map("MAPA DESCONOCIDO");
        break;
    }

    return mapa_id;
}

int ejecutar_base()
{
    // Cerrar ncurses antes de iniciar el juego base
    endwin();
    clear();
    int resultado = mostrar_base(player_character);

    return resultado;
}
void setPlayChar(char c) {
    player_character = c;
}


