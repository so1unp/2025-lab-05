CC=gcc
CFLAGS=-std=c99 -Wall -Wextra -D_XOPEN_SOURCE=700
LDFLAGS=-pthread -lrt

# Directorios
SRC_DIR=src
BUILD_DIR=.

# Archivos fuente del servidor modular
SERVER_SOURCES=$(SRC_DIR)/main.c \
               $(SRC_DIR)/comunicacion.c \
               $(SRC_DIR)/operaciones.c \
               $(SRC_DIR)/persistencia.c \
               $(SRC_DIR)/senales.c \
               $(SRC_DIR)/ping.c

# Archivos objeto del servidor
SERVER_OBJECTS=$(SERVER_SOURCES:.c=.o)

# Programas a compilar
PROGS=server clienteD

.PHONY: all
all: $(PROGS)

# Regla para el servidor modular
server: $(SERVER_OBJECTS)
	$(CC) -o $(BUILD_DIR)/$@ $^ $(LDFLAGS)

# Regla para clienteD (sin cambios)
clienteD: clienteD.c
	$(CC) -o $(BUILD_DIR)/$@ $< $(CFLAGS) $(LDFLAGS)

# Regla para compilar archivos .c a .o
$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: clean
clean:
	rm -f $(BUILD_DIR)/$(PROGS)
	rm -f $(SERVER_OBJECTS)

.PHONY: info
info:
	@echo "📋 INFORMACIÓN DEL PROYECTO MODULAR"
	@echo "├─ Archivos fuente del servidor:"
	@echo "│  $(SERVER_SOURCES)"
	@echo "├─ Archivos de cabecera:"
	@echo "│  $(SRC_DIR)/*.h"
	@echo "├─ Ejecutables generados:"
	@echo "│  $(addprefix $(BUILD_DIR)/,$(PROGS))"
	@echo "└─ Directorio de compilación: $(BUILD_DIR)"