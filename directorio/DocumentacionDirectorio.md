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
    char nombre[MAX_NOMBRE];
    char direccion[MAX_DIRECCION];
};
```

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
// resp.datos = "Catacumba1:Direccion1\nCatacumba2:Direccion2\n..."
```

### 2. Agregar Catacumba (OP_AGREGAR)

**Propósito**: Registrar una nueva catacumba en el directorio.

**Solicitud**:
```c
struct solicitud msg;
msg.mtype = getpid();
msg.tipo = OP_AGREGAR;
strcpy(msg.texto, "NombreCatacumba:DireccionCatacumba");
```

**Formato del texto**: `"nombre:direccion"`

**Respuesta**:
```c
// Éxito:
// resp.codigo = RESP_OK
// resp.datos = "Catacumba agregada exitosamente."

// Error (directorio lleno):
// resp.codigo = RESP_ERROR
// resp.datos = "Directorio lleno. No se puede agregar más catacumbas."
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
// resp.datos = "NombreCatacumba:DireccionCatacumba"

// No encontrada:
// resp.codigo = RESP_ERROR
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
// resp.datos = "Catacumba eliminada exitosamente."

// No encontrada:
// resp.codigo = RESP_ERROR
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
    
    // Ejemplo: Listar catacumbas
    msg.mtype = mi_pid;
    msg.tipo = OP_LISTAR;
    msg.texto[0] = '\0';
    
    // Enviar solicitud
    msgsnd(mailbox_solicitudes, &msg, sizeof(msg) - sizeof(long), 0);
    
    // Recibir respuesta (SOLO para este PID)
    msgrcv(mailbox_respuestas, &resp, sizeof(resp) - sizeof(long), mi_pid, 0);
    
    printf("Respuesta: %s\n", resp.datos);
    
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
} else {
    printf("Error en la operación: %s\n", resp.datos);
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
- Tamaño máximo de datos de respuesta: `MAX_DATA` caracteres

