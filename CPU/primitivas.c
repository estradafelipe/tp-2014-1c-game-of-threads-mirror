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
	log_debug(logger,"sizeContext: %d, cursorStack: %d",pcb->sizeContext,pcb->cursorStack);
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
	log_debug(logger, "Variable buscada: %s, posicion encontrada: %d",key,posicion);
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
	if(finprograma==false){
	log_trace(logger,"Ejecutando Primitiva GameOfThread_asignar");
	t_pun offset;
	char* buffer = malloc(sizeof(int32_t));

	offset = direccion_variable + 1;

	memcpy(buffer,&valor,sizeof(t_valor_variable));

	Escribir(pcb->segmentoStack,offset,4,buffer);
	log_debug(logger, "Posicion a escribir: %d, valor: %d",direccion_variable + 1,valor);
	}
}
t_valor_variable GameOfThread_obtenerValorCompartida(t_nombre_compartida variable){
	log_trace(logger,"Ejecutando Primitiva GameOfThread_obtenerValorCompartida");
	package *paquete;
	t_valor_variable val;

	char *payload = malloc(strlen(variable));
	memcpy(payload,variable,strlen(variable));
	char *varcomp=strdup(variable);
	string_trim(&varcomp);


	printf("variable compartida: %s\n",varcomp);
	paquete = crear_paquete(solicitarValorVariableCompartida,varcomp,strlen(varcomp)+1);
	enviar_paquete(paquete,socketKernel);
	destruir_paquete(paquete);
	paquete = recibir_paquete(socketKernel);
	if(paquete->type != solicitarValorVariableCompartida){
		notificarError_kernel("ERROR AL SOLICITAR EL VALOR DE LA VARIABLE COMPARTIDA");
	}
	memcpy(&val,paquete->payload,sizeof(t_valor_variable));
	log_debug(logger, "Variable compartida: %s, valor obtenido: %d",variable,val);

	return val;

	//

}

t_valor_variable GameOfThread_asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor){
	log_trace(logger,"Ejecutando Primitiva GameOfThread_asignarValorCompartida");
	package *solicitud;
	t_iVARCOM *asig = malloc(sizeof(t_iVARCOM));
	asig->nombre = malloc(strlen(variable)+1);
	asig->valor = valor;
	memcpy(asig->nombre,variable,strlen(variable)+1);
	//asig->nombre = variable;
	printf("variable compartida tamanio %d",sizeof(variable));
	log_debug(logger, "Variable compartida: %s, valor a asignar: %d",asig->nombre,asig->valor);
	int32_t size = strlen(asig->nombre)+1;
	char* payload = serializar_datos_variable(asig,size);
	solicitud =  crear_paquete(asignarValorVariableCompartida,payload,size+sizeof(int32_t));
	enviar_paquete(solicitud,socketKernel);
	destruir_paquete(solicitud);
	solicitud = recibir_paquete(socketKernel);
	if(solicitud->type != asignarValorVariableCompartida){
		printf("Fallo asignar variable compartida!!!");
		log_debug(logger,"Fallo asignar variable compartida!!!");
	}
	return valor;
}
void GameOfThread_irAlLabel(t_nombre_etiqueta etiqueta){
	log_trace(logger,"Ejecutando Primitiva GameOfThread_irAlLabel");
	char* etiq = strndup(etiqueta,strlen(etiqueta));
	package* paq;
	t_puntero_instruccion instruccion;
	log_debug(logger, "Pidiendo indiceEtiquetas a la UMV");
	paq=Leer(pcb->indiceEtiquetas,0,pcb->sizeIndexLabel);
		
	log_debug(logger, "La etiqueta buscada es: %s",quitarSaltoLinea(etiq));
	instruccion = metadata_buscar_etiqueta(quitarSaltoLinea(etiq), paq->payload, pcb->sizeIndexLabel);
	if (instruccion == -1){
		notificarError_kernel("Error al encontrar label");
	}
	log_debug(logger, "Label encontrada, posicion: %d",instruccion);

	pcb->programcounter = instruccion - 1;
	log_debug(logger,"PC: %d",pcb->programcounter);

}

