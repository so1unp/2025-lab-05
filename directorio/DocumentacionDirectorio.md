# Documentación del Directorio de Catacumbas

## Descripción General

El **Directorio de Catacumbas** es un servidor que implementa un registro centralizado de catacumbas utilizando colas de mensajes (message queues) para la comunicación entre procesos (IPC). El sistema permite a múltiples clientes realizar operaciones sobre un directorio compartido de manera concurrente.

## Arquitectura del Sistema

### Componentes Principales

1. **Servidor (`server.c`)**: Proceso que mantiene el directorio y procesa solicitudes
2. **Cliente (`clienteD.c`)**: Programa de ejemplo para interactuar con el servidor
3. **Estructuras de datos (`directorio.h`)**: Definiciones compartidas

### Sistema de Comunicación

El sistema utiliza **dos colas de mensajes** independientes:

- **Mailbox de Solicitudes** (Clave: 12345): Para enviar peticiones al servidor
- **Mailbox de Respuestas** (Clave: 12346): Para recibir respuestas del servidor

## Estructuras de Datos

### Estructura de Solicitud
```c
struct solicitud {
    long mtype;      // PID del cliente (requerido por msgsnd/msgrcv)
    int tipo;        // Tipo de operación (OP_LISTAR, OP_AGREGAR, etc.)
    char texto[MAX_TEXT]; // Datos de la solicitud
};
```

### Estructura de Respuesta
```c
struct respuesta {
    long mtype;      // PID del cliente destinatario
    int codigo;      // Código de resultado (RESP_OK, RESP_ERROR)
    int num_elementos; // Número de elementos en la respuesta
    char datos[MAX_DATA]; // Datos de la respuesta
};
```

### Estructura de Catacumba
```c
struct catacumba {
    char nombre[MAX_NOM];     // Nombre único identificador de la catacumba
    char direccion[MAX_RUTA]; // Ruta al archivo de memoria compartida de la catacumba
    char mailbox[MAX_NOM];    // Mailbox de mensajes de la catacumba
    int cantJug;              // Cantidad actual de jugadores en la catacumba
    int cantMaxJug;           // Cantidad máxima de jugadores permitidos
};
```

## Formatos de Datos

### Formato de Agregar Catacumba
Para agregar una nueva catacumba, el campo `texto` de la solicitud debe contener:
```
"nombre|direccion|mailbox"
```

Los campos de cantidad de jugadores (`cantJug` y `cantMaxJug`) se inicializan automáticamente en 0 y se actualizan consultando la dirección de la catacumba.

**Ejemplo**:
```
"Catacumba_Norte|/tmp/catacumba_norte.shm|mq_norte"
```

### Formato de Respuesta de Listado
Las catacumbas se devuelven en el campo `datos` de la respuesta con el formato:
```
"cat1|dir1|mbox1|jugadores1|max1;cat2|dir2|mbox2|jugadores2|max2;..."
```

**Ejemplo**:
```
"Catacumba_Norte|/tmp/catacumba_norte.shm|mq_norte|5|15;Catacumba_Sur|/tmp/catacumba_sur.shm|mq_sur|0|10"
```

### Validaciones
- Todos los campos (`nombre`, `direccion`, `mailbox`) son obligatorios
- Los campos `cantJug` y `cantMaxJug` se inicializan automáticamente en 0
- Los valores reales de jugadores se obtienen consultando la dirección de la catacumba

## Operaciones Soportadas

### 1. Listar Catacumbas (OP_LISTAR)

**Propósito**: Obtener una lista de todas las catacumbas registradas.

**Solicitud**:
```c
struct solicitud msg;
msg.mtype = getpid();          // PID del cliente
msg.tipo = OP_LISTAR;          // Tipo de operación
msg.texto[0] = '\0';           // No se requiere texto
```

**Respuesta**:
```c
struct respuesta resp;
// resp.mtype = PID del cliente
// resp.codigo = RESP_OK
// resp.num_elementos = número de catacumbas
// resp.datos = "Catacumba1|Dir1|Mailbox1|0|10;Catacumba2|Dir2|Mailbox2|5|20;..."
```

**Formato de datos**: Cada catacumba se representa como `"nombre|direccion|mailbox|cantJug|maxJug"` y múltiples catacumbas se separan con `;`

### 2. Agregar Catacumba (OP_AGREGAR)

**Propósito**: Registrar una nueva catacumba en el directorio con información completa.

**Solicitud**:
```c
struct solicitud msg;
msg.mtype = getpid();
msg.tipo = OP_AGREGAR;
strcpy(msg.texto, "NombreCatacumba|DireccionCatacumba|MailboxCatacumba");
```

