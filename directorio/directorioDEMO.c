/**
 * @file server.c
 * @brief Gestor de memoria compartida para el directorio de catacumbas
 *
 * Este programa permite crear, listar y eliminar un segmento de memoria compartida
 * utilizando la API POSIX de memoria compartida (shm_open, mmap, etc.).
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>

/** Nombre del segmento de memoria compartida */
#define SHM_NAME "/directorio_catacumbas"
/** Número máximo de catacumbas en el directorio */
#define MAX_CATACUMBAS 16
/** Longitud máxima del nombre de una catacumba */
#define MAX_NOMBRE_CATACUMBA 32

/**
 * @brief Estructura que representa una catacumba
 */
typedef struct
{
    char nombre[MAX_NOMBRE_CATACUMBA];
    int activa; // 1 = activa, 0 = no activa
} Catacumba;

/**
 * @brief Estructura del directorio de catacumbas
 */
typedef struct
{
    int num_catacumbas; // Número actual de catacumbas
    Catacumba catacumbas[MAX_CATACUMBAS];
} Directorio;

/** Tamaño del segmento de memoria compartida calculado según la estructura */
#define SHM_SIZE (sizeof(Directorio))

/**
 * @brief Crea un nuevo segmento de memoria compartida
 *
 * Utiliza shm_open con O_CREAT | O_EXCL para asegurar que el segmento
 * no exista previamente. Establece el tamaño con ftruncate.
 */
void crear_segmento()
{
    int fd = shm_open(SHM_NAME, O_CREAT | O_EXCL | O_RDWR, 0666);
    if (fd == -1)
    {
        perror("shm_open (crear)");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(fd, SHM_SIZE) == -1)
    {
        perror("ftruncate");
        close(fd);
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }

    // Inicializar el directorio
    void *ptr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED)
    {
        perror("mmap");
        close(fd);
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }

    // Inicializar el directorio a ceros
    Directorio *directorio = (Directorio *)ptr;
    memset(directorio, 0, sizeof(Directorio));

    munmap(ptr, SHM_SIZE);
    printf("🔨 Segmento creado en /dev/shm/%s (tamaño: %zu bytes, capacidad: %d catacumbas)\n",
           SHM_NAME + 1, SHM_SIZE, MAX_CATACUMBAS);
    close(fd);
}

/**
 * @brief Lista el contenido del segmento de memoria compartida
 *
 * Abre el segmento en modo lectura, lo mapea en el espacio de direcciones
 * del proceso y muestra el contenido del directorio.
 */