void GameOfThread_llamarSinRetorno(t_nombre_etiqueta etiqueta){
	log_trace(logger,"Ejecutando Primitiva GameOfThread_llamarSinRetorno");
	t_pun pc;
	t_pun offset;
	char* buffer = malloc(sizeof(int32_t));

	//Guardamos el Contexto de Ejecucion Anterior;
	log_debug(logger,"Guardando contexto de ejecución actual");
	log_debug(logger, "CursorStack: %d",pcb->cursorStack);
	log_debug(logger, "sizeContext: %d",pcb->sizeContext);
	offset =  pcb->sizeContext*5 + pcb->cursorStack;
	memcpy(buffer,&pcb->cursorStack,sizeof(t_pun));
	Escribir(pcb->segmentoStack,offset,4,buffer);
	if (finprograma==false){
		//Guardamos el Program Counter siguiente;
		log_debug(logger,"Guardando programCounter siguiente");
		offset =  pcb->sizeContext*5 + pcb->cursorStack + 4; //Son los 4 del Contexto Anterior
		pc = pcb->programcounter;
		pc++;
		log_debug(logger, "ProgramCounter siguiente: %d",pc);
		memcpy(buffer,&pc,sizeof(t_pun));
		Escribir(pcb->segmentoStack,offset,4,buffer);
		if (finprograma==false){
			//Cambio de Contexto
			log_debug(logger,"Cambiando contexto de ejecución");
			pcb->cursorStack = offset + 4;
			log_debug(logger, "CursorStack nuevo: %d",pcb->cursorStack);
			pcb->sizeContext = 0;
			log_debug(logger, "sizeContext nuevo: %d",pcb->sizeContext);
			dictionary_clean(diccionarioVariables);
			log_debug(logger,"Diccionario vacio: %d\n",dictionary_is_empty(diccionarioVariables));
			GameOfThread_irAlLabel(etiqueta); //Me lleva al procedimiento que debo ejecutar;
		}
	}
}

void GameOfThread_llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar){
	log_trace(logger,"Ejecutando Primitiva GameOfThread_llamarConRetorno");
	char* buffer = malloc(sizeof(t_pun));
	t_pun offset;
	t_pun pc;

	//Guardamos el Contexto de Ejecucion Anterior;
	log_debug(logger,"Guardando contexto de ejecución actual");
	log_debug(logger, "CursorStack: %d",pcb->cursorStack);
	log_debug(logger, "sizeContext: %d",pcb->sizeContext);
	offset =  pcb->sizeContext*5 + pcb->cursorStack;
	memcpy(buffer,&pcb->cursorStack,sizeof(t_pun));
	Escribir(pcb->segmentoStack,offset,4,buffer);
	if (finprograma==false){
		//Guardamos el Program Counter siguiente;
		log_debug(logger,"Guardando programCounter siguiente");
		offset =  pcb->sizeContext*5 + pcb->cursorStack + 4; //Son los 4 del Contexto Anterior
		pc = pcb->programcounter;
		pc++;
		log_debug(logger, "ProgramCounter siguiente: %d",pc);
		memcpy(buffer,&pc,sizeof(t_pun));
		Escribir(pcb->segmentoStack,offset,4,buffer);
		if (finprograma==false){

			//Guardamos a donde retornar;
			log_debug(logger,"Guardando direccion de retorno");
			log_debug(logger, "Dir retorno: %d",donde_retornar);
			offset =  pcb->sizeContext*5 + pcb->cursorStack + 8;
			memcpy(buffer,&donde_retornar,sizeof(uint32_t));
			Escribir(pcb->segmentoStack,offset,4,buffer);
			if (finprograma==false){
				//Cambio de Contexto
				log_debug(logger,"Cambiando contexto de ejecución");
				pcb->cursorStack = offset + 4;
				log_debug(logger, "CursorStack: %d",pcb->cursorStack);
				pcb->sizeContext = 0;
				log_debug(logger, "sizeContext nuevo: %d",pcb->sizeContext);

				//Limpio el Diccionario
				log_debug(logger,"Limpiando diccionario");
				dictionary_clean(diccionarioVariables);
				GameOfThread_irAlLabel(etiqueta); //Me lleva al procedimiento que debo ejecutar;
			}
		}
	}
}