**Formato del texto**: `"nombre|direccion|mailbox"`
- `nombre`: Nombre único de la catacumba
- `direccion`: Ruta al archivo de memoria compartida
- `mailbox`: Identificador del mailbox de la catacumba

**Nota**: Los campos `cantJug` y `cantMaxJug` se inicializan automáticamente en 0. Los valores reales se obtienen consultando la dirección de la catacumba.

**Respuesta**:
```c
// Éxito:
// resp.codigo = RESP_OK
// resp.datos = "Catacumba agregada correctamente."

// Error (directorio lleno):
// resp.codigo = RESP_LIMITE_ALCANZADO
// resp.datos = "Error: máximo de catacumbas alcanzado."

// Error (formato incorrecto):
// resp.codigo = RESP_ERROR
// resp.datos = "Error: formato incorrecto. Use 'nombre|direccion|mailbox'"
```

### 3. Buscar Catacumba (OP_BUSCAR)

**Propósito**: Encontrar una catacumba específica por su nombre.

**Solicitud**:
```c
struct solicitud msg;
msg.mtype = getpid();
msg.tipo = OP_BUSCAR;
strcpy(msg.texto, "NombreBuscado");
```

**Respuesta**:
```c
// Encontrada:
// resp.codigo = RESP_OK
// resp.num_elementos = 1
// resp.datos = "NombreCatacumba|DireccionCatacumba|MailboxCatacumba|5|10"

// No encontrada:
// resp.codigo = RESP_NO_ENCONTRADO
// resp.datos = "Catacumba no encontrada."
```

### 4. Eliminar Catacumba (OP_ELIMINAR)

**Propósito**: Remover una catacumba del directorio.

**Solicitud**:
```c
struct solicitud msg;
msg.mtype = getpid();
msg.tipo = OP_ELIMINAR;
strcpy(msg.texto, "NombreAEliminar");
```

**Respuesta**:
```c
// Eliminada:
// resp.codigo = RESP_OK
// resp.datos = "Catacumba eliminada correctamente."

// No encontrada:
// resp.codigo = RESP_NO_ENCONTRADO
// resp.datos = "Catacumba no encontrada."
```

## Protocolo de Comunicación

### Envío de Solicitudes

1. **Conectar a los mailboxes**:
```c
int mailbox_solicitudes = msgget(MAILBOX_KEY, 0666);
int mailbox_respuestas = msgget(MAILBOX_RESPUESTA_KEY, 0666);
```

2. **Preparar la solicitud**:
```c
struct solicitud msg;
msg.mtype = getpid();  // ¡IMPORTANTE! Usar el PID del proceso
msg.tipo = OP_LISTAR;  // Tipo de operación deseada
// Completar msg.texto según la operación
```

3. **Enviar la solicitud**:
```c
if (msgsnd(mailbox_solicitudes, &msg, sizeof(msg) - sizeof(long), 0) == -1) {
    perror("Error al enviar solicitud");
}
```

### Recepción de Respuestas

**⚠️ PUNTO CRÍTICO**: Usar el PID propio para filtrar mensajes

```c
struct respuesta resp;
pid_t mi_pid = getpid();

// Recibir SOLO mensajes dirigidos a este proceso
if (msgrcv(mailbox_respuestas, &resp, sizeof(resp) - sizeof(long), mi_pid, 0) == -1) {
    perror("Error al recibir respuesta");
} else {
    printf("Código de respuesta: %d\n", resp.codigo);
    printf("Datos: %s\n", resp.datos);
    if (resp.num_elementos > 0) {
        printf("Número de elementos: %d\n", resp.num_elementos);
    }
}
```

### ¿Por qué es importante el PID en msgrcv?

El tercer parámetro de `msgrcv()` filtra los mensajes por tipo (`mtype`). Al usar el PID:

- **Sin filtro** (`msgrcv(..., 0, ...)`): El cliente podría recibir respuestas destinadas a otros clientes
- **Con filtro PID** (`msgrcv(..., mi_pid, ...)`): Solo recibe sus propias respuestas

## Códigos de Ejemplo

### Cliente Básico

