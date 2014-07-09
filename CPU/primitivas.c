/*
 * primitivas.c
 *
 *  Created on: 27/06/2014
 *      Author: utnso
 */

#include "primitivas.h"
#include <serializadores.h>

t_puntero GameOfThread_definirVariable(t_nombre_variable identificador_variable){

	char* var = malloc(sizeof(t_nombre_variable)+1);
	sprintf(var,"%c",identificador_variable);
	t_puntero puntero;
	t_pun offset;
	char* id = malloc(sizeof(char));
	offset = pcb->sizeContext*5 + pcb->cursorStack;

	memcpy(id,var,strlen(var));

	memcpy(&puntero,&offset,sizeof(t_pun));

	Escribir(pcb->segmentoStack,offset,1,id);
	dictionary_put(diccionarioVariables, var,(void*) puntero);
	pcb->sizeContext++;
	log_debug(logger,"Ejecutando Primitiva GameOfThread_definirVariable");
	return puntero;
}


t_puntero GameOfThread_obtenerPosicionVariable(t_nombre_variable identificador_variable ){

	char* key = malloc(sizeof(t_nombre_variable)+1);
	sprintf(key,"%c",identificador_variable);
	t_puntero posicion = (t_puntero)dictionary_get(diccionarioVariables, key);
	log_debug(logger,"Ejecutando Primitiva GameOfThread_obtenerPosicionVariable");
	return posicion;
}

t_valor_variable GameOfThread_dereferenciar(t_puntero direccion_variable){
	t_valor_variable valorVariable;
	package* paquete = malloc(sizeof(package));


	paquete = Leer(pcb->segmentoStack,direccion_variable,4);

	memcpy(&valorVariable,paquete->payload,sizeof(t_valor_variable));
	log_debug(logger,"Ejecutando Primitiva GameOfThread_dereferenciar");
	return valorVariable;
}

void GameOfThread_asignar(t_puntero direccion_variable, t_valor_variable valor){

	t_pun offset;
	char* buffer = malloc(sizeof(int32_t));


	offset = direccion_variable + 1;

	memcpy(buffer,&valor,sizeof(t_valor_variable));

	Escribir(pcb->segmentoStack,offset,4,buffer);
	log_debug(logger,"Ejecutando Primitiva GameOfThread_asignar");
	}

t_valor_variable GameOfThread_obtenerValorCompartida(t_nombre_compartida variable){
	package *paquete = malloc(sizeof(package));
	t_valor_variable val;

	paquete = crear_paquete(solicitarValorVariableCompartida,variable,strlen(variable)+1);
	enviar_paquete(paquete,socketKernel);
	destruir_paquete(paquete);
	paquete = recibir_paquete(socketKernel);
	if(paquete->type != solicitarValorVariableCompartida){
		notificarError_kernel("ERROR AL SOLICITAR EL VALOR DE LA VARIABLE COMPARTIDA");
	}
	memcpy(&val,paquete->payload,sizeof(t_nombre_compartida));
	log_debug(logger,"Ejecutando Primitiva GameOfThread_obtenerValorCompartida");
	return val;
}

t_valor_variable GameOfThread_asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor){
	package *solicitud = malloc(sizeof(package));
	t_iVARCOM *asig = malloc(sizeof(t_iVARCOM));

	asig->valor = valor;
	asig->nombre = variable;

	char* payload = serializar_datos_variable(asig,strlen(asig->nombre)+1 + sizeof(int32_t));
	solicitud =  crear_paquete(asignarValorVariableCompartida,payload,sizeof(t_nombre_compartida)+sizeof(t_valor_variable)+sizeof(int32_t));
	enviar_paquete(solicitud,socketKernel);
	log_debug(logger,"Ejecutando Primitiva GameOfThread_asignarValorCompartida");
	return valor;
}
void GameOfThread_irAlLabel(t_nombre_etiqueta etiqueta){
	t_puntero_instruccion instruccion;
	char* etiquetas = malloc(sizeof(t_pun));
	memcpy(etiquetas,&pcb->indiceEtiquetas,sizeof(t_pun));
	instruccion = metadata_buscar_etiqueta(etiqueta, etiquetas, pcb->sizeIndexLabel);
	if (instruccion == -1){
		notificarError_kernel("Error al encontrar label");
		exit(1);
	}

	pcb->programcounter = instruccion;
	log_debug(logger,"Ejecutando Primitiva GameOfThread_irAlLabel");
}

void GameOfThread_llamarSinRetorno(t_nombre_etiqueta etiqueta){
	t_pun pc;
	t_pun offset;
	char* buffer = malloc(sizeof(int32_t));

	//Guardamos el Contexto de Ejecucion Anterior;
	offset =  pcb->sizeContext*5 + pcb->cursorStack;
	memcpy(buffer,&pcb->cursorStack,sizeof(t_pun));
	Escribir(pcb->segmentoStack,offset,4,buffer);

	//Guardamos el Program Counter siguiente;
	offset =  pcb->sizeContext*5 + pcb->cursorStack + 4; //Son los 4 del Contexto Anterior
	pc = pcb->programcounter;
	pc++;
	memcpy(buffer,&pc,sizeof(t_pun));
	Escribir(pcb->segmentoStack,offset,4,buffer);

	//Cambio de Contexto
	pcb->cursorStack = offset + 4;
	dictionary_clean(diccionarioVariables);
	GameOfThread_irAlLabel(etiqueta); //Me lleva al procedimiento que debo ejecutar;

	log_debug(logger,"Ejecutando Primitiva GameOfThread_llamarSinRetorno");
}

