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

