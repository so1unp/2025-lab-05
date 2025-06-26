#ifndef STATUS_H
#define STATUS_H

#include <sys/types.h>

// Tipos de mensaje (mtype), para diferenciar estados y eventos
#define TYPE_SERVER_STATUS 1 // estado del servidor/ juego
#define TYPE_GAME_EVENT 2    // eventos de juego

// Clave común para mailbox de estado/eventos
#define MAILBOX_STATUS_KEY 54321

#define MAX_MSG 256

/** --- Códigos de estado de servidor/juego --- */
// esta todo OK
#define ST_SERVER_OK 1
// hubo una desconexion o error
#define ST_SERVER_ERROR 2
// un define de reconectando??

// Codigos de eventos de juego
#define ST_CONNECTED 10      // Explorador se ha conectado al juego
#define ST_TREASURE_FOUND 11 // Explorador ha encontrado un tesoro
#define ST_ALL_TREASURES 12  // Se han tomado todos los tesoros disponibles
#define ST_PLAYER_CAUGHT 13 // Un explorador ha sido atrapado por un guardian
#define ST_GAME_OVER 14 // La partida ha terminado
#define ST_GAME_STARTED 15 // La partida ha comenzado
#define ST_GAME_PAUSED 16 // La partida ha sido pausada

// Mensaje de estado o evento enviado por el servidor
struct status_msg
{
    long mtype;         // TYPE_SERVER_STATUS o TYPE_GAME_EVENT
    int code;           // uno de los ST_*
    char text[MAX_MSG]; // descripción del estado o evento
};

#endif
