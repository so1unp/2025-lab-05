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

extern void mostrar_game_over();
extern void mostrar_pantalla_victoria_guardian();
/** @brief Muestra la pantalla de derrota */
extern void mostrar_pantalla_derrota();

/** @brief Muestra la pantalla de victoria por tesoros */
extern void mostrar_pantalla_victoria_tesoros();
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

/**
 * @brief Flag para evitar doble liberación de recursos
 *
 * Esta variable se utiliza para asegurarse de que la función terminarPartida()
 * solo limpie los recursos (memoria compartida, ncurses, mailboxes, etc.)
 * una sola vez, incluso si es llamada desde varios hilos o por señales.
 * 
 * Evita errores como segmentation fault por doble free o doble endwin().
 */
int recursos_limpiados = 0;

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
    /* static int pantalla_final_mostrada = 0; */
    while (running) {
        dibujar_mapa_coloreado();
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
    
    while (running) {
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
                    pthread_join(thread_refresco, NULL); // Espera a que termine el hilo de refresco
                    mostrar_game_over();
                    break;
                }                
                if (evento.codigo == SIN_RAIDERS) {
                    running = 0;
                    pthread_mutex_unlock(&pos_mutex);
                    pthread_join(thread_refresco, NULL); // Espera a que termine el hilo de refresco
                    if (player_character == GUARDIAN) {
                        mostrar_pantalla_victoria_guardian();
                    }
                    break;
                }
                if (evento.codigo == SIN_TESOROS) {
                    running = 0;
                    pthread_mutex_unlock(&pos_mutex);
                    pthread_join(thread_refresco, NULL); // Espera a que termine el hilo de refresco
                    
                    if (player_character == RAIDER) {
                        mostrar_pantalla_victoria_tesoros();
                    } else if (player_character == GUARDIAN) {
                        mostrar_pantalla_derrota();
                    }
                    break;
                }
            }
        }
        pthread_mutex_unlock(&pos_mutex);
    }
    return NULL;
}

WINDOW *ventana_mapa = NULL; // Ventana secundaria para el mapa
// ============================================================================
// FUNCIONES DE RENDERIZADO Y VISUALIZACIÓN
// ============================================================================

/**
 * @brief Renderiza el mapa de juego con colores en la ventana secundaria.
 *
 * Esta función utiliza la ventana secundaria `ventana_mapa` para dibujar el estado
 * actual del mapa de la catacumba, aplicando diferentes colores a cada tipo de celda:
 * - Paredes: Gris oscuro
 * - Espacios vacíos: Gris claro
 * - Título e información: Verde lima
 * - Raiders: Verde
 * - Guardianes: Rojo
 * - Tesoros: Amarillo claro
 *
 * El jugador local se representa con la letra 'J'. El resto de los elementos
 * se dibujan según el carácter presente en la memoria compartida del mapa.
 * Al final, se muestra información de controles y la posición actual del jugador.
 *
 * El refresco se realiza usando `wnoutrefresh()` y `doupdate()` para evitar parpadeos.
 */
