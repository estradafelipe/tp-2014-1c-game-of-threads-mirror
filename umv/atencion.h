/*
 * atencion.h
 *
 *  Created on: 02/05/2014
 *      Author: utnso
 */

#ifndef ATENCION_H_
#define ATENCION_H_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <commons/log.h>
#include <sockets.h>
#include <string.h>
#include <paquetes.h>
#include <serializadores.h>
#include <time.h>

int retardo; // retardo en milisegundos que hay que esperar entre solicitudes
int algoritmo; //algoritmo para ubicar los segmentos
int logConsola; //0 si no hay que logear a archivo, 1 si hay que logear a archivo
t_log* logger;
t_log* loggerConsola;

typedef int32_t t_pun;

typedef struct{
	int id_programa; //programa al que pertenece el segmento
	char* base; //base fisica (solo para la umv)
	int tamanio; // tamaño del segmento
	t_pun base_logica; //base para el kernel y cpus
}t_segmento;

pthread_rwlock_t lockSegmentos, lockMemoria, lockAlgoritmo;
//pthread_mutex_t mutexAlgoritmo;

t_list* hilos; //lista de hilos
t_list* segmentos; //lista de segmentos

char* bloqueDeMemoria;

int scriptCreacionSegmentos();
void* atenderNuevaConexion();
void* atenderConsola();

int atenderKernel(int socket);
int atenderCpu(int socket);

int _menor_id_programa(t_segmento *seg, t_segmento *segMayor);
int _mayor_tamanio(t_segmento *seg, t_segmento *segMayor);
int _esta_vacio(t_segmento* seg);
t_segmento* _existe_algun_seg(int id_programa);

char* generarHexa(char* buffer,int tamanio);

int validarBaseLogica(int id, int tamanio,t_pun aleatorio);
t_pun calcularBaseLogica(int id_programa, int tamanio);

int first_fit(t_list* lista,int id_programa,int tamanio);
int worst_fit(t_list* lista,int id_programa,int tamanio);

char* leer(int id_programa,t_solicitudLectura* solicitud);
int escribir(int id_programa,t_solicitudEscritura* solicitud);
int crear_segmento(t_crearSegmentoUMV* datos);
int destruir_segmentos(int id_programa);

int compactar();
int imprimir_estructuras(int id_programa);
int imprimir_segmentos_memoria();
int imprimir_contenido(int offset, int tamanio);

#endif /* ATENCION_H_ */
