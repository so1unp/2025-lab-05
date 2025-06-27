#!/bin/bash

# Script para detener todos los servidores
echo "🛑 Deteniendo servidores..."

# Matar procesos de servidores
pkill -f directorio-server 2>/dev/null && echo "📁 Servidor de directorio detenido"
pkill -f catacumbas-server 2>/dev/null && echo "🗿 Servidor de catacumbas detenido"

# Limpiar recursos IPC
echo "🧹 Limpiando recursos del sistema..."
ipcs -q | grep $(whoami) | awk '{print $2}' | xargs -r ipcrm -q 2>/dev/null
ipcs -m | grep $(whoami) | awk '{print $2}' | xargs -r ipcrm -m 2>/dev/null

# Limpiar archivos de log
rm -f directorio.log catacumbas.log 2>/dev/null

echo "✅ Limpieza completada"
