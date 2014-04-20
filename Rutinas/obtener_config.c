/*
 * obtener_config.c
 *
 *  Created on: 11/10/2013
 *      Author: utnso
 */
#include "obtener_config.h"

t_personaje obtenerConfiguracion(){
	t_personaje personaje;
	personaje.nombre = malloc(10);
	personaje.simbolo = malloc(sizeof(char));
	personaje.dir_orq = malloc(sizeof(t_ip));
	char* path =malloc(200);
	path = "config.cfg";
	t_config* config;
	config = config_create(path);
	personaje.nombre = obtenerNombre(config);
	personaje.simbolo = obtenerSimbolo(config);
	personaje.vidas = obtenerVidas(config);
	personaje.dir_orq = obtenerOrquestador(config);
	planNiveles = list_create();
	planNiveles = obtenerPlanNiveles(config);
	return (personaje);
}
////////////////////////////////////////////////////////////////////
/* Para el personaje */
///////////////////////////////////////////////////////////////////
char* obtenerNombre(t_config* config){
	char* key = "nombre";
	char* name = malloc(10);
	name = config_get_string_value(config,key);

	return (name);
}

char* obtenerSimbolo(t_config* config){
	char* key = "simbolo";
	char* simbolo = malloc(sizeof(char));
	simbolo = config_get_string_value(config,key);

	return(simbolo);
}

int obtenerVidas(t_config* config){
	 char* key = "vidas";
	 int vidas;
	 vidas = config_get_int_value(config,key);

	 return(vidas);
}

t_ip* obtenerOrquestador(t_config* config){
	char* key = "orquestador";
	t_ip* dir_orq = malloc(sizeof(t_ip));
	char* dir = malloc(22);
	dir = config_get_string_value(config,key);
	char* separator = ":";
	char** array_values = string_split(dir,separator);
	dir_orq->ip = array_values[0];
	dir_orq->port = atoi(array_values[1]);

	return(dir_orq);
}

t_list* obtenerPlanNiveles(t_config* config){
	printf("obtenerPlanNiveles\n");
	char* key = "planDeNiveles";
	char** pNiveles = malloc(sizeof(6)*10); //declaro un array de como mucho 10 niveles
	int i = 0;//,h = 0;
	t_nivel* nivel = malloc(sizeof(t_nivel));
	nivel->nombreNivel = malloc(10);
	//t_nivel* test;
	t_list* niveles = list_create();
	pNiveles =config_get_array_value(config,key);
	cantNiveles = 0;

	while(pNiveles[i] != NULL){
		t_nivel *unNivel=malloc(sizeof(t_nivel));
		unNivel->nombreNivel=string_new();
		string_append(&unNivel->nombreNivel,pNiveles[i]);
		unNivel->listaObjetivos=obtenerObjetivos(config,pNiveles[i]);
		list_add(niveles,unNivel);
		i++;
	}
	printf("Fin obtenerPlaNiveles\n");
	return(niveles);
}

t_queue* obtenerObjetivos(t_config* config, char* level){
	int i=0;
	char* key = malloc(20);
	t_queue* lObjetivos = queue_create();
	char** objetivos;
	strcpy(key,"obj[");// armo key
	strcat(key,level);
	strcat(key,"]");
	objetivos = config_get_array_value(config,key);
	while(objetivos[i] != NULL){
		queue_push(lObjetivos,objetivos[i]);
		i++;
	}

	return(lObjetivos);
}

///////////////////////////////////////////////////////////////////
// Para el nivel
//////////////////////////////////////////////////////////////////
t_ip* obtenerPlataforma(t_config* config){
	char* key = "Plataforma";
	t_ip* dir_plat = malloc(sizeof(t_ip));
	char* dir = malloc(22);
	dir = config_get_string_value(config,key);
	char* separator = ":";
	char** array_values = string_split(dir,separator);
	dir_plat->ip = array_values[0];
	dir_plat->port = atoi(array_values[1]);

	return(dir_plat);
}

t_list* obtenerCajas(t_config* config){
	int nro = 1;
	char* key = malloc(6);
	t_list* cajas = list_create();
	t_caja* caja = malloc(sizeof(t_caja));
	caja->nombreCaja = malloc(10);
	char* cnro = malloc(sizeof(char));
	while(1){
		sprintf(cnro,"%c",nro);
		strcpy(key,"Caja");// armo key
		strcat(key,cnro);
		char** pCaja = malloc(9+1+sizeof(int)*3);
		pCaja =config_get_array_value(config,key);
		if(pCaja[0] != NULL){
			caja->nombreCaja = pCaja[0];
			caja->simbolo = pCaja[1];
			caja->instancias = atoi(pCaja[2]);
			caja->posX = atoi(pCaja[3]);
			caja->posY = atoi(pCaja[4]);
			list_add(cajas,caja);
			nro++;
		} else {
			break;
		}
	}

	return(cajas);
}

char* obtenerNombreNivel(t_config* config){
	char* key = "Nombre";
	char* name = malloc(7);
	name = config_get_string_value(config,key);
	return (name);
}

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

int obtenerEnemigos(t_config* config){
	 char* key = "Enemigos";
	 int enemigos;
	 enemigos = config_get_int_value(config,key);

	 return(enemigos);
}

int obtenerSleepEnemigos(t_config* config){
	 char* key = "Sleep_Enemigos";
	 int sleepEnemigos;
	 sleepEnemigos = config_get_int_value(config,key);

	 return(sleepEnemigos);
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

///////////////////////////////////////////////////////////////////
// Para la plataforma
//////////////////////////////////////////////////////////////////

int obtenerPuerto(t_config* config){
	char* key = "puerto";
	int puerto;
	puerto = config_get_int_value(config,key);

	return(puerto);
}

char* obtenerPathKoopa(t_config* config){
	char* key = "koopa";
	char* koopa = malloc(strlen("/home/utnso/koopa"));
	koopa = config_get_string_value(config,key);

	return(koopa);
}

char* obtenerPathScript(t_config* config){
	char* key = "script";
	char* script = malloc(strlen("/home/utnso/evaluacion.sh"));
	script = config_get_string_value(config,key);

	return(script);
}


