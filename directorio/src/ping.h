#ifndef PING_H
#define PING_H

#include <pthread.h>
#include <stdbool.h>
#include "../directorio.h"

// Estructura para pasar parámetros al hilo de ping
struct ping_params
{
    struct catacumba *catacumbas;
    int *num_catacumbas;
};

// Variables globales para el hilo de ping (declaradas como extern)
extern pthread_t hilo_ping;
extern volatile bool servidor_activo;
extern pthread_mutex_t mutex_catacumbas;

// Prototipos de funciones de ping
int procesoActivo(int pid);
void estadoServidor(struct catacumba catacumbas[], int *num_catacumbas);
void *hiloPing(void *arg);
int leerEstadoCatacumba(struct catacumba *catacumba);

#endif // PING_H
