#ifndef SOLICITUDES_H
#define SOLICITUDES_H
#include "catacumbas.h"  // << Necesario
#include "../directorio/directorio.h"

void enviarSolicitudDirectorio(struct Comunicacion *comunicacion, struct solicitud *solicitud, struct respuesta *respuesta);
void recibirRespuestaDirectorio(int mailbox_directorio_respuestas_id, struct respuesta *respuesta);

int recibirSolicitudes(struct SolicitudServidor *solicitud, int mailbox_solicitudes_id);
void atenderSolicitud(struct SolicitudServidor *solicitud, struct Arena *arena);
void responderSolicitud(int clave_mailbox_respuestas, struct RespuestaServidor *respuesta);

void atenderConexion(struct Jugador *jugador, struct RespuestaServidor *respuesta, struct Arena *arena);
void atenderDesconexion(struct Jugador *jugador, struct RespuestaServidor *respuesta, struct Arena *arena);
void atenderMovimiento(struct Jugador *jugador, struct RespuestaServidor *respuesta, struct Arena *arena);
void atenderCapturaTesoro(struct Jugador *jugador, struct RespuestaServidor *respuesta, struct Arena *arena);
void atenderCapturaRaider(struct Jugador *jugador, struct RespuestaServidor *respuesta, struct Arena *arena);

#endif