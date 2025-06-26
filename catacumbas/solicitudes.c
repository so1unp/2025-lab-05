#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include "solicitudes.h"
#include "utils.h"

#define RANDOM_FILAS()(1 + rand() % (FILAS-2));
#define RANDOM_COLMS()(1 + rand() % (COLUMNAS-2));

// ===================================
//      MENSAJERIA CON DIRECTORIO
// ===================================

void recibirRespuestaDirectorio(int mailbox_directorio_respuestas_id, struct respuesta *respuesta){
    printf("Recibiendo respuesta de directorio...\n");
    // MUY IMPORTANTE usar getpid() para solo recibir los mensajes de este servidor
    if (msgrcv(mailbox_directorio_respuestas_id, respuesta, sizeof(struct respuesta) - sizeof(long), getpid(), 0) == -1) {
        perror("🚫 Error al recibir respuesta de Directorio");
    } else {
        printf("Respuesta de directorio recibida:\n");
        printf("- Mtype: %li\n", respuesta->mtype);
        printf("- Código: %i\n", respuesta->codigo);
        printf("- Datos: %s\n", respuesta->datos);
        printf("- Elementos: %i\n", respuesta->num_elementos);
    }
}

void enviarSolicitudDirectorio(struct Comunicacion *comunicacion, struct solicitud *solicitud, struct respuesta *respuesta) {

    printf("Enviando solicitud a directorio:\n");
    printf("- Mtype: %li\n", solicitud->mtype);
    printf("- Tipo: %i\n", solicitud->tipo);
    printf("- Texto: %s\n", solicitud->texto);
    msgsnd(comunicacion->mailbox_directorio_solicitudes_id, solicitud, sizeof(struct solicitud) - sizeof(long), 0);

    recibirRespuestaDirectorio(comunicacion->mailbox_directorio_respuestas_id, respuesta);
}

// ===================================
//      PARTE LOGICA
// ===================================

int aceptarJugador(struct Estado *estado, struct Arena *arena, struct Jugador *jugador) {
    if (estado->cant_jugadores >= MAX_JUGADORES) {
        printf("[LOG] Rechazado: servidor lleno (%d/%d).\n", estado->cant_jugadores,
               MAX_JUGADORES);
        return 0;
    }
    switch (jugador->tipo) {
    case RAIDER:
        if (estado->cant_raiders >= arena->max_raiders) {
            printf("[LOG] Rechazado: equipo RAIDER lleno (%d/%d).\n",
                   estado->cant_raiders, arena->max_raiders);
            return 0;
        }
        break;
    case GUARDIAN:
        if (estado->cant_guardianes >= arena->max_guardianes) {
            printf("[LOG] Rechazado: equipo GUARDIAN lleno (%d/%d).\n",
                   estado->cant_guardianes, arena->max_guardianes);
            return 0;
        }
        break;
    default:
        printf("[LOG] Rechazado: tipo de jugador desconocido '%c'.\n",
               jugador->tipo);
        return 0;
    }
    printf("[LOG] Jugador '%s' (tipo: %c) puede ser aceptado.\n", jugador->nombre,
           jugador->tipo);
    return 1;
}

void spawnearJugador(struct Jugador *jugador, struct Arena *arena) {
    int centro_fila = FILAS / 2;
    int centro_columna = COLUMNAS / 2;
    int max_radius = (FILAS > COLUMNAS) ? (FILAS / 2) : (COLUMNAS / 2);
    int min_radius = max_radius / 2;

    struct Posicion valid_spawns[FILAS * COLUMNAS];
    int count = 0;
    int fila, columna;
    int dist_fila, dist_columna, distancia;
    for (fila = 0; fila < FILAS; fila++) {
        for (columna = 0; columna < COLUMNAS; columna++) {
            if (arena->mapa[fila][columna] == VACIO) {
                dist_fila = abs(fila - centro_fila);
                dist_columna = abs(columna - centro_columna);
                distancia = (dist_fila > dist_columna) ? dist_fila : dist_columna;

                if (distancia > min_radius) {
                    valid_spawns[count].fila = fila;
                    valid_spawns[count].columna = columna;
                    count++;
                }
            }
        }
    }
    int choice;
    if (count > 0) {
        choice = rand() % count;
        fila = valid_spawns[choice].fila;
        columna = valid_spawns[choice].columna;
    } else {
        do {
            fila = RANDOM_FILAS();
            columna = RANDOM_COLMS();
        } while (arena->mapa[fila][columna] != VACIO);
    }
    jugador->posicion = (struct Posicion){fila, columna};
    arena->mapa[fila][columna] = jugador->tipo;
    printf("[LOG] Jugador '%s' spawneado en (%d, %d).\n", jugador->nombre,
           jugador->posicion.fila, jugador->posicion.columna);
}

// ===================================
//      MENSAJERIA CON CLIENTES
// ===================================

