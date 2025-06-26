#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <unistd.h>
#include <string.h>
#include "../juego_constantes.h"
#include "jugador.h"
#include "../../directorio/directorio.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "../../catacumbas/catacumbas.h"
#include "status.h"

#define MENU_PRINCIPAL_ITEMS 5
#define MENU_WIDTH 50

typedef struct
{
    char nombre[64];
    char direccion[128]; // Dirección de la memoria compartida
    int mailbox;         // Mailbox del mapa
    int players_connected;
    int max_players;
} Map;

// Variables globales para rol y mapa seleccionados
int tipo_jugador = JUGADOR_EXPLORADOR;     // o JUGADOR_GUARDIAN según selección
char nombre_catacumba[128] = "catacumba1"; // ejemplo
char selected_role[50] = "NO SELECCIONADO";
char selected_map[50] = "NO SELECCIONADO";
char player_character = 'E'; // Valor por defecto para el personaje del jugador
char selected_shm_path[128];
int selected_mailbox;
int jugador_x = 1, jugador_y = 1; // Valores por defecto

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
extern int mostrar_seleccion_mapa(Map *maps, int num_maps);

// Declaraciones de funciones internas
int ejecutar_base();
int ejecutar_seleccion_mapa();
int ejecutar_seleccion_rol();
int mostrar_listado_mapas_y_seleccionar();
void mostrar_mapa_real(); // <-- AGREGA ESTA LÍNEA
void dibujar_mapa_coloreado(const char *mapa, int filas, int columnas, int jugador_x, int jugador_y, char playerChar);


void mostrar_catacumbas_formateadas(char *datos);

typedef struct
{
    char *texto;
    char *descripcion;
    int (*funcion)();
} OpcionMenuPrincipal;

