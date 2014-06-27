/*
 * primitivas.c
 *
 *  Created on: 27/06/2014
 *      Author: utnso
 */

#include "primitivas.h"


t_puntero GameOfThread_definirVariable(t_nombre_variable identificador_variable){

	char* var = malloc(sizeof(t_nombre_variable)+1);
	sprintf(var,"%c",identificador_variable);
	uint32_t sizeContext = pcb->sizeContext;
	t_solicitudEscritura *sol =  malloc(sizeof(t_solicitudEscritura));
	package* package_escritura = malloc(sizeof(package));
	package* package_respuesta = malloc(sizeof(package));
	t_puntero puntero;

	sol->base = pcb->segmentoStack;
	sol->offset = sizeContext*5 + (pcb->cursorStack - pcb->segmentoStack);
	sol->tamanio = TAMANIO_ID_VAR;
	memcpy(sol->buffer,var,strlen(var));
	puntero = (uint32_t)sol->offset;

	char* payloadSerializado = serializarSolicitudEscritura(sol);

	package_escritura = crear_paquete(escritura,payloadSerializado,sizeof(t_puntero)*3 + strlen(sol->buffer));
	enviar_paquete(package_escritura,socketUMV);

	package_respuesta = recibir_paquete(socketUMV);
	int payload;
	memcpy(&payload, package_respuesta->payload, package_respuesta->payloadLength);

	if ((payload) == -1){
		printf("Violacion de segmento\n");
		notificar_kernel(violacionSegmento);
		exit(1);
	}

	dictionary_put(diccionarioVariables, var,(void*) puntero);

	pcb->sizeContext++;

	return puntero;
}


t_puntero GameOfThread_obtenerPosicionVariable(t_nombre_variable identificador_variable ){
	char* key = malloc(sizeof(t_nombre_variable)+1);
	sprintf(key,"%c",identificador_variable);
	t_puntero posicion = (t_puntero) dictionary_get(diccionarioVariables, key);
	return posicion;
}

t_valor_variable GameOfThread_dereferenciar(t_puntero direccion_variable){
	t_valor_variable valorVariable;
	package* solicitudLectura = malloc(sizeof(package));;
	package* respuesta = malloc(sizeof(package));;
	t_solicitudLectura* sol = malloc(sizeof(t_solicitudLectura));
	sol->base = pcb->segmentoStack;
	sol->offset = direccion_variable;
	sol->tamanio = TAMANIO_VAR;

	char* payloadSerializado = serializarSolicitudLectura(sol);
	solicitudLectura = crear_paquete(lectura,payloadSerializado,sizeof(t_puntero)*3);
	enviar_paquete(solicitudLectura, socketUMV);

	respuesta = recibir_paquete(socketUMV);
	valorVariable = (t_valor_variable)respuesta->payload;
	return valorVariable;
}

void GameOfThread_asignar(t_puntero direccion_variable, t_valor_variable valor){
	package* solicitudEscritura = malloc(sizeof(package));
	package* respuesta = malloc(sizeof(package));
	t_solicitudEscritura* sol = malloc(sizeof(t_solicitudEscritura));
	int32_t error;
	sol->base = pcb->segmentoStack;
	sol->offset = direccion_variable + 1;
	sol->tamanio = TAMANIO_VAR;
	memcpy(sol->buffer,&valor,sizeof(t_valor_variable)); //TODO: Preguntar si esta bien

	char* payloadSerializado = serializarSolicitudEscritura(sol);
	solicitudEscritura = crear_paquete(escritura, payloadSerializado, (sizeof(t_pun)*3 + strlen(sol->buffer)));
	enviar_paquete(solicitudEscritura, socketUMV);

	respuesta = recibir_paquete(socketUMV);
	memcpy(&error,respuesta->payload,sizeof(int32_t));
	if (error == -1){
		printf("Violacion de segmento\n");
		notificar_kernel(violacionSegmento);
		exit(1);
	}
}

