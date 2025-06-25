#ifndef COMUNICACION_H
#define COMUNICACION_H
#include "catacumbas.h"  // << Necesario

// void atenderSolicitud(struct SolicitudServidor *solicitud);
void responderSolicitud(int clave_mailbox_respuestas, struct RespuestaServidor *respuesta);

#endif