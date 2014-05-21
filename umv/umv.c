/* Para Eclipse agregar commons, Rutinas y pthread en properties/ c-c++ build / settings / libraries */
/* Para compilar por consola: -lRutinas -lcommons -lpthread */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <sockets.h>
#include <string.h>
#include "atencion.h"

int main(int argc, char **argv){
/* Creo la lista que va a tener un elemento por cada hilo lanzado */
	hilos = list_create();
	printf("Lista de hilos creada\n");

/* Obtener datos del archivo de configuracion */
	//TODO Obtener tamaño del bloque de memoria
	//TODO Obtener puerto en el que se va a estar escuchando
	int tamanioMemoria=1024; // el 1024 se cambiara por lo que obtenga del archivo de config
	int puerto = 5021; // el puerto se obtendra por archivo de config
/* Fin obtener datos */
	printf("El tamaño de memoria es %d y el puerto es %d\n",tamanioMemoria,puerto);

/* Pido la memoria que va a tener disponible el sistema */
	bloqueDeMemoria = malloc(tamanioMemoria);
	printf("Se creo el bloque de memoria con direccion inicial %p\n",&bloqueDeMemoria);

/* Creo lista para administrar el bloque de memoria */
	segmentos = list_create();
	//Inicializo mutex de la lista
	pthread_mutex_init(mutexSegmentos,NULL);
	printf("Lista de segmentos creada\n");
	t_segmento* vacio = malloc(sizeof(t_segmento));
	vacio->id_programa = -1;
	vacio->base = bloqueDeMemoria; // TODO revisar esto!!
	vacio->base_logica = 0;
	vacio->tamanio = tamanioMemoria;
	list_add(segmentos,vacio);
	printf("La lista tiene %d segmentos\n",list_size(segmentos));
	t_segmento* elemento = list_get(segmentos,0);
	printf("El segmento tiene:\n");
	printf("Id_programa: %d\n",elemento->id_programa);
	printf("Base fisica: %p\n",&elemento->base);
	printf("Base logica: %d\n",elemento->base_logica);
	printf("Tamaño: %d\n",elemento->tamanio);

/* Creo el socket que escucha */
	int socketEscucha, socketNuevaConexion;
	socketEscucha = abrir_socket();
	vincular_socket(socketEscucha, puerto);
	escuchar_socket(socketEscucha);
	printf("Socket creado, escuchando... \n");
/* Fin creacion socket */

/* Variables hilos */
	pthread_t nuevoHilo;

/* Creo hilo que atendera la consola */
	pthread_create(&nuevoHilo, NULL,(void*)atenderConsola,NULL);
	list_add(hilos,(void*)nuevoHilo);

	while(1){
		// Aceptar una nueva conexion entrante. Se genera un nuevo socket con la nueva conexion.
		if ((socketNuevaConexion = aceptar_conexion(socketEscucha)) < 0) {
			perror("Error al aceptar conexion entrante");
			return EXIT_FAILURE;
		} else {
		/* Creo nuevo hilo por cada nueva conexion */
		//TODO Hacer funcion que atiende a una nueva conexion
			pthread_create(&nuevoHilo, NULL,(void*)atenderNuevaConexion,(void*)socketNuevaConexion);
		/* Agrego el identificador del hilo a la lista de hilos */
			list_add(hilos,(void*)nuevoHilo);
		}
	}

	//pthread_join(nuevoHilo,NULL);
	return 0;
}