t_valor_variable GameOfThread_obtenerValorCompartida(t_nombre_compartida variable){
	package *solicitud = malloc(sizeof(package));
	package *respuesta = malloc(sizeof(package));
	t_valor_variable val;
	char * payload = malloc(sizeof(t_nombre_compartida));
	memcpy(payload,variable,sizeof(t_nombre_compartida));
	solicitud = crear_paquete(solicitarValorVariableCompartida,payload,sizeof(t_nombre_compartida));
	enviar_paquete(solicitud,socketKernel);
	respuesta = recibir_paquete(socketKernel);
	memcpy(&val,respuesta->payload,sizeof(t_nombre_compartida));
	return val;
}

t_valor_variable GameOfThread_asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor){
	package *solicitud = malloc(sizeof(package));
	t_asignacion *asig = malloc(sizeof(t_asignacion));

	asig->valor = valor;
	asig->variable = variable;

	char* payload = serializarAsignacionVariable(asig);
	solicitud =  crear_paquete(asignarValorVariableCompartida,payload,sizeof(t_nombre_compartida)+sizeof(t_valor_variable));
	enviar_paquete(solicitud,socketKernel);
	return valor;
}
t_puntero_instruccion GameOfThread_irAlLabel(t_nombre_etiqueta etiqueta){
	t_puntero_instruccion instruccion;
	char* etiquetas = malloc(sizeof(t_pun));
	memcpy(etiquetas,&pcb->indiceEtiquetas,sizeof(t_pun));
	instruccion = metadata_buscar_etiqueta(etiqueta, etiquetas, pcb->sizeIndexLabel);
	if (instruccion == -1){
		notificar_kernel(error_label);
		exit(1);
	}else{
		return instruccion;
	}
}

void GameOfThread_llamarSinRetorno(t_nombre_etiqueta etiqueta){
	t_solicitudEscritura *sol = malloc(sizeof(t_solicitudEscritura));
	package* solicitudEscritura = malloc(sizeof(package));
	package* respuesta = malloc(sizeof(package));
	int32_t error;
	t_pun pc;
	char* serializarEscritura = malloc(sizeof(t_pun)*3 + strlen(sol->buffer)+1);
	sol->base = pcb->segmentoStack;
	sol->offset =  pcb->sizeContext*5 + (pcb->cursorStack - pcb->segmentoStack);
	sol->tamanio = 4;
	memcpy(sol->buffer,&pcb->cursorStack,sizeof(t_pun));

	//Guardamos el Contexto de Ejecucion Anterior;
	serializarEscritura = serializarSolicitudEscritura(sol);
	solicitudEscritura = crear_paquete(escritura,serializarEscritura,sizeof(sizeof(t_pun)*3 + strlen(sol->buffer)+1));
	enviar_paquete(solicitudEscritura,socketUMV);
	respuesta = recibir_paquete(socketUMV);
	memcpy(&error,respuesta->payload,sizeof(int32_t));
	if(error == -1){
		notificar_kernel(violacionSegmento);
		exit(1);
	}

	//Guardamos el Program Counter siguiente;
	sol->base = pcb->segmentoStack;
	sol->offset =  pcb->sizeContext*5 + (pcb->cursorStack - pcb->segmentoStack) + 4; //Son los 4 del Contexto Anterior
	sol->tamanio = 4;
	pc = pcb->programcounter;
	pc++;
	memcpy(sol->buffer,&pc,sizeof(t_pun));
	serializarEscritura = serializarSolicitudEscritura(sol);
	solicitudEscritura = crear_paquete(escritura,serializarEscritura,sizeof(sizeof(t_pun)*3 + strlen(sol->buffer)+1));
		enviar_paquete(solicitudEscritura,socketUMV);
		respuesta = recibir_paquete(socketUMV);
		memcpy(&error,respuesta->payload,sizeof(int32_t));
		if(error == -1){
			notificar_kernel(violacionSegmento);
			exit(1);
		}

	pcb->cursorStack = sol->offset + 4;
	dictionary_clean(diccionarioVariables);

	GameOfThread_irAlLabel(etiqueta); //Me lleva al procedimiento que debo ejecutar;

}

