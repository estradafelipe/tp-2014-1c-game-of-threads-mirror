/*
 * obtener_config.c
 *
 *  Created on: 11/10/2013
 *      Author: utnso
 */
#include "obtener_config.h"


int obtenerTiempoDeadlock(t_config* config){
	 char* key = "TiempoChequeoDeadlock";
	 int tDeadlock;
	 tDeadlock = config_get_int_value(config,key);

	 return(tDeadlock);
}

int obtenerRecovery(t_config* config){
	 char* key = "Recovery";
	 int recovery;
	 recovery = config_get_int_value(config,key);

	 return(recovery);
}

int obtenerQuantum(t_config* config){
	 char* key = "quantum";
	 int quantum;
	 quantum = config_get_int_value(config,key);

	 return(quantum);
}

int obtenerRetardo(t_config* config){
	 char* key = "retardo";
	 int retardo;
	 retardo = config_get_int_value(config,key);

	 return(retardo);
}

char* obtenerAlgoritmo(t_config* config){
	char* key = "algoritmo";
	char* algoritmo = malloc(4);
	algoritmo = config_get_string_value(config,key);

	return(algoritmo);
}


int obtenerPuerto(t_config* config){
	char* key = "puerto";
	int puerto;
	puerto = config_get_int_value(config,key);

	return(puerto);
}

char* obtenerIP(t_config* config){
	char* key = "ip";
	char* ip;
	ip = config_get_string_value(config,key);

	return(ip);
}

int obtenerTamanioStack(t_config* config){
	char* key = "stack";
	int tamanio;
	tamanio = config_get_int_value(config,key);

	return(tamanio);
}

