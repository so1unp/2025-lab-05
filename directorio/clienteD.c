#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include "directorio.h"

void mostrar_menu()
{
    printf("\n--- Cliente de Debug para Directorio de Catacumbas ---\n");
    printf("1. Listar catacumbas\n");
    printf("2. Agregar catacumba\n");
    printf("3. Buscar catacumba\n");
    printf("4. Eliminar catacumba\n");
    printf("0. Salir\n");
    printf("Ingrese una opción: ");
}

int main()
{
    int mailbox_solicitudes_id, mailbox_respuestas_id;
    struct solicitud msg;
    struct respuesta resp;
    int opcion;
    char buffer[150];
    pid_t mi_pid = getpid();

    printf("Cliente iniciado con PID: %d\n", mi_pid);

    // Conectar al mailbox de solicitudes
    mailbox_solicitudes_id = msgget(MAILBOX_KEY, 0666);
    if (mailbox_solicitudes_id == -1)
    {
        perror("Error al conectar con el mailbox de solicitudes");
        printf("¿Está ejecutándose el servidor?\n");
        exit(EXIT_FAILURE);
    }

    // Conectar al mailbox de respuestas
    mailbox_respuestas_id = msgget(MAILBOX_RESPUESTA_KEY, 0666);
    if (mailbox_respuestas_id == -1)
    {
        perror("Error al conectar con el mailbox de respuestas");
        printf("¿Está ejecutándose el servidor?\n");
        exit(EXIT_FAILURE);
    }

    printf("Conectado a los mailboxes. Solicitudes ID: %d, Respuestas ID: %d\n",
           mailbox_solicitudes_id, mailbox_respuestas_id);

    while (1)
    {
        mostrar_menu();
        if (scanf("%d", &opcion) != 1)
        {
            // Limpiar el buffer de entrada si hay error
            while (getchar() != '\n')
                ;
            printf("Entrada inválida. Intente de nuevo.\n");
            continue;
        }

        // Limpiar el buffer de entrada
        while (getchar() != '\n')
            ;

        if (opcion == 0)
        {
            break;
        }

        // Preparar mensaje común
        msg.mtype = mi_pid; // Usar PID como identificador

        switch (opcion)
        {
        case 1: // Listar catacumbas
            msg.tipo = OP_LISTAR;
            msg.texto[0] = '\0'; // No se necesita texto

            if (msgsnd(mailbox_solicitudes_id, &msg, sizeof(msg) - sizeof(long), 0) == -1)
            {
                perror("Error al enviar solicitud");
                break;
            }
            printf("Solicitud de listado enviada.\n");
            break;

        case 2: // Agregar catacumba
            msg.tipo = OP_AGREGAR;

            printf("Ingrese nombre de la catacumba: ");
            fgets(buffer, sizeof(buffer), stdin);
            buffer[strcspn(buffer, "\n")] = '\0'; // Eliminar el salto de línea

            // Copiar el nombre al mensaje
            strncpy(msg.texto, buffer, MAX_TEXT - 1);

            printf("Ingrese dirección de la catacumba: ");
            fgets(buffer, sizeof(buffer), stdin);
            buffer[strcspn(buffer, "\n")] = '\0'; // Eliminar el salto de línea

            // Adjuntar la dirección con formato nombre:dirección
            strncat(msg.texto, ":", MAX_TEXT - strlen(msg.texto) - 1);
            strncat(msg.texto, buffer, MAX_TEXT - strlen(msg.texto) - 1);

            if (msgsnd(mailbox_solicitudes_id, &msg, sizeof(msg) - sizeof(long), 0) == -1)
            {
                perror("Error al enviar solicitud");
                break;
            }
            printf("Solicitud de agregar catacumba enviada.\n");
            break;

        case 3: // Buscar catacumba
            msg.tipo = OP_BUSCAR;

            printf("Ingrese nombre de la catacumba a buscar: ");
            fgets(buffer, sizeof(buffer), stdin);
            buffer[strcspn(buffer, "\n")] = '\0'; // Eliminar el salto de línea

            strncpy(msg.texto, buffer, MAX_TEXT - 1);

            if (msgsnd(mailbox_solicitudes_id, &msg, sizeof(msg) - sizeof(long), 0) == -1)
            {
                perror("Error al enviar solicitud");
                break;
            }
            printf("Solicitud de búsqueda enviada.\n");
            break;

        case 4: // Eliminar catacumba
            msg.tipo = OP_ELIMINAR;

            printf("Ingrese nombre de la catacumba a eliminar: ");
            fgets(buffer, sizeof(buffer), stdin);
            buffer[strcspn(buffer, "\n")] = '\0'; // Eliminar el salto de línea

            strncpy(msg.texto, buffer, MAX_TEXT - 1);

            if (msgsnd(mailbox_solicitudes_id, &msg, sizeof(msg) - sizeof(long), 0) == -1)
            {
                perror("Error al enviar solicitud");
                break;
            }
            printf("Solicitud de eliminación enviada.\n");
            break;

        default:
            printf("Opción no válida. Intente de nuevo.\n");
            continue;
        }

        // Recibir respuesta del servidor (solo mensajes dirigidos a este PID)
        printf("Esperando respuesta del servidor...\n");
        if (msgrcv(mailbox_respuestas_id, &resp, sizeof(resp) - sizeof(long), mi_pid, 0) == -1)
        {
            perror("Error al recibir respuesta");
        }
        else
        {
            printf("Respuesta recibida - Código: %d\n", resp.codigo);
            printf("Tamaño de la respuesta recibida: %lu bytes\n", sizeof(resp) - sizeof(long));
            
            printf("Datos: %s\n", resp.datos);
            if (resp.num_elementos > 0)
            {
                printf("Número de elementos: %d\n", resp.num_elementos);
            }
        }

        // Pequeña pausa para poder ver los resultados
        printf("Presione Enter para continuar...");
        getchar();
    }

    printf("Cliente de debug finalizado.\n");
    return 0;
}