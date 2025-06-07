#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include "directorio.h"

void mostrar_catacumbas_formateadas(char *datos)
{
    if (strlen(datos) == 0)
    {
        printf("ℹ️  No hay datos para mostrar.\n");
        return;
    }

    // Si es un mensaje simple (no formato pipe), mostrarlo directamente
    if (strchr(datos, '|') == NULL)
    {
        printf("📝 %s\n", datos);
        return;
    }

    // Contar cuántas catacumbas hay (separadas por ;)
    int num_catacumbas = 0;
    int len = strlen(datos);

    // Contar separadores ';' y agregar 1 para la última catacumba
    for (int i = 0; i < len; i++)
    {
        if (datos[i] == ';')
        {
            num_catacumbas++;
        }
    }
    // Si hay datos pero no termina con ';', hay una catacumba más
    if (len > 0)
    {
        num_catacumbas++;
    }

    // Título apropiado según el número de catacumbas
    if (num_catacumbas > 1)
    {
        printf("\n╔═══════════════════════════════════════════════════════════════╗\n");
        printf("║          📋 LISTA DE CATACUMBAS (%d encontradas)              ║\n", num_catacumbas);
        printf("╚═══════════════════════════════════════════════════════════════╝\n");
    }
    else
    {
        printf("\n╔═══════════════════════════════════════════════════════════════╗\n");
        printf("║               🏛️  INFORMACIÓN DE CATACUMBA                    ║\n");
        printf("╚═══════════════════════════════════════════════════════════════╝\n");
    }

    // Procesar cada catacumba manualmente
    int start = 0;
    int index = 1;

    for (int i = 0; i <= len; i++)
    {
        // Encontramos el final de una catacumba (punto y coma o final de cadena)
        if (datos[i] == ';' || datos[i] == '\0')
        {
            if (i > start)
            { // Solo si hay contenido
                // Extraer la catacumba actual
                char catacumba[300];
                int cat_len = i - start;
                strncpy(catacumba, datos + start, cat_len);
                catacumba[cat_len] = '\0';

                // Parsear los campos de la catacumba
                char catacumba_copia[300];
                strcpy(catacumba_copia, catacumba);

                char *nombre = strtok(catacumba_copia, "|");
                char *direccion = strtok(NULL, "|");
                char *mailbox = strtok(NULL, "|");
                char *cantJug_str = strtok(NULL, "|");
                char *maxJug_str = strtok(NULL, "|");

                if (nombre && direccion && mailbox && cantJug_str && maxJug_str)
                {
                    int cantJug = atoi(cantJug_str);
                    int maxJug = atoi(maxJug_str);

                    // Si hay múltiples catacumbas, mostrar número de índice
                    if (num_catacumbas > 1)
                    {
                        printf("\n🏛️  Catacumba #%d:\n", index);
                        printf("   ├─ 📝 Nombre:     \"%s\"\n", nombre);
                        printf("   ├─ 📍 Dirección:  \"%s\"\n", direccion);
                        printf("   ├─ 📬 Mailbox:    \"%s\"\n", mailbox);
                        printf("   └─ 👥 Jugadores:  %d/%d", cantJug, maxJug);
                    }
                    else
                    {
                        printf("\n🏛️  Información de la catacumba:\n");
                        printf("   ├─ 📝 Nombre:     \"%s\"\n", nombre);
                        printf("   ├─ 📍 Dirección:  \"%s\"\n", direccion);
                        printf("   ├─ 📬 Mailbox:    \"%s\"\n", mailbox);
                        printf("   └─ 👥 Jugadores:  %d/%d", cantJug, maxJug);
                    }

                    // Indicador visual del estado
                    if (cantJug == 0)
                    {
                        printf(" 🟢 (Vacía)");
                    }
                    else if (cantJug == maxJug)
                    {
                        printf(" 🔴 (Llena)");
                    }
                    else
                    {
                        printf(" 🟡 (Disponible)");
                    }
                    printf("\n");
                }
                else
                {
                    if (num_catacumbas > 1)
                    {
                        printf("%d. Datos incompletos: %s\n", index, catacumba);
                    }
                    else
                    {
                        printf("Datos incompletos: %s\n", catacumba);
                    }
                }

                index++;
            }
            start = i + 1; // Siguiente catacumba comienza después del punto y coma
        }

        if (datos[i] == '\0')
            break; // Final de la cadena
    }
}

