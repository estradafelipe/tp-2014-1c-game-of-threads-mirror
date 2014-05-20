/*
 * serializadores.c
 *
 *  Created on: 30/10/2013
 *      Author: utnso
 */
#include <stdlib.h>
#include "serializadores.h"
#include <string.h>

char* serializarSolicitudLectura(t_solicitudLectura* solicitud){
	char *stream = malloc(sizeof(t_puntero)*3);
	int size=0, offset=0;
	size = sizeof(t_puntero);
	memcpy(stream, &solicitud->base, size);
	offset += size;
	size = sizeof(t_puntero);
	memcpy (stream + offset, &solicitud->offset, size);
	offset += size;
	size = sizeof(t_puntero);
	memcpy (stream + offset, &solicitud->tamanio, size);

	return stream;
}

t_solicitudLectura* desserializarSolicitudLectura(char* solicitud){
	int offset = 0, tmp_size = 0;
	t_solicitudLectura * solic ;
	solic = malloc(sizeof(t_solicitudLectura));

	tmp_size = sizeof(t_puntero);
	memcpy(&solic->base,solicitud+offset,tmp_size);

	offset += tmp_size;
	tmp_size = sizeof(t_puntero);
	memcpy(&solic->offset,solicitud+offset,tmp_size);

	offset += tmp_size;
	tmp_size = sizeof(t_puntero);
	memcpy(&solic->tamanio,solicitud+offset,tmp_size);

	return solic;
}

char* serializarSolicitudEscritura(t_solicitudEscritura* solicitud){
	char *stream = malloc(sizeof(t_puntero)*3 + strlen(solicitud->buffer)+1);
	int size=0, offset=0;
	size = sizeof(t_puntero);
	memcpy(stream, &solicitud->base, size);
	offset += size;
	size = sizeof(t_puntero);
	memcpy (stream + offset, &solicitud->offset, size);
	offset += size;
	size = sizeof(t_puntero);
	memcpy (stream + offset, &solicitud->tamanio, size);
	offset += size;
	size = strlen(solicitud->buffer)+1;
	memcpy (stream + offset, solicitud->buffer, size);

	return stream;
}
t_solicitudEscritura* desserializarSolicitudEscritura(char* solicitud){
	int offset = 0, tmp_size = 0;
	t_solicitudEscritura * solic ;
	solic = malloc(sizeof(t_solicitudEscritura));

	tmp_size = sizeof(t_puntero);
	memcpy(&solic->base,solicitud+offset,tmp_size);

	offset += tmp_size;
	tmp_size = sizeof(t_puntero);
	memcpy(&solic->offset,solicitud+offset,tmp_size);

	offset += tmp_size;
	tmp_size = sizeof(t_puntero);
	memcpy(&solic->tamanio,solicitud+offset,tmp_size);

	offset += tmp_size;
	for (tmp_size=1 ; (solicitud+offset) [tmp_size-1] != '\0'; tmp_size++);
	solic->buffer = malloc (tmp_size);
	memcpy(solic->buffer, solicitud+offset, tmp_size);

	return solic;
}


char* serializarPresentacionPersonaje(t_presentacionPersonaje * pers){
	char *stream = malloc(strlen(pers->nomNivel)+1 + strlen(pers->nomPersonaje)+1);
	int size=0, offset=0;
	memcpy(stream, pers->nomNivel, size = strlen(pers->nomNivel)+1);
	offset += size;
	size = strlen(pers->nomPersonaje)+1;
	memcpy (stream + offset, pers->nomPersonaje, size);
	return stream;
}

t_presentacionPersonaje * desserializarPresentPersonaje (char* pers){
	int offset = 0, tmp_size = 0;
	t_presentacionPersonaje * present ;
	present = malloc(sizeof(t_presentacionPersonaje));

	for (tmp_size=1 ; (pers+offset) [tmp_size-1] != '\0'; tmp_size++);
	present->nomNivel = malloc (tmp_size);
	memcpy(present->nomNivel, pers+offset, tmp_size);

	offset += tmp_size;

	for (tmp_size=1 ; (pers+offset) [tmp_size-1] != '\0'; tmp_size++);
	present->nomPersonaje = malloc (tmp_size);
	memcpy(present->nomPersonaje, pers+offset, tmp_size);

	return present;

}


