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
	int base;
	int offset;
	int size;
	char* buffer;
}__attribute__((packed)) t_envioBytesUMV;

typedef struct{
	int programid;
	int size;
}__attribute__((packed)) t_crearSegmentoUMV;

typedef struct{
	char* nomNivel;
	char* nomPersonaje;
	char* simbolo;
}__attribute__((packed)) t_presentacionPersonaje;

typedef struct{
	char *nomNivel;
	int16_t *retardo;
	int16_t *quantum;
	char *algoritmo;
}__attribute__((packed)) t_presentacionNivel;

typedef struct{
	char *nomNivel;
	char *recursos;
}__attribute__((packed)) t_nivelConRecursos;

typedef struct{
	char *nomNivel;
	char *personajes;
}__attribute__((packed)) t_nivelConCulpables;

typedef struct{
	t_puntero* base;
	t_puntero* offset;
	t_puntero* tamanio;
}__attribute__((packed)) t_solicitudLectura;

typedef struct{
	t_puntero* base;
	t_puntero* offset;
	t_puntero* tamanio;
	char* buffer;
}__attribute__((packed)) t_solicitudEscritura;


char* serializarSolicitudLectura(t_solicitudLectura* solicitud);
t_solicitudLectura* desserializarSolicitudLectura(char* solicitud);

char* serializarSolicitudEscritura(t_solicitudEscritura* solicitud);
t_solicitudEscritura* desserializarSolicitudEscritura(char* solicitud);

char* serializarSolicitudSegmento(t_crearSegmentoUMV *segmento);
char* serializarEnvioBytes(t_envioBytesUMV *envioBytes);
char* serializarPresentacionPersonaje(t_presentacionPersonaje * pers);
t_presentacionPersonaje * desserializarPresentPersonaje (char* pers);

char* serializarPresNivel(t_presentacionNivel * pres);
t_presentacionNivel * desserializarPresNivel (char* pres);

char* serializarNivelConRecursos(t_nivelConRecursos * pres);
t_nivelConRecursos * desserializarNivelConRecursos(char* pres);

char* serializarNivelConCulpables(t_nivelConCulpables * pres);
t_nivelConCulpables * desserializarNivelConCulpables(char* pres);

//char* serializarSolicitudSegmentosPCB(t_solicitudSegmentosPCB * solicitudSegmentosPCB);

#endif /* SERIALIZADORES_H_ */
