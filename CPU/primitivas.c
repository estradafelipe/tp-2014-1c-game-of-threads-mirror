/*
 * primitivas.c
 *
 *  Created on: 27/06/2014
 *      Author: utnso
 */

#include "primitivas.h"
#include <serializadores.h>

t_puntero GameOfThread_definirVariable(t_nombre_variable identificador_variable){
	log_trace(logger,"Ejecutando Primitiva GameOfThread_definirVariable");
	log_debug(logger, "Variable a definir: %c",identificador_variable);
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
	log_debug(logger, "Posicion: %d, sizeContext: %d",puntero,pcb->sizeContext);

	return puntero;
}


t_puntero GameOfThread_obtenerPosicionVariable(t_nombre_variable identificador_variable ){
	log_trace(logger,"Ejecutando Primitiva GameOfThread_obtenerPosicionVariable");
	char* key = malloc(sizeof(t_nombre_variable)+1);
	sprintf(key,"%c",identificador_variable);
	t_puntero posicion = (t_puntero)dictionary_get(diccionarioVariables, key);
	log_debug(logger, "Variable buscada: %c, posicion encontrada: %d",identificador_variable,posicion);
	return posicion;
}

t_valor_variable GameOfThread_dereferenciar(t_puntero direccion_variable){
	log_trace(logger,"Ejecutando Primitiva GameOfThread_dereferenciar");
	t_valor_variable valorVariable;
	package* paquete = malloc(sizeof(package));
	

	paquete = Leer(pcb->segmentoStack,direccion_variable+1,4);

	memcpy(&valorVariable,paquete->payload,sizeof(t_valor_variable));
	log_debug(logger, "Posicion buscada: %d, valor encontrado: %d",direccion_variable,valorVariable);
	return valorVariable;
}

void GameOfThread_asignar(t_puntero direccion_variable, t_valor_variable valor){
	log_trace(logger,"Ejecutando Primitiva GameOfThread_asignar");
	t_pun offset;
	char* buffer = malloc(sizeof(int32_t));

	offset = direccion_variable + 1;

	memcpy(buffer,&valor,sizeof(t_valor_variable));

	Escribir(pcb->segmentoStack,offset,4,buffer);
	log_debug(logger, "Posicion a escribir: %d, valor: %d",direccion_variable + 1,valor);
	}

t_valor_variable GameOfThread_obtenerValorCompartida(t_nombre_compartida variable){
	log_trace(logger,"Ejecutando Primitiva GameOfThread_obtenerValorCompartida");
	package *paquete = malloc(sizeof(package));
	t_valor_variable val;

	paquete = crear_paquete(solicitarValorVariableCompartida,variable,strlen(variable)+1);
	enviar_paquete(paquete,socketKernel);
	destruir_paquete(paquete);
	paquete = recibir_paquete(socketKernel);
	if(paquete->type != solicitarValorVariableCompartida){
		notificarError_kernel("ERROR AL SOLICITAR EL VALOR DE LA VARIABLE COMPARTIDA");
	}
	memcpy(&val,paquete->payload,sizeof(t_valor_variable));
	log_debug(logger, "Variable compartida: %s, valor obtenido: %d",variable,val);

	return val;
}

t_valor_variable GameOfThread_asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor){
	log_trace(logger,"Ejecutando Primitiva GameOfThread_asignarValorCompartida");
	package *solicitud = malloc(sizeof(package));
	t_iVARCOM *asig = malloc(sizeof(t_iVARCOM));

	asig->valor = valor;
	asig->nombre = variable;

	log_debug(logger, "Variable compartida: %s, valor a asignar: %d",variable,valor);

	char* payload = serializar_datos_variable(asig,strlen(asig->nombre)+1 + sizeof(int32_t));
	solicitud =  crear_paquete(asignarValorVariableCompartida,payload,sizeof(t_nombre_compartida)+sizeof(t_valor_variable)+sizeof(int32_t));
	enviar_paquete(solicitud,socketKernel);

	return valor;
}
void GameOfThread_irAlLabel(t_nombre_etiqueta etiqueta){
	log_trace(logger,"Ejecutando Primitiva GameOfThread_irAlLabel");
	package* paq = malloc(sizeof(package));
	t_puntero_instruccion instruccion;
	log_debug(logger, "Pidiendo indiceEtiquetas a la UMV");
	paq=Leer(pcb->indiceEtiquetas,0,pcb->sizeIndexLabel);
		
	log_debug(logger, "La etiqueta buscada es: %s",etiqueta);
	instruccion = metadata_buscar_etiqueta(etiqueta, paq->payload, pcb->sizeIndexLabel);
	if (instruccion == -1){
		notificarError_kernel("Error al encontrar label");
	}
	log_debug(logger, "Label encontrada, posicion: %d",instruccion);

	pcb->programcounter = instruccion;

}

