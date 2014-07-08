/*
 * serializadores.c
 *
 *  Created on: 30/10/2013
 *      Author: utnso
 */
#include <stdlib.h>
#include "serializadores.h"
#include <string.h>


char* serializarPCB(t_PCB * PCB){
	char* stream = malloc(sizeof(t_pun)*9);
	int size=0, offset=0;
	size = sizeof(t_pun);
	memcpy(stream, &PCB->cursorStack, size);
	offset += size;
	size = sizeof(t_pun);
	memcpy (stream + offset, &PCB->id, size);
	offset += size;
	size = sizeof(t_pun);
	memcpy (stream + offset, &PCB->indiceCodigo, size);
	offset += size;
	size = sizeof(t_pun);
	memcpy (stream + offset, &PCB->indiceEtiquetas, size);
	offset += size;
	size = sizeof(t_pun);
	memcpy (stream + offset, &PCB->programcounter, size);
	offset += size;
	size = sizeof(t_pun);
	memcpy (stream + offset, &PCB->segmentoCodigo, size);
	offset += size;
	size = sizeof(t_pun);
	memcpy (stream + offset, &PCB->segmentoStack, size);
	offset += size;
	size = sizeof(t_pun);
	memcpy (stream + offset, &PCB->sizeContext, size);
	offset += size;
	size = sizeof(t_pun);
	memcpy (stream + offset, &PCB->sizeIndexLabel, size);
	return stream;
}

t_PCB *desserializarPCB(char* PCBSerializada){
	int offset = 0, tmp_size = 0;
		t_PCB * PCB ;
		PCB = malloc(sizeof(t_PCB));

		tmp_size = sizeof(t_pun);
		memcpy(&PCB->cursorStack,PCBSerializada+offset,tmp_size);

		offset += tmp_size;
		tmp_size = sizeof(t_pun);
		memcpy(&PCB->id,PCBSerializada+offset,tmp_size);

		offset += tmp_size;
		tmp_size = sizeof(t_pun);
		memcpy(&PCB->indiceCodigo,PCBSerializada+offset,tmp_size);

		offset += tmp_size;
		tmp_size = sizeof(t_pun);
		memcpy(&PCB->indiceEtiquetas,PCBSerializada+offset,tmp_size);

		offset += tmp_size;
		tmp_size = sizeof(t_pun);
		memcpy(&PCB->programcounter,PCBSerializada+offset,tmp_size);

		offset += tmp_size;
		tmp_size = sizeof(t_pun);
		memcpy(&PCB->segmentoCodigo,PCBSerializada+offset,tmp_size);

		offset += tmp_size;
		tmp_size = sizeof(t_pun);
		memcpy(&PCB->segmentoStack,PCBSerializada+offset,tmp_size);

		offset += tmp_size;
		tmp_size = sizeof(t_pun);
		memcpy(&PCB->sizeContext,PCBSerializada+offset,tmp_size);

		offset += tmp_size;
		tmp_size = sizeof(t_pun);
		memcpy(&PCB->sizeIndexLabel,PCBSerializada+offset,tmp_size);

		return PCB;
}


char* serializarSolicitudSegmento(t_crearSegmentoUMV *segmento){
	char *stream = malloc(sizeof(t_crearSegmentoUMV));
	int size =0, offset =0;
	size = sizeof(int);
	memcpy(stream+offset,&segmento->programid,size);
	offset +=size;
	size = sizeof(int);
	memcpy(stream+offset,&segmento->size,size);

	return stream;
}

t_crearSegmentoUMV *deserializarSolicitudSegmento(char *solicitud){
	int offset = 0, tmp_size = 0;
	t_crearSegmentoUMV * segmento = malloc(sizeof(t_crearSegmentoUMV));
	tmp_size = sizeof(t_pun);
	memcpy(&segmento->programid,solicitud+offset,tmp_size);

	offset += tmp_size;
	tmp_size = sizeof(t_pun);
	memcpy(&segmento->size,solicitud+offset,tmp_size);

	return segmento;
}

