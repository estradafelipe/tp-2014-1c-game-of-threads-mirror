/*
 * pcp.h
 *
 *  Created on: 02/05/2014
 *      Author: utnso
 */

#ifndef PCP_H_
#define PCP_H_
#include "plp.h"
typedef struct
{
	t_PCB *pcb; // pcb del programa que esta ejecutando
	int fd; 	// file descriptor para socket del CPU
	int estado; // 1 Ocupado; 0 Libre.
}t_CPU;

void hiloPCP();

#endif /* PCP_H_ */
