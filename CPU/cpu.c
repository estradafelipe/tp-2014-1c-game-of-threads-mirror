/*
 * cpu.c
 *
 *  Created on: 03/05/2014
 *      Author: utnso
 */

#include "cpu.h"


int main(int argc, char **argv){
	desconectarse = false;
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
	int32_t programcounter;
	char* solicitudEscritura = malloc(sizeof(t_pun)*3);
	t_datoSentencia *datos = malloc(sizeof(t_datoSentencia));

	//Defino solicitud de lectura y reservo espacio
	t_solicitudLectura *sol = malloc(sizeof(t_solicitudLectura));

	//Defino las estructuras para la configuracion del kernel y la UMV
	t_config *configKernel = config_create((char*)argv[1]);
	t_config *configUMV = config_create((char*)argv[2]);

	printf("Configuracion Kernel en : %s\n",configKernel->path);
	printf("Configuracion Umv en : %s\n",configUMV->path);

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
	umvIP.port = obtenerPuerto(configUMV);

	printf("puerto kernel: %d\n",kernel.port);
	printf("puerto umv: %d\n",umvIP.port);

	//me conecto al kernel
	socketKernel = abrir_socket();
	conectar_socket(socketKernel, kernel.ip, (int)kernel.port);
	printf("socket kernel: %d\n",socketKernel);
	handshake(handshakeKernelCPU);


	//me conecto a la UMV
	socketUMV = abrir_socket();
	conectar_socket(socketUMV,umvIP.ip, (int)umvIP.port);
	handshake(handshakeCpuUmv);

	while(1){ //para recibir los PCB

			notificar_kernel(estoyDisponible);
			packagePCB = recibir_paquete(socketKernel);
			pcb = desserializarPCB(packagePCB->payload);
			log_debug(logger,"RECIBIDA UNA PCB. Su program id es: %d\n",pcb->id);
			cargar_diccionarioVariables(pcb->sizeContext);
			quantumPrograma = 0;

			while(quantumPrograma<quantumKernel){

				programcounter = pcb->programcounter;
				programcounter++;

				sol->base = pcb->indiceCodigo;
				sol->offset = programcounter*8;
				sol->tamanio = TAMANIO_INSTRUCCION;

				solicitudEscritura = serializarSolicitudLectura(sol);

				paq = crear_paquete(lectura,solicitudEscritura,sizeof(t_pun)*3);
				enviar_paquete(paq,socketUMV);

				paq = recibir_paquete(socketUMV);

				memcpy(&datos->inicio,paq->payload,sizeof(int32_t));
				memcpy(&datos->longitud,paq->payload + sizeof(int32_t),sizeof(int32_t));

				sol->base = pcb->segmentoCodigo; // TODO: Verificar, estaba pcb->indiceCodigo
				sol->offset = datos->inicio;
				sol->tamanio = datos->longitud;

				solicitudEscritura = serializarSolicitudLectura(sol);
				paq = crear_paquete(lectura,solicitudEscritura,sizeof(t_pun)*3);
				enviar_paquete(paq, socketUMV);
				respuesta = recibir_paquete(socketUMV);
				if(respuesta->type != respuestaUmv){
					//TODO: notificar_kernel()
					exit(1);
				}

				// TODO:Ejecutar parser
				quantumPrograma ++;
		}
			dictionary_clean(diccionarioVariables); //limpio el diccionario de variables

			//TODO: Enviar PCB

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




void cargar_diccionarioVariables(int32_t cant_var){
	package* paq = malloc(sizeof(package));
	int32_t offset;
	char* var = malloc(sizeof(char));

			while(cant_var > 0){

				offset = (cant_var - 1) * 5;

				paq = Leer(pcb->cursorStack,offset,1);
				memcpy(var,paq->payload,sizeof(char));
				dictionary_put(diccionarioVariables, var,(void*)offset);
				cant_var--;
			}

}


void notificar_kernel(t_paquete pa){
	package* paquete = malloc(sizeof(package));
		switch(pa){
			case estoyDisponible:
				paquete = crear_paquete(estoyDisponible,"ESTOY DISPONIBLE",strlen("ESTOY DISPONIBLE")+1);
				enviar_paquete(paquete,socketKernel);
				break;
			case cpuDesconectada:
				paquete =  crear_paquete(cpuDesconectada,"Me Desconecto",strlen("Me Desconecto")+1);
				enviar_paquete(paquete,socketKernel);
				break;
			case violacionSegmento:
				paquete = crear_paquete(violacionSegmento,"Violacion Segmento", strlen("Violacion Segmento")+1);
				enviar_paquete(paquete,socketKernel);
				break;
			case error_label:
				paquete = crear_paquete(error_label,"Error Label Instruccion",strlen("Error Label Instruccion")+1);
				enviar_paquete(paquete,socketKernel);
				break;
			case bloquearProgramaCPU:
				paquete = crear_paquete(bloquearProgramaCPU,"Bloquear programa", strlen("Bloquear programa")+1);
				enviar_paquete(paquete,socketKernel);
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
					//handshake =  recibir_paquete(socketKernel);
					//printf("recibi %s\n", handshake->payload);
					quantum_package = recibir_paquete(socketKernel);
					printf("recibi %s\n", quantum_package->payload);
					memcpy(&quantumKernel,quantum_package->payload,sizeof(t_pun));
					//handshake= crear_paquete(handshakeKernelCPU,"RECIBIDO OK",strlen("RECIBIDO OK")+1);
					handshake= crear_paquete(recibiACKDeCPU,"RECIBIDO OK",strlen("RECIBIDO OK")+1);
					enviar_paquete(handshake,socketKernel);
					//destruir_paquete(handshake);
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
	solicitud = crear_paquete(lectura,payload,sizeof(t_pun));
	enviar_paquete(solicitud,socketUMV);
	destruir_paquete(solicitud);
	solicitud = recibir_paquete(socketUMV);
	memcpy(&err,solicitud->payload,sizeof(int32_t));
	if(err == -1){
		notificar_kernel(violacionSegmento);
		exit(1);
	}


	free(payload);
	return solicitud;
}

void Escribir(t_pun base, t_pun offset, t_pun tamanio, char* buffer){
	t_solicitudEscritura *sol = malloc(sizeof(t_solicitudEscritura));
	package *paquete = malloc(sizeof(package));
	int32_t err;
	sol->base = base;
	sol->offset = offset;
	sol->tamanio = tamanio;
	sol->buffer = malloc(strlen(buffer)+1);
	memcpy(sol->buffer, buffer,strlen(buffer));

	char* payload = serializarSolicitudEscritura(sol);

	paquete = crear_paquete(escritura,payload,sizeof(t_pun)*3 + strlen(sol->buffer));
	enviar_paquete(paquete,socketUMV);
	destruir_paquete(paquete);
	paquete = recibir_paquete(socketUMV);
	memcpy(&err,paquete->payload,sizeof(int32_t));
	if(err == -1){
		notificar_kernel(violacionSegmento);
		exit(1);
	}
}