char* serializarPresNivel(t_presentacionNivel * pres){
	char *stream = malloc(strlen(pres->nomNivel)+1 + strlen(pres->algoritmo)+1 + sizeof(int16_t)*2);
	int size=0, offset=0;
	memcpy(stream, pres->nomNivel, size = strlen(pres->nomNivel)+1);
	offset += size;
	size = strlen(pres->algoritmo)+1;
	memcpy (stream + offset, pres->algoritmo, size);
	offset += size;
	size = sizeof(int16_t);
	memcpy (stream + offset, &pres->quantum, size);
	offset += size;
	size = sizeof(int16_t);
	memcpy (stream + offset, &pres->retardo, size);

	return stream;
}


t_presentacionNivel * desserializarPresNivel (char* pres){
	int offset = 0, tmp_size = 0;
	t_presentacionNivel * present ;
	present = malloc(sizeof(t_presentacionNivel));

	for (tmp_size=1 ; (pres+offset) [tmp_size-1] != '\0'; tmp_size++);
	present->nomNivel = malloc (tmp_size);
	memcpy(present->nomNivel, pres+offset, tmp_size);

	offset += tmp_size;

	for (tmp_size=1 ; (pres+offset) [tmp_size-1] != '\0'; tmp_size++);
	present->algoritmo = malloc (tmp_size);
	memcpy(present->algoritmo, pres+offset, tmp_size);

	offset += tmp_size;
	tmp_size = sizeof(int16_t);
	memcpy(&present->quantum,pres+offset,tmp_size);

	offset += tmp_size;
	tmp_size = sizeof(int16_t);
	memcpy(&present->retardo,pres+offset,tmp_size);


	return present;
}



char* serializarNivelConRecursos(t_nivelConRecursos * pres){
	char *stream = malloc(strlen(pres->nomNivel)+1 + strlen(pres->recursos)+1);
	int size=0, offset=0;
	memcpy(stream, pres->nomNivel, size = strlen(pres->nomNivel)+1);
	offset += size;
	size = strlen(pres->recursos)+1;
	memcpy (stream + offset, pres->recursos, size);
	return stream;
}


t_nivelConRecursos * desserializarNivelConRecursos (char* pres){
	int offset = 0, tmp_size = 0;
	t_nivelConRecursos * present ;
	present = malloc(sizeof(t_nivelConRecursos));

	for (tmp_size=1 ; (pres+offset) [tmp_size-1] != '\0'; tmp_size++);
	present->nomNivel = malloc (tmp_size);
	memcpy(present->nomNivel, pres+offset, tmp_size);

	offset += tmp_size;

	for (tmp_size=1 ; (pres+offset) [tmp_size-1] != '\0'; tmp_size++);
	present->recursos = malloc (tmp_size);
	memcpy(present->recursos, pres+offset, tmp_size);


	return present;
}





// PARA deadlcok
char* serializarNivelConCulpables(t_nivelConCulpables * pres){
	char *stream = malloc(strlen(pres->nomNivel)+1 + strlen(pres->personajes)+1);
	int size=0, offset=0;
	memcpy(stream, pres->nomNivel, size = strlen(pres->nomNivel)+1);
	offset += size;
	size = strlen(pres->personajes)+1;
	memcpy (stream + offset, pres->personajes, size);
	return stream;
}


t_nivelConCulpables * desserializarNivelConCulpables (char* pres){
	int offset = 0, tmp_size = 0;
	t_nivelConCulpables * present ;
	present = malloc(sizeof(t_nivelConCulpables));

	for (tmp_size=1 ; (pres+offset) [tmp_size-1] != '\0'; tmp_size++);
	present->nomNivel = malloc (tmp_size);
	memcpy(present->nomNivel, pres+offset, tmp_size);

	offset += tmp_size;

	for (tmp_size=1 ; (pres+offset) [tmp_size-1] != '\0'; tmp_size++);
	present->personajes = malloc (tmp_size);
	memcpy(present->personajes, pres+offset, tmp_size);


	return present;
}



