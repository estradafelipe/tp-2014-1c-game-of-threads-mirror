/*
 * pcp.h
 *
 *  Created on: 02/05/2014
 *      Author: utnso
 */

#ifndef PCP_H_
#define PCP_H_
#include "plp.h"

#define ERROR_ENVIO_CPU -1
#define EXITO_ENVIO_PCB_A_CPU 0
#define ERROR_RESPUESTA_CPU -2
#define CPU_NO_DISPONIBLE -1
#define CPU_DISPONIBLE 0
#define CPU_CONFIRMA_RECEPCION_PCB 0
#define CPU_NO_CONFIRMA_RECEPCION_PCB -1
#define ERROR_RECEPCION_PCB -1

typedef struct
{
	uint32_t fd;     // file descriptor para socket del CPU
	uint16_t estado;	// 0 Disponible 1 Ocupada -1 No Disponible
	t_PCB * pcb; // PCB
}t_CPU;

void hiloPCP();

#endif /* PCP_H_ */