// Función para buscar catacumbas disponibles y mostrar menú de selección
int buscar_catacumbas_disponibles()
{
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
        if (has_colors())
        {
            start_color();
            init_pair(1, COLOR_MAGENTA, COLOR_MAGENTA); // Paredes
            init_pair(2, COLOR_RED, COLOR_YELLOW);      // Tesoro y jugadores
            init_pair(3, COLOR_BLACK, COLOR_GREEN);     // Fondo/caminable
            init_pair(4, COLOR_MAGENTA, COLOR_BLACK);   // Header
            init_pair(5, COLOR_YELLOW, COLOR_BLACK);    // Instrucciones
        }
        return 0;
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
        if (has_colors())
        {
            start_color();
            init_pair(1, COLOR_MAGENTA, COLOR_MAGENTA); // Paredes
            init_pair(2, COLOR_RED, COLOR_YELLOW);      // Tesoro y jugadores
            init_pair(3, COLOR_BLACK, COLOR_GREEN);     // Fondo/caminable
            init_pair(4, COLOR_MAGENTA, COLOR_BLACK);   // Header
            init_pair(5, COLOR_YELLOW, COLOR_BLACK);    // Instrucciones
        }
        return 0;
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
        if (has_colors())
        {
            start_color();
            init_pair(1, COLOR_MAGENTA, COLOR_MAGENTA); // Paredes
            init_pair(2, COLOR_RED, COLOR_YELLOW);      // Tesoro y jugadores
            init_pair(3, COLOR_BLACK, COLOR_GREEN);     // Fondo/caminable
            init_pair(4, COLOR_MAGENTA, COLOR_BLACK);   // Header
            init_pair(5, COLOR_YELLOW, COLOR_BLACK);    // Instrucciones
        }
        return 0;
    }

    if (resp.codigo == RESP_OK)
    {
        // Parsear y mostrar catacumbas disponibles
        Map maps[20];
        int num_maps = 0;

        char *saveptr1, *saveptr2;
        char *catacumba = strtok_r(resp.datos, ";", &saveptr1);
        while (catacumba && num_maps < 20)
        {
            char *nombre = strtok_r(catacumba, "|", &saveptr2);
            char *direccion = strtok_r(NULL, "|", &saveptr2); // direccion
            strtok_r(NULL, "|", &saveptr2);                   // propCatacumba (puedes ignorar)
            char *mailbox_str = strtok_r(NULL, "|", &saveptr2);
            char *cantJug = strtok_r(NULL, "|", &saveptr2);
            char *maxJug = strtok_r(NULL, "|", &saveptr2);

            if (nombre && direccion && mailbox_str && cantJug && maxJug)
            {
                strncpy(maps[num_maps].nombre, nombre, sizeof(maps[num_maps].nombre) - 1);
                maps[num_maps].nombre[sizeof(maps[num_maps].nombre) - 1] = '\0';
                strncpy(maps[num_maps].direccion, direccion, sizeof(maps[num_maps].direccion) - 1);
                maps[num_maps].direccion[sizeof(maps[num_maps].direccion) - 1] = '\0';
                maps[num_maps].mailbox = atoi(mailbox_str);
                maps[num_maps].players_connected = atoi(cantJug);
                maps[num_maps].max_players = atoi(maxJug);
                num_maps++;
            }
            catacumba = strtok_r(NULL, ";", &saveptr1);
        }

        if (num_maps == 0)
        {
            endwin();
            printf("ℹ️  No hay catacumbas disponibles.\n");
            printf("Presiona Enter para continuar...");
            getchar();
            initscr();
            cbreak();
            noecho();
            keypad(stdscr, TRUE);
            curs_set(0);
            if (has_colors())
            {
                start_color();
                init_pair(1, COLOR_MAGENTA, COLOR_MAGENTA); // Paredes
                init_pair(2, COLOR_RED, COLOR_YELLOW);      // Tesoro y jugadores
                init_pair(3, COLOR_BLACK, COLOR_GREEN);     // Fondo/caminable
                init_pair(4, COLOR_MAGENTA, COLOR_BLACK);   // Header
                init_pair(5, COLOR_YELLOW, COLOR_BLACK);    // Instrucciones
            }
            return 0;
        }

        int seleccionado = mostrar_seleccion_mapa(maps, num_maps);
        if (seleccionado >= 0 && seleccionado < num_maps)
        {
            set_game_map(maps[seleccionado].nombre);
            strncpy(selected_shm_path, maps[seleccionado].direccion, sizeof(selected_shm_path) - 1);
            selected_shm_path[sizeof(selected_shm_path) - 1] = '\0';
            selected_mailbox = maps[seleccionado].mailbox;

            mostrar_mapa_real(); // <-- Muestra el mapa real
        }
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
        if (has_colors())
        {
            start_color();
            init_pair(1, COLOR_MAGENTA, COLOR_MAGENTA); // Paredes
            init_pair(2, COLOR_RED, COLOR_YELLOW);      // Tesoro y jugadores
            init_pair(3, COLOR_BLACK, COLOR_GREEN);     // Fondo/caminable
            init_pair(4, COLOR_MAGENTA, COLOR_BLACK);   // Header
            init_pair(5, COLOR_YELLOW, COLOR_BLACK);    // Instrucciones
        }
        return 0;
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
        init_pair(1, COLOR_MAGENTA, COLOR_MAGENTA); // Paredes
        init_pair(2, COLOR_RED, COLOR_YELLOW);      // Tesoro y jugadores
        init_pair(3, COLOR_BLACK, COLOR_GREEN);     // Fondo/caminable
        init_pair(4, COLOR_MAGENTA, COLOR_BLACK);   // Header
        init_pair(5, COLOR_YELLOW, COLOR_BLACK);    // Instrucciones
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
        init_pair(1, COLOR_MAGENTA, COLOR_MAGENTA); // Paredes
        init_pair(2, COLOR_RED, COLOR_YELLOW);      // Tesoro y jugadores
        init_pair(3, COLOR_BLACK, COLOR_GREEN);     // Fondo/caminable
        init_pair(4, COLOR_MAGENTA, COLOR_BLACK);   // Header
        init_pair(5, COLOR_YELLOW, COLOR_BLACK);    // Instrucciones
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
                        init_pair(1, COLOR_MAGENTA, COLOR_MAGENTA); // Paredes
                        init_pair(2, COLOR_RED, COLOR_YELLOW);      // Tesoro y jugadores
                        init_pair(3, COLOR_BLACK, COLOR_GREEN);     // Fondo/caminable
                        init_pair(4, COLOR_MAGENTA, COLOR_BLACK);   // Header
                        init_pair(5, COLOR_YELLOW, COLOR_BLACK);    // Instrucciones
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
        player_character = 'E'; // carácter que se use para explorador
        break;
    case 'G': // JUGADOR_GUARDIAN
        set_game_role("GUARDIAN");
        player_character = 'G'; // carácter que se use para guardián
        break;
    default:
        set_game_role("NO SELECCIONADO");
        player_character = 'E'; // valor por defecto
        break;
    }

    return resultado;
}

