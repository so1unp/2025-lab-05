#include "../juego_constantes.h"
#include <ncurses.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "../../catacumbas/catacumbas.h" 

char caracter_atrapable;
char caracter_jugador;
extern char (*map)[WIDTH + 1]; // Añade 'extern'
extern char selected_role[50];
extern char selected_map[50];

// Variables para comunicación con el servidor
int mailbox_servidor_id = -1;
int mailbox_cliente_id = -1;
char *mapa_compartido = NULL;

// Función para conectar con el servidor de catacumbas
int conectar_servidor(char *nombre_catacumba) {

    // Crear mailbox único para este cliente
    key_t key_cliente = ftok("/tmp",getpid());
    mailbox_cliente_id = msgget(key_cliente, IPC_CREAT | 0666);
    if (mailbox_cliente_id == -1) {
        perror("Error creando mailbox cliente");
        return -1;
    }

    // Conectar al mailbox del servidor (usando el nombre de la catacumba)
    key_t key_servidor = ftok(nombre_catacumba, MAILBOX_SOLICITUDES_SUFIJO); // Asumiendo que MAILBOX_SOLICITUDES_SUFIJO está definido y es correcto
    mailbox_servidor_id = msgget(key_servidor, 0666);
    if (mailbox_servidor_id == -1) {
        perror("Error conectando al mailbox del servidor");
        return -1;
    }
    return 0;
}


// ... resto de jugador.c ...