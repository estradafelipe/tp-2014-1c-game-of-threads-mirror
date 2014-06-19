/*
 * hilos.h
 *
 *  Created on: 12/06/2014
 *      Author: utnso
 */

#ifndef HILOS_H_
#define HILOS_H_

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

void crea_tablasSitema();
void hiloIO(t_entradasalida *IO);
void crea_hilosIO(char* key, t_entradasalida *IO);
void imprimepantalla(char * key, t_entradasalida *IO);
void wait_semaforo(char *semaforo,uint32_t fd);
void signal_semaforo(char *semaforo);

#endif /* HILOS_H_ */
