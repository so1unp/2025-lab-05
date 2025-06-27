#!/bin/bash

# Script para inicializar los servidores necesarios para el juego de catacumbas

# Variables por defecto
CATACUMBAS_COUNT=1
SHOW_HELP=false

# Colores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Función para imprimir mensajes con color
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Función para mostrar ayuda
show_help() {
    echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
    echo -e "${BLUE}           INICIALIZADOR DE SERVIDORES DE CATACUMBAS           ${NC}"
    echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
    echo ""
    echo -e "${GREEN}DESCRIPCIÓN:${NC}"
    echo "  Este script compila e inicia los servidores necesarios para el juego"
    echo "  de catacumbas: un servidor de directorio y uno o más servidores de"
    echo "  catacumbas según se especifique."
    echo ""
    echo -e "${GREEN}USO:${NC}"
    echo "  $0 [OPCIONES]"
    echo ""
    echo -e "${GREEN}OPCIONES:${NC}"
    echo -e "  ${YELLOW}-h, --help${NC}     Muestra esta ayuda y sale"
    echo -e "  ${YELLOW}-c, --count N${NC}  Inicia N servidores de catacumbas (por defecto: 1)"
    echo ""
    echo -e "${GREEN}EJEMPLOS:${NC}"
    echo "  $0                 # Inicia 1 servidor de directorio + 1 de catacumbas"
    echo "  $0 -c 3            # Inicia 1 servidor de directorio + 3 de catacumbas"
    echo "  $0 --count 5       # Inicia 1 servidor de directorio + 5 de catacumbas"
    echo "  $0 -h              # Muestra esta ayuda"
    echo ""
    echo -e "${GREEN}ARCHIVOS REQUERIDOS:${NC}"
    echo "  • catacumbas/mapa.txt         - Mapa del juego"
    echo "  • catacumbas/config.properties - Configuración del servidor"
    echo ""
    echo -e "${GREEN}FUNCIONAMIENTO:${NC}"
    echo "  • Los servidores se ejecutan en segundo plano"
    echo "  • El output se muestra directamente en la terminal"
    echo "  • Presiona Ctrl+C para detener todos los servidores"
    echo "  • Los recursos IPC se limpian automáticamente al salir"
    echo ""
    exit 0
}

# Función para parsear argumentos
parse_arguments() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                SHOW_HELP=true
                shift
                ;;
            -c|--count)
                if [[ -n $2 && $2 =~ ^[1-9][0-9]*$ ]]; then
                    CATACUMBAS_COUNT=$2
                    shift 2
                else
                    print_error "Error: -c/--count requiere un número válido mayor a 0"
                    echo "Usa '$0 -h' para ver la ayuda"
                    exit 1
                fi
                ;;
            *)
                print_error "Opción desconocida: $1"
                echo "Usa '$0 -h' para ver la ayuda"
                exit 1
                ;;
        esac
    done
    
    if [ "$SHOW_HELP" = true ]; then
        show_help
    fi
    
    # Validar límites razonables
    if [ "$CATACUMBAS_COUNT" -gt 10 ]; then
        print_warning "Advertencia: Iniciar más de 10 servidores puede ser intensivo en recursos"
        read -p "¿Continuar? (y/N): " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            print_info "Operación cancelada"
            exit 0
        fi
    fi
}

# Función para limpiar procesos al salir
cleanup() {
    print_warning "Deteniendo servidores..."
    
    # Matar los servidores de catacumbas si están ejecutándose
    for i in $(seq 1 $CATACUMBAS_COUNT); do
        pid_var="CATACUMBAS_PID_$i"
        pid=${!pid_var}
        if [ ! -z "$pid" ]; then
            kill $pid 2>/dev/null
            print_info "Servidor de catacumbas $i detenido (PID: $pid)"
        fi
    done
    
    # Matar el servidor de directorio si está ejecutándose
    if [ ! -z "$DIRECTORIO_PID" ]; then
        kill $DIRECTORIO_PID 2>/dev/null
        print_info "Servidor de directorio detenido (PID: $DIRECTORIO_PID)"
    fi
    
    # Limpiar recursos IPC si es necesario
    print_info "Limpiando recursos del sistema..."
    ipcs -q | grep $(whoami) | awk '{print $2}' | xargs -r ipcrm -q 2>/dev/null
    ipcs -m | grep $(whoami) | awk '{print $2}' | xargs -r ipcrm -m 2>/dev/null
    
    print_success "Limpieza completada"
    exit 0
}

# Configurar trap para cleanup
trap cleanup SIGINT SIGTERM EXIT

# Parsear argumentos de línea de comandos
parse_arguments "$@"

print_info "═══════════════════════════════════════════════════════════════"
print_info "           INICIALIZADOR DE SERVIDORES DE CATACUMBAS           "
print_info "═══════════════════════════════════════════════════════════════"
print_info "Configuración:"
print_info "  • Servidores de catacumbas a iniciar: $CATACUMBAS_COUNT"
print_info "═══════════════════════════════════════════════════════════════"