void GameOfThread_llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar){
	t_solicitudEscritura *sol = malloc(sizeof(t_solicitudEscritura));
	package* solicitudEscritura = malloc(sizeof(package));
	package* respuesta = malloc(sizeof(package));
	char* serializarEscritura = malloc(sizeof(t_pun)*3 + strlen(sol->buffer)+1);
	int32_t error;
	t_pun pc;

	//Guardamos el Contexto de Ejecucion Anterior;
	sol->base = pcb->segmentoStack;
	sol->offset =  pcb->sizeContext*5 + (pcb->cursorStack - pcb->segmentoStack);
	sol->tamanio = 4;
	memcpy(sol->buffer,&pcb->cursorStack,sizeof(t_pun));
	serializarEscritura = serializarSolicitudEscritura(sol);
	solicitudEscritura = crear_paquete(escritura,serializarEscritura,sizeof(sizeof(t_pun)*3 + strlen(sol->buffer)+1));
	enviar_paquete(solicitudEscritura,socketUMV);
	respuesta = recibir_paquete(socketUMV);
	memcpy(&error,respuesta->payload,sizeof(int32_t));
	if(error == -1){
		notificar_kernel(violacionSegmento);
		exit(1);
	}

	//Guardamos el Program Counter siguiente;
		sol->base = pcb->segmentoStack;
		sol->offset =  pcb->sizeContext*5 + (pcb->cursorStack - pcb->segmentoStack) + 4; //Son los 4 del Contexto Anterior
		sol->tamanio = 4;
		pc = pcb->programcounter;
		pc++;
		memcpy(sol->buffer,&pc,sizeof(t_pun));
		serializarEscritura = serializarSolicitudEscritura(sol);
		solicitudEscritura = crear_paquete(escritura,serializarEscritura,sizeof(sizeof(t_pun)*3 + strlen(sol->buffer)+1));
			enviar_paquete(solicitudEscritura,socketUMV);
			respuesta = recibir_paquete(socketUMV);
			memcpy(&error,respuesta->payload,sizeof(int32_t));
			if(error == -1){
				notificar_kernel(violacionSegmento);
				exit(1);
			}
	//Guardamos a donde retornar;
		sol->base = pcb->segmentoStack;
		sol->offset =  pcb->sizeContext*5 + (pcb->cursorStack - pcb->segmentoStack) + 8;
		sol->tamanio = 4;
		memcpy(sol->buffer,&donde_retornar,sizeof(uint32_t));
		serializarEscritura = serializarSolicitudEscritura(sol);
		solicitudEscritura = crear_paquete(escritura,serializarEscritura,sizeof(sizeof(t_pun)*3 + strlen(sol->buffer)+1));
					enviar_paquete(solicitudEscritura,socketUMV);
					respuesta = recibir_paquete(socketUMV);
					memcpy(&error,respuesta->payload,sizeof(int32_t));
					if(error == -1){
						notificar_kernel(violacionSegmento);
						exit(1);
					}

		pcb->cursorStack = sol->offset + 4;
		dictionary_clean(diccionarioVariables);
		GameOfThread_irAlLabel(etiqueta); //Me lleva al procedimiento que debo ejecutar;

}

void GameOfThread_finalizar(void){

t_solicitudLectura *sol = malloc(sizeof(t_solicitudLectura));
package *paquete = malloc(sizeof(package));
char* serializado = malloc(sizeof(t_pun)*3);
	if(pcb->cursorStack == pcb->segmentoCodigo){
		dictionary_clean(diccionarioVariables);
		notificar_kernel(finPrograma);
		free(pcb);
		quantumPrograma = quantumKernel;
		}
	else{
		//Obtengo el Program Counter (Instruccion Siguiente)
		sol->base = pcb->segmentoStack;
		sol->offset = pcb->cursorStack - 4;
		sol->tamanio = 4;
		serializado = serializarSolicitudLectura(sol);
		paquete = crear_paquete(lectura,serializado,sizeof(t_pun)*3);
		enviar_paquete(paquete,socketUMV);
		paquete = recibir_paquete(socketUMV);
		memcpy(&pcb->programcounter,paquete->payload,sizeof(t_pun));
		//Obtengo el Cursor del Contexto Anterior
		sol->base = pcb->segmentoStack;
		sol->offset = pcb->cursorStack - 8;
		sol->tamanio = 4;

		serializado = serializarSolicitudLectura(sol);
		paquete = crear_paquete(lectura,serializado,sizeof(t_pun)*3);
		enviar_paquete(paquete,socketUMV);
		paquete = recibir_paquete(socketUMV);
		memcpy(&pcb->cursorStack,paquete->payload,sizeof(t_pun));

		//Limpio el diccionario y lo cargo
		dictionary_clean(diccionarioVariables);
		int32_t cant_var = (sol->offset - pcb->cursorStack)/5;
		cargar_diccionarioVariables(cant_var);
		}
}

