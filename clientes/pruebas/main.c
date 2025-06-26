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
    char direccion[128];
    int mailbox;
    int players_connected;
    int max_players;
} Map;

int tipo_jugador = JUGADOR_EXPLORADOR;
char nombre_catacumba[128] = "catacumba1";
char selected_role[50] = "NO SELECCIONADO";
char selected_map[50] = "NO SELECCIONADO";
char player_character = 'E';
char selected_shm_path[128];
int selected_mailbox;
int jugador_x = 1, jugador_y = 1;
key_t key_respuestas_global = 0;

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

extern int mostrar_base(char player_character);
extern int mostrar_menu_rol();
extern int mostrar_seleccion_mapa(Map *maps, int num_maps);

int ejecutar_base();
int ejecutar_seleccion_mapa();
int ejecutar_seleccion_rol();
int mostrar_listado_mapas_y_seleccionar();
void mostrar_mapa_real();
void dibujar_mapa_coloreado(const char *mapa, int filas, int columnas, int jugador_x, int jugador_y, char playerChar);
void mostrar_catacumbas_formateadas(char *datos);

typedef struct
{
    char *texto;
    char *descripcion;
    int (*funcion)();
} OpcionMenuPrincipal;

int conectar_al_servidor() {
    key_t key_respuestas = ftok("/tmp", getpid());
    int mailbox_respuestas = msgget(key_respuestas, IPC_CREAT | 0666);
    key_respuestas_global = key_respuestas;
    
    if (mailbox_respuestas == -1) {
        perror("Error creando mailbox de respuestas");
        return -1;
    }
    
    struct SolicitudServidor solicitud;
    solicitud.mtype = getpid();
    solicitud.codigo = CONEXION;
    solicitud.clave_mailbox_respuestas = key_respuestas;
    solicitud.fila = 0;
    solicitud.columna = 0;
    solicitud.tipo = (selected_role[0] == 'E') ? RAIDER : GUARDIAN;
    
    int mailbox_solicitudes_id = msgget(selected_mailbox, 0666);
    if (mailbox_solicitudes_id == -1) {
        perror("Error conectando al mailbox del servidor");
        return -1;
    }
    
    if (msgsnd(mailbox_solicitudes_id, &solicitud, sizeof(solicitud) - sizeof(long), 0) == -1) {
        perror("Error enviando solicitud de conexión");
        return -1;
    }
    
    struct RespuestaServidor respuesta;
    if (msgrcv(mailbox_respuestas, &respuesta, sizeof(respuesta) - sizeof(long), 
               getpid(), 0) == -1) {
        perror("Error recibiendo respuesta de conexión");
        return -1;
    }
    
    if (respuesta.codigo != S_OK) {
        mvprintw(FILAS + 4, 0, "Error del servidor: %s", respuesta.mensaje);
        return -1;
    }
    
    mvprintw(FILAS + 4, 0, "Conectado: %s", respuesta.mensaje);
    refresh();
    
    return 0;
}

void desconectar_del_servidor() {
    struct SolicitudServidor solicitud;
    solicitud.mtype = getpid();
    solicitud.codigo = DESCONEXION;
    solicitud.clave_mailbox_respuestas = key_respuestas_global;
    solicitud.tipo = (selected_role[0] == 'E') ? RAIDER : GUARDIAN;
    
    int mailbox_solicitudes_id = msgget(selected_mailbox, 0666);
    if (mailbox_solicitudes_id != -1) {
        msgsnd(mailbox_solicitudes_id, &solicitud, sizeof(solicitud) - sizeof(long), IPC_NOWAIT);
    }
    
    int mailbox_respuestas = msgget(key_respuestas_global, 0666);
    if (mailbox_respuestas != -1) {
        msgctl(mailbox_respuestas, IPC_RMID, NULL);
    }
    
    mvprintw(FILAS + 4, 0, "Desconectado del servidor");
    refresh();
}

void enviar_movimiento_al_servidor(int jugador_x, int jugador_y, key_t clave_mailbox_respuestas, int mailbox_solicitudes_id)
{
    struct SolicitudServidor solicitud;
    solicitud.mtype = getpid();
    solicitud.codigo = MOVIMIENTO;
    solicitud.clave_mailbox_respuestas = key_respuestas_global;
    solicitud.fila = jugador_y;
    solicitud.columna = jugador_x;
    solicitud.tipo = (selected_role[0] == 'E') ? RAIDER : GUARDIAN;
    
    if (msgsnd(mailbox_solicitudes_id, &solicitud, sizeof(solicitud) - sizeof(long), IPC_NOWAIT) == -1) {
        perror("Error enviando movimiento");
    }
    
    struct RespuestaServidor respuesta;
    int mailbox_respuestas = msgget(key_respuestas_global, 0666);
    if (msgrcv(mailbox_respuestas, &respuesta, sizeof(respuesta) - sizeof(long), 
               getpid(), IPC_NOWAIT) != -1) {
        
        switch(respuesta.codigo) {
            case S_OK:
                mvprintw(FILAS + 5, 0, "Movimiento OK");
                break;
            case ERROR:
                mvprintw(FILAS + 5, 0, "Error: %s", respuesta.mensaje);
                break;
            case SIN_TESOROS:
                mvprintw(FILAS + 5, 0, "Victoria! Todos los tesoros recolectados");
                break;
            case SIN_RAIDERS:
                mvprintw(FILAS + 5, 0, "Victoria! Todos los raiders capturados");
                break;
        }
        refresh();
    }
}

