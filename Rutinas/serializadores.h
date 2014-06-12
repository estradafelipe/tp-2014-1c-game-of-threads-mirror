/*
 * serializadores.h
 *
 *  Created on: 30/10/2013
 *      Author: utnso
 */

#ifndef SERIALIZADORES_H_
#define SERIALIZADORES_H_

#include <stdint.h>
#include <commons/collections/list.h>
#include "colas.h"

typedef int32_t t_pun;


typedef struct{
	t_pun programid;
	t_pun size;
}__attribute__((packed)) t_crearSegmentoUMV;


typedef struct{
	t_pun base;
	t_pun offset;
	t_pun tamanio;
}__attribute__((packed)) t_solicitudLectura;

typedef struct{
	t_pun base;
	t_pun offset;
	t_pun tamanio;
	char* buffer;
}__attribute__((packed)) t_solicitudEscritura;

typedef struct{
	t_pun id;
	t_pun segmentoCodigo;
	t_pun segmentoStack;
	t_pun cursorStack;
	t_pun indiceCodigo;
	t_pun indiceEtiquetas;
	t_pun programcounter;
	t_pun sizeContext;
	t_pun sizeIndexLabel;
}__attribute__((packed)) t_PCB;



char* serializarPCB(t_PCB * PCB);
t_PCB *desserializarPCB(char* PCBSerializada);


char* serializarSolicitudLectura(t_solicitudLectura* solicitud);
t_solicitudLectura* desserializarSolicitudLectura(char* solicitud);

char* serializarSolicitudEscritura(t_solicitudEscritura* solicitud);
t_solicitudEscritura* desserializarSolicitudEscritura(char* solicitud);

char* serializarSolicitudSegmento(t_crearSegmentoUMV *segmento);
t_crearSegmentoUMV *deserializarSolicitudSegmento(char *solicitud);

#endif /* SERIALIZADORES_H_ */
