# Servidor

El servidor se encarga de gestionar **una catacumba**, que representa la arena donde se desarrolla una partida del juego.

Su funcionamiento se basa en un bucle donde **espera y atiende solicitudes de los clientes**, mientras queden tesoros en el mapa y haya exploradores activos que no hayan sido capturados.

Cuando la partida termina (ya sea porque se recolectaron todos los tesoros o porque todos los exploradores fueron capturados), el servidor **no finaliza su ejecución**, sino que **reinicia la partida de forma automática**:

- Restablece el mapa a su estado original (incluyendo paredes, espacios y tesoros).
- Reinicia los valores del estado (jugadores conectados, tesoros restantes, etc.).

De esta manera, al finalizar una partida, el servidor simplemente restablece los datos y comienza una nueva, **sin necesidad de reiniciar el proceso manualmente**. Esta lógica se explica con más detalle en [Conceptos de diseño](#conceptos-de-diseño).

## Arena

### Mapa

Es el escenario donde se desarrolla la partida. Está definido en el archivo [`mapa.txt`](../mapa.txt), que describe la disposición de paredes y espacios transitables.

> Se pueden encontrar otros mapas en el directorio `~/2025-lab-05/mapas/`.

**Formato del mapa:**

- Bidimensional, con tamaño fijo de **25 filas por 80 columnas**.
- Cada celda se representa con un carácter.

| Carácter  | Representa    | Descripción                           |
| --------- | ------------- | ------------------------------------- |
| `#`       | Pared         | Obstáculo, bloquea el movimiento      |
| (espacio) | Espacio libre | Zona transitable                      |
| `$`       | Tesoro        | Puede ser recolectado por los raiders |

**Reglas del mapa:**

- Solo se puede mover por espacios libres.
- Las paredes son obstáculos infranqueables.
- Los tesoros están ubicados en espacios libres y pueden ser recolectados por los raiders.

### Jugadores

Los tipos de jugadores aceptados son `exploradores` y `guardianes`.
La cantidad máxima está determinada por los valores configurados en [`config.properties`](../config.properties), donde se define el límite tanto para exploradores como para guardianes.

| Tipo                   | Descripción                              | Acciones principales                                                                |
| ---------------------- | ---------------------------------------- | ----------------------------------------------------------------------------------- |
| Exploradores (Raiders) | Jugadores que exploran y buscan tesoros. | - Conectarse y desconectarse <br> - Moverse en el mapa <br> - Recolectar tesoros    |
| Guardianes             | Jugadores que protegen la catacumba.     | - Conectarse y desconectarse <br> - Moverse en el mapa <br> - Capturar exploradores |

## Recursos de comunicación

### Memoria compartida

El servidor crea y administra dos espacios de memoria compartida el **mapa** y el **estado**

- **Mapa**:

  - Representa la estructura del juego (espacios libres, paredes, tesoros).
  - Es accedido en lectura y escritura **por el servidor** (por ejemplo, para inicializar o actualizar el mapa).
  - Los clientes acceden solo en lectura para conocer el terreno.

- **Estado**:
  - Contiene información dinámica del juego (cantidad de jugadores, tesoros, etc.).
  - Es accedido en lectura y escritura exclusivamente por el servidor.
  - Los clientes **no acceden directamente al estado**.

| Espacio de memoria | Acceso servidor     | Acceso clientes | Propósito                      |
| ------------------ | ------------------- | --------------- | ------------------------------ |
| Mapa               | Lectura y escritura | Solo lectura    | Mostrar y actualizar el mapa   |
| Estado             | Lectura y escritura | No acceden      | Mantener y controlar el estado |

### Mensajería

#### Comunicación con el directorio

Cuando el servidor se ejecuta por primera vez, envía una solicitud al **Directorio** para registrar la catacumba.

- Si recibe la respuesta **`R_LIMITE_ALCANZADO`**, significa que no hay espacio disponible para nuevas catacumbas, por lo que el servidor se cierra inmediatamente.
- Si ocurre un **error de comunicación** (por ejemplo, el Directorio está caído o no responde), también se procede al cierre del servidor.
- Si recibe **`RESP_OK`**, la catacumba queda registrada y el servidor pasa a estado operativo, quedando a la espera de solicitudes de los clientes.

#### Comunicación con los clientes:

Los clientes utilizan la mailbox para enviar mensajes al servidor, en los cuales incluyen un **código de operación** que define el tipo de solicitud.

El servidor recibe estas solicitudes y las despacha a las funciones correspondientes para su atención:

- `atenderConexion()`
- `atenderDesconexion()`
- `atenderMovimiento()`
- `atenderCapturaTesoro()`
- `atenderCapturaRaider()`

Luego de procesar cada solicitud, el servidor responde el resultado al cliente.

## Nombramiento de los recursos

### espacios de memoria

Los nombres de los espacios de memoria compartida se generan de manera dinámica para evitar conflictos entre múltiples servidores en ejecución.
Se construyen concatenando un prefijo fijo más el PID del proceso servidor.
| Espacio | Prefijo utilizado | Ejemplo de nombre |
| ------------------ | ------------------- | ------------------------ |
| Memoria del mapa | `/servidor-mapa-` | `/servidor-mapa-12345` |
| Memoria del estado | `/servidor-estado-` | `/servidor-estado-12345` |

Esto garantiza que cada instancia del servidor tenga sus propios recursos de memoria, evitando colisiones cuando se ejecutan múltiples servidores en la misma máquina.

### Mensajería

La **clave del mailbox de solicitudes** se genera utilizando la siguiente fórmula: 
$$
\text{clave\_mailbox} = PID \times MAILBOX\_SOLICITUDES\_SUFIJO
$$

Donde:

- `PID` es el identificador de proceso del servidor.
- `MAILBOX_SOLICITUDES_SUFIJO` es una constante con valor `10`.

## Eventos principales en una partida:

- **Captura de tesoros:** cuando un **raider** toma un tesoro, se actualiza el estado del juego. Si es el último tesoro, los raiders ganan la partida.
- **Captura de raiders:** cuando un **guardián** captura a un raider (por ejemplo, el guardián con PID 1025 captura al raider con PID 1026), el servidor desconecta al raider (1026) capturado y le envía un mensaje notificándole la captura. Si es el último raider, los guardianes ganan la partida.

## Codigos

### Códigos de solicitud (cliente → servidor)

| Código             | Valor | Descripción                                       |
| ------------------ | ----- | ------------------------------------------------- |
| `CONEXION`         | 5     | Solicita conectarse al servidor                   |
| `DESCONEXION`      | 6     | Solicita desconectarse del servidor               |
| `MOVIMIENTO`       | 7     | Envía una solicitud para moverse en el mapa       |
| `TESORO_CAPTURADO` | 8     | Notifica que un raider capturó un tesoro          |
| `RAIDER_CAPTURADO` | 9     | Notifica que un guardián ha capturado a un raider |

### Códigos de respuesta (servidor → cliente)

| Código        | Valor | Descripción                                      |
| ------------- | ----- | ------------------------------------------------ |
| `S_OK`        | 1     | Operación exitosa                                |
| `SIN_TESOROS` | 2     | No quedan tesoros, la partida terminó            |
| `SIN_RAIDERS` | 3     | No quedan raiders activos, la partida terminó    |
| `MUERTO`      | 4     | El jugador ha sido capturado (solo para raiders) |
| `ERROR`       | -1    | Ocurrió un error en la operación                 |

## Estructuras principales

El servidor se apoya en dos estructuras principales: **`Arena`** y **`Comunicacion`**.  
Ambas encapsulan los datos del juego y los recursos de comunicación necesarios para la gestión de la partida.

### `struct Arena`

Guarda toda la información del juego:

- El mapa (memoria compartida).
- El estado general (jugadores conectados, tesoros, etc., también en memoria compartida).
- Listado de tesoros y jugadores.
- Los límites de la partida y tamaños de memoria.

```c
struct Arena {
    struct Tesoro tesoros[MAX_TESOROS];
    struct Jugador jugadores[MAX_JUGADORES];
    struct Estado *estado;
    char (*mapa)[COLUMNAS];
    int max_guardianes;
    int max_raiders;
    int max_tesoros;
    int size_mapa;
    int size_estado;
};
```

### `struct Comunicacion`

Administra los recursos para que el servidor se comunique:

- Nombres y descriptores de la memoria compartida (mapa y estado).
- Mailbox para recibir solicitudes de clientes.
- Mailboxes para comunicarse con el directorio.

```c
struct Comunicacion {
    char memoria_mapa_nombre[128];
    char memoria_estado_nombre[128];
    int mailbox_solicitudes_clave;
    int memoria_mapa_fd;
    int memoria_estado_fd;
    int mailbox_solicitudes_id;
    int mailbox_directorio_solicitudes_id;
    int mailbox_directorio_respuestas_id;
};
```

### Resumen de estructuras principales

| Estructura     | Función / Propósito                                 |
| -------------- | --------------------------------------------------- |
| `Arena`        | Representa el estado y entorno completo del juego.  |
| `Comunicacion` | Gestiona los recursos y mecanismos de comunicación. |
| `Jugador`      | Contiene la información básica de cada jugador.     |
| `Tesoro`       | Representa los tesoros en el mapa.                  |
| `Estado`       | Controla el estado dinámico de la partida.          |

### Diagrama de clases

Representacion de cómo se relacionan las estructuras principales (`Arena`, `Comunicacion`, `Jugador`, `Tesoro` y `Estado`)

![diagrama de clases](/catacumbas/assets/Server-Documentacion.jpg)

## Uso

El servidor requiere dos archivos para su funcionamiento:

- [`mapa.txt`](../mapa.txt): define el mapa del juego.
- [`config.properties`](../config.properties): contiene la configuración general.

Ejemplo de ejecución:

```bash
make server      #  compilar el servidor
./server mapa.txt config.properties #  ejecucion del servidor
```

## Salida Limpia

Al presionar `Ctrl + C` o al recibir la señal `SIGINT`, el servidor realiza un cierre ordenado que incluye:

- Notificar a todos los jugadores que la partida ha finalizado.
- Eliminar las **mailboxes** utilizadas para la mensajería.
- Liberar los espacios de **memoria compartida**

> **Atencion:** En caso de no realizar una salida limpia, pueden quedar recursos persistentes en el sistema (mailboxes o memoria compartida).  
> Se recomienda verificar con los siguientes comandos y eliminarlos manualmente si es necesario:
>
> ```bash
> ipcs               # Lista recursos IPC activos
> ipcrm -q <id>      # Elimina una mailbox (cola de mensajes)
> ipcrm -m <id>      # Elimina un segmento de memoria compartida
> ls /dev/shm        # Lista archivos de memoria compartida en sistemas Linux modernos
> rm /dev/shm/<archivo>  # Elimina archivos residuales de memoria compartida (usar con precaución)
> ```

## Conceptos de diseño

### Ejecución continua del servidor

#### ¿Por qué el servidor se ejecuta continuamente sin detenerse?

El servidor está diseñado para ejecutarse de manera continua, reiniciando automáticamente la partida cuando termina, sin necesidad de cerrarse y volver a iniciarse manualmente.

Aunque se podría automatizar el arranque y cierre con un script externo, mantener una única ejecución constante ofrece importantes beneficios:

- Evita el _overhead_ (sobrecarga) asociado a iniciar y finalizar procesos repetidamente, que incluye consumo de CPU, memoria y tiempo para cargar y liberar recursos.
- Permite la reutilización eficiente de recursos compartidos (memoria, colas de mensajes), mejorando el rendimiento general.
- Reduce la carga operativa y el riesgo de errores que pueden surgir al reiniciar manualmente el servidor.
- Garantiza una experiencia fluida para los jugadores, con partidas consecutivas sin interrupciones.
- Simplifica la gestión y el ciclo de vida del servidor, manteniéndolo activo hasta que se reciba una señal explícita de cierre.

### Uso de un hilo

#### ¿Por qué un solo hilo?

El servidor funciona con un **único hilo** para atender a los clientes. Esta decisión se tomó porque, en el contexto del proyecto, es la opción más simple y eficiente.

- Menor cantidad de jugadores.
- Flujo de solicitudes simple y controlado.
- Menor complejidad: sin mutex ni sincronización.
- Evita errores por concurrencia.
- Buen rendimiento para los objetivos actuales.

Si en el futuro el proyecto creciera (más jugadores, más carga), se podría pasar a un modelo con múltiples hilos.

### Modularización y división de responsabilidades

El servidor está dividido en varios módulos para mantener el código organizado y facilitar su mantenimiento.

- **server.c:** Es el punto de entrada principal del servidor. Se encarga de la inicialización, del ciclo principal que recibe y procesa las solicitudes de los clientes, y de la gestión general del juego.
- **solicitudes.c:** Contiene las funciones que gestionan la comunicación del servidor tanto con los clientes como con el directorio. Incluye el registro de la catacumba y el manejo de las solicitudes enviadas por los jugadores.
- **utils.c:** Agrupa funciones auxiliares y utilitarias que son usadas en distintos módulos.
- **config.c:** Carga y procesa la configuración del juego desde el archivo `config.properties`.

Esta organización permite que el código sea más claro, modular y fácil de mantener o ampliar.

```bash
catacumbas/
├── catacumbas.h
├── config.c
├── config.h
├── server.c
├── solicitudes.c
├── solicitudes.h
├── utils.c
├── utils.h
└── ...
```

## Limitaciones y mejoras futuras

- Actualmente el servidor funciona con un solo hilo, lo cual es suficiente para este contexto académico.
- En un entorno con mayor carga o más jugadores concurrentes, sería recomendable implementar un modelo multihilo
- Se podrían agregar logs, validación de errores más robusta y persistencia del estado del servidor.
