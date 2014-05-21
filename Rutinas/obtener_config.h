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
	char* nombre;
	char* simbolo;
	int vidas;
	t_ip* dir_orq;
}t_personaje;

typedef struct{
	char* nombreNivel;
	t_queue* listaObjetivos;
}t_nivel;

typedef struct{
	char* nombreCaja;
	char* simbolo;
	int instancias;
	int posX;
	int posY;
}t_caja;

t_list* planNiveles;
int cantNiveles;

t_personaje obtenerConfiguracion();
char* obtenerNombre(t_config* config);
char* obtenerSimbolo(t_config* config);
int obtenerVidas(t_config* config);
t_ip* obtenerOrquestador(t_config* config);
t_queue* obtenerObjetivos(t_config* config, char* level);
t_list* obtenerPlanNiveles(t_config* config);
char* obtenerIP(t_config* config);
t_ip* obtenerPlataforma(t_config* config);
t_list* obtenerCajas(t_config* config);
char* obtenerNombreNivel(t_config* config);
int obtenerTiempoDeadlock(t_config* config);
int obtenerRecovery(t_config* config);
int obtenerEnemigos(t_config* config);
int obtenerSleepEnemigos(t_config* config);
int obtenerQuantum(t_config* config);
int obtenerRetardo(t_config* config);
char* obtenerAlgoritmo(t_config* config);

int obtenerPuerto(t_config* config);
char* obtenerPathKoopa(t_config* config);
char* obtenerPathScript(t_config* config);



#endif /* OBTENER_CONFIG_H_ */
