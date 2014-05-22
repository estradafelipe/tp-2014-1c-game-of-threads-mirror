/*
 * plp.h
 *
 *  Created on: 02/05/2014
 *      Author: utnso
 */

#ifndef PLP_H_
#define PLP_H_

#include <sockets.h>

typedef struct
{
	int id; // el mismo que el del pcb
	int peso;
	int fd; // file descriptor para socket.
}t_programa;

/*
typedef struct
{
	int id;
	// **direcciones del primer byte en la umv
	t_puntero segmentoCodigo;
	t_puntero segmentoStack;
	t_puntero cursorStack;
	t_puntero indiceCodigo;
	t_puntero indiceEtiquetas;
	// **
	int programcounter;
	int sizeContext;
	int sizeIndexLabel;
}t_PCB;
*/

void hiloPLP();

#endif /* PLP_H_ */
