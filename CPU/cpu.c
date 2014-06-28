/*
 * cpu.c
 *
 *  Created on: 03/05/2014
 *      Author: utnso
 */

#include "cpu.h"


int main(int argc, char **argv){

	signal(SIGUSR1, rutina);
	//Creo los logs
	logger = log_create("loggerCPU.log","CPU_LOG",false,LOG_LEVEL_DEBUG);

	//Creo el diccionario de variables
	diccionarioVariables = dictionary_create();

	//Defino los paquetes y reservo la memoria
	package* packagePCB = malloc(sizeof(package));
	package* paq = malloc(sizeof(package));
	package* respuesta = malloc(sizeof(package));

	//Defino variables locales
	int32_t programcounter, segmentoCodigo, indiceCodigo;
	char* solicitudLectura = malloc(sizeof(t_pun)*3);
	t_datoSentencia *datos = malloc(sizeof(t_datoSentencia));

	//Defino solicitud de lectura y reservo espacio
	t_solicitudLectura *sol = malloc(sizeof(t_solicitudLectura));

	//Defino las estructuras para la configuracion del kernel y la UMV
	t_config *configKernel = config_create((char*)argv[1]);
	t_config *configUMV = config_create((char*)argv[1]);

	t_confKernel kernel;
	t_ip umvIP;

	pcb = malloc(sizeof(t_PCB));

	//levanto configuracion del kernel
	kernel.ip = malloc((sizeof(char))*15);
	kernel.port = obtenerPuerto(configKernel);
	kernel.ip = obtenerIP(configKernel);
	kernel.tamanioStack = obtenerTamanioStack(configKernel);

	//levanto configuracion de la UMV
	umvIP.ip = malloc((sizeof(char))*15);
	umvIP.ip = obtenerIP(configUMV);
	umvIP.port = obtenerPuerto(configKernel);

	//me conecto al kernel
	socketKernel = abrir_socket();
	conectar_socket(socketKernel, kernel.ip, (int)kernel.port);
	handshake_kernel();


	//me conecto a la UMV
	socketUMV = abrir_socket();
	conectar_socket(socketUMV,umvIP.ip, (int)umvIP.port);
	handshake(handshakeCpuUmv);

	while(1){ //para recibir los PCB


			/* Recibo PCB */
			notificar_kernel(cpuDisponible);
			packagePCB = recibir_paquete(socketKernel);
			pcb = desserializarPCB(packagePCB->payload);
			log_debug(logger,"RECIBIDA UNA PCB. Su program id es: %d\n",pcb->id);
			destruir_paquete(packagePCB);
			/* Envio PCB.id A Pipe */
			char* payload = malloc(sizeof(t_pun));
			memcpy(payload,&pcb->id,sizeof(t_pun));
			paq = crear_paquete(pcbIDaUMV,payload,sizeof(t_pun));
			enviar_paquete(paq,socketUMV);
			free(payload);
			destruir_paquete(paq);


			cargar_diccionarioVariables(pcb->sizeContext);
			quantumPrograma = 0;

			while(quantumPrograma<quantumKernel){

				programcounter = pcb->programcounter;
				programcounter++;
				indiceCodigo = pcb->indiceCodigo;

				sol->base = pcb->indiceCodigo;
				sol->offset = programcounter*8;
				sol->tamanio = TAMANIO_INSTRUCCION;

				solicitudLectura = serializarSolicitudLectura(sol);
				paq = crear_paquete(lectura,solicitudLectura,sizeof(t_pun)*3);
				enviar_paquete(paq,socketUMV);
				destruir_paquete(paq);
				free(solicitudLectura);
				free(sol);
				paq = recibir_paquete(socketUMV);
				if(paq->type != respuestaUmv){
					//TODO: notificar_kernel();
					exit(1);
				}


				memcpy(&datos->inicio,paq->payload,sizeof(int32_t));
				memcpy(&datos->longitud,paq->payload + sizeof(int32_t),sizeof(int32_t));
				destruir_paquete(paq);

				sol->base = pcb->segmentoCodigo; // TODO: Verificar, estaba pcb->indiceCodigo
				sol->offset = datos->inicio;
				sol->tamanio = datos->longitud;
				solicitudLectura = serializarSolicitudLectura(sol);
				free(sol);

				paq = crear_paquete(lectura,solicitudLectura,sizeof(t_pun)*3);
				enviar_paquete(paq, socketUMV);
				destruir_paquete(paq);
				free(solicitudLectura);
				respuesta = recibir_paquete(socketUMV);
				if(respuesta->type != respuestaUmv){
					//TODO: notificar_kernel()
					exit(1);
				}

				// TODO: analizadorLinea();

				destruir_paquete(respuesta);

				quantumPrograma ++;
		}
			dictionary_clean(diccionarioVariables); //limpio el diccionario de variables

			//TODO: Enviar PCB completo o Enviar lo modificado???? Preguntar Silvina y Pablo

			if (desconectarse == true){
				notificar_kernel(cpuDesconectada);
				log_debug(logger,"CPU DESCONECTADA");
				exit(1);
					}
	}
	return 0;
}


//Defino el interrupt handler
void rutina(int n){
	switch(n){
	 case SIGUSR1:
		 desconectarse = true;
	}
}