int recibirSolicitudes(struct SolicitudServidor *solicitud, int mailbox_solicitudes_id) {
    if (msgrcv(mailbox_solicitudes_id, solicitud, sizeof(solicitud) - sizeof(long), 0, 0) == -1) {
        perror("🚫 msgrcv");
        return 0;
    } else {
        printf("Mensaje recibido:\n");
        printf("- Mtype: %li\n", solicitud->mtype);
        printf("- Codigo: %i\n", solicitud->codigo);
        printf("- Mailbox: %i\n", solicitud->clave_mailbox_respuestas);
        printf("- Fila: %i\n", solicitud->fila);
        printf("- Columna: %i\n", solicitud->columna);
        printf("- Tipo: %c\n\n", solicitud->tipo);
        return 1;
    }
}

void responderSolicitud(int clave_mailbox_respuestas,
    struct RespuestaServidor *respuesta) {
   int id_mailbox_cliente = msgget(clave_mailbox_respuestas, 0);
   if (id_mailbox_cliente == -1) {
       perror("🚫 No se pudo obtener el ID del mailbox del cliente");
       return;
   }

   if (msgsnd(id_mailbox_cliente, respuesta,
       sizeof(struct RespuestaServidor) - sizeof(long), 0) == -1) {
      perror("🚫 msgsnd");
   } else {
       printf("Enviando respuesta al cliente:\n");
       printf("- Mtype: %li\n", respuesta->mtype);
       printf("- Codigo: %i\n", respuesta->codigo);
       printf("- Mensaje: %s\n\n", respuesta->mensaje);
  }   
}

// Delega la atencion a los distintos atender X accion
void atenderSolicitud(struct SolicitudServidor *solicitud, struct Arena *arena) {
    struct Jugador jugador; // el jugador que realizo la solicitud
    jugador.pid = solicitud->mtype;
    jugador.posicion = (struct Posicion) {solicitud->fila , solicitud->columna};
    // TODO: jugador.nombre
    jugador.tipo = solicitud->tipo;
    
    struct RespuestaServidor respuesta;
    respuesta.mtype = solicitud->mtype;

    switch (solicitud->codigo) {
    case CONEXION:
        atenderConexion(&jugador, &respuesta, arena);
        break;
    case DESCONEXION:
        atenderDesconexion(&jugador, &respuesta, arena);
        break;
    case MOVIMIENTO:
        atenderMovimiento(&jugador, &respuesta, arena);
        break;
    case TESORO_CAPTURADO:
        atenderCapturaTesoro(&jugador, &respuesta, arena);
        break;
    case RAIDER_CAPTURADO:
        atenderCapturaRaider(&jugador, &respuesta, arena);
        break;
    default:
        break;
    }
    responderSolicitud(solicitud->clave_mailbox_respuestas, &respuesta);
    imprimirEstado(arena->estado);
}

// ===================================
//      PARTE LOGICA
// ===================================

int buscarJugador(long pid, struct Arena *arena) {
    int i;
    for (i = 0; i < arena->estado->cant_jugadores; i++)
        if (arena->jugadores[i].pid == pid) return i;
    return -1;
}

int conectarJugador(struct Jugador *jugador, struct Arena *arena) {
    // if (buscarJugador(jugador->pid) != -1) return -1; // ya existe
    if (!aceptarJugador(arena->estado, arena, jugador)) return -1; // verificar
    spawnearJugador(jugador, arena); // darle una posicion
    arena->jugadores[arena->estado->cant_jugadores++] = *jugador; // incrementar cantidad
    (jugador->tipo == RAIDER) ? arena->estado->cant_raiders++ : arena->estado->cant_guardianes++;
    return 0;
}

int moverJugador(struct Jugador *jugador, struct Arena *arena) {
    int pos = buscarJugador(jugador->pid, arena);
    if (pos < 0) return -1;

    arena->mapa[arena->jugadores[pos].posicion.fila][arena->jugadores[pos].posicion.columna] = VACIO;
    arena->jugadores[pos].posicion = jugador->posicion;
    arena->mapa[jugador->posicion.fila][jugador->posicion.columna] = jugador->tipo; 
    return 0;
}

int desconectarJugador(long pid, struct Arena *arena) {
    int pos = buscarJugador(pid, arena);
    if (pos < 0) return -1;

    arena->mapa[arena->jugadores[pos].posicion.fila]
        [arena->jugadores[pos].posicion.columna] = VACIO;

    (arena->jugadores[pos].tipo == RAIDER) ?
        arena->estado->cant_raiders--:
        arena->estado->cant_guardianes--;
    
    int j;
    for (j = pos; j < arena->estado->cant_jugadores - 1; j++) {
        arena->jugadores[j] = arena->jugadores[j + 1];
    }
    arena->estado->cant_jugadores--;
    return 0;
}

