/*
 ============================================================================
 Name        : kernel.c
 Author      : silvina
 Version     :
 Copyright   : Your copyright notice
 Description :
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <parser/metadata_program.h>
#include <commons/collections/list.h>
#include <commons/collections/dictionary.h>
#include <commons/log.h>
#include <commons/config.h>
#include "kernel.h"
#include "plp.h"
#include "colas.h"
#include "pcp.h"
#include "hilos.h"

t_kernel *kernel;
sem_t *semaforo_fin;
sem_t *sem_exit;
sem_t *sem_multiprogramacion;
int ultimoid;
t_cola *cola_ready;
t_cola *cola_exit;
t_cola *cola_block;
t_log *logger;
char *pathconfig;
t_cola *cpus_disponibles; // contendra los fd de las cpus con el id del PCB e id de cpu

pthread_mutex_t mutex_ready = PTHREAD_MUTEX_INITIALIZER;

void leerconfiguracion(char *path_config){
	t_config *config = config_create(path_config);
	char *key;

	key = "PUERTO_PROG";
	if (config_has_property(config, key))
		kernel->puertoprog = config_get_int_value(config,key);
	key = "PUERTO_CPU";
	if (config_has_property(config,key ))
		kernel->puertocpu = config_get_int_value(config, key);
	key = "PUERTO_UMV";
	if (config_has_property(config,key ))
		kernel->puertoumv = config_get_int_value(config, key);
	key = "IP_UMV";
	if (config_has_property(config,key ))
		kernel->ip_umv = config_get_string_value(config, key);
	key = "QUANTUM";
	if (config_has_property(config, key))
		kernel->quantum = config_get_int_value(config, key);
	key = "RETARDO";
	if (config_has_property(config, key))
		kernel->retardo = config_get_int_value(config, key);
	key = "MULTIPROGRAMACION";
	if (config_has_property(config, key))
		kernel->multiprogramacion = config_get_int_value(config, key);
	key = "SIZE_STACK";
	if (config_has_property(config, key))
		kernel->sizeStack = config_get_int_value(config, key);
	key = "SEMAFOROS";
	if (config_has_property(config,key))
		kernel->semaforosid = config_get_array_value(config,key);
	key = "VALOR_SEMAFORO";
	if (config_has_property(config,key))
		kernel->semaforosvalor =config_get_array_value(config,key);
	key = "ID_HIO";
	if (config_has_property(config,key))
		kernel->entradasalidaid =	config_get_array_value(config,key);
	key = "HIO";
	if (config_has_property(config,key))
		kernel->entradasalidaret =	config_get_array_value(config,key);

	kernel->programas = dictionary_create();
	kernel->cpus = dictionary_create();
	pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
	kernel->mutex_programas = mutex;
	kernel->mutex_cpus = mutex;
  }


int main(int argc, char**argv) {
	ultimoid = 0;
	kernel = malloc(sizeof(t_kernel));
	semaforo_fin = malloc(sizeof(sem_t));
	cola_ready = cola_create();
	cola_exit = cola_create();
	cola_block = cola_create();
	cpus_disponibles = cola_create();
	sem_exit = malloc(sizeof(sem_t));
	sem_init(sem_exit,0,0);
	sem_init(semaforo_fin,0,0);
	char * path = argv[1]; // path del archivo de configuracion
	leerconfiguracion(path);
	crea_tablasSitema();
	/* Creo loggers */
	logger = log_create("loggerKERNEL.log", "KERNEL", true, LOG_LEVEL_DEBUG);
	// ** codigo debug para ver que levanto el archivo de config
	log_debug(logger, string_from_format("\nCONFIGURACION:\nPUERTO_PROG:%d\nPUERTO_CPU:%d\nQUANTUM:%d\nRETARDO:%d\nMULTIPROGRAMACION:%d\n",kernel->puertoprog,kernel->puertocpu,kernel->quantum,kernel->retardo,kernel->multiprogramacion));

	// Crea hilos I/O
	dictionary_iterator(kernel->entradasalida,(void*)crea_hilosIO);

	int thr;

	pthread_t * plpthr = malloc(sizeof(pthread_t)); // hilo plp
	//pthread_t * pcpthr = malloc(sizeof(pthread_t)); // hilo pcp


	thr = pthread_create( plpthr, NULL, (void*)hiloPLP, NULL);

	if (thr== 0)
		log_debug(logger,"Se creo el hilo lo mas bien\n");//se pudo crear el hilo
	else log_debug(logger,"no se pudo crear el hilo\n");//no se pudo crear el hilo

/*
	//dejo comentado para cuando este el PCP
	thr = pthread_create( pcpthr, NULL, (void*)hiloPCP, NULL);
	if (thr== 0)
			printf("Se creo el hilo pcp lo mas bien\n");
*/
	/*
		En vez de hacer el join de los threads me bloqueo con un semaforo
		hasta que se termine todo.
	*/
	sem_wait(semaforo_fin);
	// Libero recursos
	free(plpthr);
	free(semaforo_fin);
	free(sem_exit);
	free(kernel);


	return EXIT_SUCCESS;
}
