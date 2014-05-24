/*
 ============================================================================
 Name        : kernel.c
 Author      : silvina
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
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
#include <commons/config.h>
#include "kernel.h"
#include "plp.h"
#include "colas.h"

//#include "pcp.h"


t_kernel *kernel;
sem_t *semaforo_fin;
sem_t *sem_exit;
int ultimoid;
t_cola *cola_ready;
t_cola *cola_exit;
char *pathconfig;
t_dictionary *semaforos;
t_dictionary *entradasalida;
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

  }
/* Crea las tablas de semaforos y de I/O */
void crea_tablasSitema(){
	int i;
	semaforos = malloc(sizeof(t_dictionary));
	entradasalida = malloc(sizeof(t_dictionary));
	semaforos = dictionary_create();
	entradasalida = dictionary_create();
	int valorsem;
	// Crea tabla de semaforos
	i = 0;
	while (1) {
		if (kernel->semaforosid[i]!='\0'){
			valorsem = atoi(kernel->semaforosvalor[i]);
			dictionary_put(semaforos,kernel->semaforosid[i],&valorsem);
			i++;
		}else break;

	}

	// Crea tabla de IO
	i = 0;
	while (1) {
		if (kernel->entradasalidaid[i]!='\0'){

			t_entradasalida *IO = malloc(sizeof(t_entradasalida));
			IO->id = kernel->entradasalidaid[i];
			IO->retardo = atoi(kernel->entradasalidaret[i]);
			IO->semaforo_IO = malloc(sizeof(sem_t));
			sem_init(IO->semaforo_IO,0,0); //inicializo semaforo del hilo en 0
			dictionary_put(entradasalida,IO->id,IO);
			i++;
		}else break;
	}

}

void hiloIO(t_entradasalida *IO){
	// hilo de entrada salida

	while(1){
		sem_wait(IO->semaforo_IO); // cuando deba ejecutar este hilo, darle signal
		usleep(IO->retardo);
	}
}

void crea_hilosIO(char* key, t_entradasalida *IO){
	int thr;
	pthread_t * IOthr = malloc(sizeof(pthread_t));
	thr = pthread_create( IOthr, NULL, (void*)hiloIO, IO);

	if (thr== 0)
		printf("Se creo el hilo de IO %s\n",IO->id);
	else printf("no se pudo crear el hilo IO %s\n",IO->id);

}

void imprimepantalla(char * key, t_entradasalida *IO){
	printf("%s: %d\n",key,IO->retardo);
}

int main(int argc, char**argv) {
	ultimoid = 0;
	kernel = malloc(sizeof(t_kernel));
	semaforo_fin = malloc(sizeof(sem_t));
	cola_ready = cola_create();
	cola_exit = cola_create();
	sem_exit = malloc(sizeof(sem_t));
	sem_init(sem_exit,0,0);
	sem_init(semaforo_fin,0,0);
	char * path = argv[1]; // path del archivo de configuracion
	leerconfiguracion(path);
	crea_tablasSitema();

	// ** codigo debug para ver que levanto el archivo de config
	printf("**MOSTRANDO CONFIGURACION**\n");
	printf("PUERTO_PROG:%d\n",kernel->puertoprog);
	printf("PUERTO_CPU:%d\n",kernel->puertocpu);
	printf("QUANTUM:%d\n",kernel->quantum);
	printf("RETARDO:%d\n",kernel->retardo);
	printf("MULTIPROGRAMACION:%d\n",kernel->multiprogramacion);
	printf("SEMAFOROS:\n");
	//dictionary_iterator(semaforos,(void*)imprimepantalla);
	printf("\nENTRADA Y SALIDA:\n");
	dictionary_iterator(entradasalida,(void*)imprimepantalla);
	// **

	// Crea hilos I/O
	dictionary_iterator(entradasalida,(void*)crea_hilosIO);
	int thr;

	pthread_t * plpthr = malloc(sizeof(pthread_t)); // hilo plp
	//pthread_t * pcpthr = malloc(sizeof(pthread_t)); // hilo pcp

	thr = pthread_create( plpthr, NULL, (void*)hiloPLP, NULL);

	if (thr== 0)
		printf("Se creo el hilo lo mas bien\n");//se pudo crear el hilo
	else printf("no se pudo crear el hilo\n");//no se pudo crear el hilo

	//dejo comentado para cuando este el PCP
	//thr = pthread_create( pcpthr, NULL, (void*)hiloPCP, NULL);

	/*
		En vez de hacer el join de los threads me bloqueo con un semaforo
		hasta que se termine todo.
	*/
	sem_wait(semaforo_fin);
	printf("Esperando a que se termine todo\n");
	// Libero recursos
	free(plpthr);
	free(semaforo_fin);
	free(sem_exit);
	free(kernel);


	return EXIT_SUCCESS;
}
