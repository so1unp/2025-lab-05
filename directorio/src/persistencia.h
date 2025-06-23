#ifndef PERSISTENCIA_H
#define PERSISTENCIA_H

#include "../directorio.h"

// Prototipos de funciones de persistencia
int cargarCatacumbas(struct catacumba catacumbas[], int *num_catacumbas);
int guardarCatacumbas(struct catacumba catacumbas[], int num_catacumbas);

#endif // PERSISTENCIA_H
