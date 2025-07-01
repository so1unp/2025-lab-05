# Instructivo de Uso - Inicializador y Terminador de Servidores

Este documento explica cómo usar los scripts de automatización para ejecutar los servidores necesarios para el juego de catacumbas.

## 📋 Requisitos Previos

Antes de usar los scripts, asegúrate de tener:

1. **Archivos de configuración:**
   - `mapa.txt` - Mapa del juego
   - `config.properties` - Configuración del servidor

2. **Estructura del proyecto:**
   ```
   proyecto/
   ├── Makefile
   ├── catacumbas/
   ├── directorio/
   ├── clientes/
   ├── so.sh
   └── stop.sh
   ```

## 🚀 Script de Inicialización (`so.sh`)

### Paso 1: Dar Permisos de Ejecución

```bash
chmod +x ./so.sh
```

### Paso 2: Uso del Script

#### Mostrar Ayuda
```bash
./so.sh -h
# o
./so.sh --help
```

#### Iniciar un Servidor (por defecto)
```bash
./so.sh
# o
./so.sh up
```
- Compila e inicia 1 servidor de directorio
- Compila e inicia 1 servidor de catacumbas

#### Iniciar Múltiples Servidores de Catacumbas
```bash
./so.sh up 3
# o
./so.sh -u 3
```
- Compila e inicia 1 servidor de directorio
- Compila e inicia 3 servidores de catacumbas

### ¿Qué Hace el Script?

1. **Validación:**
   - Verifica que estés en el directorio correcto
   - Confirma la existencia de archivos necesarios
   - Valida los parámetros de entrada

2. **Compilación:**
   - Compila el servidor de directorio (`make directorio`)
   - Compila el servidor de catacumbas (`make catacumbas`)

3. **Ejecución:**
   - Inicia el servidor de directorio en segundo plano
   - Inicia los servidores de catacumbas especificados
   - Verifica que todos los procesos se inicien correctamente

4. **Monitoreo:**
   - Muestra el estado de todos los servidores en tiempo real
   - Actualiza la información cada 5 segundos
   - Permite detener todos los servidores con `Ctrl+C`

5. **Limpieza Automática:**
   - Al presionar `Ctrl+C` o cerrar el terminal
   - Detiene todos los procesos iniciados
   - Limpia recursos IPC (memoria compartida y colas de mensajes)

## 🛑 Script de Parada (`stop.sh`)

### Paso 1: Dar Permisos de Ejecución

```bash
chmod +x ./stop.sh
```

### Paso 2: Usar el Script

```bash
./stop.sh
```

### ¿Qué Hace el Script?

1. **Detener Procesos:**
   - Busca y termina todos los procesos `directorio-server`
   - Busca y termina todos los procesos `catacumbas-server`

2. **Limpieza de Recursos:**
   - Elimina colas de mensajes IPC del usuario
   - Elimina segmentos de memoria compartida del usuario
   - Elimina archivos de log temporales

3. **Confirmación:**
   - Muestra mensajes confirmando qué se detuvo
   - Informa cuando la limpieza está completa

## 📝 Ejemplos de Uso Completo

### Escenario 1: Juego Simple (1 servidor)
```bash
# Dar permisos (solo la primera vez)
chmod +x ./so.sh ./stop.sh

# Iniciar servidores
./so.sh

# En otra terminal, compilar y ejecutar cliente
make cliente
./cliente

# Cuando termines, detener servidores
# Opción 1: Ctrl+C en la terminal del script
# Opción 2: Desde otra terminal
./stop.sh
```

### Escenario 2: Servidor Múltiple (3 catacumbas)
```bash
# Iniciar múltiples servidores
./so.sh up 3
# o
./so.sh -u 3

# Los clientes podrán conectarse a cualquiera de las 3 catacumbas
# El servidor de directorio gestiona la lista de catacumbas disponibles

# Detener todos
./stop.sh
```

## 🔧 Opciones Avanzadas

### Parámetros del Script de Inicialización

| Parámetro    | Descripción                       | Ejemplo        |
| ------------ | --------------------------------- | -------------- |
| `-h, --help` | Muestra ayuda detallada           | `./so.sh -h`   |
| `up [N]`     | Inicia N servidores de catacumbas | `./so.sh up 5` |
| `-u [N]`     | Alias corto para 'up'             | `./so.sh -u 3` |

### Límites y Validaciones

- **Mínimo:** 1 servidor de catacumbas
- **Máximo:** 10 servidores (límite del sistema - MAX_CATACUMBAS)
- **Validación:** Solo acepta números enteros positivos
- **Auto-limitación:** Si solicitas más de 10, automáticamente se limita a 10

## 🐛 Solución de Problemas

### Error: "No se encontraron los directorios necesarios"
- **Causa:** No estás en el directorio raíz del proyecto
- **Solución:** Navega al directorio que contiene el `Makefile`

### Error: "No se encontró el archivo mapa.txt"
- **Causa:** Falta el archivo de configuración del mapa
- **Solución:** Verifica que existe `mapa.txt`

