SUBDIRS = catacumbas clientes directorio

# Variables para compilación directa del servidor de directorio
CC=gcc
CFLAGS=-std=c99 -Wall -Wextra -D_XOPEN_SOURCE=700
LDFLAGS=-pthread -lrt

# Directorios para compilación directa
DIRECTORIO_DIR=directorio
SRC_DIR=$(DIRECTORIO_DIR)/src
BUILD_DIR=.

# Archivos fuente del servidor modular del directorio
SERVER_SOURCES=$(SRC_DIR)/main.c \
               $(SRC_DIR)/comunicacion.c \
               $(SRC_DIR)/operaciones.c \
               $(SRC_DIR)/persistencia.c \
               $(SRC_DIR)/senales.c \
               $(SRC_DIR)/ping.c

# Archivos objeto del servidor
SERVER_OBJECTS=$(SERVER_SOURCES:.c=.o)

.PHONY: all clean directorio-server $(SUBDIRS)

all: $(SUBDIRS)

# Regla para compilar cada subdirectorio (funcionalidad original)
$(SUBDIRS):
	$(MAKE) -s -C $@

# Regla especial para compilar el servidor del directorio en la raíz  
directorio-server: $(SERVER_OBJECTS)
	$(CC) -o $(BUILD_DIR)/directorio-server $^ $(LDFLAGS)

# Alias: make directorio llama a directorio-server
directorio: directorio-server

# Regla para compilar archivos .c a .o del directorio
$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm -f $(BUILD_DIR)/directorio-server
	rm -f $(SERVER_OBJECTS)
	@for dir in $(SUBDIRS); do \
		$(MAKE) -s -C $$dir clean; \
	done

zip:
	git archive --format zip --output ${USER}-TP4.zip HEAD
