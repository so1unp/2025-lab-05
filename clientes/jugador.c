/**
 * @file jugador.c
 * @brief Módulo de juego principal - Lógica de juego y comunicación con servidor
 * @author Equipo de desarrollo
 * @date 2025
 * 
 * Este archivo contiene toda la lógica de juego del cliente, incluyendo:
 * - Conexión y comunicación con el servidor de catacumbas
 * - Manejo de hilos para renderizado y entrada del usuario
 * - Procesamiento de movimientos y colisiones
 * - Gestión de señales del sistema
 * - Interfaz gráfica del juego usando ncurses
 */

#define _GNU_SOURCE

// ============================================================================
// INCLUDES DEL PROYECTO
// ============================================================================
#include "../catacumbas/catacumbas.h"

// ============================================================================
// INCLUDES DEL SISTEMA
// ============================================================================
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ncurses.h>
#include <pthread.h>
#include <signal.h>
#include "status.h"

extern void mostrar_game_over();
// ============================================================================
// VARIABLES GLOBALES COMPARTIDAS (definidas en main.c)
// ============================================================================

/** @brief Rol seleccionado por el jugador */
extern char selected_role[50];

/** @brief Carácter que representa al jugador */
extern char player_character;

/** @brief Ruta de memoria compartida del mapa */
extern char selected_shm_path[128];

/** @brief ID del mailbox del servidor */
extern int selected_mailbox;

/** @brief Clave para mailbox de respuestas */
extern key_t key_respuestas_global;

/** @brief Puntero a memoria compartida del mapa */
extern char *mapa;

/** @brief Descriptor de archivo de memoria compartida */
extern int fd;

// ============================================================================
// VARIABLES GLOBALES DEL HILO DE JUEGO
// ============================================================================

/** @brief Flag que controla el bucle principal del juego */
int running;

/** @brief Mutex para proteger las posiciones del jugador */
pthread_mutex_t pos_mutex;

/** @brief Posición global del jugador en el mapa */
int jugador_x_global, jugador_y_global;

/** @brief ID del mailbox para solicitudes al servidor */
int mailbox_solicitudes_id_global;

/** @brief ID del mailbox para recibir respuestas del servidor */
int mailbox_respuestas_global;

/** @brief Hilos para renderizado y entrada de usuario */
pthread_t thread_refresco, thread_entrada;

// ============================================================================
// DECLARACIONES DE FUNCIONES PRIVADAS
// ============================================================================

/** @brief Conecta al servidor de catacumbas */
int conectar_al_servidor();

/** @brief Desconecta del servidor y limpia recursos */
void desconectar_del_servidor();

/** @brief Envía un movimiento del jugador al servidor */
void enviar_movimiento_al_servidor(int jugador_x, int jugador_y, key_t clave_mailbox_respuestas, int mailbox_solicitudes_id);

/** @brief Inicializa la memoria compartida del mapa */
int inicializar_memoria_mapa();

void enviar_captura_tesoro_al_servidor(int jugador_x, int jugador_y, key_t clave_mailbox_respuestas, int mailbox_solicitudes_id);

int contar_tesoros_restantes();

/** @brief Procesa la lógica de movimiento y colisiones */
int procesar_movimiento(char destino, int *jugador_x, int *jugador_y, int new_x, int new_y);

/** @brief Muestra la pantalla de victoria por tesoros */
void mostrar_pantalla_victoria_tesoros();

void notificar_evento_juego(int code, const char *texto);

/** @brief Hilo dedicado al renderizado continuo del mapa */
void *hilo_refresco(void *arg);

/** @brief Hilo dedicado a la captura de entrada del usuario */
void *hilo_entrada(void *arg);

/** @brief Renderiza el mapa con colores en la pantalla */
void dibujar_mapa_coloreado();

/** @brief Termina la partida y limpia todos los recursos */
void terminarPartida();

/** @brief Maneja la señal SIGINT (Ctrl+C) */
void manejar_sigint(int signal);

/** @brief Configura el manejo de señales del sistema */
void configurar_senales();

/** @brief Muestra la pantalla de derrota */
void mostrar_pantalla_derrota();

// ============================================================================
// FUNCIONES DE COMUNICACIÓN CON EL SERVIDOR
// ============================================================================