int ejecutar_seleccion_mapa()
{
    return mostrar_listado_mapas_y_seleccionar();
}

int ejecutar_base()
{
    // Cerrar ncurses antes de iniciar el juego base
    endwin();
    clear();
    int resultado = mostrar_base(player_character);

    return resultado;
}
void setPlayChar(char c)
{
    player_character = c;
}
int mostrar_listado_mapas_y_seleccionar()
{
    int mailbox_solicitudes = msgget(MAILBOX_KEY, 0666);
    int mailbox_respuestas = msgget(MAILBOX_RESPUESTA_KEY, 0666);

    if (mailbox_solicitudes == -1 || mailbox_respuestas == -1)
    {
        endwin();
        printf("❌ Directorio no disponible\n");
        printf("Presiona Enter para regresar al menú principal...");
        getchar();
        initscr();
        return 0;
    }

    struct solicitud msg;
    struct respuesta resp;
    pid_t mi_pid = getpid();

    msg.mtype = mi_pid;
    msg.tipo = OP_LISTAR;
    msg.texto[0] = '\0';

    if (msgsnd(mailbox_solicitudes, &msg, sizeof(msg) - sizeof(long), 0) == -1)
    {
        endwin();
        perror("Error enviando solicitud");
        printf("Presiona Enter para regresar al menú principal...");
        getchar();
        initscr();
        return 0;
    }

    if (msgrcv(mailbox_respuestas, &resp, sizeof(resp) - sizeof(long), mi_pid, 0) == -1)
    {
        endwin();
        perror("Error recibiendo respuesta");
        printf("Presiona Enter para regresar al menú principal...");
        getchar();
        initscr();
        return 0;
    }

    if (resp.codigo == RESP_OK)
    {
        Map maps[20];
        int num_maps = 0;
        char *saveptr1, *saveptr2;
        char *catacumba = strtok_r(resp.datos, ";", &saveptr1);
        while (catacumba && num_maps < 20)
        {
            char *nombre = strtok_r(catacumba, "|", &saveptr2);
            char *direccion = strtok_r(NULL, "|", &saveptr2); // direccion
            strtok_r(NULL, "|", &saveptr2);                   // propCatacumba (se podria ignorar
            char *mailbox_str = strtok_r(NULL, "|", &saveptr2);
            char *cantJug = strtok_r(NULL, "|", &saveptr2);
            char *maxJug = strtok_r(NULL, "|", &saveptr2);

            if (nombre && direccion && mailbox_str && cantJug && maxJug)
            {
                strncpy(maps[num_maps].nombre, nombre, sizeof(maps[num_maps].nombre) - 1);
                maps[num_maps].nombre[sizeof(maps[num_maps].nombre) - 1] = '\0';
                strncpy(maps[num_maps].direccion, direccion, sizeof(maps[num_maps].direccion) - 1);
                maps[num_maps].direccion[sizeof(maps[num_maps].direccion) - 1] = '\0';
                maps[num_maps].mailbox = atoi(mailbox_str);
                maps[num_maps].players_connected = atoi(cantJug);
                maps[num_maps].max_players = atoi(maxJug);
                num_maps++;
            }
            catacumba = strtok_r(NULL, ";", &saveptr1);
        }

        if (num_maps == 0)
        {
            endwin();
            printf("ℹ️  No hay catacumbas disponibles.\n");
            printf("Presiona Enter para continuar...");
            getchar();
            initscr();
            return 0;
        }

        int seleccionado = mostrar_seleccion_mapa(maps, num_maps);
        if (seleccionado >= 0 && seleccionado < num_maps)
        {
            set_game_map(maps[seleccionado].nombre);
            strncpy(selected_shm_path, maps[seleccionado].direccion, sizeof(selected_shm_path) - 1);
            selected_shm_path[sizeof(selected_shm_path) - 1] = '\0';
            selected_mailbox = maps[seleccionado].mailbox;
        }
        return seleccionado;
    }
    else
    {
        endwin();
        printf("❌ Error: %s\n", resp.datos);
        printf("Presiona Enter para regresar al menú principal...");
        getchar();
        initscr();
        return 0;
    }
}

