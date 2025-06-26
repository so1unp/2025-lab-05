#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <errno.h>
#include "comunicacion.h"

/**
 * @brief Recibe y procesa solicitudes de los clientes
 *
 * Esta función es bloqueante y espera a que llegue un mensaje al mailbox
 * de solicitudes. Imprime información de depuración sobre el mensaje recibido.
 *
 * @param recibido Puntero donde se almacena el número de bytes recibidos
 * @param mailbox_solicitudes_id ID del mailbox de solicitudes
 * @param msg Puntero donde se almacenará la solicitud recibida
 **/
void RecibirSolicitudes(int *recibido, int mailbox_solicitudes_id, struct solicitud *msg)
{
    printf("⏳ Esperando nuevas solicitudes...\n");

    // Recibir solicitud (bloqueante)
    *recibido = msgrcv(mailbox_solicitudes_id, msg, sizeof(struct solicitud) - sizeof(long), 0, 0);
    if (*recibido == -1)
    {
        perror("❌ Error al recibir solicitud");
        return;
    }

    printf("───────────────────────────────────────────────────────────────\n");
    printf("📨 NUEVA SOLICITUD RECIBIDA\n");
    printf("  ├─ Cliente PID: %ld\n", msg->mtype);
    printf("  ├─ Tipo Op.:    %d\n", msg->tipo);
    printf("  └─ Datos:       \"%s\"\n", msg->texto);
    printf("───────────────────────────────────────────────────────────────\n\n");
}

/**
 * @brief Envía una respuesta al cliente a través del mailbox de respuestas
 *
 * @param mailbox_respuestas_id ID del mailbox de respuestas
 * @param resp Puntero a la estructura de respuesta a enviar
 **/
void enviarRespuesta(int mailbox_respuestas_id, struct respuesta *resp)
{
    if (msgsnd(mailbox_respuestas_id, resp, sizeof(struct respuesta) - sizeof(long), 0) == -1)
    {
        perror("❌ Error al enviar respuesta");
    }
    else
    {
        printf("📤 Respuesta enviada al cliente PID %ld\n", resp->mtype);
        printf("═══════════════════════════════════════════════════════════════\n\n");
    }
}
