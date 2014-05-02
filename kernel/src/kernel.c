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
#include <parser/metadata_program.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include "plp.h"
//#include "pcp.h"


t_kernel *kernel;
pthread_mutex_t mutex_fin = PTHREAD_MUTEX_INITIALIZER;
int ultimoid;
t_list *cola_exec;
t_list *cola_ready;
char *pathconfig;

void leerconfiguracion(char *path_config){
	t_config *config = config_create(path_config);
	char *key;
	key = "PUERTO_PROG";


	if (config_has_property(config, key))
		kernel->puertoprog = config_get_int_value(config,key);
	key = "PUERTO_CPU";
	if (config_has_property(config,key ))
		kernel->puertocpu = config_get_int_value(config, key);
	key = "QUANTUM";
	if (config_has_property(config, key))
		kernel->quantum = config_get_int_value(config, key);
	key = "RETARDO";
	if (config_has_property(config, key))
		kernel->retardo = config_get_int_value(config, key);
	key = "MULTIPROGRAMACION";
	if (config_has_property(config, key))
		kernel->multiprogramacion = config_get_int_value(config, key);

	// FALTAN LOS ID DE LOS SEMAFOROS DEL SISTEMA
  }

int main(int argc, char**argv) {
	ultimoid = 0;
	kernel = malloc(sizeof(t_kernel));

	cola_ready = list_create();
	cola_exec = list_create();
	char * path = argv[1]; // path del script ansisop / del archivo de configuracion
	leerconfiguracion(path);
	// codigo debug para ver que levanto el archivo de config
	printf("**MOSTRANDO CONFIGURACION**\n");
	printf("PUERTO_PROG:%d\n",kernel->puertoprog);
	printf("PUERTO_CPU:%d\n",kernel->puertocpu);
	printf("QUANTUM:%d\n",kernel->quantum);
	printf("RETARDO:%d\n",kernel->retardo);
	printf("MULTIPROGRAMACION:%d\n",kernel->multiprogramacion);
	/*
	struct stat stat_file;
	stat(path, &stat_file);
	char* buffer = calloc(1, stat_file.st_size + 1);
	FILE* file = NULL;
	file = fopen(path,"r");
	if (file==NULL)
		printf("no se puede abrir el archivo\n");

	else {
		fread(buffer, stat_file.st_size, 1, file); // levanto el archivo en buffer
	}

	t_medatada_program *programa = metadatada_desde_literal(buffer);
	printf("Cantidad de Etiquetas:%d\n",programa->cantidad_de_etiquetas);
	printf("Cantidad de Funciones:%d\n",programa->cantidad_de_funciones);
	printf("Cantidad de Instrucciones:%d\n",programa->instrucciones_size);
	*/


	int thr;
	pthread_t * plpthr = malloc(sizeof(pthread_t)); // hilo plp
	//pthread_t * pcpthr = malloc(sizeof(pthread_t)); // hilo pcp

	thr = pthread_create( plpthr, NULL, (void*)hiloPLP, NULL);
	//thr = pthread_create(plpthr,NULL,(void*)hiloPLP,NULL);

	if (thr== 0)
		printf("Se creo el hilo lo mas bien\n");//se pudo crear el hilo
	else printf("no se pudo crear el hilo\n");//no se pudo crear el hilo

	//dejo comentado para cuando este el PCP
	//thr = pthread_create( pcpthr, NULL, (void*)hiloPCP, NULL);

	/*
		En vez de hacer el join de los threads me bloqueo con un semaforo
		hasta que se termine todo.
	*/

	pthread_mutex_lock(&mutex_fin);
	printf("Esperando a que se termine todo\n");
	pthread_mutex_lock(&mutex_fin);

	return EXIT_SUCCESS;
}
