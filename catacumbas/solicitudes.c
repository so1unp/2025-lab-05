#include <stdio.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include "solicitudes.h"

// atenderSolicitudes y todos los metodos necesarios
//

void responderSolicitud(int clave_mailbox_respuestas,
    struct RespuestaServidor *respuesta) {
   int id_mailbox_cliente = msgget(clave_mailbox_respuestas, 0);
   if (id_mailbox_cliente == -1) {
       perror("🚫 No se pudo obtener el ID del mailbox del cliente");
       return;
   }

   if (msgsnd(id_mailbox_cliente, respuesta,
       sizeof(struct RespuestaServidor) - sizeof(long), 0) == -1) {
      perror("🚫 msgsnd");
   } else {
       printf("Enviando respuesta al cliente:\n");
       printf("- Mtype: %li\n", respuesta->mtype);
       printf("- Codigo: %i\n", respuesta->codigo);
       printf("- Mensaje: %s\n\n", respuesta->mensaje);
  }   
}

// void construirRespuesta(struct RespuestaServidor *respuesta,
//     int codigo, const char *mensaje) {
//    snprintf(respuesta->mensaje, MAX_LONGITUD_MENSAJES, mensaje);
//    respuesta->codigo = codigo;
// }

// void atenderConexion(struct Jugador *jugador, struct RespuestaServidor *respuesta) {
//    imprimirTituloSolicitud(jugador->pid, "solicita conectarse...");
//    if (conectarJugador(jugador) < 0) {
//        construirRespuesta(respuesta, ERROR, "No se conectó el jugador");
//    } else {
//        construirRespuesta(respuesta, OK, "Jugador conectado con éxito");
//    }
// }
