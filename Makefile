SUBDIRS = catacumbas clientes directorio

# Variables para compilación directa del servidor de directorio
CC=gcc
CFLAGS=-std=c99 -Wall -Wextra -D_XOPEN_SOURCE=700
LDFLAGS=-pthread -lrt

# Directorios para compilación directa
DIRECTORIO_DIR=directorio
CATACUMBAS_DIR=catacumbas
SRC_DIR=$(DIRECTORIO_DIR)/src
BUILD_DIR=.

# Archivos fuente del servidor modular del directorio
DIRECTORIO_SOURCES=$(SRC_DIR)/main.c \
               $(SRC_DIR)/comunicacion.c \
               $(SRC_DIR)/operaciones.c \
               $(SRC_DIR)/persistencia.c \
               $(SRC_DIR)/senales.c \
               $(SRC_DIR)/ping.c

# Archivos fuente del servidor de catacumbas
CATACUMBAS_SOURCES=$(CATACUMBAS_DIR)/server.c \
				$(CATACUMBAS_DIR)/utils.c \
				$(CATACUMBAS_DIR)/solicitudes.c \
				$(CATACUMBAS_DIR)/config.c 

# Archivos objeto de los servidores
DIRECTORIO_OBJECTS=$(DIRECTORIO_SOURCES:.c=.o)
CATACUMBAS_OBJECTS=$(CATACUMBAS_SOURCES:.c=.o)

.PHONY: all clean directorio-server catacumbas-server $(SUBDIRS)

all: $(SUBDIRS)

# Regla para compilar cada subdirectorio (funcionalidad original)
$(SUBDIRS):
	$(MAKE) -s -C $@

# Regla especial para compilar el servidor del directorio en la raíz  
directorio-server: $(DIRECTORIO_OBJECTS)
	$(CC) -o $(BUILD_DIR)/directorio-server $^ $(LDFLAGS)

# Regla especial para compilar el servidor de catacumbas en la raíz
catacumbas-server: $(CATACUMBAS_OBJECTS)
	$(CC) -o $(BUILD_DIR)/catacumbas-server $^ $(LDFLAGS)

# Alias: make directorio llama a directorio-server
directorio: directorio-server

# Alias: make catacumbas llama a catacumbas-server  
catacumbas: catacumbas-server

# Regla para compilar archivos .c a .o del directorio
$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) -c -o $@ $< $(CFLAGS)

# Regla para compilar archivos .c a .o de catacumbas
$(CATACUMBAS_DIR)/%.o: $(CATACUMBAS_DIR)/%.c
	$(CC) -c -o $@ $< $(CFLAGS) -I$(DIRECTORIO_DIR)

clean:
	rm -f $(BUILD_DIR)/directorio-server
	rm -f $(BUILD_DIR)/catacumbas-server
	rm -f $(DIRECTORIO_OBJECTS)
	rm -f $(CATACUMBAS_OBJECTS)
	@for dir in $(SUBDIRS); do \
		$(MAKE) -s -C $$dir clean; \
	done

zip:
	git archive --format zip --output ${USER}-TP4.zip HEAD