void GameOfThread_finalizar(void){
	log_trace(logger,"Ejecutando Primitiva GameOfThread_finalizar");
	if (finprograma==false){
		package *paquete = malloc(sizeof(package));
		log_debug(logger, "CursorStack: %d",pcb->cursorStack);
		if(pcb->cursorStack == 0){
			log_debug(logger,"No hay mas instrucciones a ejecutar, finalizando programa");
			dictionary_clean(diccionarioVariables);
			notificar_kernel(retornoCPUFin);
			quantumPrograma = quantumKernel;
			paquete=recibir_paquete(socketKernel);
			destruir_paquete(paquete);
			finprograma = true;
		} else {
			log_debug(logger,"Hay instrucciones para seguir ejecutando");
			//Obtengo el Program Counter (Instruccion Siguiente)
			paquete = Leer(pcb->segmentoStack,pcb->cursorStack - 4, 4);
			memcpy(&pcb->programcounter,paquete->payload,sizeof(t_pun));
			destruir_paquete(paquete);
			log_debug(logger,"ProgramCounter a ejecutar: %d", pcb->programcounter);
			pcb->programcounter--;
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
	Escribir(base,dirRetorno+1,tamanio,buffer);

	if (finprograma==false){
		//Obtengo la proxima instruccion (Program Counter)
		paquete = Leer(base,offset_tmp - 8, tamanio);
		if (finprograma==false){
			memcpy(&pcb->programcounter,paquete->payload,sizeof(t_pun));
			log_debug(logger, "Proxima instruccion a ejecutar: %d",pcb->programcounter);
			pcb->programcounter--;
			destruir_paquete(paquete);
			//Cambio de Contexto
			paquete = Leer(base,offset_tmp - 12, tamanio);
			if (finprograma==false){
				memcpy(&pcb->cursorStack,paquete->payload,sizeof(t_pun));
				log_debug(logger, "CursorStack: %d",pcb->cursorStack);
				//Limpio el diccionario y lo cargo
				log_debug(logger, "Limpiando diccionario de variables");
				dictionary_clean(diccionarioVariables);
				int32_t cant_var = (offset_tmp - 12)/5;
				log_debug(logger, "Cargando diccionario de variables");
				pcb->sizeContext = cant_var;
				cargar_diccionarioVariables(cant_var);
				destruir_paquete(paquete);
			}
		}
	}
}

void GameOfThread_imprimir(t_valor_variable retorno){
	if (finprograma==false){
		log_trace(logger,"Ejecutando Primitiva GameOfThread_imprimir");
		log_debug(logger, "Valor a imprimir: %d", retorno);
		package* paquete = malloc(sizeof(package));
		char* payload = malloc(sizeof(t_valor_variable));
		memcpy(payload,&retorno,sizeof(int32_t));
		paquete = crear_paquete(imprimirValor,payload,sizeof(int32_t));
		enviar_paquete(paquete,socketKernel);
		destruir_paquete(paquete);
		paquete = recibir_paquete(socketKernel);
		destruir_paquete(paquete);
	}
}


void GameOfThread_imprimirTexto(char* texto){
	log_trace(logger,"Ejecutando Primitiva GameOfThread_imprimirTexto");
	if (finprograma==false){
		log_debug(logger,"Texto a imprimir: %s",texto);
		package* paquete = malloc(sizeof(package));
		paquete = crear_paquete(imprimirTexto,texto,strlen(texto)+1);
		enviar_paquete(paquete,socketKernel);
		destruir_paquete(paquete);
		paquete = recibir_paquete(socketKernel);
		destruir_paquete(paquete);
	}
}

void GameOfThread_entradaSalida(t_nombre_dispositivo dispositivo, int tiempo){

	log_trace(logger,"Ejecutando Primitiva GameOfThread_entradaSalida");
	if (finprograma==false){
		log_debug(logger,"Dispositivo: %s, tiempo: %d",dispositivo,tiempo);
		t_iESdeCPU *es = malloc(sizeof(t_iESdeCPU));
		package *paquete;

		char *nombre = malloc(strlen(dispositivo)+1);
		memcpy(nombre,dispositivo,strlen(dispositivo)+1);
		string_trim(&nombre);

		quantumPrograma = quantumKernel;
		pcb->programcounter++;

		es->id = nombre;
		//memcpy(es->id,dispositivo,strlen(dispositivo));
		es->tiempo = tiempo;
		es->tamanioID = strlen(dispositivo)+1;

		int tamanioDatosES = strlen(dispositivo)+1 + sizeof(int32_t)*2;
		char* IOSerializada = serializar_mensaje_ES(es);

		char* PCBSerializado = serializar_datos_pcb_para_cpu(pcb);
		int size = (sizeof(int32_t)*2) + (sizeof(t_pun)*5) + strlen(dispositivo)+1;

		char* payload = malloc(size);
		memcpy(payload,IOSerializada,tamanioDatosES);
		memcpy(payload + tamanioDatosES,PCBSerializado,sizeof(t_pun)*5);

		paquete = crear_paquete(retornoCPUPorES,payload,size);
		enviar_paquete(paquete,socketKernel);

		paquete=recibir_paquete(socketKernel);
		printf("recibi respuesta\n");
		destruir_paquete(paquete);
		finprograma = true;
	}
}

void GameOfThread_wait(t_nombre_semaforo identificador_semaforo){
	log_trace(logger,"Ejecutando Primitiva GameOfThread_wait");
	package *paquete;

	char *payload = malloc(strlen(identificador_semaforo));
	memcpy(payload,identificador_semaforo,strlen(identificador_semaforo));
	char *semaforo=strdup(identificador_semaforo);
	string_trim(&semaforo);

	paquete = crear_paquete(tomarSemaforo,semaforo,strlen(semaforo)+1);
	log_debug(logger,"Identificador: %s, semaforo: %s, paquete: %s",identificador_semaforo,semaforo,paquete->payload);
	enviar_paquete(paquete,socketKernel);
	destruir_paquete(paquete);
	paquete = recibir_paquete(socketKernel);

	if (paquete->type == semaforolibre){
		log_debug(logger,"El semaforo %s esta libre",identificador_semaforo);
	} else if(paquete->type == bloquearProgramaCPU) {
		log_debug(logger,"El semaforo %s esta bloqueado",identificador_semaforo);
		pcb->programcounter++;
		quantumPrograma = quantumKernel;
		printf("mando datosPCB id %d, indice %d, pc %d, sizecontext %d, cursor %d\n",pcb->id, pcb->indiceEtiquetas, pcb->programcounter, pcb->sizeContext, pcb->cursorStack);
		char *payload = serializar_datos_pcb_para_cpu(pcb);
		destruir_paquete(paquete);
		paquete = crear_paquete(retornoCPUBloqueado,payload,sizeof(t_pun)*5);
		enviar_paquete(paquete,socketKernel);
		destruir_paquete(paquete);
		paquete=recibir_paquete(socketKernel);
		finprograma = true;
	}
	destruir_paquete(paquete);
}

void GameOfThread_signal(t_nombre_semaforo identificador_semaforo){
	log_trace(logger,"Ejecutando Primitiva GameOfThread_signal");

	package *paquete;


	char *payload = malloc(strlen(identificador_semaforo));
	memcpy(payload,identificador_semaforo,strlen(identificador_semaforo));
	char *semaforo=strdup(identificador_semaforo);
	string_trim(&semaforo);

	paquete = crear_paquete(liberarSemaforo,semaforo,strlen(semaforo)+1);
	enviar_paquete(paquete,socketKernel);
	destruir_paquete(paquete);
	paquete = recibir_paquete(socketKernel);
	destruir_paquete(paquete);
	log_debug(logger,"Hubo un signal del semaforo %s ",identificador_semaforo);

}