int buscar_catacumbas_disponibles()
{
    int mailbox_solicitudes = msgget(MAILBOX_KEY, 0666);
    int mailbox_respuestas = msgget(MAILBOX_RESPUESTA_KEY, 0666);

    if (mailbox_solicitudes == -1 || mailbox_respuestas == -1)
    {
        printf("Directorio no disponible\n");
        printf("Asegúrate de que el servidor de directorio esté ejecutándose:\n");
        printf("cd ../../directorio\n");
        printf("./server &\n\n");
        printf("Presiona Enter para regresar al menú principal...");
        getchar();
        initscr();
        cbreak();
        noecho();
        keypad(stdscr, TRUE);
        curs_set(0);
        if (has_colors())
        {
            start_color();
            init_pair(1, COLOR_MAGENTA, COLOR_MAGENTA);
            init_pair(2, COLOR_RED, COLOR_YELLOW);
            init_pair(3, COLOR_BLACK, COLOR_GREEN);
            init_pair(4, COLOR_MAGENTA, COLOR_BLACK);
            init_pair(5, COLOR_YELLOW, COLOR_BLACK);
        }
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
        perror("Error enviando solicitud");
        printf("Presiona Enter para regresar al menú principal...");
        getchar();
        initscr();
        cbreak();
        noecho();
        keypad(stdscr, TRUE);
        curs_set(0);
        if (has_colors())
        {
            start_color();
            init_pair(1, COLOR_MAGENTA, COLOR_MAGENTA);
            init_pair(2, COLOR_RED, COLOR_YELLOW);
            init_pair(3, COLOR_BLACK, COLOR_GREEN);
            init_pair(4, COLOR_MAGENTA, COLOR_BLACK);
            init_pair(5, COLOR_YELLOW, COLOR_BLACK);
        }
        return 0;
    }

    if (msgrcv(mailbox_respuestas, &resp, sizeof(resp) - sizeof(long), mi_pid, 0) == -1)
    {
        perror("Error recibiendo respuesta");
        printf("Presiona Enter para regresar al menú principal...");
        getchar();
        initscr();
        cbreak();
        noecho();
        keypad(stdscr, TRUE);
        curs_set(0);
        if (has_colors())
        {
            start_color();
            init_pair(1, COLOR_MAGENTA, COLOR_MAGENTA);
            init_pair(2, COLOR_RED, COLOR_YELLOW);
            init_pair(3, COLOR_BLACK, COLOR_GREEN);
            init_pair(4, COLOR_MAGENTA, COLOR_BLACK);
            init_pair(5, COLOR_YELLOW, COLOR_BLACK);
        }
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
            char *direccion = strtok_r(NULL, "|", &saveptr2);
            strtok_r(NULL, "|", &saveptr2);
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
            printf("No hay catacumbas disponibles.\n");
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
                init_pair(1, COLOR_MAGENTA, COLOR_MAGENTA);
                init_pair(2, COLOR_RED, COLOR_YELLOW);
                init_pair(3, COLOR_BLACK, COLOR_GREEN);
                init_pair(4, COLOR_MAGENTA, COLOR_BLACK);
                init_pair(5, COLOR_YELLOW, COLOR_BLACK);
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

            mostrar_mapa_real();
        }
        return 0;
    }
    else
    {
        printf("Error: %s\n", resp.datos);
        printf("Presiona Enter para regresar al menú principal...");
        getchar();
        initscr();
        cbreak();
        noecho();
        keypad(stdscr, TRUE);
        curs_set(0);
        if (has_colors())
        {
            start_color();
            init_pair(1, COLOR_MAGENTA, COLOR_MAGENTA);
            init_pair(2, COLOR_RED, COLOR_YELLOW);
            init_pair(3, COLOR_BLACK, COLOR_GREEN);
            init_pair(4, COLOR_MAGENTA, COLOR_BLACK);
            init_pair(5, COLOR_YELLOW, COLOR_BLACK);
        }
        return 0;
    }
}