void GameOfThread_llamarSinRetorno(t_nombre_etiqueta etiqueta){
	log_trace(logger,"Ejecutando Primitiva GameOfThread_llamarSinRetorno");
	t_pun pc;
	t_pun offset;
	char* buffer = malloc(sizeof(int32_t));

	//Guardamos el Contexto de Ejecucion Anterior;
	log_debug(logger,"Guardando contexto de ejecución actual");
	offset =  pcb->sizeContext*5 + pcb->cursorStack;
	memcpy(buffer,&pcb->cursorStack,sizeof(t_pun));
	Escribir(pcb->segmentoStack,offset,4,buffer);

	//Guardamos el Program Counter siguiente;
	log_debug(logger,"Guardando programCounter siguiente");
	offset =  pcb->sizeContext*5 + pcb->cursorStack + 4; //Son los 4 del Contexto Anterior
	pc = pcb->programcounter;
	pc++;
	memcpy(buffer,&pc,sizeof(t_pun));
	Escribir(pcb->segmentoStack,offset,4,buffer);

	//Cambio de Contexto
	log_debug(logger,"Cambiando contexto de ejecución");
	pcb->cursorStack = offset + 4;
	dictionary_clean(diccionarioVariables);
	GameOfThread_irAlLabel(etiqueta); //Me lleva al procedimiento que debo ejecutar;

}

void GameOfThread_llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar){
	log_trace(logger,"Ejecutando Primitiva GameOfThread_llamarConRetorno");
	char* buffer = malloc(sizeof(t_pun));
	t_pun offset;
	t_pun pc;

	//Guardamos el Contexto de Ejecucion Anterior;
	log_debug(logger,"Guardando contexto de ejecución actual");
	offset =  pcb->sizeContext*5 + pcb->cursorStack;
	memcpy(buffer,&pcb->cursorStack,sizeof(t_pun));
	Escribir(pcb->segmentoStack,offset,4,buffer);

	//Guardamos el Program Counter siguiente;
	log_debug(logger,"Guardando programCounter siguiente");
	offset =  pcb->sizeContext*5 + pcb->cursorStack + 4; //Son los 4 del Contexto Anterior
	pc = pcb->programcounter;
	pc++;
	memcpy(buffer,&pc,sizeof(t_pun));
	Escribir(pcb->segmentoStack,offset,4,buffer);


	//Guardamos a donde retornar;
	log_debug(logger,"Guardando direccion de retorno");
	offset =  pcb->sizeContext*5 + pcb->cursorStack + 8;
	memcpy(buffer,&donde_retornar,sizeof(uint32_t));
	Escribir(pcb->segmentoStack,offset,4,buffer);

	//Cambio de Contexto
	log_debug(logger,"Cambiando contexto de ejecución");
	pcb->cursorStack = offset + 4;

	//Limpio el Diccionario
	log_debug(logger,"Limpiando diccionario");
	dictionary_clean(diccionarioVariables);
	GameOfThread_irAlLabel(etiqueta); //Me lleva al procedimiento que debo ejecutar;

}

void GameOfThread_finalizar(void){
	log_trace(logger,"Ejecutando Primitiva GameOfThread_finalizar");
	package *paquete = malloc(sizeof(package));

	if(pcb->cursorStack == pcb->segmentoStack){
		log_debug(logger,"No hay mas instrucciones a ejecutar, finalizando programa");
		dictionary_clean(diccionarioVariables);
		notificar_kernel(finPrograma);
		quantumPrograma = quantumKernel;
	} else {
		log_debug(logger,"Hay instrucciones para seguir ejecutando");
		//Obtengo el Program Counter (Instruccion Siguiente)
		paquete = Leer(pcb->segmentoStack,pcb->cursorStack - 4, 4);
		memcpy(&pcb->programcounter,paquete->payload,sizeof(t_pun));
		destruir_paquete(paquete);
		log_debug(logger,"ProgramCounter a ejecutar: %d", pcb->programcounter);

		//Obtengo el Cursor del Contexto Anterior
		paquete = Leer(pcb->segmentoStack,pcb->cursorStack - 8 , 4);
		memcpy(&pcb->cursorStack,paquete->payload,sizeof(t_pun));
		log_debug(logger,"CursorStack del contexto anterior: %d", pcb->cursorStack);
		//Limpio el diccionario y lo cargo
		log_debug(logger, "Limpiando diccionario de variables");
		dictionary_clean(diccionarioVariables);
		int32_t cant_var = (pcb->cursorStack - 8)/5;
		log_debug(logger, "Cargando diccionario de variables");
		cargar_diccionarioVariables(cant_var);
	}

}