// Inicializa la memoria compartida y retorna el puntero al mapa
char *inicializar_memoria_mapa(int *fd, size_t *size)
{
    *fd = shm_open(selected_shm_path, O_RDONLY, 0666);
    if (*fd == -1)
    {
        endwin();
        perror("Error abriendo memoria compartida del mapa");
        printf("Presiona Enter para continuar...");
        getchar();
        initscr();
        return NULL;
    }
    *size = FILAS * COLUMNAS;
    char *mapa = mmap(NULL, *size, PROT_READ, MAP_SHARED, *fd, 0);
    if (mapa == MAP_FAILED)
    {
        endwin();
        perror("Error mapeando memoria compartida del mapa");
        printf("Presiona Enter para continuar...");
        getchar();
        initscr();
        close(*fd);
        return NULL;
    }
    return mapa;
}

// Busca la posición inicial del jugador en el mapa
void buscar_posicion_inicial(const char *mapa, int *jugador_x, int *jugador_y)
{
    for (int y = 0; y < FILAS; y++)
    {
        for (int x = 0; x < COLUMNAS; x++)
        {
            if (mapa[y * COLUMNAS + x] == ' ')
            {
                *jugador_y = y;
                *jugador_x = x;
                return;
            }
        }
    }
    // Si no encuentra, deja los valores por defecto
}

// Dibuja el mapa y el jugador en pantalla
void dibujar_mapa(const char *mapa, int jugador_x, int jugador_y, char player_character)
{
    clear();
    for (int y = 0; y < FILAS; y++)
    {
        for (int x = 0; x < COLUMNAS; x++)
        {
            mvaddch(y, x, mapa[y * COLUMNAS + x]);
        }
    }
    mvaddch(jugador_y, jugador_x, player_character);
    refresh();
}

// Procesa el movimiento del jugador y colisiones
int procesar_movimiento(char destino, int *jugador_x, int *jugador_y, int new_x, int new_y)
{
    if (destino == '#')
    {
        // Es una pared, no moverse
        return 0;
    }
    if (destino == '$')
    {
        // Es un tesoro, lo recoges
        mvprintw(FILAS + 1, 0, "¡Tesoro recogido!");
        refresh();
        sleep(0.2);
    }
    if (destino == 'A' || destino == 'J')
    {
        // Colisión con otro jugador
        mvprintw(FILAS + 1, 0, "¡Colisión con otro jugador!");
        refresh();
        sleep(0.2);
        return 0;
    }
    // Movimiento válido
    *jugador_x = new_x;
    *jugador_y = new_y;
    return 1;
}

// Envía el movimiento al servidor
void enviar_movimiento_al_servidor(int jugador_x, int jugador_y, key_t clave_mailbox_respuestas, int mailbox_solicitudes_id)
{
    struct SolicitudServidor solicitud;
    solicitud.mtype = getpid();
    solicitud.codigo = MOVIMIENTO;
    solicitud.clave_mailbox_respuestas = clave_mailbox_respuestas;
    solicitud.fila = jugador_y;
    solicitud.columna = jugador_x;
    solicitud.tipo = (selected_role[0] == 'E') ? RAIDER : GUARDIAN;
    msgsnd(mailbox_solicitudes_id, &solicitud, sizeof(solicitud) - sizeof(long), 0);
}

// Bucle principal del juego
void mostrar_mapa_real()
{
    // Inicializa colores SOLO para el mapa real
    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_MAGENTA, COLOR_MAGENTA); // Paredes
        init_pair(2, COLOR_RED, COLOR_YELLOW);      // Tesoro y jugadores
        init_pair(3, COLOR_BLACK, COLOR_GREEN);     // Fondo/caminable
        init_pair(4, COLOR_MAGENTA, COLOR_BLACK);   // Header
        init_pair(5, COLOR_YELLOW, COLOR_BLACK);    // Instrucciones
    }

    int fd;
    size_t size;
    char *mapa = inicializar_memoria_mapa(&fd, &size);
    if (!mapa)
        return;

    key_t clave_mailbox_respuestas = getpid() * MAILBOX_SOLICITUDES_SUFIJO;
    int mailbox_solicitudes_id = msgget(selected_mailbox, 0666);

    int jugador_x = 1, jugador_y = 1;
    buscar_posicion_inicial(mapa, &jugador_x, &jugador_y);

    int ch;
    keypad(stdscr, TRUE);
    curs_set(0);

    while ((ch = getch()) != 'q')
    {
        int new_x = jugador_x, new_y = jugador_y;
        if (ch == KEY_UP)
            new_y--;
        if (ch == KEY_DOWN)
            new_y++;
        if (ch == KEY_LEFT)
            new_x--;
        if (ch == KEY_RIGHT)
            new_x++;

        // Control de bordes
        if (new_y < 0 || new_y >= FILAS || new_x < 0 || new_x >= COLUMNAS)
            continue;

        char destino = mapa[new_y * COLUMNAS + new_x];

        // Procesar movimiento y colisiones
        if (procesar_movimiento(destino, &jugador_x, &jugador_y, new_x, new_y))
        {
            enviar_movimiento_al_servidor(jugador_x, jugador_y, clave_mailbox_respuestas, mailbox_solicitudes_id);
        }

        // --- Aquí usas la función coloreada ---
        dibujar_mapa_coloreado(mapa, FILAS, COLUMNAS, jugador_x, jugador_y, player_character);
    }

    munmap(mapa, size);
    close(fd);
}

