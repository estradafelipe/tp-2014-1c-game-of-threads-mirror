/*
 * kernel.h
 *
 *  Created on: 11/05/2014
 *      Author: utnso
 */

#ifndef KERNEL_H_
#define KERNEL_H_
#include <semaphore.h>
#include <sockets.h>
#include <parser/parser.h>
#include "varcom.h"

#define FIN_PROGRAM_SUCCESS 0
#define PROGRAM_DISCONNECT -1
#define PROGRAM_SEG_FAULT -2
#define PROGRAM_SEGSIZE_FAULT -3

typedef struct
{
	int puertoprog;
	int puertocpu;
	int puertoumv;
	char * ip_umv;
	int quantum;
	int retardo;
	int multiprogramacion;
	char** entradasalidaid;
	char** entradasalidaret;
	char** semaforosid;
	char** semaforosvalor;
	int sizeStack; // en bytes
	t_puntero fd_UMV;
	t_dictionary *programas;
	t_dictionary *cpus;
	t_dictionary *semaforos;
	t_dictionary *entradasalida;
	t_dictionary *variables_compartidas;
	pthread_mutex_t mutex_programas;
	pthread_mutex_t mutex_cpus;
}t_kernel;

typedef struct
{
	int id; 	   // el mismo que el del pcb
	int peso;
	int fd;        // file descriptor para socket.
	int estado;    // estado 1 Activo, 0 Inactivo
	int exit_code; // como termino?
}t_programa;

typedef struct
{
	char *id;
	int retardo;
	sem_t *semaforo_IO;
	t_cola *cola;
}t_entradasalida;

typedef struct
{
	char *id; //nombre del semaforo
	int valor;
	pthread_mutex_t *mutex;
	t_cola *cola;
}t_semaforo;

typedef struct
{
	t_PCB *PCB;
	int unidadesTiempo;
}t_progIO;

typedef struct
{
	uint32_t fd;     // file descriptor para socket del CPU
	uint16_t estado;	// 0 Disponible 1 Ocupada -1 No Disponible
	t_PCB * pcb; // PCB
}t_CPU;

#endif /* KERNEL_H_ */
