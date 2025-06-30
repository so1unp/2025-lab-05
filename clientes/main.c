/**
 * @file main.c
 * @brief Cliente principal del juego de catacumbas - Sistema de menús y navegación
 * @author Equipo de desarrollo
 * @date 2025
 *
 * Este archivo contiene la implementación del sistema de menús principal del juego,
 * incluyendo la navegación entre opciones, conexión al directorio de servidores,
 * y la gestión de la interfaz de usuario principal.
 */

#define _GNU_SOURCE

// ============================================================================
// INCLUDES DEL SISTEMA
// ============================================================================
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <pthread.h>
#include <signal.h>

// ============================================================================
// INCLUDES DEL PROYECTO
// ============================================================================
#include <ncurses.h>
#include "../directorio/directorio.h"
#include "../catacumbas/catacumbas.h"

// ============================================================================
// CONSTANTES DEL MENU PRINCIPAL
// ============================================================================
#define MENU_PRINCIPAL_ITEMS 5
#define MENU_WIDTH 50

// ============================================================================
// ESTRUCTURAS Y TIPOS DE DATOS
// ============================================================================

/**
 * @struct Map
 * @brief Estructura que representa una catacumba disponible
 *
 * Contiene toda la información necesaria para identificar y conectarse
 * a una catacumba específica en el sistema.
 */
typedef struct
{
    char nombre[64];       /**< Nombre identificativo de la catacumba */
    char direccion[128];   /**< Ruta de memoria compartida del mapa */
    int mailbox;           /**< ID del mailbox para comunicación */
    int players_connected; /**< Número actual de jugadores conectados */
    int max_players;       /**< Número máximo de jugadores permitidos */
} Map;

/**
 * @struct OpcionMenuPrincipal
 * @brief Estructura que define una opción del menú principal
 *
 * Encapsula la información de visualización y funcionalidad de cada
 * opción disponible en el menú principal del juego.
 */
typedef struct
{
    char *texto;       /**< Texto que se muestra en el menú */
    char *descripcion; /**< Descripción detallada de la opción */
    int (*funcion)();  /**< Puntero a función que ejecuta la opción */
} OpcionMenuPrincipal;

// ============================================================================
// VARIABLES GLOBALES DEL SISTEMA
// ============================================================================

/** @brief Tipo de jugador por defecto (RAIDER o GUARDIAN) */
int tipo_jugador = RAIDER;

/** @brief Nombre de la catacumba seleccionada */
char nombre_catacumba[128] = "catacumba1";

/** @brief Rol seleccionado por el jugador */
char selected_role[50] = "NO SELECCIONADO";

/** @brief Mapa seleccionado por el jugador */
char selected_map[50] = "NO SELECCIONADO";

/** @brief Carácter que representa al jugador en el mapa */
char player_character = RAIDER;

/** @brief Ruta de memoria compartida del mapa seleccionado */
char selected_shm_path[128];

/** @brief ID del mailbox del servidor seleccionado */
int selected_mailbox;

/** @brief Posición inicial del jugador en el mapa */
int jugador_x = 1, jugador_y = 1;

/** @brief Clave global para mailbox de respuestas */
key_t key_respuestas_global = 0;

/** @brief Puntero a la memoria compartida del mapa */
char *mapa = NULL;

/** @brief Descriptor de archivo de memoria compartida */
int fd = -1;

// ============================================================================
// DECLARACIONES DE FUNCIONES EXTERNAS
// ============================================================================

/** @brief Función que muestra el juego en modo demo */
extern int mostrar_base(char player_character);

/** @brief Función que muestra el menú de selección de rol */
extern int mostrar_menu_rol();

/** @brief Función que muestra el menú de selección de mapa */
extern int mostrar_seleccion_mapa(Map *maps, int num_maps);

/** @brief Función principal del juego implementada en jugador.c */
extern void jugar();

// ============================================================================
// DECLARACIONES DE FUNCIONES PRIVADAS
// ============================================================================

/** @brief Ejecuta el modo demo del juego */
int ejecutar_base();

/** @brief Ejecuta la selección de mapa desde el directorio */
int ejecutar_seleccion_mapa();

/** @brief Ejecuta la selección de rol del jugador */
int ejecutar_seleccion_rol();

/** @brief Busca y muestra las catacumbas disponibles */
int mostrar_listado_mapas_y_seleccionar();

/** @brief Formatea y muestra la lista de catacumbas */
void mostrar_catacumbas_formateadas(char *datos);

/** @brief Establece el carácter del jugador */
void setPlayChar(char c);

/** @brief Busca catacumbas disponibles y permite seleccionar una */
int buscar_catacumbas_disponibles();

/** @brief Muestra el menú principal del sistema */
int mostrar_menu_principal();

// ============================================================================
// FUNCIONES DE CONFIGURACIÓN DEL JUEGO
// ============================================================================

/**
 * @brief Establece el rol del jugador en el sistema
 * @param role Cadena que representa el rol seleccionado
 *
 * Actualiza la variable global selected_role con el rol proporcionado,
 * asegurando que la cadena esté correctamente terminada.
 */