// Función para jugar
void jugar()
{
    // Conexión al servidor
    if (conectar_servidor(nombre_catacumba, tipo_jugador) != 0)
    {
        printf("No se pudo conectar al servidor.\n");
        return;
    }

    int x = 1, y = 1; // posición inicial
    int jugando = 1;
    while (jugando)
    {
        // Lógica de movimiento (leer teclado, calcular new_x, new_y, etc)
        // ...

        // Enviar movimiento
        if (enviar_movimiento(x, y, tipo_jugador) == 0)
        {
            char mensaje[256];
            int codigo;
            if (recibir_respuesta(mensaje, &codigo) == 0)
            {
                // Mostrar mensaje en pantalla
                mvprintw(22, 0, "%s", mensaje);
                refresh();
                // Si el código indica fin de juego, salir del bucle
                //el servidor nos tiene que mandar un mensaje de fin de juego, la info esta en status.h 
                if (codigo == ST_GAME_OVER)
                    jugando = 0;
            }
        }
        // ...redibujar mapa, actualizar posición, etc...
    }

    desconectar_servidor();
}

// Dibuja el mapa recibido del directorio with colores y desplazamiento
void dibujar_mapa_coloreado(const char *mapa, int filas, int columnas, int jugador_x, int jugador_y, char playerChar)
{
    // Inicializa colores SOLO una vez en tu programa principal, no aquí.
    // Aquí solo asume que los pares ya están definidos:
    // 1: Paredes, 2: Tesoro/Jugador, 3: Fondo, 4: Header, 5: Instrucciones

    // Header fijo (opcional)
    attron(COLOR_PAIR(4));
    mvprintw(1, 2, "=== MAPA DE CATACUMBAS ===");
    attroff(COLOR_PAIR(4));

    // Dibuja el mapa desplazado (como en base.c)
    for (int y = 0; y < filas; y++)
    {
        for (int x = 0; x < columnas; x++)
        {
            char c = mapa[y * columnas + x];
            if (c == '#' || c == CELDA_PARED)
            {
                attron(COLOR_PAIR(1));
                mvaddch(y + 4, x + 2, c);
                attroff(COLOR_PAIR(1));
            }
            else if (c == '$' || c == CELDA_TESORO)
            {
                attron(COLOR_PAIR(2));
                mvaddch(y + 4, x + 2, c);
                attroff(COLOR_PAIR(2));
            }
            else if (c == ' ' || c == CELDA_VACIA)
            {
                attron(COLOR_PAIR(3));
                mvaddch(y + 4, x + 2, c);
                attroff(COLOR_PAIR(3));
            }
            else if (c == 'E' || c == 'A' || c == 'J' || c == CELDA_EXPLORADOR || c == CELDA_GUARDIAN)
            {
                // Jugadores (puedes usar otro color si quieres distinguirlos)
                attron(COLOR_PAIR(2) | A_BOLD);
                mvaddch(y + 4, x + 2, c);
                attroff(COLOR_PAIR(2) | A_BOLD);
            }
            else
            {
                // Otros caracteres
                mvaddch(y + 4, x + 2, c);
            }
        }
    }

    // Dibuja TU jugador (por encima, para que se vea siempre)
    attron(COLOR_PAIR(2) | A_BOLD);
    mvaddch(jugador_y + 4, jugador_x + 2, playerChar);
    attroff(COLOR_PAIR(2) | A_BOLD);

    // Instrucciones
    attron(COLOR_PAIR(5));
    mvprintw(filas + 6, 2, "Controles: flechas = Mover, 'q' = Salir");
    attroff(COLOR_PAIR(5));

    refresh();
}
