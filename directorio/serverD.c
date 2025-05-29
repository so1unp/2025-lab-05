#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>

#define MAX_CATACUMBAS 100
#define MAILBOX_KEY 12345 // Clave constante para el mailbox
#define MAX_RUTA 200

// Definición de la estructura de catacumba
struct catacumba
{
    char nombre[100];
    char direccion[MAX_RUTA]; // Ruta al archivo de memoria compartida
};

// Definición de la estructura del mensaje a recibir
struct mensaje
{
    long mtype; // Tipo de mensaje (requerido por msgrcv)
    int tipo;   // Tipo de operación
    char texto[100];
};

// Códigos de operación
#define OP_LISTAR 1
#define OP_AGREGAR 2
#define OP_BUSCAR 3
#define OP_ELIMINAR 4

int main(int argc, char *argv[])
{
    struct catacumba catacumbas[MAX_CATACUMBAS];
    int num_catacumbas = 0;

    int mailbox_id;
    struct mensaje msg;
    int recibido;

    printf("Directorio de Catacumbas iniciando...\n");

    // Crear o conectar al mailbox
    mailbox_id = msgget(MAILBOX_KEY, 0666 | IPC_CREAT);
    if (mailbox_id == -1)
    {
        perror("Error al crear el mailbox");
        exit(EXIT_FAILURE);
    }

    printf("Mailbox creado/conectado correctamente. ID: %d\n", mailbox_id);

    // Bucle principal para recibir mensajes
    while (1)
    {
        printf("Esperando mensajes...\n");

        // Recibir mensaje (bloqueante)
        recibido = msgrcv(mailbox_id, &msg, sizeof(msg) - sizeof(long), 0, 0);
        if (recibido == -1)
        {
            perror("Error al recibir mensaje");
            continue;
        }

        printf("Mensaje recibido. Tipo: %d, Texto: %s\n", msg.tipo, msg.texto);

        // Procesar mensaje según el tipo
        switch (msg.tipo)
        {
        case OP_LISTAR:
            printf("Listando catacumbas (%d total):\n", num_catacumbas);
            for (int i = 0; i < num_catacumbas; i++)
            {
                printf("%d. Nombre: %s, Dirección: %s\n",
                       i + 1, catacumbas[i].nombre, catacumbas[i].direccion);
            }
            break;

        case OP_AGREGAR:
            if (num_catacumbas < MAX_CATACUMBAS)
            {
                // Formato esperado: "nombre:direccion"
                char *token = strtok(msg.texto, ":");
                if (token != NULL)
                {
                    strncpy(catacumbas[num_catacumbas].nombre, token, 99);
                    token = strtok(NULL, ":");
                    if (token != NULL)
                    {
                        strncpy(catacumbas[num_catacumbas].direccion, token, MAX_RUTA - 1);
                        num_catacumbas++;
                        printf("Catacumba agregada correctamente.\n");
                    }
                    else
                    {
                        printf("Error: formato incorrecto.\n");
                    }
                }
            }
            else
            {
                printf("Error: máximo de catacumbas alcanzado.\n");
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
                    encontrado = 1;
                    break;
                }
            }
            if (!encontrado)
            {
                printf("Catacumba '%s' no encontrada.\n", msg.texto);
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
            }
            else
            {
                printf("Catacumba '%s' no encontrada para eliminar.\n", msg.texto);
            }
        }
        break;

        default:
            printf("Operación desconocida: %d\n", msg.tipo);
        }

        printf("\n");
    }

    // Nunca llega aquí, pero buena práctica
    exit(EXIT_SUCCESS);
}