# Verificar que estamos en el directorio correcto
if [ ! -f "Makefile" ] || [ ! -d "catacumbas" ] || [ ! -d "directorio" ]; then
    print_error "Error: No se encontraron los directorios necesarios."
    print_error "Asegúrate de ejecutar este script desde el directorio raíz del proyecto."
    exit 1
fi

# Verificar que existen los archivos necesarios
if [ ! -f "catacumbas/mapa.txt" ]; then
    print_error "Error: No se encontró el archivo catacumbas/mapa.txt"
    exit 1
fi

if [ ! -f "catacumbas/config.properties" ]; then
    print_error "Error: No se encontró el archivo catacumbas/config.properties"
    exit 1
fi

print_info "Archivos de configuración encontrados ✓"

# Paso 1: Compilar el servidor de directorio
print_info "Compilando servidor de directorio..."
if make directorio > /dev/null 2>&1; then
    print_success "Servidor de directorio compilado exitosamente"
else
    print_error "Error al compilar el servidor de directorio"
    exit 1
fi

# Paso 2: Iniciar el servidor de directorio
print_info "Iniciando servidor de directorio..."
./directorio-server &
DIRECTORIO_PID=$!

# Esperar un momento para que el servidor se inicialice
sleep 2

# Verificar que el servidor de directorio esté ejecutándose
if ! kill -0 $DIRECTORIO_PID 2>/dev/null; then
    print_error "Error: El servidor de directorio no se pudo iniciar"
    exit 1
fi

print_success "Servidor de directorio iniciado (PID: $DIRECTORIO_PID)"

# Paso 3: Compilar el servidor de catacumbas
print_info "Compilando servidor de catacumbas..."
if make catacumbas > /dev/null 2>&1; then
    print_success "Servidor de catacumbas compilado exitosamente"
else
    print_error "Error al compilar el servidor de catacumbas"
    exit 1
fi

# Paso 4: Iniciar los servidores de catacumbas
print_info "Iniciando $CATACUMBAS_COUNT servidor(es) de catacumbas..."

for i in $(seq 1 $CATACUMBAS_COUNT); do
    print_info "  Iniciando servidor de catacumbas #$i..."
    
    # Iniciar el servidor sin redirección de logs
    ./catacumbas-server ./catacumbas/mapa.txt ./catacumbas/config.properties &
    
    # Guardar el PID en una variable dinámica
    pid=$!
    declare "CATACUMBAS_PID_$i=$pid"
    
    # Esperar un momento para que el servidor se inicialice
    sleep 2
    
    # Verificar que el servidor esté ejecutándose
    if ! kill -0 $pid 2>/dev/null; then
        print_error "Error: El servidor de catacumbas #$i no se pudo iniciar"
        exit 1
    fi
    
    print_success "Servidor de catacumbas #$i iniciado (PID: $pid)"
done

print_info "═══════════════════════════════════════════════════════════════"
print_success "           TODOS LOS SERVIDORES ESTÁN EJECUTÁNDOSE              "
print_info "═══════════════════════════════════════════════════════════════"
print_info ""
print_info "Servidores activos:"
print_info "  • Directorio: PID $DIRECTORIO_PID"

# Mostrar información de todos los servidores de catacumbas
for i in $(seq 1 $CATACUMBAS_COUNT); do
    pid_var="CATACUMBAS_PID_$i"
    pid=${!pid_var}
    print_info "  • Catacumbas #$i: PID $pid"
done

print_info ""
print_info "Ahora puedes ejecutar los clientes para jugar:"
print_info "  • Para compilar clientes: make -C clientes"
print_info "  • Para ejecutar cliente: ./clientes/main"
print_info ""
print_success "¡Sistema listo! Los servidores están ejecutándose en segundo plano."
print_warning "Presiona Ctrl+C para detener todos los servidores cuando termines."
print_info ""
print_info "Monitoreando estado de servidores..."

# Función simplificada para monitorear procesos
monitor_servers() {
    while true; do
        sleep 3
        
        # Verificar estado de los servidores
        all_running=true
        
        # Verificar directorio
        if ! kill -0 $DIRECTORIO_PID 2>/dev/null; then
            print_error "⚠️  Servidor de directorio se detuvo inesperadamente (PID: $DIRECTORIO_PID)"
            all_running=false
        fi
        
        # Verificar catacumbas
        for i in $(seq 1 $CATACUMBAS_COUNT); do
            pid_var="CATACUMBAS_PID_$i"
            pid=${!pid_var}
            if ! kill -0 $pid 2>/dev/null; then
                print_error "⚠️  Servidor de catacumbas #$i se detuvo inesperadamente (PID: $pid)"
                all_running=false
            fi
        done
        
        if [ "$all_running" = false ]; then
            print_error "Algunos servidores se detuvieron. Iniciando limpieza..."
            break
        fi
    done
}

# Ejecutar el monitoreo
monitor_servers