### Error: "El servidor no se pudo iniciar"
- **Causa:** Error de compilación o problema en el código
- **Solución:** Revisa los logs mostrados en pantalla para más detalles

### Los servidores no se detienen correctamente
- **Solución:** Ejecuta `./stop.sh` para limpieza forzada

### Recursos IPC no se liberan
```bash
# Ver recursos IPC en uso
ipcs

# Limpiar manualmente (si es necesario)
ipcs -q | grep $USER | awk '{print $2}' | xargs -r ipcrm -q
```

# EXTRA - Daemon para ejecutar directorio automáticamente

## ¿Qué es un Daemon?

Un **daemon** (o servicio) es un proceso que corre en segundo plano, sin necesidad de intervención humana directa (sin consola), y que inicia automáticamente al arrancar el sistema o cuando se necesita.

## ¿Qué es systemd?

**systemd** es un sistema de inicialización y gestión de servicios utilizado en la mayoría de las distribuciones modernas de Linux. Actúa como un gestor de procesos del sistema que controla cómo se inician, detienen y gestionan los servicios, demonios y recursos del sistema durante el arranque y la ejecución normal del sistema operativo.

## Configuración del Daemon

### 1. Verificar la Ubicación del Ejecutable

Asegúrate de que el script esté en el servidor:
```bash
/ruta/al/ejecutable
```

### 2. Crear el Archivo de Unidad systemd

Crear un archivo de servicio en `/etc/systemd/system/[nombre_del_archivo].service`:

```ini
[Unit]
Description=Mi daemon personalizado
After=network.target

[Service]
Type=simple
ExecStart=/bin/bash -c 'ruta/al/ejecutable & ruta/al/ejecutable_2'
Restart=always
User=usuario_del_sistema
WorkingDirectory=/directorio/donde/se/ejecuta
StandardOutput=append:/ruta/al/escribir_salida_estandar.log
StandardError=append:/ruta/al/escribir_salida_errores.log

[Install]
WantedBy=multi-user.target
```

### 3. Explicación de los Campos del Archivo .service

#### `[Unit]` - Información General del Servicio

| Campo         | Descripción                                                                             |
| ------------- | --------------------------------------------------------------------------------------- |
| `Description` | Breve descripción del servicio. Se ve al hacer `systemctl status` o en los logs         |
| `After`       | Indica que este servicio debe iniciarse después de que el sistema haya levantado la red |

#### `[Service]` - Configuración de Ejecución

| Campo              | Descripción                                                      |
| ------------------ | ---------------------------------------------------------------- |
| `ExecStart`        | El comando o ruta para ejecutar el servidor                      |
| `Restart`          | Indica si el servicio debe reiniciarse automáticamente si se cae |
| `User`             | Usuario del sistema Linux que ejecuta el servicio                |
| `WorkingDirectory` | Directorio donde el servicio se ejecuta (Opcional)               |
| `StandardOutput`   | A dónde se escribe la salida estándar                            |
| `StandardError`    | A dónde se escribe la salida de errores                          |

#### `[Install]` - Configuración de Arranque

| Campo      | Descripción                                                                       |
| ---------- | --------------------------------------------------------------------------------- |
| `WantedBy` | Define en qué nivel de arranque del sistema se activa automáticamente el servicio |

### 4. Habilitar y Arrancar el Servicio

#### Paso 1: Recargar systemd
```bash
sudo systemctl daemon-reexec
sudo systemctl daemon-reload
```

#### Paso 2: Habilitar el servicio para arranque automático
```bash
sudo systemctl enable [nombre_del_archivo].service
```

#### Paso 3: Iniciar el servicio
```bash
sudo systemctl start [nombre_del_archivo].service
```

> **Nota:** Esto hace que el servidor se inicie automáticamente cada vez que se encienda el servidor principal, y se reiniciará si falla.

## Instructivo de Uso - Daemon para Ejecutar Servidores de Catacumbas y Directorio

### 1. Darle permisos de ejecución al script.

   ```bash
   chmod +x /ruta/a/so.sh
   ```
### 2. Crear un archivo de servicio systemd.
   
   ```bash
   sudo nano /etc/systemd/system/catacumbas-daemon.service
   ```
   
   **Contenido:**
   
   ```bash
   [Unit]
   Description=Daemon para iniciar servidores de Catacumbas
   After=network.target

   [Service]
   Type=simple
   ExecStart=/ruta/a/sistemas_operativos/2025-lab-05/so.sh up
   WorkingDirectory=/ruta/a/sistemas_operativos/2025-lab-05
   Restart=on-failure

   [Install]
   WantedBy=multi-user.target
   ```
### 3. Recargar systemd y habilitar el servicio.
   ```bash
   sudo systemctl daemon-reexec
   sudo systemctl daemon-reload
   sudo systemctl enable catacumbas-daemon.service
   sudo systemctl start catacumbas-daemon.service
   ```
### 4. Verificar que el daemon esté corriendo.
   ```bash
   sudo systemctl status catacumbas-daemon.service
   ```
