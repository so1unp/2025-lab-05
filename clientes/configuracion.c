#ifndef JUGADOR_CONFIG_H
#define JUGADOR_CONFIG_H

typedef struct {
    char atrapa;
    char camina;
} JugadorConfig;

static inline int cargar_config(const char* archivo, JugadorConfig* config) {
    FILE* f = fopen(archivo, "r");
    if (!f) return 0;
    char clave[32], valor[32];
    while (fscanf(f, "%31[^:]: %31s\n", clave, valor) == 2) {
        if (strcmp(clave, "atrapa") == 0) config->atrapa = valor[0];
        if (strcmp(clave, "camina") == 0) config->camina = valor[0];
    }
    fclose(f);
    return 1;
}

static inline int is_walkable(char cell, JugadorConfig* config) {
    return cell == config->camina || cell == config->atrapa;
}

static inline int is_captura(char cell, JugadorConfig* config) {
    return cell == config->atrapa;
}

#endif