SUBDIRS = catacumbas clientes directorio

# Variables para compilación directa del servidor de directorio
CC=gcc
CFLAGS=-std=c99 -Wall -Wextra -D_XOPEN_SOURCE=700
LDFLAGS=-pthread -lrt

# Directorios para compilación directa
DIRECTORIO_DIR=directorio
CATACUMBAS_DIR=catacumbas
CLIENTES_DIR=clientes
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

# Archivos fuente del cliente
CLIENTES_SOURCES=$(CLIENTES_DIR)/main.c \
				$(CLIENTES_DIR)/seleccion-mapa.c \
				$(CLIENTES_DIR)/seleccion-rol.c \
				$(CLIENTES_DIR)/base.c \
				$(CLIENTES_DIR)/jugador.c \
				$(CLIENTES_DIR)/gameOver.c 

# Archivos objeto de los servidores
DIRECTORIO_OBJECTS=$(DIRECTORIO_SOURCES:.c=.o)
CATACUMBAS_OBJECTS=$(CATACUMBAS_SOURCES:.c=.o)
CLIENTES_OBJECTS=$(CLIENTES_SOURCES:.c=.o)

.PHONY: all clean directorio-server catacumbas-server cliente $(SUBDIRS)

all: directorio-server catacumbas-server cliente

# Regla para compilar cada subdirectorio (funcionalidad original)
$(SUBDIRS):
	$(MAKE) -s -C $@

# Regla especial para compilar el servidor del directorio en la raíz  
directorio-server: $(DIRECTORIO_OBJECTS)
	$(CC) -o $(BUILD_DIR)/directorio-server $^ $(LDFLAGS)

# Regla especial para compilar el servidor de catacumbas en la raíz
catacumbas-server: $(CATACUMBAS_OBJECTS)
	$(CC) -o $(BUILD_DIR)/catacumbas-server $^ $(LDFLAGS)

# Regla especial para compilar el cliente en la raíz
cliente: $(CLIENTES_OBJECTS)
	$(CC) -o $(BUILD_DIR)/cliente $^ $(LDFLAGS) -lncurses

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

# Regla para compilar archivos .c a .o de clientes
$(CLIENTES_DIR)/%.o: $(CLIENTES_DIR)/%.c
	$(CC) -c -o $@ $< $(CFLAGS) -lncurses

clean:
	rm -f $(BUILD_DIR)/directorio-server
	rm -f $(BUILD_DIR)/catacumbas-server
	rm -f $(BUILD_DIR)/cliente
	rm -f $(DIRECTORIO_OBJECTS)
	rm -f $(CATACUMBAS_OBJECTS)
	rm -f $(CLIENTES_OBJECTS)
	@for dir in $(SUBDIRS); do \
		$(MAKE) -s -C $$dir clean; \
	done

zip:
	git archive --format zip --output ${USER}-TP4.zip HEAD
