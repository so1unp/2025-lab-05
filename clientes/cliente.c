// ...includes necesarios...
#include <ncurses.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ncurses.h>
#include "../directorio/directorio.h"

#define MAX_CATAS 100

struct CatacumbaInfo {
    char nombre[MAX_NOM];
    char direccion[MAX_RUTA];
    char mailbox[MAX_NOM];
    int cantJug;
    int cantMaxJug;
};

int parsear_catacumbas(const char *datos, struct CatacumbaInfo *catacumbas, int max) {
    int count = 0;
    char buffer[MAX_DAT_RESP];
    strncpy(buffer, datos, MAX_DAT_RESP-1);
    buffer[MAX_DAT_RESP-1] = '\0';

    char *token = strtok(buffer, ";");
    while (token && count < max) {
        char *nombre = strtok(token, "|");
        char *direccion = strtok(NULL, "|");
        char *mailbox = strtok(NULL, "|");
        char *cantJug = strtok(NULL, "|");
        char *cantMaxJug = strtok(NULL, "|");
        if (nombre && direccion && mailbox && cantJug && cantMaxJug) {
            strncpy(catacumbas[count].nombre, nombre, MAX_NOM-1);
            strncpy(catacumbas[count].direccion, direccion, MAX_RUTA-1);
            strncpy(catacumbas[count].mailbox, mailbox, MAX_NOM-1);
            catacumbas[count].cantJug = atoi(cantJug);
            catacumbas[count].cantMaxJug = atoi(cantMaxJug);
            count++;
        }
        token = strtok(NULL, ";");
    }
    return count;
}

int main() {
    int mailbox_solicitudes_id, mailbox_respuestas_id;
    struct solicitud msg;
    struct respuesta resp;
    pid_t mi_pid = getpid();

    // Conectar a mailboxes
    mailbox_solicitudes_id = msgget(MAILBOX_KEY, 0666);
    mailbox_respuestas_id = msgget(MAILBOX_RESPUESTA_KEY, 0666);
    if (mailbox_solicitudes_id == -1 || mailbox_respuestas_id == -1) {
        perror("Error conectando a mailboxes");
        return 1;
    }

    // Solicitar listado de catacumbas
    msg.mtype = mi_pid;
    msg.tipo = OP_LISTAR;
    msg.texto[0] = '\0';
    if (msgsnd(mailbox_solicitudes_id, &msg, sizeof(msg) - sizeof(long), 0) == -1) {
        perror("Error al enviar solicitud");
        return 1;
    }

    // Recibir respuesta
    if (msgrcv(mailbox_respuestas_id, &resp, sizeof(resp) - sizeof(long), mi_pid, 0) == -1) {
        perror("Error al recibir respuesta");
        return 1;
    }

    // Parsear catacumbas
    struct CatacumbaInfo catacumbas[MAX_CATAS];
    int n = parsear_catacumbas(resp.datos, catacumbas, MAX_CATAS);

    // Mostrar con ncurses
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    int highlight = 0;
    int ch;
    while (1) {
        clear();
        mvprintw(0, 0, "Catacumbas registradas en el directorio:");
        for (int i = 0; i < n; i++) {
            if (i == highlight)
                attron(A_REVERSE);
            mvprintw(i+2, 2, "%s [%d/%d]", catacumbas[i].nombre, catacumbas[i].cantJug, catacumbas[i].cantMaxJug);
            attroff(A_REVERSE);
        }
        mvprintw(n+4, 0, "Usa flechas para navegar, ENTER para seleccionar, q para salir.");
        refresh();

        ch = getch();
        if (ch == 'q') break;
        if (ch == KEY_UP && highlight > 0) highlight--;
        if (ch == KEY_DOWN && highlight < n-1) highlight++;
        if (ch == '\n' || ch == KEY_ENTER) {
            // Aquí puedes hacer algo con la catacumba seleccionada
            mvprintw(n+6, 0, "Seleccionaste: %s", catacumbas[highlight].nombre);
            refresh();
            getch();
            break;
        }
    }
    endwin();
    return 0;
}