void mostrar_menu()
{
    printf("\n╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║          🛠️  CLIENTE DEBUG - DIRECTORIO DE CATACUMBAS          ║\n");
    printf("╠═══════════════════════════════════════════════════════════════╣\n");
    printf("║  1. 📋 Listar catacumbas                                      ║\n");
    printf("║  2. ➕ Agregar catacumba                                      ║\n");
    printf("║  3. 🔍 Buscar catacumba                                       ║\n");
    printf("║  4. 🗑️  Eliminar catacumba                                    ║\n");
    printf("║  0. 🚪 Salir                                                  ║\n");
    printf("╚═══════════════════════════════════════════════════════════════╝\n");
    printf("🎯 Ingrese una opción: ");
}

int main()
{
    int mailbox_solicitudes_id, mailbox_respuestas_id;
    struct solicitud msg;
    struct respuesta resp;
    int opcion;
    char buffer[150];
    pid_t mi_pid = getpid();

    printf("══════════════════════════════════════════════════════════════\n");
    printf("         🛠️  CLIENTE DEBUG - INICIANDO CONEXIÓN               \n");
    printf("══════════════════════════════════════════════════════════════\n");
    printf("📋 Cliente iniciado con PID: %d\n\n", mi_pid);

    // Conectar al mailbox de solicitudes
    mailbox_solicitudes_id = msgget(MAILBOX_KEY, 0666);
    if (mailbox_solicitudes_id == -1)
    {
        perror("❌ Error al conectar con el mailbox de solicitudes");
        printf("❓ ¿Está ejecutándose el servidor?\n");
        exit(EXIT_FAILURE);
    }

    // Conectar al mailbox de respuestas
    mailbox_respuestas_id = msgget(MAILBOX_RESPUESTA_KEY, 0666);
    if (mailbox_respuestas_id == -1)
    {
        perror("❌ Error al conectar con el mailbox de respuestas");
        printf("❓ ¿Está ejecutándose el servidor?\n");
        exit(EXIT_FAILURE);
    }

    printf("✅ Conectado a los mailboxes correctamente\n");
    printf("  ├─ Solicitudes ID: %d\n", mailbox_solicitudes_id);
    printf("  └─ Respuestas ID:  %d\n", mailbox_respuestas_id);
    printf("══════════════════════════════════════════════════════════════\n");

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
            printf("\n🚪 Cerrando cliente debug...\n");
            printf("👋 ¡Hasta luego!\n\n");
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
                perror("❌ Error al enviar solicitud");
                break;
            }
            printf("📤 Solicitud de listado enviada al servidor...\n");
            break;

        case 2: // Agregar catacumba
            msg.tipo = OP_AGREGAR;

            printf("\n╔═══════════════════════════════════════════════════════════════╗\n");
            printf("║                    ➕ AGREGAR NUEVA CATACUMBA                  ║\n");
            printf("╚═══════════════════════════════════════════════════════════════╝\n");
            printf("📝 Ingrese nombre de la catacumba: ");
            fgets(buffer, sizeof(buffer), stdin);
            buffer[strcspn(buffer, "\n")] = '\0'; // Eliminar el salto de línea
            strncpy(msg.texto, buffer, MAX_TEXT - 1);

            printf("📍 Ingrese dirección de la catacumba: ");
            fgets(buffer, sizeof(buffer), stdin);
            buffer[strcspn(buffer, "\n")] = '\0'; // Eliminar el salto de línea
            strncat(msg.texto, "|", MAX_TEXT - strlen(msg.texto) - 1);
            strncat(msg.texto, buffer, MAX_TEXT - strlen(msg.texto) - 1);

            printf("📬 Ingrese mailbox de la catacumba: ");
            fgets(buffer, sizeof(buffer), stdin);
            buffer[strcspn(buffer, "\n")] = '\0'; // Eliminar el salto de línea
            strncat(msg.texto, "|", MAX_TEXT - strlen(msg.texto) - 1);
            strncat(msg.texto, buffer, MAX_TEXT - strlen(msg.texto) - 1);

            printf("\n📤 Enviando datos: %s\n", msg.texto);

            if (msgsnd(mailbox_solicitudes_id, &msg, sizeof(msg) - sizeof(long), 0) == -1)
            {
                perror("❌ Error al enviar solicitud");
                break;
            }
            printf("✅ Solicitud de agregar catacumba enviada al servidor\n");
            break;

        case 3: // Buscar catacumba
            msg.tipo = OP_BUSCAR;

            printf("\n🔍 Ingrese nombre de la catacumba a buscar: ");
            fgets(buffer, sizeof(buffer), stdin);
            buffer[strcspn(buffer, "\n")] = '\0'; // Eliminar el salto de línea

            strncpy(msg.texto, buffer, MAX_TEXT - 1);

            if (msgsnd(mailbox_solicitudes_id, &msg, sizeof(msg) - sizeof(long), 0) == -1)
            {
                perror("❌ Error al enviar solicitud");
                break;
            }
            printf("📤 Solicitud de búsqueda enviada al servidor...\n");
            break;

        case 4: // Eliminar catacumba
            msg.tipo = OP_ELIMINAR;

            printf("\n🗑️ Ingrese nombre de la catacumba a eliminar: ");
            fgets(buffer, sizeof(buffer), stdin);
            buffer[strcspn(buffer, "\n")] = '\0'; // Eliminar el salto de línea

            strncpy(msg.texto, buffer, MAX_TEXT - 1);

            if (msgsnd(mailbox_solicitudes_id, &msg, sizeof(msg) - sizeof(long), 0) == -1)
            {
                perror("❌ Error al enviar solicitud");
                break;
            }
            printf("📤 Solicitud de eliminación enviada al servidor...\n");
            break;

        default:
            printf("❌ Opción no válida. Intente de nuevo.\n");
            continue;
        }

        // Recibir respuesta del servidor (solo mensajes dirigidos a este PID)
        printf("⏳ Esperando respuesta del servidor...\n");
        if (msgrcv(mailbox_respuestas_id, &resp, sizeof(resp) - sizeof(long), mi_pid, 0) == -1)
        {
            perror("❌ Error al recibir respuesta");
        }
        else
        {
            printf("\n══════════════════════════════════════════════════════════════\n");
            printf("📨 RESPUESTA DEL SERVIDOR\n");
            printf("══════════════════════════════════════════════════════════════\n");

            // Mostrar código de respuesta
            switch (resp.codigo)
            {
            case RESP_OK:
                printf("✅ [OK] Operación exitosa\n");
                break;
            case RESP_ERROR:
                printf("❌ [ERROR] Error en la operación\n");
                break;
            case RESP_NO_ENCONTRADO:
                printf("🔍 [ERROR] Elemento no encontrado\n");
                break;
            case RESP_LIMITE_ALCANZADO:
                printf("⚠️  [ERROR] Límite alcanzado\n");
                break;
            default:
                printf("❓ [?] Código desconocido: %d\n", resp.codigo);
                break;
            }

            if (resp.num_elementos > 0)
            {
                printf("📊 Elementos encontrados: %d\n", resp.num_elementos);
            }

            // Si es una respuesta de listado, formatear nicely
            if (opcion == 1 && resp.codigo == RESP_OK)
            {
                printf("\n=== Respuesta Cruda del Servidor ===\n");
                printf("Datos sin procesar: \"%s\"\n", resp.datos);
                mostrar_catacumbas_formateadas(resp.datos);
            }
            else if (opcion == 3 && resp.codigo == RESP_OK)
            {
                // Buscar catacumba - mostrar datos formateados
                printf("\n=== Respuesta Cruda del Servidor ===\n");
                printf("Datos sin procesar: \"%s\"\n", resp.datos);
                printf("\n=== Catacumba Encontrada ===\n");
                mostrar_catacumbas_formateadas(resp.datos);
            }
            else
            {
                // Para otras operaciones, mostrar mensaje simple
                printf("Mensaje: %s\n", resp.datos);
            }
        }

        // Pequeña pausa para poder ver los resultados
        printf("Presione Enter para continuar...");
        getchar();
    }

    printf("👋 Cliente de debug finalizado.\n");
    return 0;
}