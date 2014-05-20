/*
 * plp.h
 *
 *  Created on: 02/05/2014
 *      Author: utnso
 */

#ifndef PLP_H_
#define PLP_H_



typedef struct
{
	int id; // el mismo que el del pcb
	int peso;
	int fd; // file descriptor para socket.
}t_programa;


typedef struct
{
	int id;
	// **direcciones del primer byte en la umv
	int segmentoCodigo;
	int segmentoStack;
	int cursorStack;
	int indiceCodigo;
	int indiceEtiquetas;
	// **
	int programcounter;
	int sizeContext;
	int sizeIndexLabel;
}t_PCB;

typedef struct
{
	int fd_UMV;
}t_PLP;
void hiloPLP();

#endif /* PLP_H_ */
