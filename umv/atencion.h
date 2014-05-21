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
#include <sockets.h>
#include <string.h>
#include <paquetes.h>
#include <serializadores.h>

int retardo; // retardo en milisegundos que hay que esperar entre solicitudes
int algoritmo; //algoritmo para ubicar los segmentos

typedef u_int32_t t_puntero;

typedef struct{
	int id_programa; //programa al que pertenece el segmento
	char* base; //base fisica (solo para la umv)
	int tamanio; // tamaño del segmento
	t_puntero base_logica; //base para el kernel y cpus
}t_segmento;

typedef struct{
	t_puntero segCodigo;
	t_puntero indiceEtiquetas;
	t_puntero indiceFunciones;
	t_puntero indiceCodigo;
	t_puntero segStack;
}t_direcciones; // estructura que se le respondera al kernel cuando se crean los segmentos de un programa

pthread_mutex_t* mutexSegmentos;

t_list* hilos; //lista de hilos
t_list* segmentos; //lista de segmentos

char* bloqueDeMemoria;

int id_prog_buscado;
int base_buscada;

void* atenderNuevaConexion();
void* atenderConsola();

int atenderKernel(int socket);
int atenderCpu(int socket);

int _menor_id_programa(t_segmento *seg, t_segmento *segMayor);
int _mayor_tamanio(t_segmento *seg, t_segmento *segMayor);
int _es_el_buscado(t_segmento* seg);
int _esta_vacio(t_segmento* seg);
int _existe_algun_seg(t_segmento* seg);

int first_fit(t_list* lista,int id_programa,int tamanio);
int worst_fit(t_list* lista,int id_programa,int tamanio);

char* leer(int id_programa,int base,int offset,int tamanio);
int escribir(int id_programa,int base,int offset,int tamanio,char* buffer);
int crear_segmento(t_crearSegmentoUMV* datos);
int destruir_segmentos(int id_programa);

int compactar();
int imprimir_estructuras(int id_programa);
int imprimir_segmentos_memoria();
int imprimir_contenido(int offset, int tamanio);
#endif /* ATENCION_H_ */