void dibujar_mapa_coloreado()
{
    werase(ventana_mapa); // Limpiar solo la ventana

    // Título del juego
    wattron(ventana_mapa, COLOR_PAIR(3));
    mvwprintw(ventana_mapa, 1, 2, "=== MAPA DE CATACUMBAS === ROL: %s", selected_role);
    wattroff(ventana_mapa, COLOR_PAIR(3));

    // Renderizar cada celda del mapa con su color correspondiente
    for (int y = 0; y < FILAS; y++)
    {
        for (int x = 0; x < COLUMNAS; x++)
        {
            char c = mapa[y * COLUMNAS + x];

            if (y == jugador_y_global && x == jugador_x_global) {
                wattron(ventana_mapa, COLOR_PAIR(4) | A_BOLD);
                mvwaddch(ventana_mapa, y + 4, x + 2, 'J');
                wattroff(ventana_mapa, COLOR_PAIR(4) | A_BOLD);
            }
            else if (c == PARED)
            {
                wattron(ventana_mapa, COLOR_PAIR(1));
                mvwaddch(ventana_mapa, y + 4, x + 2, c);
                wattroff(ventana_mapa, COLOR_PAIR(1));
            }
            else if (c == TESORO)
            {
                wattron(ventana_mapa, COLOR_PAIR(7));
                mvwaddch(ventana_mapa, y + 4, x + 2, c);
                wattroff(ventana_mapa, COLOR_PAIR(7));
            }
            else if (c == VACIO)
            {
                wattron(ventana_mapa, COLOR_PAIR(2));
                mvwaddch(ventana_mapa, y + 4, x + 2, c);
                wattroff(ventana_mapa, COLOR_PAIR(2));
            }
            else if (c == RAIDER)
            {
                wattron(ventana_mapa, COLOR_PAIR(5) | A_BOLD);
                mvwaddch(ventana_mapa, y + 4, x + 2, c);
                wattroff(ventana_mapa, COLOR_PAIR(5) | A_BOLD);
            }
            else if (c == GUARDIAN)
            {
                wattron(ventana_mapa, COLOR_PAIR(6) | A_BOLD);
                mvwaddch(ventana_mapa, y + 4, x + 2, c);
                wattroff(ventana_mapa, COLOR_PAIR(6) | A_BOLD);
            }
            else
            {
                mvwaddch(ventana_mapa, y + 4, x + 2, c);
            }
        }
    }

    // Mostrar información de controles y estado
    wattron(ventana_mapa, COLOR_PAIR(3));
    mvwprintw(ventana_mapa, FILAS + 6, 2, "Controles: flechas = Mover, 'q' = Salir");
    mvwprintw(ventana_mapa, FILAS + 7, 2, "Posición: [%d, %d]", jugador_x_global, jugador_y_global);
    wattroff(ventana_mapa, COLOR_PAIR(3));

    // Refrescar la ventana de forma eficiente
    wnoutrefresh(ventana_mapa);
    doupdate();
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
    if (ventana_mapa != NULL) {
        delwin(ventana_mapa);
        ventana_mapa = NULL;
    }

    if (recursos_limpiados) return;
    recursos_limpiados = 1;
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
    if (has_colors()) {
        start_color();
        use_default_colors();
        init_pair(1, 235, 235);//paredes gris oscuro
        init_pair(2, 8, 8);//fondo/piso gris "claro"
        init_pair(3, 82, -1);//Titulo del mapa y informacion de estado
        init_pair(4, 11, 8);//jugador: caracter naranja fondo gris "claro"
        init_pair(5, 2, 8);//Raider: caracter verde fondo gris "claro"
        init_pair(6, 160, 8); //Guardian: caracter rojo fondo gris "claro"
        init_pair(7, 227, 8); //Tesoro: caracter amarillo fondo gris "claro"
        init_pair(8, 88, -1); //Titulo grande de patanalla Game over
        init_pair(9, 122, -1); //Titulo grande de pantalla Victoria
        init_pair(10, 68, -1); //subtitulos de pantalla (Has sido capturado, Se llevaron todo el tesoro,Has ganado!)
        init_pair(11, 94, -1); //texto de informacion de pantalla (Presiona enter para continuar)
        init_pair(110, 16, -1); //si un texto aparece en negro es porque no tiene definido ningun color

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
    if (player_character == RAIDER) {
        jugador_x_global = 1;
        jugador_y_global = 1;
        // Si en la posicion inicial hay una pared o tesoro, reaparece al lado
        if (mapa[jugador_y_global * COLUMNAS + jugador_x_global] != VACIO && mapa[jugador_y_global * COLUMNAS + jugador_x_global] != RAIDER) {
            for (int x = 1; x < COLUMNAS - 1; x++) {
                if (mapa[1 * COLUMNAS + x] == VACIO || mapa[1 * COLUMNAS + x] == RAIDER) {
                    jugador_x_global = x;
                    break;
                }
            }
        }
    } else if (player_character == GUARDIAN) {
        jugador_x_global = COLUMNAS - 2;
        jugador_y_global = 1;
        if (mapa[jugador_y_global * COLUMNAS + jugador_x_global] != VACIO && mapa[jugador_y_global * COLUMNAS + jugador_x_global] != GUARDIAN) {
            for (int x = COLUMNAS - 2; x > 0; x--) {
                if (mapa[1 * COLUMNAS + x] == VACIO || mapa[1 * COLUMNAS + x] == GUARDIAN) {
                    jugador_x_global = x;
                    break;
                }
            }
        }
    }
    running = 1;

    // Configurar ncurses para el juego
    keypad(stdscr, TRUE);  // Habilitar teclas especiales (flechas)
    curs_set(0);           // Ocultar cursor
    timeout(100);          // Timeout para getch()

    // Crear ventana secundaria para el mapa
    if (ventana_mapa == NULL)
        ventana_mapa = newwin(FILAS + 10, COLUMNAS + 10, 0, 0);

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
