#ifndef COMUNICACION_H
#define COMUNICACION_H

#include "../directorio.h"

// Prototipos de funciones de comunicación
void RecibirSolicitudes(int *recibido, int mailbox_solicitudes_id, struct solicitud *msg);
void enviarRespuesta(int mailbox_respuestas_id, struct respuesta *resp);

#endif // COMUNICACION_H