char* serializarSolicitudLectura(t_solicitudLectura* solicitud){
	char *stream = malloc(sizeof(t_pun)*3);
	int size=0, offset=0;
	size = sizeof(t_pun);
	memcpy(stream, &solicitud->base, size);
	offset += size;
	size = sizeof(t_pun);
	memcpy (stream + offset, &solicitud->offset, size);
	offset += size;
	size = sizeof(t_pun);
	memcpy (stream + offset, &solicitud->tamanio, size);

	return stream;
}

t_solicitudLectura* desserializarSolicitudLectura(char* solicitud){
	int offset = 0, tmp_size = 0;
	t_solicitudLectura * solic ;
	solic = malloc(sizeof(t_solicitudLectura));

	tmp_size = sizeof(t_pun);
	memcpy(&solic->base,solicitud+offset,tmp_size);

	offset += tmp_size;
	tmp_size = sizeof(t_pun);
	memcpy(&solic->offset,solicitud+offset,tmp_size);

	offset += tmp_size;
	tmp_size = sizeof(t_pun);
	memcpy(&solic->tamanio,solicitud+offset,tmp_size);

	return solic;
}

char* serializarSolicitudEscritura(t_solicitudEscritura* solicitud){
	char *stream = malloc(sizeof(t_pun)*3 + solicitud->tamanio);
	int size=0, offset=0;
	size = sizeof(t_pun);
	memcpy(stream, &solicitud->base, size);
	offset += size;
	size = sizeof(t_pun);
	memcpy (stream + offset, &solicitud->offset, size);
	offset += size;
	size = sizeof(t_pun);
	memcpy (stream + offset, &solicitud->tamanio, size);
	offset += size;
	size = solicitud->tamanio;
	memcpy (stream + offset, solicitud->buffer, size);

	return stream;
}

t_solicitudEscritura* desserializarSolicitudEscritura(char* solicitud){
	int offset = 0, tmp_size = 0;
	t_solicitudEscritura * solic ;
	solic = malloc(sizeof(t_solicitudEscritura));

	tmp_size = sizeof(t_pun);
	memcpy(&solic->base,solicitud+offset,tmp_size);

	offset += tmp_size;
	tmp_size = sizeof(t_pun);
	memcpy(&solic->offset,solicitud+offset,tmp_size);

	offset += tmp_size;
	tmp_size = sizeof(t_pun);
	memcpy(&solic->tamanio,solicitud+offset,tmp_size);

	offset += tmp_size;
	//for (tmp_size=1 ; (solicitud+offset) [tmp_size-1] != '\0'; tmp_size++);
	tmp_size = solic->tamanio;
	solic->buffer = malloc (tmp_size);
	memcpy(solic->buffer, solicitud+offset, tmp_size);

	return solic;
}

char * serializar_datos_pcb_para_cpu(t_PCB * pcb){
        char *stream = malloc(sizeof(t_pun)*3);
        int size=0, offset=0;
        size = sizeof(t_pun);
        memcpy(stream, &pcb->id, size);
        offset += size;
        size = sizeof(t_pun);
        memcpy (stream + offset, &pcb->indiceEtiquetas, size);
        offset += size;
        size = sizeof(t_pun);
        memcpy (stream + offset, &pcb->programcounter, size);

        return stream;
}

t_iPCBaCPU* deserializarRetornoPCBdeCPU(char * payload){
        int offset = 0, tmp_size = 0;
        t_iPCBaCPU * datosPCB;
        datosPCB = malloc(sizeof(t_iPCBaCPU));

        tmp_size = sizeof(t_pun);
        memcpy(&datosPCB->id,payload+offset,tmp_size);

        offset += tmp_size;
        tmp_size = sizeof(t_pun);
        memcpy(&datosPCB->indiceEtiquetas,payload+offset,tmp_size);

        offset += tmp_size;
        tmp_size = sizeof(t_pun);
        memcpy(&datosPCB->programcounter,payload+offset,tmp_size);

        return datosPCB;
}