/**
 * @brief Establece conexión con el servidor de catacumbas
 * @return 0 si la conexión es exitosa, -1 en caso de error
 * 
 * Esta función realiza el proceso completo de conexión al servidor:
 * 1. Crea un mailbox único para recibir respuestas del servidor
 * 2. Se conecta al mailbox de solicitudes del servidor
 * 3. Envía una solicitud de conexión con el tipo de jugador
 * 4. Espera y procesa la respuesta del servidor
 * 
 * En caso de error, limpia automáticamente los recursos creados.
 */
int conectar_al_servidor() {
    // Crear clave única basada en el PID del proceso
    key_t key_respuestas = ftok("/tmp", getpid());
    mailbox_respuestas_global = msgget(key_respuestas, IPC_CREAT | 0666);
    key_respuestas_global = key_respuestas;
    
    if (mailbox_respuestas_global == -1) {
        perror("Error creando mailbox de respuestas");
        return -1;
    }
    
    // Preparar solicitud de conexión
    struct SolicitudServidor solicitud;
    solicitud.mtype = getpid();
    solicitud.codigo = CONEXION;
    solicitud.clave_mailbox_respuestas = key_respuestas;
    solicitud.fila = 0;
    solicitud.columna = 0;
    solicitud.tipo = player_character;
    
    // Conectar al mailbox del servidor
    mailbox_solicitudes_id_global = msgget(selected_mailbox, 0666);
    if (mailbox_solicitudes_id_global == -1) {
        perror("Error conectando al mailbox del servidor");
        // Limpiar mailbox de respuestas si falló la conexión
        msgctl(mailbox_respuestas_global, IPC_RMID, NULL);
        mailbox_respuestas_global = -1;
        return -1;
    }
    
    // Enviar solicitud de conexión
    if (msgsnd(mailbox_solicitudes_id_global, &solicitud, sizeof(solicitud) - sizeof(long), 0) == -1) {
        perror("Error enviando solicitud de conexión");
        return -1;
    }
    
    // Esperar respuesta del servidor
    struct RespuestaServidor respuesta;
    if (msgrcv(mailbox_respuestas_global, &respuesta, sizeof(respuesta) - sizeof(long), 
               getpid(), 0) == -1) {
        perror("Error recibiendo respuesta de conexión");
        return -1;
    }
    
    // Verificar que la conexión fue exitosa
    if (respuesta.codigo != S_OK) {
        mvprintw(FILAS + 4, 0, "Error del servidor: %s", respuesta.mensaje);
        return -1;
    }
    
    mvprintw(FILAS + 4, 0, "Conectado: %s", respuesta.mensaje);
    refresh();
    
    return 0;
}

/**
 * @brief Desconecta del servidor y limpia los recursos de comunicación
 * 
 * Esta función realiza una desconexión limpia del servidor:
 * 1. Envía una solicitud de desconexión al servidor
 * 2. Cierra y elimina el mailbox de respuestas
 * 3. Resetea los identificadores de mailbox
 * 4. Muestra mensaje de confirmación al usuario
 */
void desconectar_del_servidor() {
    // Preparar solicitud de desconexión
    struct SolicitudServidor solicitud;
    solicitud.mtype = getpid();
    solicitud.codigo = DESCONEXION;
    solicitud.clave_mailbox_respuestas = key_respuestas_global;
    solicitud.tipo = player_character;

    // Enviar solicitud de desconexión (no bloqueante)
    if (mailbox_solicitudes_id_global != -1) {
        msgsnd(mailbox_solicitudes_id_global, &solicitud, sizeof(solicitud) - sizeof(long), IPC_NOWAIT);
        mailbox_solicitudes_id_global = -1;
    }
    
    // Limpiar mailbox de respuestas
    if (mailbox_respuestas_global != -1) {
        msgctl(mailbox_respuestas_global, IPC_RMID, NULL);
        mailbox_respuestas_global = -1;
    }
    
    mvprintw(FILAS + 4, 0, "Desconectado del servidor");
    refresh();
}
/**
 * @brief Envía un movimiento del jugador al servidor y procesa la respuesta
 * @param jugador_x Nueva posición X del jugador
 * @param jugador_y Nueva posición Y del jugador
 * @param clave_mailbox_respuestas Clave del mailbox para respuestas
 * @param mailbox_solicitudes_id ID del mailbox de solicitudes
 * 
 * Esta función envía las coordenadas del movimiento al servidor y
 * procesa inmediatamente la respuesta para mostrar el resultado
 * al jugador (éxito, error, victoria, etc.).
 */
