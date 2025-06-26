#ifndef JUGADOR_H
#define JUGADOR_H

int conectar_servidor(const char *nombre_catacumba, int tipo_jugador);
void desconectar_servidor();
int enviar_movimiento(int x, int y, int tipo_jugador);
int recibir_respuesta(char *mensaje, int *codigo);

#endif