char* serializar_mensaje_ES(t_iESdeCPU* datosES){
	char *stream = malloc(sizeof(int32_t)*2+datosES->tamanioID);
	int size=0, offset =0;
	size = sizeof(int32_t);
	memcpy(stream,&datosES->tiempo,size);
	offset+=size;
	size = sizeof(int32_t);
	memcpy(stream + offset,&datosES->tamanioID,size);
	offset+=size;
	size = datosES->tamanioID;
	memcpy(stream+offset,datosES->id,size);
	return stream;
}

t_iESdeCPU * deserializar_mensaje_ES(char * payload){
        t_iESdeCPU* datosES;
        datosES = malloc(sizeof(t_iESdeCPU));
		int size=0, offset=0;

		size = sizeof(int32_t);
		memcpy (&datosES->tiempo, payload, size);
		offset += size;
		size = sizeof(int32_t);
		memcpy (&datosES->tamanioID, payload+offset, size);
		offset += size;
		size = datosES->tamanioID;
		datosES->id = malloc(datosES->tamanioID);
		memcpy(datosES->id, payload+offset, size);
		return datosES;





}

char * deserializar_mensaje_excepcion(char * cadena, uint32_t longitud){
	char * mensaje=malloc(longitud +1);
	memcpy(mensaje, cadena, longitud+1); //el mensaje viaja con "\0" incluido La longitud tambien viene con el +1 o se agrega aca
	//memcpy(mensaje+longitud, "\0", 1);
	return mensaje;
}

char * deserializar_nombre_recurso(char * mensaje, uint32_t longitud){
	char * recurso = malloc(longitud);
	memcpy(recurso, mensaje, longitud);
	return recurso;
}

char* serializar_datos_variable(t_iVARCOM* asig,uint32_t longitud){
	char* stream = malloc(sizeof(int32_t) + longitud );
	int size=0, offset=0;
	size = longitud;
	memcpy(stream, asig->nombre, size);
	offset += size;
	size = sizeof(int32_t);
	memcpy (stream + offset, &asig->valor, size);
	return stream;


}


t_iVARCOM * deserializar_datos_variable(char * mensaje, uint32_t longitud){
    int offset = 0, tmp_size = 0;
    printf("Hola\n");
    t_iVARCOM * datos_variables = malloc(sizeof(t_iVARCOM));
    tmp_size=longitud-sizeof(int32_t);
    datos_variables->nombre = malloc(tmp_size);
    memcpy(datos_variables->nombre, mensaje+offset, tmp_size);
    printf("Hola\n");

    offset += tmp_size;
    tmp_size = sizeof(uint32_t); //pasamos mensajes cadena con "\0"
    memcpy(&datos_variables->valor, mensaje+offset, tmp_size);
    printf("Hola\n");

	return datos_variables;
}

char* serializarAsignacionVariable(t_asignacion *asig){
	char *stream = malloc(sizeof(int)+ sizeof(int32_t) + asig->tamanio);
	int size=0, offset=0;
	size = sizeof(int);
	memcpy(stream, &asig->valor, size);
	offset += size;
	size = sizeof(int32_t);
	memcpy (stream + offset, &asig->tamanio, size);
	offset += size;
	size = asig->tamanio;
	memcpy (stream + offset, asig->variable, size);
	return stream;
}


t_asignacion * desserializarAsignacionVariable(char* payload){
		int offset = 0, tmp_size = 0;
		t_asignacion * asig ;
		asig = malloc(sizeof(t_asignacion));

		tmp_size = sizeof(int);
		memcpy(&asig->valor,payload+offset,tmp_size);

		offset += tmp_size;
		tmp_size = sizeof(int32_t);
		memcpy(&asig->tamanio,payload+offset,tmp_size);

		offset += tmp_size;
		tmp_size = asig->tamanio;
		asig->variable = malloc (tmp_size);
		memcpy(asig->variable, payload+offset, tmp_size);

		return asig;
}
