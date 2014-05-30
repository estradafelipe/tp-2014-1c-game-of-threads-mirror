/*
 * stack.c
 *
 *  Created on: 24/05/2014
 *      Author: utnso
 */

#include <commons/collections/queue.h>
#include "colas.h"
#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>
#include <stdio.h>
#include <commons/collections/queue.h>
#include <commons/collections/list.h>
#include "stack.h"

t_pila *stack_create(){
	t_pila * pila = malloc(sizeof(t_pila));
	pila->mutexStack= malloc(sizeof(pthread_mutex_t));
	t_stack *stack= malloc(sizeof(t_stack));
	t_list* elements = list_create();
	stack->elements = elements;
	pila->stack = stack;
	pthread_mutex_init(pila->mutexStack,NULL);
	sem_init(&pila->contador,0,0);
	return pila;
}

void stack_push(t_pila* pila, void* data){
	pthread_mutex_lock(pila->mutexStack);
		list_add_in_index(pila->stack->elements,0,data);
		sem_post(&pila->contador);
	pthread_mutex_unlock(pila->mutexStack);
}

void* stack_pop(t_pila* pila){
	pthread_mutex_lock(pila->mutexStack);
		sem_wait(&pila->contador);
		void *data=list_remove(pila->stack->elements,0);
		pthread_mutex_unlock(pila->mutexStack);
		return data;
}

int stack_size(t_pila* pila){
	int tamanio;
	tamanio = list_size(pila->stack->elements);
	return tamanio;
}

void stack_destroy(t_pila* pila){
	list_destroy(pila->stack->elements);

}
