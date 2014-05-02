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
	int puertoprog;
	int puertocpu;
	int quantum;
	int retardo;
	int multiprogramacion;
}t_kernel;

typedef struct
{
	//segun lo que me diga Felipe.
}t_dirUMV;

typedef struct
{
	int id;
	t_dirUMV segmentoCodigo;
	t_dirUMV segmentoStack;
	t_dirUMV cursorStack;
	//(tipo?)indiceCodigo;
	//(tipo?)indiceEtiquetas;
	int programcounter;
	int sizeContext;
	int peso;
}t_PCB;

void hiloPLP();

#endif /* PLP_H_ */