void GameOfThread_llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar){
	char* buffer = malloc(sizeof(t_pun));
	t_pun offset;
	t_pun pc;

	//Guardamos el Contexto de Ejecucion Anterior;
	offset =  pcb->sizeContext*5 + pcb->cursorStack;
	memcpy(buffer,&pcb->cursorStack,sizeof(t_pun));
	Escribir(pcb->segmentoStack,offset,4,buffer);

	//Guardamos el Program Counter siguiente;
	offset =  pcb->sizeContext*5 + pcb->cursorStack + 4; //Son los 4 del Contexto Anterior
	pc = pcb->programcounter;
	pc++;
	memcpy(buffer,&pc,sizeof(t_pun));
	Escribir(pcb->segmentoStack,offset,4,buffer);


	//Guardamos a donde retornar;
	offset =  pcb->sizeContext*5 + pcb->cursorStack + 8;
	memcpy(buffer,&donde_retornar,sizeof(uint32_t));
	Escribir(pcb->segmentoStack,offset,4,buffer);

	//Cambio de Contexto
	pcb->cursorStack = offset + 4;

	//Limpio el Diccionario
	dictionary_clean(diccionarioVariables);
	GameOfThread_irAlLabel(etiqueta); //Me lleva al procedimiento que debo ejecutar;


	log_debug(logger,"Ejecutando Primitiva GameOfThread_llamarConRetorno");

}

void GameOfThread_finalizar(void){

package *paquete = malloc(sizeof(package));


	if(pcb->cursorStack == pcb->segmentoCodigo){
		dictionary_clean(diccionarioVariables);
		notificar_kernel(finPrograma);
		quantumPrograma = quantumKernel;
		}
	else{
		//Obtengo el Program Counter (Instruccion Siguiente)
		paquete = Leer(pcb->segmentoStack,pcb->cursorStack - 4, 4);
		memcpy(&pcb->programcounter,paquete->payload,sizeof(t_pun));
		destruir_paquete(paquete);

		//Obtengo el Cursor del Contexto Anterior
		paquete = Leer(pcb->segmentoStack,pcb->cursorStack - 8 , 4);
		memcpy(&pcb->cursorStack,paquete->payload,sizeof(t_pun));

		//Limpio el diccionario y lo cargo
		dictionary_clean(diccionarioVariables);
		int32_t cant_var = (pcb->cursorStack - 8)/5;
		cargar_diccionarioVariables(cant_var);
	}

	log_debug(logger,"Ejecutando Primitiva GameOfThread_finalizar");
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
	Escribir(base,dirRetorno,tamanio,buffer);
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
	int32_t cant_var = (offset_tmp - 12)/5;
	cargar_diccionarioVariables(cant_var);


	log_debug(logger,"Ejecutando Primitiva GameOfThread_retornar");
}

void GameOfThread_imprimir(t_valor_variable retorno){
	package* paquete = malloc(sizeof(package));
	char* payload = malloc(sizeof(t_valor_variable));
	memcpy(payload,&retorno,sizeof(int32_t));
	paquete = crear_paquete(imprimirValor,payload,sizeof(int32_t));
	enviar_paquete(paquete,socketKernel);
	destruir_paquete(paquete);


	log_debug(logger,"Ejecutando Primitiva GameOfThread_imprimir");
}


void GameOfThread_imprimirTexto(char* texto){
	package* paquete = malloc(sizeof(package));
	paquete = crear_paquete(imprimirTexto,texto,strlen(texto));
	enviar_paquete(paquete,socketKernel);
	destruir_paquete(paquete);


	log_debug(logger,"Ejecutando Primitiva GameOfThread_imprimirTexto");
}

void GameOfThread_entradaSalida(t_nombre_dispositivo dispositivo, int tiempo){
	t_iESdeCPU *es = malloc(sizeof(t_iESdeCPU));
	package *paquete;

	memcpy(es->id,dispositivo,strlen(dispositivo));
	es->tiempo = tiempo;
	es->tamanioID = strlen(dispositivo);

	char* payload = serializar_mensaje_ES(es);

	paquete = crear_paquete(entrada_salida,payload,sizeof(int32_t)*2 + strlen(es->id));
	enviar_paquete(paquete,socketKernel);
	destruir_paquete(paquete);
	quantumPrograma = quantumKernel;


	log_debug(logger,"Ejecutando Primitiva GameOfThread_entradaSalida");
}

void GameOfThread_wait(t_nombre_semaforo identificador_semaforo){
	package *paquete;
	char *payload = malloc(sizeof(t_nombre_semaforo));
	memcpy(payload,identificador_semaforo,sizeof(t_nombre_semaforo));
	paquete = crear_paquete(tomarSemaforo,payload,sizeof(t_nombre_semaforo));
	enviar_paquete(paquete,socketKernel);
	destruir_paquete(paquete);
	paquete = recibir_paquete(socketKernel);
	destruir_paquete(paquete);
	if (paquete->type == semaforolibre){
		log_debug(logger,"El semaforo %s esta libre",identificador_semaforo);
	} else if(paquete->type == bloquearProgramaCPU) {
		log_debug(logger,"El semaforo %s esta bloqueado",identificador_semaforo);
		quantumPrograma = quantumKernel;
	}


	log_debug(logger,"Ejecutando Primitiva GameOfThread_wait");
}

void GameOfThread_signal(t_nombre_semaforo identificador_semaforo){
	package *paquete;
	char *payload = malloc(sizeof(t_nombre_semaforo));
	memcpy(payload,identificador_semaforo,sizeof(t_nombre_semaforo));
	paquete = crear_paquete(liberarSemaforo,payload,sizeof(t_nombre_semaforo));
	enviar_paquete(paquete,socketKernel);
	destruir_paquete(paquete);
	log_debug(logger,"Hubo un signal del semaforo %s ",identificador_semaforo);


	log_debug(logger,"Ejecutando Primitiva GameOfThread_signal");
}
