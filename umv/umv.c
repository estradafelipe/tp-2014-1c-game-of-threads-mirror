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

	t_config *configUMV = config_create(argv[1]);
/* Creo loggers */
	logger = log_create("loggerUMV.log", "UMV", false, LOG_LEVEL_DEBUG);//loger comun
	loggerConsola = log_create("loggerConsola.log", "UMV-Consola", false, LOG_LEVEL_DEBUG);//loger de la consola

/* Creo la lista que va a tener un elemento por cada hilo lanzado */
	hilos = list_create();
	printf("Lista de hilos creada\n");

/* Obtener datos del archivo de configuracion */
	int tamanioMemoria=obtenerTamanioMemoria(configUMV);
	int puerto = obtenerPuerto(configUMV);
	algoritmo = obtenerAlgoritmoUMV(configUMV);
	retardo = obtenerRetardo(configUMV)/1000;
	logConsola = obtenerLogConsola(configUMV);

/* Fin obtener datos */
	printf("El tamaño de memoria es %d, el puerto es %d, el algoritmo es: %s y el retardo es: %d milisegundos\n",tamanioMemoria,puerto,algoritmo==1?"FIRST-FIT":"WORST-FIT",retardo*1000);
	log_info(logger,"El tamaño de memoria es %d, el puerto es %d, el algoritmo es: %s y el retardo es: %d milisegundos",tamanioMemoria,puerto,algoritmo==1?"FIRST-FIT":"WORST-FIT",retardo*1000);
/* Pido la memoria que va a tener disponible el sistema */
	bloqueDeMemoria = malloc(tamanioMemoria);
	printf("Se creo el bloque de memoria con direccion inicial %p\n",&bloqueDeMemoria);
	log_info(logger, "Se creo el bloque de memoria con direccion inicial %p",&bloqueDeMemoria);

/* Creo lista para administrar el bloque de memoria */
	segmentos = list_create();
	//Inicializo semaforos de la lista, memoria y algoritmo
	pthread_rwlock_init(&lockSegmentos,NULL);
	pthread_rwlock_init(&lockMemoria,NULL);
	pthread_rwlock_init(&lockAlgoritmo,NULL);

	printf("Lista de segmentos creada\n");
	t_segmento* vacio = malloc(sizeof(t_segmento));
	vacio->id_programa = -1;
	vacio->base = bloqueDeMemoria;
	vacio->base_logica = 0;
	vacio->tamanio = tamanioMemoria;
	list_add(segmentos,vacio);

/* Creo el socket que escucha */
	int socketEscucha, socketNuevaConexion;
	socketEscucha = abrir_socket();
	vincular_socket(socketEscucha, puerto);
	printf("Socket creado\n");
	log_debug(logger, "Socket creado, escuchando...");
	escuchar_socket(socketEscucha);
/* Fin creacion socket */

/* Variables hilos */
	pthread_t nuevoHilo;
/* Seteo semilla para el calculo aleatorio de las bases logicas */
	srand(time(NULL));
/* Creo hilo que atendera la consola */
	pthread_create(&nuevoHilo, NULL,(void*)atenderConsola,NULL);
	list_add(hilos,(void*)nuevoHilo);
	log_debug(logger, "Hilo que atiende la consola corriendo");

	while(1){
		// Aceptar una nueva conexion entrante. Se genera un nuevo socket con la nueva conexion.
		if ((socketNuevaConexion = aceptar_conexion(socketEscucha)) < 0) {
			perror("Error al aceptar conexion entrante");
			return EXIT_FAILURE;
		} else {
		/* Creo nuevo hilo por cada nueva conexion */
			pthread_create(&nuevoHilo, NULL,(void*)atenderNuevaConexion,(void*)socketNuevaConexion);
		/* Agrego el identificador del hilo a la lista de hilos */
			list_add(hilos,(void*)nuevoHilo);
			log_debug(logger,"Se conecto alguien, lanzo hilo para atenderlo");
		}
	}

	pthread_rwlock_destroy(&lockSegmentos);
	pthread_rwlock_destroy(&lockMemoria);
	pthread_rwlock_destroy(&lockAlgoritmo);
	//pthread_join(nuevoHilo,NULL);
	return 0;
}