void set_game_role(const char *role)
{
    if (role != NULL)
    {
        strncpy(selected_role, role, 49);
        selected_role[49] = '\0';
    }
}

/**
 * @brief Establece el mapa seleccionado en el sistema
 * @param map Cadena que representa el nombre del mapa seleccionado
 *
 * Actualiza la variable global selected_map con el mapa proporcionado,
 * asegurando que la cadena esté correctamente terminada.
 */
void set_game_map(const char *map)
{
    if (map != NULL)
    {
        strncpy(selected_map, map, 49);
        selected_map[49] = '\0';
    }
}

// ============================================================================
// FUNCIONES DE COMUNICACIÓN CON EL DIRECTORIO
// ============================================================================
/**
 * @brief Busca catacumbas disponibles en el directorio y permite al jugador conectarse
 * @return 0 en caso de éxito o error, -1 si se debe salir del programa
 *
 * Esta función se conecta al servidor de directorio para obtener la lista
 * de catacumbas disponibles, permite al usuario seleccionar una, y luego
 * inicia la partida en la catacumba seleccionada.
 *
 * Flujo de ejecución:
 * 1. Se conecta a los mailboxes del directorio
 * 2. Solicita la lista de catacumbas disponibles
 * 3. Procesa la respuesta y crea una lista de mapas
 * 4. Permite al usuario seleccionar un mapa
 * 5. Inicia el juego en el mapa seleccionado
 */
