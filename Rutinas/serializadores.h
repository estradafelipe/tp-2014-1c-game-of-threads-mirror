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
#include <parser/parser.h>
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

typedef struct{
        t_pun id;
        t_pun indiceEtiquetas;
        t_pun programcounter;
}__attribute__((packed)) t_iPCBaCPU;

typedef struct{
        t_pun tamanioID;
        t_pun id;
        t_pun tiempo;
}__attribute__((packed)) t_iESdeCPU;

typedef struct{
	t_nombre_compartida variable;
	t_valor_variable valor;
}__attribute__((packed)) t_asignacion;

char* serializarPCB(t_PCB * PCB);
t_PCB *desserializarPCB(char* PCBSerializada);

char* serializarSolicitudSegmento(t_crearSegmentoUMV *segmento);
t_crearSegmentoUMV *deserializarSolicitudSegmento(char *solicitud);

char* serializarSolicitudLectura(t_solicitudLectura* solicitud);
t_solicitudLectura* desserializarSolicitudLectura(char* solicitud);

char* serializarSolicitudEscritura(t_solicitudEscritura* solicitud);
t_solicitudEscritura* desserializarSolicitudEscritura(char* solicitud);

char* serializarPCBCPUKernel(t_PCB *pcb);
t_iPCBaCPU* desserializarPCBCPUKernel(char *payload);

t_iESdeCPU* deserializar_mensaje_ES(char * payload);

#endif /* SERIALIZADORES_H_ */
