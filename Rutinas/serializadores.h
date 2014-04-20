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





char* serializarPresentacionPersonaje(t_presentacionPersonaje * pers);
t_presentacionPersonaje * desserializarPresentPersonaje (char* pers);

char* serializarPresNivel(t_presentacionNivel * pres);
t_presentacionNivel * desserializarPresNivel (char* pres);

char* serializarNivelConRecursos(t_nivelConRecursos * pres);
t_nivelConRecursos * desserializarNivelConRecursos(char* pres);

char* serializarNivelConCulpables(t_nivelConCulpables * pres);
t_nivelConCulpables * desserializarNivelConCulpables(char* pres);



#endif /* SERIALIZADORES_H_ */