void listar_segmento()
{
    int fd = shm_open(SHM_NAME, O_RDONLY, 0666);
    if (fd == -1)
    {
        perror("shm_open (listar)");
        exit(EXIT_FAILURE);
    }

    void *ptr = mmap(NULL, SHM_SIZE, PROT_READ, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED)
    {
        perror("mmap");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Mostrar información del directorio
    Directorio *directorio = (Directorio *)ptr;
    printf("📋 DIRECTORIO DE CATACUMBAS (%d/%d catacumbas)\n",
           directorio->num_catacumbas, MAX_CATACUMBAS);
    printf("------------------------------------\n");

    for (int i = 0; i < directorio->num_catacumbas; i++)
    {
        printf("%2d. %s [%s]\n",
               i + 1,
               directorio->catacumbas[i].nombre,
               directorio->catacumbas[i].activa ? "ACTIVA" : "INACTIVA");
    }

    if (directorio->num_catacumbas == 0)
    {
        printf("(El directorio está vacío)\n");
    }

    printf("\n");

    // También mostrar vista hexadecimal para depuración
    printf("📦 Contenido crudo de la memoria compartida (primeros 64 bytes):\n");
    unsigned char *bytes = (unsigned char *)ptr;
    for (int i = 0; i < (SHM_SIZE < 64 ? SHM_SIZE : 64); i++)
    {
        printf("%02X ", bytes[i]);
        if ((i + 1) % 16 == 0)
            printf("\n");
    }
    if ((SHM_SIZE % 16) != 0)
    {
        printf("\n");
    }

    munmap(ptr, SHM_SIZE);
    close(fd);
}

/**
 * @brief Agrega una nueva catacumba al directorio
 *
 * @param nombre Nombre de la catacumba a agregar
 */
void agregar_catacumba(const char *nombre)
{
    if (strlen(nombre) >= MAX_NOMBRE_CATACUMBA)
    {
        fprintf(stderr, "❌ Error: El nombre de la catacumba es demasiado largo (máximo %d caracteres)\n",
                MAX_NOMBRE_CATACUMBA - 1);
        exit(EXIT_FAILURE);
    }

    int fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (fd == -1)
    {
        perror("shm_open (agregar)");
        exit(EXIT_FAILURE);
    }

    void *ptr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED)
    {
        perror("mmap");
        close(fd);
        exit(EXIT_FAILURE);
    }

    Directorio *directorio = (Directorio *)ptr;

    // Verificar si el directorio está lleno
    if (directorio->num_catacumbas >= MAX_CATACUMBAS)
    {
        fprintf(stderr, "❌ Error: El directorio está lleno (máximo %d catacumbas)\n", MAX_CATACUMBAS);
        munmap(ptr, SHM_SIZE);
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Verificar si la catacumba ya existe
    for (int i = 0; i < directorio->num_catacumbas; i++)
    {
        if (strcmp(directorio->catacumbas[i].nombre, nombre) == 0)
        {
            fprintf(stderr, "❌ Error: La catacumba '%s' ya existe\n", nombre);
            munmap(ptr, SHM_SIZE);
            close(fd);
            exit(EXIT_FAILURE);
        }
    }

    // Agregar la nueva catacumba
    strcpy(directorio->catacumbas[directorio->num_catacumbas].nombre, nombre);
    directorio->catacumbas[directorio->num_catacumbas].activa = 1;
    directorio->num_catacumbas++;

    printf("✅ Catacumba '%s' agregada al directorio (total: %d/%d)\n",
           nombre, directorio->num_catacumbas, MAX_CATACUMBAS);

    munmap(ptr, SHM_SIZE);
    close(fd);
}

/**
 * @brief Quita una catacumba del directorio
 *
 * @param nombre Nombre de la catacumba a quitar
 */
void quitar_catacumba(const char *nombre)
{
    int fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (fd == -1)
    {
        perror("shm_open (quitar)");
        exit(EXIT_FAILURE);
    }

    void *ptr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED)
    {
        perror("mmap");
        close(fd);
        exit(EXIT_FAILURE);
    }

    Directorio *directorio = (Directorio *)ptr;
    int encontrada = 0;

    for (int i = 0; i < directorio->num_catacumbas; i++)
    {
        // Si encontramos la catacumba
        if (strcmp(directorio->catacumbas[i].nombre, nombre) == 0)
        {
            // Mover todas las catacumbas siguientes una posición hacia atrás
            for (int j = i; j < directorio->num_catacumbas - 1; j++)
            {
                strcpy(directorio->catacumbas[j].nombre, directorio->catacumbas[j + 1].nombre);
                directorio->catacumbas[j].activa = directorio->catacumbas[j + 1].activa;
            }
            directorio->num_catacumbas--;
            encontrada = 1;
            break;
        }
    }

    if (encontrada)
    {
        printf("🗑 Catacumba '%s' eliminada del directorio (total: %d/%d)\n",
               nombre, directorio->num_catacumbas, MAX_CATACUMBAS);
    }
    else
    {
        fprintf(stderr, "❌ Error: La catacumba '%s' no existe en el directorio\n", nombre);
    }

    munmap(ptr, SHM_SIZE);
    close(fd);
}

/**
 * @brief Elimina el segmento de memoria compartida
 *
 * Elimina el segmento utilizando shm_unlink, lo que libera los recursos
 * asociados y elimina la entrada en /dev/shm/.
 */
void borrar_segmento()
{
    if (shm_unlink(SHM_NAME) == -1)
    {
        perror("shm_unlink");
        exit(EXIT_FAILURE);
    }

    printf("🗑 Segmento eliminado: /dev/shm/%s\n", SHM_NAME + 1);
}

/**
 * @brief Muestra las opciones de uso del programa
 * @param prog Nombre del programa
 */
void mostrar_uso(const char *prog)
{
    printf("📖 Uso: %s [opción] [argumentos]\n", prog);
    printf("Opciones:\n");
    printf("  -c          Crear el segmento de memoria compartida\n");
    printf("  -l          Listar el directorio de catacumbas\n");
    printf("  -d          Borrar el segmento\n");
    printf("  -a nombre   Agregar una catacumba\n");
    printf("  -r nombre   Quitar una catacumba\n");
    printf("  -h          Mostrar esta ayuda\n");
}

/**
 * @brief Punto de entrada principal
 *
 * Procesa los argumentos de línea de comandos y ejecuta la función
 * correspondiente según la opción seleccionada.
 *
 * @param argc Número de argumentos
 * @param argv Vector de argumentos
 * @return 0 si la ejecución fue exitosa
 */
int main(int argc, char *argv[])
{
    int opt;

    if (argc < 2)
    {
        mostrar_uso(argv[0]);
        exit(EXIT_FAILURE);
    }

    while ((opt = getopt(argc, argv, "clda:r:h")) != -1)
    {
        switch (opt)
        {
        case 'c':
            crear_segmento();
            break;
        case 'l':
            listar_segmento();
            break;
        case 'd':
            borrar_segmento();
            break;
        case 'a':
            if (!optarg)
            {
                fprintf(stderr, "❌ Error: Debe especificar el nombre de la catacumba\n");
                exit(EXIT_FAILURE);
            }
            agregar_catacumba(optarg);
            break;
        case 'r':
            if (!optarg)
            {
                fprintf(stderr, "❌ Error: Debe especificar el nombre de la catacumba\n");
                exit(EXIT_FAILURE);
            }
            quitar_catacumba(optarg);
            break;
        case 'h':
            mostrar_uso(argv[0]);
            break;
        default:
            mostrar_uso(argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}
