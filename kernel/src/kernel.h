/*
 * kernel.h
 *
 *  Created on: 11/05/2014
 *      Author: utnso
 */

#ifndef KERNEL_H_
#define KERNEL_H_
#include <semaphore.h>

typedef struct
{
	int puertoprog;
	int puertocpu;
	int quantum;
	int retardo;
	int multiprogramacion;
	char** entradasalidaid;
	char** entradasalidaret;
	char** semaforosid;
	char** semaforosvalor;
	int sizeStack; // en bytes
}t_kernel;

typedef struct
{
	char *id;
	int retardo;
	sem_t *semaforo_IO;
}t_entradasalida;

#endif /* KERNEL_H_ */
