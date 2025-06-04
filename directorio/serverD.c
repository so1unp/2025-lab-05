#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include "directorio.h"

int main(int argc, char *argv[])
{
    struct catacumba catacumbas[MAX_CATACUMBAS];
    int num_catacumbas = 0;

    int mailbox_solicitudes_id, mailbox_respuestas_id;
    struct solicitud msg;
    struct respuesta resp;
    int recibido;

    printf("Directorio de Catacumbas iniciando...\n");

    // Crear o conectar al mailbox de solicitudes
    mailbox_solicitudes_id = msgget(MAILBOX_KEY, 0666 | IPC_CREAT);
    if (mailbox_solicitudes_id == -1)
    {
        perror("Error al crear el mailbox de solicitudes");
        exit(EXIT_FAILURE);
    }

    // Crear o conectar al mailbox de respuestas
    mailbox_respuestas_id = msgget(MAILBOX_RESPUESTA_KEY, 0666 | IPC_CREAT);
    if (mailbox_respuestas_id == -1)
    {
        perror("Error al crear el mailbox de respuestas");
        exit(EXIT_FAILURE);
    }

    printf("Mailboxes creados/conectados correctamente.\n");
    printf("Solicitudes ID: %d, Respuestas ID: %d\n", mailbox_solicitudes_id, mailbox_respuestas_id);

    // Bucle principal para recibir mensajes
    while (1)
    {
        printf("Esperando mensajes...\n");

        // Recibir solicitud (bloqueante)
        recibido = msgrcv(mailbox_solicitudes_id, &msg, sizeof(msg) - sizeof(long), 0, 0);
        if (recibido == -1)
        {
            perror("Error al recibir solicitud");
            continue;
        }

        printf("Mensaje recibido del cliente PID %ld. Tipo: %d, Texto: %s\n", msg.mtype, msg.tipo, msg.texto);

        // Preparar respuesta común
        resp.mtype = msg.mtype; // El PID del cliente
        resp.num_elementos = 0;
        resp.datos[0] = '\0';

        // Procesar solicitud según el tipo
        switch (msg.tipo)
        {
        case OP_LISTAR:
            printf("Listando catacumbas (%d total):\n", num_catacumbas);
            resp.codigo = RESP_OK;
            resp.num_elementos = num_catacumbas;

            if (num_catacumbas > 0)
            {
                for (int i = 0; i < num_catacumbas; i++)
                {
                    char temp[200]; // Buffer más grande para evitar truncamiento
                    snprintf(temp, sizeof(temp), "%d. %.40s -> %.80s\n",
                             i + 1, catacumbas[i].nombre, catacumbas[i].direccion);

                    // Verificar que no se exceda el buffer de respuesta
                    if (strlen(resp.datos) + strlen(temp) < MAX_DAT_RESP - 1)
                    {
                        strncat(resp.datos, temp, MAX_DAT_RESP - strlen(resp.datos) - 1);
                    }

                    printf("%d. Nombre: %s, Dirección: %s\n",
                           i + 1, catacumbas[i].nombre, catacumbas[i].direccion);
                }
            }
            else
            {
                strcpy(resp.datos, "No hay catacumbas registradas.\n");
            }
            break;

        case OP_AGREGAR:
            if (num_catacumbas < MAX_CATACUMBAS)
            {
                // Formato esperado: "nombre:direccion"
                char *token = strtok(msg.texto, ":");
                if (token != NULL)
                {
                    strncpy(catacumbas[num_catacumbas].nombre, token, MAX_NOM - 1);
                    catacumbas[num_catacumbas].nombre[MAX_NOM - 1] = '\0';
                    token = strtok(NULL, ":");
                    if (token != NULL)
                    {
                        strncpy(catacumbas[num_catacumbas].direccion, token, MAX_RUTA - 1);
                        catacumbas[num_catacumbas].direccion[MAX_RUTA - 1] = '\0';
                        num_catacumbas++;
                        printf("Catacumba agregada correctamente.\n");

                        resp.codigo = RESP_OK;
                        strcpy(resp.datos, "Catacumba agregada correctamente.");
                    }
                    else
                    {
                        printf("Error: formato incorrecto.\n");
                        resp.codigo = RESP_ERROR;
                        strcpy(resp.datos, "Error: formato incorrecto. Use 'nombre:direccion'");
                    }
                }
                else
                {
                    resp.codigo = RESP_ERROR;
                    strcpy(resp.datos, "Error: formato incorrecto. Use 'nombre:direccion'");
                }
            }
            else
            {
                printf("Error: máximo de catacumbas alcanzado.\n");
                resp.codigo = RESP_LIMITE_ALCANZADO;
                strcpy(resp.datos, "Error: máximo de catacumbas alcanzado.");
            }
            break;

        case OP_BUSCAR:
        {
            int encontrado = 0;
            for (int i = 0; i < num_catacumbas; i++)
            {
                if (strcmp(catacumbas[i].nombre, msg.texto) == 0)
                {
                    printf("Catacumba encontrada: %s en %s\n",
                           catacumbas[i].nombre, catacumbas[i].direccion);

                    resp.codigo = RESP_OK;
                    snprintf(resp.datos, MAX_DAT_RESP, "Catacumba encontrada: %.40s -> %.80s",
                             catacumbas[i].nombre, catacumbas[i].direccion);
                    encontrado = 1;
                    break;
                }
            }
            if (!encontrado)
            {
                printf("Catacumba '%s' no encontrada.\n", msg.texto);
                resp.codigo = RESP_NO_ENCONTRADO;
                snprintf(resp.datos, MAX_DAT_RESP, "Catacumba '%.40s' no encontrada.", msg.texto);
            }
        }
        break;

        case OP_ELIMINAR:
        {
            int encontrado = -1;
            for (int i = 0; i < num_catacumbas; i++)
            {
                if (strcmp(catacumbas[i].nombre, msg.texto) == 0)
                {
                    encontrado = i;
                    break;
                }
            }

            if (encontrado >= 0)
            {
                // Eliminar moviendo todos los elementos posteriores
                for (int i = encontrado; i < num_catacumbas - 1; i++)
                {
                    strcpy(catacumbas[i].nombre, catacumbas[i + 1].nombre);
                    strcpy(catacumbas[i].direccion, catacumbas[i + 1].direccion);
                }
                num_catacumbas--;
                printf("Catacumba '%s' eliminada.\n", msg.texto);

                resp.codigo = RESP_OK;
                snprintf(resp.datos, MAX_DAT_RESP, "Catacumba '%.40s' eliminada correctamente.", msg.texto);
            }
            else
            {
                printf("Catacumba '%s' no encontrada para eliminar.\n", msg.texto);
                resp.codigo = RESP_NO_ENCONTRADO;
                snprintf(resp.datos, MAX_DAT_RESP, "Catacumba '%.40s' no encontrada.", msg.texto);
            }
        }
        break;

        default:
            printf("Operación desconocida: %d\n", msg.tipo);
            resp.codigo = RESP_ERROR;
            strcpy(resp.datos, "Operación desconocida.");
        }

        // Enviar respuesta al cliente
        if (msgsnd(mailbox_respuestas_id, &resp, sizeof(resp) - sizeof(long), 0) == -1)
        {
            perror("Error al enviar respuesta");
        }
        else
        {
            printf("Respuesta enviada al cliente PID %ld\n", resp.mtype);
        }

        printf("\n");
    }

    // Nunca llega aquí, pero buena práctica
    exit(EXIT_SUCCESS);
}
