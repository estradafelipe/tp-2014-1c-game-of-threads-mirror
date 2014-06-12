/*
 * obtener_config.h
 *
 *  Created on: 11/10/2013
 *      Author: utnso
 */

#ifndef OBTENER_CONFIG_H_
#define OBTENER_CONFIG_H_

#include <stdlib.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/collections/queue.h>
#include <commons/collections/list.h>

typedef struct {
	char* ip;
	short port;
}t_ip;

typedef struct {
	char* ip;
	short port;
	int tamanioStack;
}t_confKernel;


char* obtenerIP(t_config* config);
int obtenerTiempoDeadlock(t_config* config);
int obtenerRecovery(t_config* config);
int obtenerQuantum(t_config* config);
int obtenerRetardo(t_config* config);
char* obtenerAlgoritmo(t_config* config);
int obtenerAlgoritmoUMV(t_config* config);
int obtenerTamanioMemoria(t_config* config);
int obtenerPuerto(t_config* config);
int obtenerTamanioStack(t_config* config);
#endif /* OBTENER_CONFIG_H_ */
