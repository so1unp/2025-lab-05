# 📚 Documentación del Sistema de Directorio de Catacumbas

## 🏗️ Arquitectura del Sistema

El sistema de directorio de catacumbas utiliza **colas de mensajes (message queues)** para la comunicación IPC entre el servidor y los clientes.

### Componentes principales:
- **Servidor (`server.c`)**: Mantiene el directorio centralizado de catacumbas
- **Cliente de prueba (`clienteD.c`)**: Interfaz para interactuar con el directorio
- **Cabeceras (`directorio.h`)**: Definiciones compartidas

---

## 📊 Estructura de Datos

### `struct catacumba`
```c
struct catacumba {
    int pid;                      // PID del proceso que maneja la catacumba
    char nombre[MAX_NOM];         // Nombre único identificador 
    char direccion[MAX_RUTA];     // Ruta de memoria compartida de la catacumba
    char propCatacumba[MAX_RUTA]; // Ruta de memoria compartida de propiedades
    char mailbox[MAX_NOM];        // Mailbox de mensajes de la catacumba
    int cantJug;                  // Cantidad actual de jugadores
    int cantMaxJug;               // Cantidad máxima de jugadores permitidos
};
```

---

## 🔄 Operaciones Disponibles

### 1. **Listar Catacumbas (OP_LISTAR)**
- **Solicitud**: Sin datos adicionales
- **Respuesta**: Lista de todas las catacumbas registradas
- **Formato de respuesta**: `nombre|direccion|propCatacumba|mailbox|cantJug|maxJug;...`

### 2. **Agregar Catacumba (OP_AGREGAR)**
- **Solicitud**: `nombrecat|dircat|dirpropcat|dirmailbox`
- **Respuesta**: Confirmación de éxito o error
- **Validaciones**: 
  - Límite máximo de catacumbas (MAX_CATACUMBAS = 10)
  - Formato correcto de entrada

### 3. **Buscar Catacumba (OP_BUSCAR)**
- **Solicitud**: `nombre_catacumba`
- **Respuesta**: Datos de la catacumba encontrada
- **Formato de respuesta**: `nombre|direccion|propCatacumba|mailbox|cantJug|maxJug`

### 4. **Eliminar Catacumba (OP_ELIMINAR)**
- **Solicitud**: `nombre_catacumba`
- **Respuesta**: Confirmación de eliminación o error

---

## 🗂️ Persistencia de Datos

### Archivo: `catacumbas_persistidas.dat`
- **Formato**: Binario
- **Estructura**: [número_catacumbas][struct catacumba][struct catacumba]...
- **Funciones**:
  - `cargarCatacumbas()`: Carga al iniciar el servidor
  - `guardarCatacumbas()`: Guarda después de modificaciones

### Características:
- ✅ **Automática**: Se guarda automáticamente después de agregar/eliminar
- ✅ **Completa**: Incluye todos los campos de la estructura `catacumba`
- ✅ **Robusta**: Manejo de errores de E/S

---

## 🚀 Compilación y Ejecución

### Compilar el Servidor:
```bash
gcc -Wall -Wextra -o server server.c
```

### Compilar el Cliente:
```bash
gcc -Wall -Wextra -o clienteD clienteD.c
```

### Ejecutar:
```bash
# 1. Iniciar el servidor (en una terminal)
./server

# 2. Iniciar el cliente (en otra terminal)
./clienteD
```

---

## 📋 Códigos de Respuesta

| Código | Constante               | Descripción                    |
| ------ | ----------------------- | ------------------------------ |
| 1      | `RESP_OK`               | Operación exitosa              |
| 2      | `RESP_ERROR`            | Error en la operación          |
| 3      | `RESP_NO_ENCONTRADO`    | Elemento no encontrado         |
| 4      | `RESP_LIMITE_ALCANZADO` | Máximo de catacumbas alcanzado |

---

## 🔧 Configuración

### Constantes principales (en `directorio.h`):
```c
#define MAX_CATACUMBAS 10        // Máximo número de catacumbas
#define MAX_NOM 50               // Longitud máxima del nombre
#define MAX_RUTA 100             // Longitud máxima de rutas
#define MAX_TEXT 4096            // Tamaño máximo de mensajes
#define MAX_DAT_RESP 4096        // Tamaño máximo de respuestas
```

### Claves de mailboxes:
```c
#define MAILBOX_KEY 12345              // Mailbox de solicitudes
#define MAILBOX_RESPUESTA_KEY 12346    // Mailbox de respuestas
```

---

## 🛡️ Manejo de Señales

### Terminación limpia:
- **SIGINT (Ctrl+C)**: Guarda estado y libera recursos
- **SIGTERM**: Terminación controlada del sistema
- **Limpieza automática**: Elimina mailboxes del sistema

---

## 🎯 Ejemplo de Uso

### Agregar una catacumba:
```
Entrada del cliente:
├─ Nombre: "MiCatacumba"
├─ Dirección: "/tmp/catacumba1.dat"
├─ Propiedades: "/tmp/props1.dat"
└─ Mailbox: "mailbox1"

Formato enviado: "MiCatacumba|/tmp/catacumba1.dat|/tmp/props1.dat|mailbox1"
```

### Respuesta del listado:
```
MiCatacumba|/tmp/catacumba1.dat|/tmp/props1.dat|mailbox1|0|0;OtraCat|/tmp/cat2.dat|/tmp/props2.dat|mailbox2|5|10
```

---

## 🔍 Notas Técnicas

### Cambios recientes:
- ✅ **Nuevo campo**: `propCatacumba` agregado a la estructura
- ✅ **Formato actualizado**: Ahora requiere 4 campos para agregar
- ✅ **Persistencia completa**: El nuevo campo se guarda automáticamente
- ✅ **Cliente actualizado**: Interfaz de usuario adaptada al nuevo formato

### Compatibilidad:
- ⚠️ **Archivos de persistencia antiguos**: No son compatibles con la nueva estructura
- 🔄 **Migración**: Eliminar `catacumbas_persistidas.dat` antes de usar la nueva versión

---

## 🐛 Solución de Problemas

### Error "No such file or directory" en mailboxes:
- **Causa**: El servidor no está ejecutándose
- **Solución**: Iniciar `./server` antes que el cliente

### Error de formato en agregar catacumba:
- **Causa**: Faltan campos en la entrada
- **Formato correcto**: `nombre|direccion|propiedades|mailbox`

### Error de límite alcanzado:
- **Causa**: Se alcanzó el máximo de 10 catacumbas
- **Solución**: Eliminar catacumbas existentes o aumentar `MAX_CATACUMBAS`

---

## 📚 Sistema de Comunicación IPC

### Mailboxes utilizados:
- **Solicitudes** (Clave: 12345): Cliente → Servidor
- **Respuestas** (Clave: 12346): Servidor → Cliente

### Protocolo de comunicación:
1. Cliente envía solicitud con su PID como `mtype`
2. Servidor procesa y responde usando el mismo PID
3. Cliente recibe solo sus respuestas usando filtro por PID

### Ejemplo de protocolo:
```c
// Cliente
pid_t mi_pid = getpid();
msg.mtype = mi_pid;  // Identificador único
msgsnd(mailbox_solicitudes, &msg, ...);

// Servidor  
resp.mtype = msg.mtype;  // Usar mismo PID para respuesta
msgsnd(mailbox_respuestas, &resp, ...);

// Cliente
msgrcv(mailbox_respuestas, &resp, ..., mi_pid, 0);  // Filtrar por PID
```

---

*Documentación actualizada - Versión con soporte completo para propiedades de catacumbas*
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