void *cargar_diccionarioVariables(int32_t cant_var){
	t_solicitudLectura *sol = malloc(sizeof(t_solicitudLectura));
	package* solicitudLectura = malloc(sizeof(package));
	package* paq = malloc(sizeof(package));
	t_puntero puntero;


			dictionary_clean(diccionarioVariables);


			while(cant_var >0){

				sol->base = pcb->cursorStack;
				sol->offset = (cant_var - 1) * 5;
				sol->tamanio = TAMANIO_ID_VAR;

				char* payloadSerializado = serializarSolicitudLectura(sol);

				solicitudLectura = crear_paquete(lectura,payloadSerializado,sizeof(t_puntero)*3);
				enviar_paquete(solicitudLectura, socketUMV);
				destruir_paquete(solicitudLectura);
				free(payloadSerializado);
				paq = recibir_paquete(socketUMV);
				if(paq->type != respuestaUmv){
					//TODO: notificar_kernel()
					exit(1);
				}
				char* var = malloc(strlen(paq->payload)+1);
				memcpy(var,paq->payload,strlen(paq->payload)+1);

				puntero = sol->offset;

				dictionary_put(diccionarioVariables, var,(void*)puntero);
				destruir_paquete(paq);
				free(var);

				cant_var--;
			}
}


void notificar_kernel(t_paquete pa){
	package* paquete = malloc(sizeof(package));
		switch(pa){
			case cpuDisponible:
				paquete = crear_paquete(cpuDisponible,"ESTOY DISPONIBLE",strlen("ESTOY DISPONIBLE")+1);
				enviar_paquete(paquete,socketKernel);
				destuir_paquete(paquete);
				break;
			case cpuDesconectada:
				paquete =  crear_paquete(cpuDesconectada,"Me Desconecto",strlen("Me Desconecto")+1);
				enviar_paquete(paquete,socketKernel);
				destuir_paquete(paquete);
				break;
			case violacionSegmento:
				paquete = crear_paquete(violacionSegmento,"Violacion Segmento", strlen("Violacion Segmento")+1);
				enviar_paquete(paquete,socketKernel);
				destuir_paquete(paquete);
				break;
			case error_label:
				paquete = crear_paquete(error_label,"Error Label Instruccion",strlen("Error Label Instruccion")+1);
				enviar_paquete(paquete,socketKernel);
				destuir_paquete(paquete);
				break;
			case bloquearProgramaCPU:
				paquete = crear_paquete(bloquearProgramaCPU,"Bloquear programa", strlen("Bloquear programa")+1);
				enviar_paquete(paquete,socketKernel);
				destuir_paquete(paquete);
				break;

			default:
				break;
		}


}

void handshake(t_paquete pa){

	package* handshake = malloc(sizeof(package));
	package *quantum_package = malloc(sizeof(package));

	switch(pa){
		case handshakeKernelCPU:
					handshake = crear_paquete(handshakeKernelCPU,"SOY UNA CPU",strlen("SOY UNA CPU")+1);
					enviar_paquete(handshake,socketKernel);
					destruir_paquete(handshake);
					handshake =  recibir_paquete(socketKernel);
					quantum_package = recibir_paquete(socketKernel);
					memcpy(&quantumKernel,quantum_package->payload,sizeof(t_pun));
					destruir_paquete(handshake);
					handshake= crear_paquete(handshakeKernelCPU,"RECIBIDO OK",strlen("RECIBIDO OK")+1);
					enviar_paquete(handshake,socketKernel);
					destruir_paquete(handshake);
					destruir_paquete(quantum_package);
					log_debug(logger,"CONECTADO AL KERNEL");
					log_debug(logger,"El Quantum es:%d",quantumKernel);
					break;


		case handshakeCpuUmv:
			handshake = crear_paquete(handshakeCpuUmv,"HOLA UMV",strlen("HOLA UMV")+1);
			enviar_paquete(handshake,socketUMV);
			destruir_paquete(handshake);
			handshake =  recibir_paquete(socketUMV);
			if(handshake->type != handshakeCpuUmv){
				notificar_kernel(cpuDesconectada);
				log_debug(logger,"CPU DESCONECTADA ,PROBLEMA CON LA UMV");
				exit(1);
				destruir_paquete(handshake);
						}
				break;


		default: break;
				}


}

package *Leer(t_pun base,t_pun offset,t_pun tamanio){
	t_solicitudLectura *sol = malloc(sizeof(t_solicitudLectura));
	int32_t err;
	package *solicitud = malloc(sizeof(package));
	char* payload = malloc(sizeof(t_pun)*3);


	sol->base = base;
	sol->offset = offset;
	sol->tamanio = tamanio;

	payload = serializarSolicitudLectura(sol);
	solicitud = crear_paquete(lectura,payload,sizeof(t_pun)*3);
	enviar_paquete(solicitud,socketUMV);
	destruir_paquete(solicitud);
	free(payload);

	solicitud = recibir_paquete(socketUMV);
	memcpy(&err,solicitud->payload,sizeof(int32_t));
	if(err == -1){
		notificar_kernel(violacionSegmento);
		exit(1);
	}

	free(payload);
	return solicitud;
	destruir_paquete(solicitud);
}

package *Escribir(t_pun base, t_pun offset, t_pun tamanio, char* buffer){
	t_solicitudEscritura *sol = malloc(sizeof(t_solicitudEscritura));
	package *paquete = malloc(sizeof(package));
	int32_t err;


	sol->base = base;
	sol->offset = offset;
	sol->tamanio = tamanio;
	sol->buffer = malloc(strlen(buffer)+1);
	memcpy(sol->buffer, buffer,strlen(buffer)+1);

	char* payload = serializarSolicitudEscritura(sol);

	paquete = crear_paquete(escritura,payload,sizeof(t_pun)*3 + strlen(sol->buffer)+1);
	enviar_paquete(paquete,socketUMV);
	destruir_paquete(paquete);
	paquete = recibir_paquete(socketUMV);
	memcpy(&err,paquete->payload,sizeof(int32_t));
	if(err == -1)
		notificar_kernel(violacionSegmento);
		exit(1);

	free(payload);
	return paquete;
}
