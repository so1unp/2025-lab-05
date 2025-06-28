#ifndef SENALES_H
#define SENALES_H

#include "../directorio.h"

// Prototipos de funciones de manejo de señales
void configurarManejoSenales(void);
void manejarSenalTerminacion(int sig);
void limpiarMailboxes(void);

// Funciones para establecer variables globales
void establecer_mailbox_solicitudes(int id);
void establecer_mailbox_respuestas(int id);
void establecer_catacumbas_globales(struct catacumba *catacumbas, int *num_catacumbas);

#endif // SENALES_H