void GameOfThread_retornar(t_valor_variable retorno){
	log_trace(logger,"Ejecutando Primitiva GameOfThread_retornar");
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
	log_debug(logger, "Direccion de retorno: %d", dirRetorno);
	log_debug(logger, "Valor de retorno: %d", retorno);
	//Escribo en dirRetorno el valor de retorno t_valor_variable
	char* buffer = malloc(sizeof(t_valor_variable));
	memcpy(buffer,&retorno,sizeof(t_valor_variable));
	Escribir(base,dirRetorno,tamanio,buffer);
	destruir_paquete(paquete);
	//Obtengo la proxima instruccion (Program Counter)
	paquete = Leer(base,offset_tmp - 8, tamanio);
	memcpy(&pcb->programcounter,paquete->payload,sizeof(t_pun));
	log_debug(logger, "Proxima instruccion a ejecutar: %d",pcb->programcounter);
	destruir_paquete(paquete);
	//Cambio de Contexto
	paquete = Leer(base,offset_tmp - 12, tamanio);
	memcpy(&pcb->cursorStack,paquete->payload,sizeof(t_pun));
	log_debug(logger, "CursorStack: %d",pcb->cursorStack);
	//Limpio el diccionario y lo cargo
	log_debug(logger, "Limpiando diccionario de variables");
	dictionary_clean(diccionarioVariables);
	int32_t cant_var = (offset_tmp - 12)/5;
	log_debug(logger, "Cargando diccionario de variables");
	cargar_diccionarioVariables(cant_var);

}

void GameOfThread_imprimir(t_valor_variable retorno){
	log_trace(logger,"Ejecutando Primitiva GameOfThread_imprimir");
	log_debug(logger, "Valor a imprimir: %d", retorno);
	package* paquete = malloc(sizeof(package));
	char* payload = malloc(sizeof(t_valor_variable));
	memcpy(payload,&retorno,sizeof(int32_t));
	paquete = crear_paquete(imprimirValor,payload,sizeof(int32_t));
	enviar_paquete(paquete,socketKernel);
	destruir_paquete(paquete);

}


void GameOfThread_imprimirTexto(char* texto){
	log_trace(logger,"Ejecutando Primitiva GameOfThread_imprimirTexto");
	log_debug(logger,"Texto a imprimir: %s",texto);
	package* paquete = malloc(sizeof(package));
	paquete = crear_paquete(imprimirTexto,texto,strlen(texto));
	enviar_paquete(paquete,socketKernel);
	destruir_paquete(paquete);

}

void GameOfThread_entradaSalida(t_nombre_dispositivo dispositivo, int tiempo){
	log_trace(logger,"Ejecutando Primitiva GameOfThread_entradaSalida");
	log_debug(logger,"Dispositivo: %s, tiempo: %d",dispositivo,tiempo);
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

}

void GameOfThread_wait(t_nombre_semaforo identificador_semaforo){
	log_trace(logger,"Ejecutando Primitiva GameOfThread_wait");

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

}

void GameOfThread_signal(t_nombre_semaforo identificador_semaforo){
	log_trace(logger,"Ejecutando Primitiva GameOfThread_signal");

	package *paquete;
	char *payload = malloc(sizeof(t_nombre_semaforo));
	memcpy(payload,identificador_semaforo,sizeof(t_nombre_semaforo));
	paquete = crear_paquete(liberarSemaforo,payload,sizeof(t_nombre_semaforo));
	enviar_paquete(paquete,socketKernel);
	destruir_paquete(paquete);
	log_debug(logger,"Hubo un signal del semaforo %s ",identificador_semaforo);

}