void enviar_movimiento_al_servidor(int jugador_x, int jugador_y, key_t clave_mailbox_respuestas, int mailbox_solicitudes_id)
{
    // Preparar solicitud de movimiento
    struct SolicitudServidor solicitud;
    solicitud.mtype = getpid();
    solicitud.codigo = MOVIMIENTO;
    solicitud.clave_mailbox_respuestas = key_respuestas_global;
    solicitud.fila = jugador_y;
    solicitud.columna = jugador_x;
    solicitud.tipo = player_character;
    
    // Enviar movimiento al servidor (no bloqueante)
    if (msgsnd(mailbox_solicitudes_id, &solicitud, sizeof(solicitud) - sizeof(long), IPC_NOWAIT) == -1) {
        perror("Error enviando movimiento");
    }
    
    // Intentar recibir respuesta inmediata del servidor
    struct RespuestaServidor respuesta;
    if (msgrcv(mailbox_respuestas_global, &respuesta, sizeof(respuesta) - sizeof(long), 
               getpid(), IPC_NOWAIT) != -1) {
        
        // Procesar diferentes tipos de respuesta del servidor
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

// ============================================================================
// FUNCIONES DE GESTIÓN DE MEMORIA COMPARTIDA
// ============================================================================

/**
 * @brief Inicializa el mapeo de memoria compartida del mapa de juego
 * @return 0 si la inicialización es exitosa, -1 en caso de error
 * 
 * Esta función:
 * 1. Abre el archivo de memoria compartida del mapa usando la ruta seleccionada
 * 2. Mapea la memoria compartida en el espacio de direcciones del proceso
 * 3. Configura el acceso de solo lectura para el cliente
 * 
 * En caso de error, muestra un mensaje al usuario y limpia los recursos.
 */
int inicializar_memoria_mapa()
{
    // Abrir el objeto de memoria compartida
    fd = shm_open(selected_shm_path, O_RDONLY, 0666);
    if (fd == -1)
    {
        endwin();
        perror("Error abriendo memoria compartida del mapa");
        printf("Presiona Enter para continuar...");
        getchar();
        initscr();
        return -1;
    }
    
    // Mapear la memoria compartida en el espacio de direcciones del proceso
    size_t size = FILAS * COLUMNAS;
    mapa = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
    if (mapa == MAP_FAILED)
    {
        endwin();
        perror("Error mapeando memoria compartida del mapa");
        printf("Presiona Enter para continuar...");
        getchar();
        initscr();
        close(fd);
        return -1;
    }
    
    close(fd);
    return 0;
}

// ============================================================================
// FUNCIONES DE LÓGICA DE JUEGO
// ============================================================================

/**
 * @brief Procesa la lógica de movimiento y colisiones del jugador
 * @param destino Carácter en la posición de destino del movimiento
 * @param jugador_x Puntero a la posición X actual del jugador
 * @param jugador_y Puntero a la posición Y actual del jugador
 * @param new_x Nueva posición X solicitada
 * @param new_y Nueva posición Y solicitada
 * @return 1 si el movimiento es válido, 0 si debe ser bloqueado
 * 
 * Esta función implementa todas las reglas de movimiento del juego:
 * - Bloquea movimientos hacia paredes
 * - Permite a raiders recoger tesoros
 * - Bloquea a guardianes sobre tesoros
 * - Maneja colisiones entre jugadores de diferentes tipos
 * - Bloquea colisiones entre jugadores del mismo tipo
 */
int procesar_movimiento(char destino, int *jugador_x, int *jugador_y, int new_x, int new_y)
{
    // Verificar colisión con paredes
    if (destino == PARED)
    {
        return 0;
    }
    
      // Manejo de tesoros
    if (destino == TESORO)
    {
        if (player_character == RAIDER) {
            // Los raiders pueden recoger tesoros
            // Enviar mensaje TESORO_CAPTURADO al servidor
            struct SolicitudServidor solicitud;
            solicitud.mtype = getpid();
            solicitud.codigo = TESORO_CAPTURADO;
            solicitud.clave_mailbox_respuestas = key_respuestas_global;
            solicitud.fila = new_y;
            solicitud.columna = new_x;
            solicitud.tipo = player_character;
            // Enviar al mailbox del servidor
            msgsnd(mailbox_solicitudes_id_global, &solicitud, sizeof(solicitud) - sizeof(long), 0);

        } else {
            // Los guardianes no pueden moverse sobre tesoros
            return 0;
        }
    }
    
    // Lógica de colisión entre jugadores
    if ((player_character == GUARDIAN && destino == RAIDER)) {
        // Guardián captura a raider
        struct SolicitudServidor solicitud;
        solicitud.mtype = getpid();
        solicitud.codigo = RAIDER_CAPTURADO;
        solicitud.clave_mailbox_respuestas = key_respuestas_global;
        solicitud.fila = new_y;
        solicitud.columna = new_x;
        solicitud.tipo = player_character;
        msgsnd(mailbox_solicitudes_id_global, &solicitud, sizeof(solicitud) - sizeof(long), 0);

        *jugador_x = new_x;
        *jugador_y = new_y;
        return 1;
    } else if (player_character == RAIDER && destino == GUARDIAN) {
        // Raider es capturado por guardián
        return 0;
    } else if ((player_character == RAIDER && destino == RAIDER) || 
               (player_character == GUARDIAN && destino == GUARDIAN)) {
        // Colisión entre jugadores del mismo tipo
        return 0;
    }
    
    // Movimiento válido
    *jugador_x = new_x;
    *jugador_y = new_y;
    return 1;
}

/**
 * @brief Notifica un evento del juego al sistema de estado
 * @param code Código del evento (tesoro encontrado, jugador capturado, etc.)
 * @param texto Descripción textual del evento
 * 
 * Envía notificaciones de eventos importantes del juego al sistema
 * de estado para su procesamiento y posible difusión a otros componentes.
 */
void notificar_evento_juego(int code, const char *texto) {
    int status_mailbox = msgget(MAILBOX_STATUS_KEY, 0666);
    struct status_msg msg;
    msg.mtype = TYPE_GAME_EVENT;    //fijate pa esto no lo usamos
    msg.code = code;
    strncpy(msg.text, texto, MAX_MSG-1);
    msg.text[MAX_MSG-1] = '\0';
    msgsnd(status_mailbox, &msg, sizeof(msg) - sizeof(long), 0);
}

/**
 * @brief Muestra la pantalla de victoria cuando no quedan tesoros
 * 
 * Presenta una pantalla de victoria para los exploradores cuando
 * todos los tesoros han sido recolectados del mapa.
 */
void mostrar_pantalla_victoria_tesoros() {
    clear();
    attron(COLOR_PAIR(2) | A_BOLD);
    mvprintw(FILAS / 2, (COLUMNAS - 35) / 2, "¡No hay más tesoros, los exploradores ganan!");
    attroff(COLOR_PAIR(2) | A_BOLD);
    attron(COLOR_PAIR(5));
    mvprintw(FILAS / 2 + 2, (COLUMNAS - 25) / 2, "Presiona cualquier tecla...");
    attroff(COLOR_PAIR(5));
    refresh();
    getch();
}

/**
 * @brief Muestra la pantalla de derrota
 * 
 * Presenta una pantalla de derrota cuando el jugador es capturado
 * por un guardián.
 */
void mostrar_pantalla_derrota() {
    clear();
    attron(COLOR_PAIR(1) | A_BOLD);
    mvprintw(FILAS / 2, (COLUMNAS - 9) / 2, "GAME OVER");
    attroff(COLOR_PAIR(1) | A_BOLD);
    attron(COLOR_PAIR(5));
    mvprintw(FILAS / 2 + 2, (COLUMNAS - 30) / 2, "¡Has sido capturado por un guardián!");
    mvprintw(FILAS / 2 + 4, (COLUMNAS - 30) / 2, "Presiona cualquier tecla...");
    attroff(COLOR_PAIR(5));
    sleep(5);
    refresh();
    getch();
}

// ============================================================================
// FUNCIONES DE HILOS DE JUEGO
// ============================================================================

/**
 * @brief Hilo dedicado al renderizado continuo del mapa de juego
 * @param arg Argumentos del hilo (no utilizados)
 * @return NULL al finalizar el hilo
 * 
 * Este hilo se ejecuta continuamente mientras el juego está activo,
 * actualizando la visualización del mapa cada 50ms para mantener
 * una experiencia de juego fluida.
 */
void *hilo_refresco(void *arg) {
    static int pantalla_final_mostrada = 0;
    while (running) {
        dibujar_mapa_coloreado();

        // Mostrar mensaje final si no quedan tesoros
        if (!pantalla_final_mostrada && contar_tesoros_restantes() == 0) {
            pantalla_final_mostrada = 1;
            clear();
            if (player_character == RAIDER) {
                attron(COLOR_PAIR(2) | A_BOLD);
                mvprintw(FILAS / 2, (COLUMNAS - 20) / 2, "¡Has ganado!");
                attroff(COLOR_PAIR(2) | A_BOLD);
                attron(COLOR_PAIR(5));
                mvprintw(FILAS / 2 + 2, (COLUMNAS - 30) / 2, "Pulsa 'q' para salir...");
                attroff(COLOR_PAIR(5));
            } else if (player_character == GUARDIAN) {
                attron(COLOR_PAIR(1) | A_BOLD);
                mvprintw(FILAS / 2 + 2, (COLUMNAS - 30) / 2, "¡Derrota! Los exploradores ganan.");
                attroff(COLOR_PAIR(1) | A_BOLD);
                attron(COLOR_PAIR(5));
                mvprintw(FILAS / 2 + 2, (COLUMNAS - 30) / 2, "Pulsa 'q' para salir...");
                attroff(COLOR_PAIR(5));
            }
            refresh();

            // Bloquear todas las teclas excepto 'q'
            int ch;
            do {
                ch = getch();
                usleep(50000);
            } while (ch != 'q');
            running = 0;
        }

        usleep(50000); // 50ms para 20 FPS
    }
    return NULL;
}

/**
 * @brief Hilo dedicado a la captura y procesamiento de entrada del usuario
 * @param arg Argumentos del hilo (no utilizados)
 * @return NULL al finalizar el hilo
 * 
 * Este hilo maneja toda la entrada del teclado:
 * - Captura las teclas de movimiento (flechas)
 * - Procesa la tecla 'q' para salir
 * - Valida movimientos y comunica con el servidor
 * - Maneja eventos especiales del juego (victoria, etc.)
 */
void *hilo_entrada(void *arg) {
    int ch;
    int fin_partida = 0;
    
    while (running && !fin_partida) {
        ch = getch();
        
        // Tecla de salida
        if (ch == 'q') {
            running = 0;
            break;
        }

        // Proteger acceso a posiciones del jugador
        pthread_mutex_lock(&pos_mutex);
        int new_x = jugador_x_global, new_y = jugador_y_global;
        
        // Procesar teclas de movimiento
        if (ch == KEY_UP) new_y--;
        if (ch == KEY_DOWN) new_y++;
        if (ch == KEY_LEFT) new_x--;
        if (ch == KEY_RIGHT) new_x++;

        // Validar límites del mapa
        if (new_y < 0 || new_y >= FILAS || new_x < 0 || new_x >= COLUMNAS) {
            pthread_mutex_unlock(&pos_mutex);
            continue;
        }

        // Obtener carácter en la posición de destino
        char destino = mapa[new_y * COLUMNAS + new_x];

        // Procesar movimiento si es válido
        if (procesar_movimiento(destino, &jugador_x_global, &jugador_y_global, new_x, new_y)) {
            enviar_movimiento_al_servidor(jugador_x_global, jugador_y_global, key_respuestas_global, mailbox_solicitudes_id_global);
        }


        struct RespuestaServidor evento;
        if (mailbox_respuestas_global != -1) {
            if (msgrcv(mailbox_respuestas_global, &evento, sizeof(evento) - sizeof(long), getpid(), IPC_NOWAIT) != -1) {
                if (evento.codigo == MUERTO) {
                    running = 0; // Detener el hilo de refresco
                    pthread_mutex_unlock(&pos_mutex);
                    mostrar_game_over();
                    fin_partida = 1;
                    break;
                }
            }
        }
        pthread_mutex_unlock(&pos_mutex);
    }
    return NULL;
}

// ============================================================================
// FUNCIONES DE RENDERIZADO Y VISUALIZACIÓN
// ============================================================================

/**
 * @brief Renderiza el mapa de juego con colores en la pantalla
 * 
 * Esta función dibuja el estado completo del mapa usando ncurses:
 * - Paredes en color magenta
 * - Tesoros en color amarillo sobre rojo
 * - Espacios vacíos en verde
 * - Jugadores en amarillo brillante
 * - Información de estado y controles
 * 
 * Utiliza diferentes pares de colores definidos para distinguir
 * visualmente los elementos del juego.
 */
void dibujar_mapa_coloreado()
{
    clear(); // Limpiar pantalla
    
    // Título del juego
    attron(COLOR_PAIR(4));
    mvprintw(1, 2, "=== MAPA DE CATACUMBAS === ROL: %s", selected_role);
    attroff(COLOR_PAIR(4));

    // Renderizar cada celda del mapa con su color correspondiente
    for (int y = 0; y < FILAS; y++)
    {
        for (int x = 0; x < COLUMNAS; x++)
        {
            char c = mapa[y * COLUMNAS + x];
            
            // Aplicar colores según el tipo de celda
            if (c == PARED)
            {
                attron(COLOR_PAIR(1));
                mvaddch(y + 4, x + 2, c);
                attroff(COLOR_PAIR(1));
            }
            else if (c == TESORO)
            {
                attron(COLOR_PAIR(2));
                mvaddch(y + 4, x + 2, c);
                attroff(COLOR_PAIR(2));
            }
            else if (c == VACIO)
            {
                attron(COLOR_PAIR(3));
                mvaddch(y + 4, x + 2, c);
                attroff(COLOR_PAIR(3));
            }
            else if (c == RAIDER || c == GUARDIAN)
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

    // Mostrar información de controles y estado
    attron(COLOR_PAIR(5));
    mvprintw(FILAS + 6, 2, "Controles: flechas = Mover, 'q' = Salir");
    mvprintw(FILAS + 7, 2, "Posición: [%d, %d]", jugador_x_global, jugador_y_global);

    // Mostrar contador de tesoros
    int tesoros = contar_tesoros_restantes();
    mvprintw(FILAS + 8, 2, "Tesoros restantes: %d", tesoros);

    attroff(COLOR_PAIR(5));
    
    refresh();
}

// ============================================================================
// FUNCIONES DE GESTIÓN DE RECURSOS Y LIMPIEZA
// ============================================================================

/**
 * @brief Termina la partida y libera todos los recursos utilizados
 * 
 * Esta función realiza una limpieza completa del sistema:
 * - Cierra la interfaz de ncurses
 * - Libera la memoria compartida del mapa
 * - Cierra descriptores de archivo
 * - Desconecta del servidor
 */
void terminarPartida() {
    endwin();
    
    // Liberar memoria compartida del mapa
    if (mapa != NULL && mapa != MAP_FAILED) {
        munmap(mapa, FILAS * COLUMNAS);
        mapa = NULL;
    }
    
    // Cerrar descriptor de archivo
    if (fd != -1) {
        close(fd);
        fd = -1;
    }
    
    // Desconectar del servidor
    desconectar_del_servidor();
}


// ============================================================================
// FUNCIONES DE MANEJO DE SEÑALES
// ============================================================================
/**
 * @brief Maneja la señal SIGINT (Ctrl+C) para terminar el juego limpiamente
 * @param signal Número de la señal recibida
 * 
 * Esta función se ejecuta cuando el usuario presiona Ctrl+C:
 * - Detiene el bucle principal del juego
 * - Espera a que terminen los hilos de forma segura
 * - Limpia todos los recursos
 * - Cierra ncurses y termina el programa
 */
void manejar_sigint(int signal) {
    running = 0;
    
    // Esperar a que terminen los hilos de forma segura
    if (pthread_join(thread_entrada, NULL) == 0) {
        // thread_entrada terminó correctamente
    }
    if (pthread_join(thread_refresco, NULL) == 0) {
        // thread_refresco terminó correctamente
    }
    
    // Terminar la partida y desconectar
    terminarPartida();
    
    endwin(); // Asegurar que ncurses se cierre correctamente
    printf("\nPartida terminada por el usuario.\n");
    exit(0);
}

/**
 * @brief Configura el manejo de señales del sistema
 * 
 * Establece el manejador personalizado para SIGINT (Ctrl+C)
 * para permitir una terminación limpia del programa.
 */
void configurar_senales() {
    struct sigaction sa;
    sa.sa_handler = manejar_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
}

// ============================================================================
// FUNCIÓN PRINCIPAL DEL JUEGO
// ============================================================================

/**
 * @brief Función principal que ejecuta el bucle completo del juego
 * 
 * Esta función coordina todo el flujo del juego:
 * 1. Inicializa los colores de ncurses
 * 2. Configura el manejo de señales
 * 3. Se conecta al servidor de catacumbas
 * 4. Inicializa la memoria compartida del mapa
 * 5. Configura las variables globales del juego
 * 6. Crea y gestiona los hilos de renderizado y entrada
 * 7. Limpia todos los recursos al finalizar
 * 
 * Es la función principal que se llama desde main.c cuando
 * el usuario selecciona jugar en una catacumba específica.
 */
void jugar()
{
    // Inicializar colores de ncurses si están disponibles
    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_MAGENTA, COLOR_MAGENTA);  // Paredes
        init_pair(2, COLOR_RED, COLOR_YELLOW);       // Tesoros y jugadores
        init_pair(3, COLOR_BLACK, COLOR_GREEN);      // Espacios vacíos
        init_pair(4, COLOR_MAGENTA, COLOR_BLACK);    // Título
        init_pair(5, COLOR_YELLOW, COLOR_BLACK);     // Información
    }

    // Configurar manejo de señales para terminación limpia
    configurar_senales();

    // Conectar al servidor de catacumbas
    if (conectar_al_servidor() != 0) {
        mvprintw(FILAS + 3, 0, "Error: No se pudo conectar al servidor");
        refresh();
        getch();
        return;
    }

    // Inicializar acceso a memoria compartida del mapa
    if (inicializar_memoria_mapa() != 0) {
        desconectar_del_servidor(); // Limpiar conexión si falla el mapa
        return;
    }

    // Inicializar variables globales del juego
    jugador_x_global = 1;
    jugador_y_global = 1;
    running = 1;

    // Configurar ncurses para el juego
    keypad(stdscr, TRUE);  // Habilitar teclas especiales (flechas)
    curs_set(0);           // Ocultar cursor
    timeout(100);          // Timeout para getch()

    // Crear hilos para renderizado y entrada
    pthread_create(&thread_refresco, NULL, hilo_refresco, NULL);
    pthread_create(&thread_entrada, NULL, hilo_entrada, NULL);

    // Esperar a que termine el hilo de entrada (cuando se presiona 'q')
    pthread_join(thread_entrada, NULL);
    running = 0;
    
    // Esperar a que termine el hilo de renderizado
    pthread_join(thread_refresco, NULL);

    // Limpiar recursos y terminar
    terminarPartida();
}

void enviar_captura_tesoro_al_servidor(int jugador_x, int jugador_y, key_t clave_mailbox_respuestas, int mailbox_solicitudes_id)
{
    struct SolicitudServidor solicitud;
    solicitud.mtype = getpid();
    solicitud.codigo = TESORO_CAPTURADO; // Debes definir este código en tu protocolo
    solicitud.clave_mailbox_respuestas = key_respuestas_global;
    solicitud.fila = jugador_y;
    solicitud.columna = jugador_x;
    solicitud.tipo = player_character;

    if (msgsnd(mailbox_solicitudes_id, &solicitud, sizeof(solicitud) - sizeof(long), IPC_NOWAIT) == -1) {
        perror("Error enviando captura de tesoro");
    }
}

int contar_tesoros_restantes() {
    int count = 0;
    for (int y = 0; y < FILAS; y++) {
        for (int x = 0; x < COLUMNAS; x++) {
            if (mapa[y * COLUMNAS + x] == TESORO)
                count++;
        }
    }
    return count;
}