#include <stdio.h>
#include <stdlib.h>
#include "persistencia.h"

/**
 * @brief Carga las catacumbas desde el archivo de persistencia al iniciar el servidor
 *
 * Lee el archivo binario donde se almacenan las catacumbas persistidas. Si el archivo
 * no existe o está vacío, la función retorna -1 y el servidor inicia con el directorio vacío.
 * Los datos se cargan en el array proporcionado y se actualiza el contador.
 *
 * @param catacumbas Array donde se cargarán las catacumbas desde el archivo
 * @param num_catacumbas Puntero al contador de catacumbas (se actualiza con el número cargado)
 * @return 0 si se carga correctamente, -1 si hay error o no existe el archivo
 **/
int cargarCatacumbas(struct catacumba catacumbas[], int *num_catacumbas)
{
    FILE *archivo = fopen(ARCHIVO_CATACUMBAS, "rb");
    if (archivo == NULL)
    {
        // El archivo no existe, es normal en la primera ejecución
        *num_catacumbas = 0;
        return -1;
    }

    // Leer el número de catacumbas del archivo
    size_t elementos_leidos = fread(num_catacumbas, sizeof(int), 1, archivo);
    if (elementos_leidos != 1)
    {
        printf("⚠️  Advertencia: No se pudo leer el contador del archivo de persistencia\n");
        fclose(archivo);
        *num_catacumbas = 0;
        return -1;
    }

    // Verificar que el número sea válido
    if (*num_catacumbas < 0 || *num_catacumbas > MAX_CATACUMBAS)
    {
        printf("⚠️  Advertencia: Número de catacumbas inválido en archivo (%d)\n", *num_catacumbas);
        fclose(archivo);
        *num_catacumbas = 0;
        return -1;
    }

    // Leer las catacumbas del archivo
    if (*num_catacumbas > 0)
    {
        elementos_leidos = fread(catacumbas, sizeof(struct catacumba), *num_catacumbas, archivo);
        if (elementos_leidos != (size_t)*num_catacumbas)
        {
            printf("⚠️  Advertencia: No se pudieron leer todas las catacumbas del archivo\n");
            printf("     Esperadas: %d, Leídas: %zu\n", *num_catacumbas, elementos_leidos);
            *num_catacumbas = (int)elementos_leidos; // Usar las que se pudieron leer
        }
    }

    fclose(archivo);

    // Mostrar resumen de carga
    if (*num_catacumbas > 0)
    {
        printf("   📋 Catacumbas cargadas desde archivo:\n");
        for (int i = 0; i < *num_catacumbas; i++)
        {
            printf("     %d. %-15s | %-20s | %-20s | %-10s\n",
                   i + 1, catacumbas[i].nombre, catacumbas[i].direccion,
                   catacumbas[i].propCatacumba, catacumbas[i].mailbox);
        }
    }

    return 0;
}

/**
 * @brief Guarda las catacumbas actuales en el archivo de persistencia
 *
 * Escribe todas las catacumbas del array en un archivo binario para persistir el estado
 * del directorio. Esto permite que el servidor mantenga la información entre reinicios.
 *
 * @param catacumbas Array de catacumbas a guardar en el archivo
 * @param num_catacumbas Número de catacumbas en el array
 * @return 0 si se guarda correctamente, -1 si hay error
 **/
int guardarCatacumbas(struct catacumba catacumbas[], int num_catacumbas)
{
    FILE *archivo = fopen(ARCHIVO_CATACUMBAS, "wb");
    if (archivo == NULL)
    {
        perror("Error al abrir archivo de persistencia para escritura");
        return -1;
    }

    // Escribir el número de catacumbas primero
    size_t elementos_escritos = fwrite(&num_catacumbas, sizeof(int), 1, archivo);
    if (elementos_escritos != 1)
    {
        printf("❌ Error al escribir contador de catacumbas\n");
        fclose(archivo);
        return -1;
    }

    // Escribir las catacumbas si hay alguna
    if (num_catacumbas > 0)
    {
        elementos_escritos = fwrite(catacumbas, sizeof(struct catacumba), num_catacumbas, archivo);
        if (elementos_escritos != (size_t)num_catacumbas)
        {
            printf("❌ Error al escribir catacumbas al archivo\n");
            printf("   Esperadas: %d, Escritas: %zu\n", num_catacumbas, elementos_escritos);
            fclose(archivo);
            return -1;
        }
    }

    fclose(archivo);
    printf("💾 Persistencia actualizada: %d catacumbas guardadas\n", num_catacumbas);
    return 0;
}
