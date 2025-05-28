![The Raiders of the Lost System Call](assets/title.png)

Los intrépidos exploradores deben ingresar bajo su propio riesgo a peligrosas catacumbas en donde buscarán tesoros antigüos para sus colecciones, custodiados por implacables guardianes.

# Objetivo

El objetivo de este trabajo es hacer uso de los servicios del sistema operativo (IPC, semáforos, sistema de archivos, administración de memoria, etc.) mediante el diseño e implementación de un sencillo juego multiusuario.

Se debe desarrollar en grupos, cada uno encarando el desarrollo de una parte del mismo. El documento describe los lineamientos principales del juego, pero quedan muchos detalles por definir: es aquí donde deberán diseñar y acordar entre los grupos.

# Descripción

El objetivo del juego, para los raiders, es apropiarse de la mayor cantidad posible de tesoros. Para los guardianes, la meta es detener a los exploradores y evitar el pillaje. La acción se desarrolla en diferentes catacumbas, donde se encuentran esparcidos los tesoros.

El videojuego debe funcionar en modo texto, simulando los entornos para explorar. Los jugadores, ya sean exploradores o guardianes, actúan como clientes y se conectan a las catacumbas, que funcionan como servidores. Existe un servidor especial, denominado Directorio, donde se encuentra un listado de las catacumbas disponibles y su estado actual. Los raiders y los guardianes deciden en que catacumba participar inspeccionando el Directorio.

Durante la ejecución del juego, es suficiente con que un explorador se tope con un guardián para que este lo expulse de la catacumba. Al ser expulsado el raider pierde todos los tesoros que recolectó en dicha catacumba. Cada explorador interceptado suma puntos al guardián que lo expulsó. A su vez, cada vez que un raider escapa de la catacumba suma puntos en base a los tesoros recuperados de la misma.

# Componentes

## Raiders y Guardianes (clientes)

* Los clientes se conectan a las catacumbas.  
* Seleccionan la catacumba a donde ingresar mediante el servidor Directorio.  
* Deben procesar los eventos de usuario y de la catacumba (por ejemplo, enviar la nueva posición del cliente en base a sus movimientos, reflejar los sucesos indicados por el servidor en la catacumba, etc).

## Catacumbas (servidor)

Es donde se desarrolla la acción principal del juego. Cada catacumba representa una arena única de juego, con una disposición propia y tesoros.

* Las arenas deben ser por defecto de 80x25 caracteres.  
* Deben gestionar los tesoros (generación, ubicación, etc).  
* Deben especificar un número máximo de participantes.  
* Deben mantener y difundir el estado de la catacumba (ubicación de los jugadores, tesoros, etc).  
* Debe coordinar la acción (procesar acciones de los jugadores, forzar las reglas, etc)

## Directorio (servidor)

Actúa como punto de encuentro central, en donde seleccionar la catacumba donde ingresar.

* Mantiene una lista de las catacumbas actualmente disponibles, que debe actualizarse de manera dinámica.  
* Las catacumbas se deben registrar en el directorio al momento de crearse (y darse de baja al ser eliminadas).  
* Debe permitir a un raider poder conectarse a una catacumba, ofreciendo la información necesaria.  
* Opcionalmente puede indicar: números de participantes en la misma y otros datos.

# Servicios del SO a utilizar

A continuación se presenta una lista de los servicios que se deben utilizar, sugiriendo que función cumplen en el juego (pueden decidir utilizarlos para otra funcionalidad si así lo consideran conveniente):

* Memoria compartida  
  * El directorio puede utilizar memoria compartida para presentar a los clientes las catacumbas disponibles (en modo lectura).  
  * Se puede utilizar también para presentar, en modo lectura, el estado de las catacumbas. Los clientes (raiders o guardianes) leen el estado y lo presentan por pantalla al jugador.  
* Paso de mensajes  
  * Entre cliente-catacumba: enviar acciones del jugador en un mensaje (por ejemplo los movimientos, acciones, mensajes a otros jugadores, etc).  
  * Entre cliente-directorio: consultar catacumbas, solicitar conexión, etc.  
  * Entre catacumba-cliente: mensajes del juego, resultado de acciones, etc.  
  * Entre catacumba-directorio: registrarse, eliminar catacumba, estado del juego, etc.  
* Semáforos   
  * Para la sincronización en el acceso a recursos compartidos, y evitar así inconsistencias.  
* E/S  
  * Modo crudo (raw mode): permite la lectura de las teclas presionadas sin esperar un Enter y actualizar la consola para presentar de manera fluida la acción en la catacumba. Utilizar la librería *ncurses*.  
* Sistemas de archivos  
  * Para persistir la configuración y el estado del juego.  
  * Clientes: nombre del jugador, perfil, opciones de configuración.  
  * Catacumba: diseño del mapa de la catacumba, configuración, etc.  
  * Directorio: configuración (por ejemplo, número máximo de catacumbas, etc).

# Otras consideraciones

* Los servidores deben ser concurrentes, para poder administrar varios jugadores simultáneamente.  
* Los clientes pueden ser también concurrentes, aunque no es estrictamente necesario.  
* Se debe tener en cuenta las situaciones de error o excepciones (desconexión de clientes o de las catacumbas, etc).  
* Diseño modular: estructurar los programas de manera clara, evitando que los distintos componentes estén acoplados.

# Otras cuestiones importantes

Quedan muchas cuestiones de diseño abiertas y deben ser definidas. Por ejemplo:

* ¿Qué sucede cuando un explorador toma un tesoro? ¿Puede tomar más de un tesoro? ¿Debe escapar por un punto del mapa específico?  
* ¿Existen otras maneras en que los guardianes a los raiders?  
* ¿Los raiders pueden atacar a los guardianes? ¿O solo pueden escapar de ellos?  
* ¿Existe la posibilidad de contar con armas?  
* ¿Cómo es la visibilidad del mapa? ¿Parcial o total?

