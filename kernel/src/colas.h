/*
 * colas.h
 *
 *  Created on: 26/10/2013
 *      Author: utnso
 */

#ifndef COLAS_H_
#define COLAS_H_

#include <commons/collections/queue.h>
#include <semaphore.h>
#include <pthread.h>

typedef struct{
	t_queue* queue;
	sem_t contador;
	pthread_mutex_t* mutexCola;
}t_cola;

t_cola* cola_create();
void cola_push(t_cola* cola, void* data);
void* cola_pop(t_cola* cola);
void cola_destroy(t_cola* cola);

#endif /* COLAS_H_ */
