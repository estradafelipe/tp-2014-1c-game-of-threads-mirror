/*
 * stack.h
 *
 *  Created on: 24/05/2014
 *      Author: utnso
 */

#ifndef STACK_H_
#define STACK_H_

#include <commons/collections/queue.h>
#include <semaphore.h>
#include <pthread.h>

typedef struct {
		t_list* elements;
	} t_stack;

typedef struct{
	t_stack* stack;
	sem_t contador;						// Creo que los semaforos no son necesarios, REVISAR
	pthread_mutex_t* mutexStack;		// Creo que los semaforos no son necesarios, REVISAR
}t_pila;


t_pila *stack_create();
void stack_push(t_pila* pila, void* data);
void* stack_pop(t_pila* pila);
int stack_size(t_pila* pila);
void stack_destroy(t_pila* pila);


#endif /* STACK_H_ */
