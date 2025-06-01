#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
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
    int mailbox_id;
    struct solicitud msg;
    int opcion;
    char buffer[150];

    // Conectar al mailbox existente
    mailbox_id = msgget(MAILBOX_KEY, 0666);
    if (mailbox_id == -1)
    {
        perror("Error al conectar con el mailbox");
        printf("¿Está ejecutándose el servidor?\n");
        exit(EXIT_FAILURE);
    }

    printf("Conectado al mailbox ID: %d\n", mailbox_id);

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

        // Común para todos los mensajes
        msg.mtype = 1; // Tipo de solicitud para msgrcv

        switch (opcion)
        {
        case 1: // Listar catacumbas
            msg.tipo = OP_LISTAR;
            msg.texto[0] = '\0'; // No se necesita texto

            if (msgsnd(mailbox_id, &msg, sizeof(msg) - sizeof(long), 0) == -1)
            {
                perror("Error al enviar solicitud");
            }
            else
            {
                printf("Solicitud de listado enviada.\n");
            }
            break;

        case 2: // Agregar catacumba
            msg.tipo = OP_AGREGAR;

            printf("Ingrese nombre de la catacumba: ");
            fgets(buffer, sizeof(buffer), stdin);
            buffer[strcspn(buffer, "\n")] = '\0'; // Eliminar el salto de línea

            // Copiar el nombre al solicitud
            strncpy(msg.texto, buffer, MAX_TEXT - 1);

            printf("Ingrese dirección de la catacumba: ");
            fgets(buffer, sizeof(buffer), stdin);
            buffer[strcspn(buffer, "\n")] = '\0'; // Eliminar el salto de línea

            // Adjuntar la dirección con formato nombre:dirección
            strncat(msg.texto, ":", MAX_TEXT - strlen(msg.texto) - 1);
            strncat(msg.texto, buffer, MAX_TEXT - strlen(msg.texto) - 1);

            if (msgsnd(mailbox_id, &msg, sizeof(msg) - sizeof(long), 0) == -1)
            {
                perror("Error al enviar solicitud");
            }
            else
            {
                printf("Solicitud de agregar catacumba enviada.\n");
            }
            break;

        case 3: // Buscar catacumba
            msg.tipo = OP_BUSCAR;

            printf("Ingrese nombre de la catacumba a buscar: ");
            fgets(buffer, sizeof(buffer), stdin);
            buffer[strcspn(buffer, "\n")] = '\0'; // Eliminar el salto de línea

            strncpy(msg.texto, buffer, MAX_TEXT - 1);

            if (msgsnd(mailbox_id, &msg, sizeof(msg) - sizeof(long), 0) == -1)
            {
                perror("Error al enviar solicitud");
            }
            else
            {
                printf("Solicitud de búsqueda enviada.\n");
            }
            break;

        case 4: // Eliminar catacumba
            msg.tipo = OP_ELIMINAR;

            printf("Ingrese nombre de la catacumba a eliminar: ");
            fgets(buffer, sizeof(buffer), stdin);
            buffer[strcspn(buffer, "\n")] = '\0'; // Eliminar el salto de línea

            strncpy(msg.texto, buffer, MAX_TEXT - 1);

            if (msgsnd(mailbox_id, &msg, sizeof(msg) - sizeof(long), 0) == -1)
            {
                perror("Error al enviar solicitud");
            }
            else
            {
                printf("Solicitud de eliminación enviada.\n");
            }
            break;

        default:
            printf("Opción no válida. Intente de nuevo.\n");
        }

        // Pequeña pausa para poder ver los resultados
        printf("Presione Enter para continuar...");
        getchar();
    }

    printf("Cliente de debug finalizado.\n");
    return 0;
}