```c
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include "directorio.h"

int main() {
    // Variables
    int mailbox_solicitudes, mailbox_respuestas;
    struct solicitud msg;
    struct respuesta resp;
    pid_t mi_pid = getpid();
    
    // Conectar a mailboxes
    mailbox_solicitudes = msgget(MAILBOX_KEY, 0666);
    mailbox_respuestas = msgget(MAILBOX_RESPUESTA_KEY, 0666);
    
    if (mailbox_solicitudes == -1 || mailbox_respuestas == -1) {
        perror("Error conectando a mailboxes");
        exit(1);
    }
    
    // Ejemplo: Agregar una catacumba
    msg.mtype = mi_pid;
    msg.tipo = OP_AGREGAR;
    strcpy(msg.texto, "MiCatacumba|/tmp/mi_catacumba.shm|mq_mi_catacumba");
    
    // Enviar solicitud
    msgsnd(mailbox_solicitudes, &msg, sizeof(msg) - sizeof(long), 0);
    
    // Recibir respuesta
    msgrcv(mailbox_respuestas, &resp, sizeof(resp) - sizeof(long), mi_pid, 0);
    
    if (resp.codigo == RESP_OK) {
        printf("Catacumba agregada: %s\n", resp.datos);
    }
    
    // Ejemplo: Listar catacumbas
    msg.mtype = mi_pid;
    msg.tipo = OP_LISTAR;
    msg.texto[0] = '\0';
    
    // Enviar solicitud
    msgsnd(mailbox_solicitudes, &msg, sizeof(msg) - sizeof(long), 0);
    
    // Recibir respuesta (SOLO para este PID)
    msgrcv(mailbox_respuestas, &resp, sizeof(resp) - sizeof(long), mi_pid, 0);
    
    printf("Lista de catacumbas: %s\n", resp.datos);
    
    return 0;
}
```

### Manejo de Errores

```c
// Verificar resultado de la operación
if (resp.codigo == RESP_OK) {
    printf("Operación exitosa: %s\n", resp.datos);
    if (resp.num_elementos > 0) {
        printf("Elementos encontrados: %d\n", resp.num_elementos);
    }
} else if (resp.codigo == RESP_NO_ENCONTRADO) {
    printf("Elemento no encontrado: %s\n", resp.datos);
} else if (resp.codigo == RESP_LIMITE_ALCANZADO) {
    printf("Límite alcanzado: %s\n", resp.datos);
} else {
    printf("Error en la operación: %s\n", resp.datos);
}
```

### Parsing de Respuestas

#### Parsear lista de catacumbas
```c
void parsearListaCatacumbas(char *datos) {
    char *catacumba = strtok(datos, ";");
    int index = 1;
    
    while (catacumba != NULL) {
        char *nombre = strtok(catacumba, "|");
        char *direccion = strtok(NULL, "|");
        char *mailbox = strtok(NULL, "|");
        char *cantJug_str = strtok(NULL, "|");
        char *maxJug_str = strtok(NULL, "|");
        
        if (nombre && direccion && mailbox && cantJug_str && maxJug_str) {
            int cantJug = atoi(cantJug_str);
            int maxJug = atoi(maxJug_str);
            
            printf("%d. %s\n", index++, nombre);
            printf("   Dirección: %s\n", direccion);
            printf("   Mailbox: %s\n", mailbox);
            printf("   Jugadores: %d/%d\n", cantJug, maxJug);
            printf("\n");
        }
        
        catacumba = strtok(NULL, ";");
    }
}
```

## Compilación y Ejecución

### Compilar el sistema

```bash
# Compilar el servidor
make server

# Compilar el cliente de debug
make clienteD
```

### Ejecutar el sistema

```bash
# 1. Ejecutar el servidor en segundo plano
./server &

# 2. Ejecutar el cliente de debug
./clienteD

# 3. Para finalizar el servidor
killall server
```

## Consideraciones Importantes

### Concurrencia
- El servidor procesa solicitudes de manera **secuencial** (una a la vez)
- Múltiples clientes pueden conectarse **simultáneamente**
- Las respuestas se entregan al cliente correcto usando su PID

### Limitaciones
- Máximo `MAX_CATACUMBAS` catacumbas (definido en `directorio.h`)
- Tamaño máximo de texto: `MAX_TEXT` caracteres
- Tamaño máximo de datos de respuesta: `MAX_DAT_RESP` caracteres
- Nombres de catacumba: máximo `MAX_NOM` caracteres
- Rutas de directorio: máximo `MAX_RUTA` caracteres

### Formato de Datos
- **Separador de campos**: `|` (pipe)
- **Separador de registros**: `;` (punto y coma)
- **Campos obligatorios**: Todos los campos son requeridos
- **Validación de jugadores**: cantJug >= 0, maxJug > 0, cantJug <= maxJug

### Códigos de Respuesta Extendidos
- `RESP_OK`: Operación exitosa
- `RESP_ERROR`: Error general en la operación
- `RESP_NO_ENCONTRADO`: Catacumba no encontrada
- `RESP_LIMITE_ALCANZADO`: Máximo de catacumbas alcanzado

