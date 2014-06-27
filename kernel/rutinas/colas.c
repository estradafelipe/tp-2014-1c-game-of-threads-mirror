/*
 * colas.c
 *
 *  Created on: 26/10/2013
 *      Author: utnso
 */

#include <commons/collections/queue.h>
#include "colas.h"
#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>
#include <stdio.h>
#include <commons/collections/queue.h>


/* Crea una cola */
t_cola* cola_create(){
	t_cola * cola = malloc(sizeof(t_cola));
	cola->mutexCola= malloc(sizeof(pthread_mutex_t));
	cola->queue= queue_create();
	pthread_mutex_init(cola->mutexCola,NULL);
	sem_init(&cola->contador,0,0);
	return cola;
}

/* Inserta un elemento al final de la cola */
void cola_push(t_cola* cola, void* data){
	pthread_mutex_lock(cola->mutexCola);
	queue_push(cola->queue,data);
	sem_post(&cola->contador);
	pthread_mutex_unlock(cola->mutexCola);
}



/* Obtiene el primer elemento de la cola */
void* cola_pop(t_cola* cola){
	pthread_mutex_lock(cola->mutexCola);
	sem_wait(&cola->contador);
	void *data=queue_pop(cola->queue);
	pthread_mutex_unlock(cola->mutexCola);
	return data;
}

/* Destruye la cola (jaja) */
void cola_destroy(t_cola* cola){
	pthread_mutex_lock(cola->mutexCola);
	sem_destroy(&cola->contador);
	queue_destroy(cola->queue);
	pthread_mutex_unlock(cola->mutexCola);
	pthread_mutex_destroy(cola->mutexCola);
}
