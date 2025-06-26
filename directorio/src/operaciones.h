#ifndef OPERACIONES_H
#define OPERACIONES_H

#include "../directorio.h"

// Prototipos de funciones de operaciones
void listarCatacumbas(struct respuesta *resp, struct catacumba catacumbas[], int *num_catacumbas);
void agregarCatacumba(struct catacumba catacumbas[], int *num_catacumbas, struct solicitud *msg, struct respuesta *resp);
void buscarCatacumba(struct catacumba catacumbas[], int *num_catacumbas, struct solicitud *msg, struct respuesta *resp);
void eliminarCatacumba(struct catacumba catacumbas[], int *num_catacumbas, struct solicitud *msg, struct respuesta *resp);

#endif // OPERACIONES_H
