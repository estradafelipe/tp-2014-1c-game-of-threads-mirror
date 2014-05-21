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

typedef u_int32_t t_puntero;


typedef struct{
	t_puntero programid;
	t_puntero size;
}__attribute__((packed)) t_crearSegmentoUMV;


typedef struct{
	t_puntero base;
	t_puntero offset;
	t_puntero tamanio;
}__attribute__((packed)) t_solicitudLectura;

typedef struct{
	t_puntero base;
	t_puntero offset;
	t_puntero tamanio;
	char* buffer;
}__attribute__((packed)) t_solicitudEscritura;

typedef struct{
		uint32_t id;
		uint32_t segmentoCodigo;
		uint32_t segmentoStack;
		uint32_t cursorStack;
		uint32_t indiceCodigo;
		uint32_t indiceEtiquetas;
		uint32_t programcounter;
		uint32_t sizeContext;
		uint32_t sizeIndexLabel;
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