int buscar_catacumbas_disponibles()
{
    // Intentar conectar con el servidor de directorio
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

            jugar();
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

// ============================================================================
// FUNCIONES DE INTERFAZ PRINCIPAL
// ============================================================================

/**
 * @brief Muestra y gestiona el menú principal del sistema
 * @return 0 si el usuario elige salir, -1 en caso de error
 *
 * Esta función implementa el bucle principal de la interfaz de usuario:
 * - Presenta las opciones disponibles del sistema
 * - Maneja la navegación con las teclas de flecha
 * - Ejecuta las funciones correspondientes a cada opción
 * - Mantiene el estado de ncurses entre operaciones
 *
 * Las opciones disponibles incluyen:
 * - Juego demo con mapa fijo
 * - Búsqueda de partidas reales
 * - Selección de rol de jugador
 * - Selección de mapa
 * - Salir del programa
 */
int mostrar_menu_principal()
{
    // Definir las opciones del menú principal
    OpcionMenuPrincipal opciones[MENU_PRINCIPAL_ITEMS] = {
        {"Juego (demo)", "Demostración básica del juego con mapa fijo", ejecutar_base},
        {"Buscar Partida", "Conectarse a una catacumba", buscar_catacumbas_disponibles},
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

    if (has_colors())
    {
        start_color();
        use_default_colors();
        init_pair(20, 63, 89);   // Menu principal - CATACUMBAS
        init_pair(21, 63, -1);   // Seleccionar opcion
        init_pair(22, 184, -1);  // opciones del menu
        init_pair(23, 17, 135); // mensajes de opciones del menu
        init_pair(24, 82, -1);   // informacion
        init_pair(25, 53, 60);   // items
        init_pair(24, 82, -1);   // informacion

    }
    // Bucle principal del menú
    while (1)
    {
        getmaxyx(stdscr, max_y, max_x);
        clear();

        attron(COLOR_PAIR(20) | A_BOLD);
        mvprintw(max_y / 2 - 8, (max_x - strlen("=== MENU PRINCIPAL - CATACUMBAS ===")) / 2,
                 "=== MENU PRINCIPAL - CATACUMBAS ===");
        attroff(COLOR_PAIR(20) | A_BOLD);

        attron(COLOR_PAIR(21));
        mvprintw(max_y / 2 - 6, (max_x - strlen("Selecciona una opción")) / 2,
                 "Selecciona una opción");
        attroff(COLOR_PAIR(21));

        for (int i = 0; i < MENU_PRINCIPAL_ITEMS; i++)
        {
            int y_pos = max_y / 2 - 3 + i * 2;
            int x_pos = max_x / 2 - MENU_WIDTH / 2;

            if (i == seleccion)
            {
                //<>
                attron(COLOR_PAIR(23));
                mvprintw(y_pos, x_pos, " > %-30s < ", opciones[i].texto);
                attroff(COLOR_PAIR(23));

                //items
                if (i < MENU_PRINCIPAL_ITEMS - 1)
                {
                    attron(COLOR_PAIR(25));
                    mvprintw(y_pos + 1, x_pos + 3, "%-45s", opciones[i].descripcion);
                    attroff(COLOR_PAIR(25));
                }
            }
            else
            {
                attron(COLOR_PAIR(21));
                mvprintw(y_pos, x_pos, "   %-30s   ", opciones[i].texto);
                attroff(COLOR_PAIR(21));
            }
        }
        attron(COLOR_PAIR(21) | A_BOLD);
        mvprintw(max_y - 2, (max_x - strlen("Presiona 'q' para salir")) / 2,
                 "Presiona 'q' para salir");
        mvprintw(max_y - 4, (max_x - strlen("Usa flechas para navegar, ENTER para seleccionar, ESC para salir")) / 2,
                 "Usa flechas para navegar, ENTER para seleccionar, ESC para salir");
        attroff(COLOR_PAIR(21) | A_BOLD);

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

// ============================================================================
// FUNCIÓN PRINCIPAL DEL PROGRAMA
// ============================================================================

/**
 * @brief Función principal del programa cliente de catacumbas
 * @return 0 si la ejecución es exitosa
 *
 * Punto de entrada del programa que:
 * 1. Muestra un mensaje de bienvenida
 * 2. Inicia el menú principal del sistema
 * 3. Maneja el resultado de la ejecución
 * 4. Muestra un mensaje de despedida
 */
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

// ============================================================================
// FUNCIONES DE EJECUCIÓN DE OPCIONES DEL MENÚ
// ============================================================================

/**
 * @brief Ejecuta la selección de rol del jugador
 * @return El tipo de jugador seleccionado (RAIDER o GUARDIAN)
 *
 * Permite al usuario elegir entre ser un explorador (raider)
 * o un guardián, actualizando las variables globales correspondientes.
 */
int ejecutar_seleccion_rol()
{
    int resultado = mostrar_menu_rol();

    switch (resultado)
    {
    case RAIDER:
        set_game_role("EXPLORADOR");
        setPlayChar(RAIDER);
        break;
    case GUARDIAN:
        set_game_role("GUARDIAN");
        setPlayChar(GUARDIAN);
        break;
    default:
        set_game_role("NO SELECCIONADO");
        setPlayChar(RAIDER);
        break;
    }

    return resultado;
}

/**
 * @brief Ejecuta la selección de mapa desde el directorio
 * @return Índice del mapa seleccionado o estado de la operación
 *
 * Delega la funcionalidad de selección de mapa al sistema
 * de listado que se conecta al directorio de servidores.
 */
int ejecutar_seleccion_mapa()
{
    return mostrar_listado_mapas_y_seleccionar();
}

/**
 * @brief Ejecuta el modo demo del juego
 * @return Resultado de la ejecución del demo
 *
 * Lanza el modo de demostración del juego con un mapa fijo,
 * cerrando temporalmente ncurses para la ejecución.
 */
int ejecutar_base()
{
    endwin();
    clear();
    int resultado = mostrar_base(player_character);

    return resultado;
}

/**
 * @brief Establece el carácter que representa al jugador
 * @param c Carácter del tipo de jugador (RAIDER o GUARDIAN)
 *
 * Actualiza la variable global que define cómo se representa
 * visualmente el jugador en el mapa de juego.
 */
void setPlayChar(char c)
{
    player_character = c;
}

/**
 * @brief Muestra el listado de mapas disponibles y permite seleccionar uno
 * @return Índice del mapa seleccionado o 0 en caso de error
 *
 * Esta función:
 * 1. Se conecta al directorio de servidores
 * 2. Solicita la lista de catacumbas disponibles
 * 3. Procesa la respuesta y crea una estructura de mapas
 * 4. Presenta la lista al usuario para selección
 * 5. Actualiza las variables globales con el mapa seleccionado
 *
 * Maneja todos los errores de comunicación y presenta mensajes
 * informativos al usuario en caso de problemas.
 */
int mostrar_listado_mapas_y_seleccionar()
{
    // Conectar a los mailboxes del directorio
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

    // Preparar solicitud de listado
    struct solicitud msg;
    struct respuesta resp;
    pid_t mi_pid = getpid();

    msg.mtype = mi_pid;
    msg.tipo = OP_LISTAR;
    msg.texto[0] = '\0';

    // Enviar solicitud al directorio
    if (msgsnd(mailbox_solicitudes, &msg, sizeof(msg) - sizeof(long), 0) == -1)
    {
        endwin();
        perror("Error enviando solicitud");
        printf("Presiona Enter para regresar al menú principal...");
        getchar();
        initscr();
        return 0;
    }

    // Recibir respuesta del directorio
    if (msgrcv(mailbox_respuestas, &resp, sizeof(resp) - sizeof(long), mi_pid, 0) == -1)
    {
        endwin();
        perror("Error recibiendo respuesta");
        printf("Presiona Enter para regresar al menú principal...");
        getchar();
        initscr();
        return 0;
    }

    // Procesar respuesta exitosa
    if (resp.codigo == RESP_OK)
    {
        Map maps[20];
        int num_maps = 0;
        char *saveptr1, *saveptr2;

        // Parsear la cadena de respuesta con información de catacumbas
        char *catacumba = strtok_r(resp.datos, ";", &saveptr1);
        while (catacumba && num_maps < 20)
        {
            char *nombre = strtok_r(catacumba, "|", &saveptr2);
            char *direccion = strtok_r(NULL, "|", &saveptr2);
            strtok_r(NULL, "|", &saveptr2); // Saltar campo no usado
            char *mailbox_str = strtok_r(NULL, "|", &saveptr2);
            char *cantJug = strtok_r(NULL, "|", &saveptr2);
            char *maxJug = strtok_r(NULL, "|", &saveptr2);

            // Validar que todos los campos estén presentes
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

        // Verificar que haya mapas disponibles
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