void mostrar_catacumbas_formateadas(char *datos)
{
    endwin();

    if (strlen(datos) == 0)
    {
        printf("No hay catacumbas disponibles.\n");
        printf("Presiona Enter para continuar...");
        getchar();
        return;
    }

    if (strchr(datos, '|') == NULL)
    {
        printf("%s\n", datos);
        printf("Presiona Enter para continuar...");
        getchar();
        return;
    }

    printf("\n--------------------------------------------------------------\n");
    printf("|          CATACUMBAS DISPONIBLES PARA JUGAR                |\n");
    printf("----------------------------------------------------------------\n");

    char datos_copia[MAX_DAT_RESP];
    strncpy(datos_copia, datos, MAX_DAT_RESP - 1);
    datos_copia[MAX_DAT_RESP - 1] = '\0';

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

            printf("\nCatacumba #%d:\n", index++);
            printf("   Nombre:     \"%s\"\n", nombre);
            printf("   Dirección:  \"%s\"\n", direccion);
            printf("   Mailbox:    \"%s\"\n", mailbox);
            printf("   Jugadores:  %d/%d", cantJug, maxJug);

            if (cantJug == 0)
            {
                printf(" (Vacía)\n");
            }
            else if (cantJug == maxJug)
            {
                printf(" (Llena)\n");
            }
            else
            {
                printf(" (Disponible)\n");
            }
        }

        catacumba = strtok(NULL, ";");
    }

    printf("\nPresiona Enter para continuar...");
    getchar();

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    if (has_colors())
    {
        start_color();
        init_pair(1, COLOR_MAGENTA, COLOR_MAGENTA);
        init_pair(2, COLOR_RED, COLOR_YELLOW);
        init_pair(3, COLOR_BLACK, COLOR_GREEN);
        init_pair(4, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(5, COLOR_YELLOW, COLOR_BLACK);
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

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    if (has_colors())
    {
        start_color();
        init_pair(1, COLOR_MAGENTA, COLOR_MAGENTA);
        init_pair(2, COLOR_RED, COLOR_YELLOW);
        init_pair(3, COLOR_BLACK, COLOR_GREEN);
        init_pair(4, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(5, COLOR_YELLOW, COLOR_BLACK);
    }

    while (1)
    {
        getmaxyx(stdscr, max_y, max_x);
        clear();

        attron(COLOR_PAIR(3));
        mvprintw(max_y / 2 - 8, (max_x - strlen("=== MENU PRINCIPAL - CATACUMBAS ===")) / 2,
                 "=== MENU PRINCIPAL - CATACUMBAS ===");
        attroff(COLOR_PAIR(3));

        attron(COLOR_PAIR(5));
        mvprintw(max_y / 2 - 6, (max_x - strlen("Selecciona una opción")) / 2,
                 "Selecciona una opción");
        attroff(COLOR_PAIR(5));

        for (int i = 0; i < MENU_PRINCIPAL_ITEMS; i++)
        {
            int y_pos = max_y / 2 - 3 + i * 2;
            int x_pos = max_x / 2 - MENU_WIDTH / 2;

            if (i == seleccion)
            {
                attron(COLOR_PAIR(1));
                mvprintw(y_pos, x_pos, " > %-30s < ", opciones[i].texto);
                attroff(COLOR_PAIR(1));

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

        mvprintw(max_y - 4, (max_x - strlen("Usa flechas para navegar, ENTER para seleccionar, ESC para salir")) / 2,
                 "Usa flechas para navegar, ENTER para seleccionar, ESC para salir");

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
                    return -1;
                }

                if (seleccion == 0)
                {
                    initscr();
                    cbreak();
                    noecho();
                    keypad(stdscr, TRUE);
                    curs_set(0);
                    if (has_colors())
                    {
                        start_color();
                        init_pair(1, COLOR_MAGENTA, COLOR_MAGENTA);
                        init_pair(2, COLOR_RED, COLOR_YELLOW);
                        init_pair(3, COLOR_BLACK, COLOR_GREEN);
                        init_pair(4, COLOR_MAGENTA, COLOR_BLACK);
                        init_pair(5, COLOR_YELLOW, COLOR_BLACK);
                    }
                }
            }
            else
            {
                endwin();
                return 0;
            }
            break;
        case 'q':
        case 'Q':
        case 27:
            endwin();
            return 0;
        }
    }
}

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

int ejecutar_seleccion_rol()
{
    int resultado = mostrar_menu_rol();

    switch (resultado)
    {
    case 'E':
        set_game_role("EXPLORADOR");
        player_character = 'E';
        break;
    case 'G':
        set_game_role("GUARDIAN");
        player_character = 'G';
        break;
    default:
        set_game_role("NO SELECCIONADO");
        player_character = 'E';
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
        printf("Directorio no disponible\n");
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
            char *direccion = strtok_r(NULL, "|", &saveptr2);
            strtok_r(NULL, "|", &saveptr2);
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
            printf("No hay catacumbas disponibles.\n");
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
        printf("Error: %s\n", resp.datos);
        printf("Presiona Enter para regresar al menú principal...");
        getchar();
        initscr();
        return 0;
    }
}

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
}

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

