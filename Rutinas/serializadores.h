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
        t_pun sizeContext;
        t_pun cursorStack;
}__attribute__((packed)) t_iPCBaCPU;

typedef struct{
        int32_t tamanioID;
        t_nombre_dispositivo id;
        int32_t tiempo;
}__attribute__((packed)) t_iESdeCPU;

typedef struct{
	t_nombre_compartida variable;
	t_valor_variable valor;
	int32_t tamanio;
}__attribute__((packed)) t_asignacion;

typedef struct
{
	char * nombre;
	int32_t valor;
	pthread_mutex_t * mutex;
}__attribute__((packed)) t_variable_compartida;

typedef struct
{
	char * nombre;
	int32_t valor;
}__attribute__((packed)) t_iVARCOM;

char* serializarPCB(t_PCB * PCB);
t_PCB *desserializarPCB(char* PCBSerializada);

char* serializarSolicitudSegmento(t_crearSegmentoUMV *segmento);
t_crearSegmentoUMV *deserializarSolicitudSegmento(char *solicitud);

char* serializarSolicitudLectura(t_solicitudLectura* solicitud);
t_solicitudLectura* desserializarSolicitudLectura(char* solicitud);

char* serializarSolicitudEscritura(t_solicitudEscritura* solicitud);
t_solicitudEscritura* desserializarSolicitudEscritura(char* solicitud);

char* serializarPCBCPUKernel(t_PCB *pcb);
t_iPCBaCPU* deserializarRetornoPCBdeCPU(char *payload);

char* serializar_mensaje_Es(t_iESdeCPU* datosES);
t_iESdeCPU* desserializar_mensaje_ES(char * payload);

char* serializarAsignacionVariable(t_asignacion *asig);
t_asignacion * desserializarAsignacionVariable(char* payload);

char * serializar_datos_pcb_para_cpu(t_PCB * pcb);

t_iPCBaCPU * deserializarRetornoPCBdeCPU(char *);

char* serializar_mensaje_ES(t_iESdeCPU*);
t_iESdeCPU * deserializar_mensaje_ES(char *, uint32_t);

char * deserializar_mensaje_excepcion(char *, uint32_t);

char* serializar_datos_variable(t_iVARCOM*,uint32_t);
t_iVARCOM * deserializar_datos_variable(char *, uint32_t);

char * deserializar_nombre_recurso(char *, uint32_t);

#endif /* SERIALIZADORES_H_ */