void GameOfThread_retornar(t_valor_variable retorno){
	package* paquete = malloc(sizeof(package));
	t_pun base,offset_tmp,tamanio;
	t_puntero dirRetorno;

	base = pcb->segmentoStack;
	offset_tmp = pcb->cursorStack;
	tamanio = 4;

	//Obtengo Donde Retornar;

	paquete = Leer(base,offset_tmp - 4 , tamanio);
	memcpy(&dirRetorno,paquete->payload,sizeof(int32_t));
	destruir_paquete(paquete);

	//Escribo en dirRetorno el valor de retorno t_valor_variable
	char* buffer = malloc(sizeof(t_valor_variable));
	memcpy(buffer,&retorno,sizeof(t_valor_variable));
	paquete = Escribir(base,dirRetorno,tamanio,buffer);
	destruir_paquete(paquete);
	//Obtengo la proxima instruccion (Program Counter)
	paquete = Leer(base,offset_tmp - 8, tamanio);
	memcpy(&pcb->programcounter,paquete->payload,sizeof(t_pun));
	destruir_paquete(paquete);
	//Cambio de Contexto
	paquete = Leer(base,offset_tmp - 12, tamanio);
	memcpy(&pcb->cursorStack,paquete->payload,sizeof(t_pun));

	//Limpio el diccionario y lo cargo
	dictionary_clean(diccionarioVariables);
	int32_t cant_var = ((offset_tmp - 12) - pcb->cursorStack)/5;
	cargar_diccionarioVariables(cant_var);
}

void GameOfThread_imprimirTexto(char* texto){
	package* paquete = malloc(sizeof(package));
	paquete = crear_paquete(programaImprimirTexto,texto,strlen(texto));
	enviar_paquete(paquete,socketKernel);
	destruir_paquete(paquete);
}

void GameOfThread_entradaSalida(t_nombre_dispositivo dispositivo, int tiempo){
	t_entradaSalida *es = malloc(sizeof(t_entradaSalida));
	package *paquete;
	es->dispositivo = dispositivo;
	es->tiempo = tiempo;
	char* payload = serializarEntradaSalida(es);
	paquete = crear_paquete(entrada_salida,payload,sizeof(t_nombre_dispositivo)+sizeof(uint32_t));
	enviar_paquete(paquete,socketKernel);
	destruir_paquete(paquete);
	quantumPrograma = quantumKernel;
	notificar_kernel(bloquearProgramaCPU);
}

void GameOfThread_wait(t_nombre_semaforo identificador_semaforo){
	package *paquete;
	char *payload = malloc(sizeof(t_nombre_semaforo));
	memcpy(payload,identificador_semaforo,sizeof(t_nombre_semaforo));
	paquete = crear_paquete(waitPrograma,payload,sizeof(t_nombre_semaforo));
	enviar_paquete(paquete,socketKernel);
}

void GameOfThread_signal(t_nombre_semaforo identificador_semaforo){
	package *paquete;
	char *payload = malloc(sizeof(t_nombre_semaforo));
	memcpy(payload,identificador_semaforo,sizeof(t_nombre_semaforo));
	paquete = crear_paquete(signalPrograma,payload,sizeof(t_nombre_semaforo));
	enviar_paquete(paquete,socketKernel);
}
