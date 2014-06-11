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
	uint32_t fd;     // file descriptor para socket del CPU
	uint32_t id_pcb; // ID PCB
}t_CPU;

void hiloPCP();

#endif /* PCP_H_ */
