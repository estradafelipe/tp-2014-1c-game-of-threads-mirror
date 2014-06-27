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

void *cargar_diccionarioVariables(int32_t);
void notificar_kernel(t_paquete);
void rutina(int n);
void noticar_kernel(t_paquete);
void handshake_kernel(void);

package *Leer(t_pun base,t_pun offset,t_pun tamanio);
package *Escribir(t_pun base, t_pun offset, t_pun tamanio, char* buffer);

//Defino variables globales a la CPU
t_dictionary *diccionarioVariables;
t_dictionary *diccionarioEtiquetas;
t_PCB *pcb;
int socketKernel, socketUMV;
int desconectarse = false;
int32_t quantumKernel;
t_log* logger;
int32_t quantumPrograma;

typedef struct{
	int32_t inicio;
	int32_t longitud;
}t_datoSentencia;




#endif /* CPU_H_ */
