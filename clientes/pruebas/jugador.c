#include "jugador.h"
#include "../juego_constantes.h"
#include "../../catacumbas/catacumbas.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int mailbox_servidor_id = -1;
static int mailbox_cliente_id = -1;
static key_t key_cliente = 0;

// Conecta al servidor y crea mailbox único para este jugador
int conectar_servidor(const char *nombre_catacumba, int tipo_jugador) {
    key_cliente = ftok("/tmp", getpid());
    mailbox_cliente_id = msgget(key_cliente, IPC_CREAT | 0666);
    if (mailbox_cliente_id == -1) {
        perror("Error creando mailbox cliente");
        return -1;
    }

    key_t key_servidor = ftok(nombre_catacumba, MAILBOX_SOLICITUDES_SUFIJO);
    mailbox_servidor_id = msgget(key_servidor, 0666);
    if (mailbox_servidor_id == -1) {
        perror("Error conectando al mailbox del servidor");
        return -1;
    }

    // Aquí podrías enviar un mensaje de "conexión" indicando el tipo de jugador (explorador o guardián)
    // Si tu protocolo lo requiere

    return 0;
}

// Envía un movimiento al servidor
int enviar_movimiento(int x, int y, int tipo_jugador) {
    struct SolicitudServidor solicitud;
    solicitud.mtype = getpid();
    solicitud.codigo = MOVIMIENTO;
    solicitud.clave_mailbox_respuestas = key_cliente;
    solicitud.fila = y;
    solicitud.columna = x;
    solicitud.tipo = tipo_jugador;

    if (msgsnd(mailbox_servidor_id, &solicitud, sizeof(solicitud) - sizeof(long), 0) == -1) {
        perror("Error enviando movimiento");
        return -1;
    }
    return 0;
}

// Recibe la respuesta del servidor (bloqueante)
int recibir_respuesta(char *mensaje, int *codigo) {
    struct RespuestaServidor respuesta;
    if (msgrcv(mailbox_cliente_id, &respuesta, sizeof(respuesta) - sizeof(long), getpid(), 0) == -1) {
        perror("Error recibiendo respuesta");
        return -1;
    }
    if (mensaje) strncpy(mensaje, respuesta.mensaje, 255);
    if (codigo) *codigo = respuesta.codigo;
    return 0;
}

// Limpia recursos del cliente
void desconectar_servidor() {
    if (mailbox_cliente_id != -1) {
        msgctl(mailbox_cliente_id, IPC_RMID, NULL);
        mailbox_cliente_id = -1;
    }
}