int procesar_movimiento(char destino, int *jugador_x, int *jugador_y, int new_x, int new_y)
{
    if (destino == '#')
    {
        return 0;
    }
    if (destino == '$')
    {
        mvprintw(FILAS + 1, 0, "Tesoro recogido!");
        refresh();
        sleep(0.2);
    }
    if (destino == 'A' || destino == 'J')
    {
        mvprintw(FILAS + 1, 0, "Colisión con otro jugador!");
        refresh();
        sleep(0.2);
        return 0;
    }
    *jugador_x = new_x;
    *jugador_y = new_y;
    return 1;
}

void mostrar_mapa_real()
{
    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_MAGENTA, COLOR_MAGENTA);
        init_pair(2, COLOR_RED, COLOR_YELLOW);
        init_pair(3, COLOR_BLACK, COLOR_GREEN);
        init_pair(4, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(5, COLOR_YELLOW, COLOR_BLACK);
    }

    if (conectar_al_servidor() != 0) {
        mvprintw(FILAS + 3, 0, "Error: No se pudo conectar al servidor");
        refresh();
        getch();
        return;
    }

    int fd;
    size_t size;
    char *mapa = inicializar_memoria_mapa(&fd, &size);
    if (!mapa)
        return;

    int mailbox_solicitudes_id = msgget(selected_mailbox, 0666);

    int jugador_x = 1, jugador_y = 1;
    buscar_posicion_inicial(mapa, &jugador_x, &jugador_y);

    int ch;
    keypad(stdscr, TRUE);
    curs_set(0);
    timeout(100);

    while ((ch = getch()) != 'q') {
        if (ch == ERR) {
            msync(mapa, FILAS * COLUMNAS, MS_ASYNC);
            dibujar_mapa_coloreado(mapa, FILAS, COLUMNAS, jugador_x, jugador_y, player_character);
            continue;
        }

        int new_x = jugador_x, new_y = jugador_y;
        if (ch == KEY_UP) new_y--;
        if (ch == KEY_DOWN) new_y++;
        if (ch == KEY_LEFT) new_x--;
        if (ch == KEY_RIGHT) new_x++;

        if (new_y < 0 || new_y >= FILAS || new_x < 0 || new_x >= COLUMNAS)
            continue;

        char destino = mapa[new_y * COLUMNAS + new_x];

        if (procesar_movimiento(destino, &jugador_x, &jugador_y, new_x, new_y)) {
            enviar_movimiento_al_servidor(jugador_x, jugador_y, key_respuestas_global, mailbox_solicitudes_id);
        }

        dibujar_mapa_coloreado(mapa, FILAS, COLUMNAS, jugador_x, jugador_y, player_character);
    }

    desconectar_del_servidor();
    munmap(mapa, size);
    close(fd);
}

void jugar()
{
    if (conectar_servidor(nombre_catacumba, tipo_jugador) != 0)
    {
        printf("No se pudo conectar al servidor.\n");
        return;
    }

    int x = 1, y = 1;
    int jugando = 1;
    while (jugando)
    {
        if (enviar_movimiento(x, y, tipo_jugador) == 0)
        {
            char mensaje[256];
            int codigo;
            if (recibir_respuesta(mensaje, &codigo) == 0)
            {
                mvprintw(22, 0, "%s", mensaje);
                refresh();
                if (codigo == ST_GAME_OVER)
                    jugando = 0;
            }
        }
    }

    desconectar_servidor();
}

void dibujar_mapa_coloreado(const char *mapa, int filas, int columnas, int jugador_x, int jugador_y, char playerChar)
{
    attron(COLOR_PAIR(4));
    mvprintw(1, 2, "=== MAPA DE CATACUMBAS ===");
    attroff(COLOR_PAIR(4));

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
                attron(COLOR_PAIR(2) | A_BOLD);
                mvaddch(y + 4, x + 2, c);
                attroff(COLOR_PAIR(2) | A_BOLD);
            }
            else
            {
                mvaddch(y + 4, x + 2, c);
            }
        }
    }

    attron(COLOR_PAIR(2) | A_BOLD);
    mvaddch(jugador_y + 4, jugador_x + 2, playerChar);
    attroff(COLOR_PAIR(2) | A_BOLD);

    attron(COLOR_PAIR(5));
    mvprintw(filas + 6, 2, "Controles: flechas = Mover, 'q' = Salir");
    attroff(COLOR_PAIR(5));

    refresh();
}
