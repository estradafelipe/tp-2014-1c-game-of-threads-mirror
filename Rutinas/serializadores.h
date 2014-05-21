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

#endif /* SERIALIZADORES_H_ */
