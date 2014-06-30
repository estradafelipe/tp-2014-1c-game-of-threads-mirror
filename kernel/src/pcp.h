/*
 * pcp.h
 *
 *  Created on: 02/05/2014
 *      Author: utnso
 */

#ifndef PCP_H_
#define PCP_H_
#include "kernel.h"
#include <serializadores.h>
#include <paquetes.h>

#define ERROR_ENVIO_CPU -1
#define EXITO_ENVIO_PCB_A_CPU 0
#define ERROR_RESPUESTA_CPU -2
#define CPU_NO_DISPONIBLE -1
#define CPU_DISPONIBLE 0
#define CPU_CONFIRMA_RECEPCION_PCB 0
#define CPU_NO_CONFIRMA_RECEPCION_PCB -1
#define ERROR_RECEPCION_PCB -1

void hiloPCP();

void modificarPCB(t_PCB *, t_iPCBaCPU *);

t_iPCBaCPU * recibir_pcb_de_cpu(uint32_t);

void poner_cpu_no_disponible(t_CPU *);

void recibirCPU(void);

void integrarCPU(uint32_t , uint32_t *, fd_set *);

void saludarCPU(uint32_t );

int recibir_respuesta_envio_pcb_a_cpu(t_CPU *);

void pasarACola(t_cola*, void *);

void imprimir_mensaje_excepcion(uint32_t, char *);

void pasar_valor_a_imprimir(char *, uint32_t, int);

char * deserializar_nombre_recurso(char *, uint32_t);

#endif /* PCP_H_ */
