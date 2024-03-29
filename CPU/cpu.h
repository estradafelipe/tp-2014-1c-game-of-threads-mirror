/*
 * cpu.h
 *
 *  Created on: 19/06/2014
 *      Author: utnso
 */

#ifndef CPU_H_
#define CPU_H_

#include <stdio.h>
#include <stdlib.h>
#include <sockets.h>
#include <commons/config.h>
#include <commons/log.h>
#include <obtener_config.h>
#include <serializadores.h>
#include <paquetes.h>
#include <parser/sintax.h>
#include <parser/parser.h>
#include <parser/metadata_program.h>
#include <colas.h>
#include <string.h>
#include <signal.h>
#include <stdarg.h>
#include "primitivas.h"

#define TAMANIO_INSTRUCCION 8
#define TAMANIO_ID_VAR 1
#define TAMANIO_VAR 4

void rutina(int n);
void cargar_diccionarioVariables(int32_t);
void notificar_kernel(t_paquete);
void notificarError_kernel(char*);
void handshake(t_paquete);

package *Leer(t_pun base,t_pun offset,t_pun tamanio);
void Escribir(t_pun base, t_pun offset, t_pun tamanio, char* buffer);
char* quitarSaltoLinea(char* cadena);

//Defino variables globales a la CPU
t_dictionary *diccionarioVariables;
t_PCB *pcb;
int socketKernel, socketUMV;
int desconectarse;
int finprograma;
int32_t quantumKernel;
t_log* logger;
int32_t quantumPrograma;

typedef struct{
	int32_t inicio;
	int32_t longitud;
}t_datoSentencia;







#endif /* CPU_H_ */