int capturarTesoro(struct Jugador *jugador, struct Arena *arena) {
    if (arena->estado->cant_tesoros == 0) return SIN_TESOROS;

    int pos = buscarJugador(jugador->pid, arena);
    if (pos < 0) return -1;
    if (arena->jugadores[pos].tipo == GUARDIAN) return -1;
    
    int fila = jugador->posicion.fila;
    int columna = jugador->posicion.columna;

    if (arena->mapa[fila][columna] == TESORO) {
        arena->mapa[fila][columna] = RAIDER;
        arena->estado->cant_tesoros--;
        arena->mapa[arena->jugadores[pos].posicion.fila]
            [arena->jugadores[pos].posicion.columna] = VACIO;
            arena->jugadores[pos].posicion = jugador->posicion;
        return 0;
    }
    return -1;
}

int capturarRaider(struct Jugador *jugador, struct Arena *arena) {
    if (arena->estado->cant_raiders == 0) return SIN_RAIDERS;
    
    int pos = buscarJugador(jugador->pid, arena);
    if (pos < 0) return -1;
    if (jugador->tipo == RAIDER) return -1;

    int fila = jugador->posicion.fila;      
    int columna = jugador->posicion.columna;

    int i, j;
    for (i = 0; i < arena->estado->cant_jugadores; i++) {
        if (i == pos) continue; // no se captura
        if (arena->jugadores[i].tipo == RAIDER &&
            arena->jugadores[i].posicion.fila == fila &&
            arena->jugadores[i].posicion.columna == columna) {
            arena->mapa[fila][columna] = GUARDIAN;
            
            arena->mapa[arena->jugadores[pos].posicion.fila]
                [arena->jugadores[pos].posicion.columna] = VACIO;
            arena->jugadores[pos].posicion = jugador->posicion;
            for (j = i; j < arena->estado->cant_jugadores - 1; j++) {
                arena->jugadores[j] = arena->jugadores[j + 1];
            }
            arena->estado->cant_jugadores--;
            arena->estado->cant_raiders--;
            return 0;
        }
    }
    return -1;
}

void construirRespuesta(struct RespuestaServidor *respuesta,
     int codigo, const char *mensaje) {
    snprintf(respuesta->mensaje, MAX_LONGITUD_MENSAJES, mensaje);
    respuesta->codigo = codigo;
}

void atenderConexion(struct Jugador *jugador,
     struct RespuestaServidor *respuesta, struct Arena *arena) {
    imprimirTituloSolicitud(jugador->pid, "solicita conectarse...");
    if (conectarJugador(jugador, arena) < 0) {
        construirRespuesta(respuesta, ERROR, "No se conectó el jugador");
    } else {
        construirRespuesta(respuesta, S_OK, "Jugador conectado con éxito");
    }
}

void atenderDesconexion(struct Jugador *jugador, struct RespuestaServidor *respuesta, struct Arena *arena) {
    imprimirTituloSolicitud(jugador->pid, "solicita desconectarse...");
    if (desconectarJugador(jugador->pid, arena) <0) {
        construirRespuesta(respuesta, ERROR, "no encontro al jugador");
    } else {
        construirRespuesta (respuesta, S_OK, "Jugador desconectado con exito");
    }
}

void atenderMovimiento(struct Jugador *jugador, struct RespuestaServidor *respuesta, struct Arena *arena) {
    imprimirTituloSolicitud(jugador->pid, "se mueve..."); 
    if (moverJugador(jugador, arena) < 0) { 
        construirRespuesta(respuesta,ERROR, "no encontro al jugador");
    } else {
        construirRespuesta (respuesta, S_OK, "Jugador se movio con exito");
    } 
}

void atenderCapturaTesoro(struct Jugador *jugador, struct RespuestaServidor *respuesta, struct Arena *arena) {
    imprimirTituloSolicitud(jugador->pid, "intenta capturar tesoro..."); 
    int codigo = capturarTesoro(jugador, arena);
    if (codigo == 0) {
        construirRespuesta(respuesta, S_OK,"Tesoro capturado con exito");
    } else if (codigo == SIN_TESOROS) {
        construirRespuesta(respuesta, SIN_TESOROS,"Ya no quedan tesoros en el mapa");
    } else {
        construirRespuesta(respuesta, ERROR, "No hay tesoro en esta posicion");
    }
}

void atenderCapturaRaider(struct Jugador *jugador, struct RespuestaServidor *respuesta, struct Arena *arena) {
        imprimirTituloSolicitud(jugador->pid, "intenta capturar raider..."); 
    int codigo = capturarRaider(jugador, arena);
    if (codigo == 0) {
        construirRespuesta(respuesta, S_OK,"Raider capturado con exito");
    } else if (codigo == SIN_RAIDERS) {
        construirRespuesta(respuesta, SIN_TESOROS,"Ya no quedan raiders en el mapa");
    } else {
        construirRespuesta(respuesta, ERROR, "No hay raider en esta posicion");
    }
}