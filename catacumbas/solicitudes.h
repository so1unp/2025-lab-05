#ifndef SOLICITUDES_H
#define SOLICITUDES_H
#include "catacumbas.h"  // << Necesario
#include "../directorio/directorio.h"

/// @brief Envía una solicitud al directorio y espera su respuesta.
/// @param comunicacion Estructura con los datos de mensajería con el directorio.
/// @param solicitud Puntero a la solicitud a enviar.
/// @param respuesta Puntero donde se almacenará la respuesta del directorio.
void enviarSolicitudDirectorio(struct Comunicacion *comunicacion, struct solicitud *solicitud, struct respuesta *respuesta);

/// @brief Recibe la respuesta del directorio para este servidor.
/// @param mailbox_directorio_respuestas_id ID del buzón de respuestas del directorio.
/// @param respuesta Puntero donde se almacenará la respuesta recibida.
void recibirRespuestaDirectorio(int mailbox_directorio_respuestas_id, struct respuesta *respuesta);

/// @brief Recibe una solicitud enviada por un cliente.
/// @param solicitud Puntero donde se almacenará la solicitud recibida.
/// @param mailbox_solicitudes_id ID del buzón de solicitudes.
/// @return 1 si msgrcv fue exitoso, 0 si ocurrió un error.
int recibirSolicitudes(struct SolicitudServidor *solicitud, int mailbox_solicitudes_id);

/// @brief Atiende la solicitud recibida y la despacha a la función correspondiente según el código.
/// @param solicitud Puntero a la solicitud del cliente.
/// @param arena Puntero a la arena del juego.
void atenderSolicitud(struct SolicitudServidor *solicitud, struct Arena *arena);

/// @brief Envía una respuesta a un cliente.
/// @param clave_mailbox_respuestas Clave del buzón del cliente.
/// @param respuesta Puntero a la respuesta que se enviará.
void enviarMensajeCliente(int clave_mailbox_respuestas, struct RespuestaServidor *respuesta);

/// @brief Atiende la solicitud de conexión de un cliente.
/// @param jugador Puntero al jugador que realiza la acción.
/// @param respuesta Puntero a la respuesta que se enviará al cliente.
/// @param arena Puntero a la arena del juego.
void atenderConexion(struct Jugador *jugador, struct RespuestaServidor *respuesta, struct Arena *arena);

/// @brief Atiende la solicitud de desconexión de un cliente.
/// @param jugador Puntero al jugador que realiza la acción.
/// @param respuesta Puntero a la respuesta que se enviará al cliente.
/// @param arena Puntero a la arena del juego.
void atenderDesconexion(struct Jugador *jugador, struct RespuestaServidor *respuesta, struct Arena *arena);

/// @brief Atiende la solicitud de movimiento de un jugador.
/// @param jugador Puntero al jugador que realiza la acción.
/// @param respuesta Puntero a la respuesta que se enviará al cliente.
/// @param arena Puntero a la arena del juego.
void atenderMovimiento(struct Jugador *jugador, struct RespuestaServidor *respuesta, struct Arena *arena);

/// @brief Atiende la solicitud de captura de un tesoro por parte de un Raider.
/// @param jugador Puntero al jugador que realiza la acción.
/// @param respuesta Puntero a la respuesta que se enviará al cliente.
/// @param arena Puntero a la arena del juego.
void atenderCapturaTesoro(struct Jugador *jugador, struct RespuestaServidor *respuesta, struct Arena *arena);

/// @brief Atiende la solicitud de captura de un Raider por parte de un Guardian.
/// @param jugador Puntero al jugador que realiza la acción.
/// @param respuesta Puntero a la respuesta que se enviará al cliente.
/// @param arena Puntero a la arena del juego.
void atenderCapturaRaider(struct Jugador *jugador, struct RespuestaServidor *respuesta, struct Arena *arena);

/// @brief Notifica a un Raider que ha sido capturado (muerto).
/// @param jugador Puntero al jugador que será notificado.
void notificarMuerte(struct Jugador *jugador);

/// @brief Notifica a todos los jugadores que el juego ha finalizado e informa quién ganó.
/// @param arena Puntero a la arena del juego.
void notificarFinalJuego(struct Arena *arena);

#endif