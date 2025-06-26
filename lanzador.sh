#!/bin/bash
./directorio/server_original >/dev/null 2>&1 & 
sleep 1
./catacumbas/server catacumbas/mapa.txt catacumbas/config.properties D >/dev/null 2>&1 & 
sleep 1
./clientes/main

pkill -15 server_original
pkill -15 server
