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
	pcb = malloc(sizeof(t_PCB));

	/* Defino las primitivas */

	AnSISOP_funciones primitivas = {

	.AnSISOP_definirVariable = GameOfThread_definirVariable,
	.AnSISOP_obtenerPosicionVariable = GameOfThread_obtenerPosicionVariable,
	.AnSISOP_dereferenciar = GameOfThread_dereferenciar,
	.AnSISOP_asignar = GameOfThread_asignar,
	.AnSISOP_obtenerValorCompartida = GameOfThread_obtenerValorCompartida,
	.AnSISOP_asignarValorCompartida = GameOfThread_asignarValorCompartida,
	.AnSISOP_irAlLabel = GameOfThread_irAlLabel,
	.AnSISOP_llamarSinRetorno = GameOfThread_llamarSinRetorno,
	.AnSISOP_llamarConRetorno = GameOfThread_llamarConRetorno,
	.AnSISOP_finalizar = GameOfThread_finalizar,
	.AnSISOP_retornar = GameOfThread_retornar,
	.AnSISOP_imprimir = GameOfThread_imprimir,
	.AnSISOP_imprimirTexto = GameOfThread_imprimirTexto,
	.AnSISOP_entradaSalida = GameOfThread_entradaSalida
	};

	AnSISOP_kernel funciones_kernel = {
		.AnSISOP_signal = GameOfThread_wait,
		.AnSISOP_wait = GameOfThread_signal
	};

	/* Fin Primitivas */


	//Creo los logs
	logger = log_create("loggerCPU.log","CPU_LOG",false,LOG_LEVEL_TRACE);

	//Creo el diccionario de variables
	diccionarioVariables = dictionary_create();

	//Defino los paquetes y reservo la memoria
	package* packagePCB = malloc(sizeof(package));
	package* paq = malloc(sizeof(package));
	package* respuesta = malloc(sizeof(package));
	char* pcbSerializado;
	char* instruccionAnsisop;
	//Defino variables locales
	int32_t programcounter;
	//char* solicitudEscritura = malloc(sizeof(t_pun)*3);
	t_datoSentencia *datos = malloc(sizeof(t_datoSentencia));

	//Defino solicitud de lectura y reservo espacio
	//t_solicitudLectura *sol = malloc(sizeof(t_solicitudLectura));

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

		if(packagePCB->type == enviarPCBACPU){

			notificar_kernel(respuestaCPU);

			pcb = desserializarPCB(packagePCB->payload);
			destruir_paquete(packagePCB);
			log_debug(logger,"RECIBIDA UNA PCB. Su program id es: %d\n",pcb->id);

			char* id = malloc(sizeof(t_pun));
			memcpy(id,&pcb->id,sizeof(t_pun));
			paq = crear_paquete(cambioProcesoActivo,id,sizeof(t_pun));
			enviar_paquete(paq,socketUMV);
			destruir_paquete(paq);
			paq = recibir_paquete(socketUMV);
			if(paq->type != respuestaUmv){
				log_debug(logger,"Fallo proceso cambio activo");
			}
			destruir_paquete(paq);
			log_debug(logger, "Cambio de proceso activo correcto");
			log_debug(logger, "Cargando diccionario de variables");
			log_debug(logger, "SizeContext: %d",pcb->sizeContext);
			log_debug(logger, "SizeIndexLabel es: %d",pcb->sizeIndexLabel);
			cargar_diccionarioVariables(pcb->sizeContext);
			quantumPrograma = 0;
			programcounter = pcb->programcounter;
			log_debug(logger, "ProgramCounter: %d",programcounter);
			while(quantumPrograma<quantumKernel){

				paq =  Leer(pcb->indiceCodigo,pcb->programcounter*8,TAMANIO_INSTRUCCION);


				memcpy(&datos->inicio,paq->payload,sizeof(int32_t));
				memcpy(&datos->longitud,paq->payload + sizeof(int32_t),sizeof(int32_t));

				paq = Leer(pcb->segmentoCodigo,datos->inicio,datos->longitud);
				instruccionAnsisop = strndup(paq->payload,paq->payloadLength);
				log_debug(logger, "PC: %d, INSTRUCCION: %s",pcb->programcounter,instruccionAnsisop);
				analizadorLinea(instruccionAnsisop,&primitivas,&funciones_kernel);

				quantumPrograma ++;
				pcb->programcounter++;//TODO: ver este tema!!!!!!
			}

			log_debug(logger,"FIN QUANTUM");
			dictionary_clean(diccionarioVariables); //limpio el diccionario de variables

			pcbSerializado = serializar_datos_pcb_para_cpu(pcb);
			respuesta = crear_paquete(retornoCPUQuantum,pcbSerializado,sizeof(t_pun)*5);
			enviar_paquete(respuesta,socketKernel);
			log_debug(logger,"SE ENVIO EL PCB AL KERNEL, tipo paquete: %d",respuesta->type);
			destruir_paquete(respuesta);
			if (desconectarse == true){
				notificar_kernel(cpuDesconectada);
				log_debug(logger,"LLEGO SEÑAL SIGUSR1,NOTIFICO AL KERNEL Y TERMINO EJECUCION");
				exit(1);
			}

		} else {
		destruir_paquete(packagePCB);
		packagePCB = crear_paquete(respuestaCPU,"",0);
		enviar_paquete(packagePCB,socketKernel);
		destruir_paquete(packagePCB);
		log_debug(logger,"ERROR AL RECIBIR PCB");
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
	char* var = malloc(sizeof(char)) + 1;

			while(cant_var > 0){

				offset = pcb->cursorStack + (cant_var - 1) * 5;
				paq = Leer(pcb->segmentoStack,offset,1);
				memcpy(var,paq->payload,sizeof(char) + 1);
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
				printf("Notifique al kernel que estoy disponible\n");
				break;
			case cpuDesconectada:
				paquete =  crear_paquete(cpuDesconectada,"Me Desconecto",strlen("Me Desconecto")+1);
				enviar_paquete(paquete,socketKernel);
				printf("Notifique al kernel que me desconecto\n");
				exit(1);
				break;
			case respuestaCPU:
				paquete = crear_paquete(respuestaCPU,"OK",strlen("OK")+1);
				enviar_paquete(paquete,socketKernel);
				break;
			case retornoCPUFin:
				paquete = crear_paquete(retornoCPUFin,"FINALIZO",strlen("FINALIZO")+1);
				enviar_paquete(paquete,socketKernel);
				break;
			default:
				break;
		}
		destruir_paquete(paquete);

}

void notificarError_kernel(char* error){
	package *paq = malloc(sizeof(package));
	paq = crear_paquete(retornoCPUExcepcion,error,strlen(error)+1);
	enviar_paquete(paq,socketKernel);
	destruir_paquete(paq);
//	free(pcb);
	log_debug(logger,error);
	quantumPrograma = quantumKernel;
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
					memcpy(&quantumKernel,quantum_package->payload,sizeof(t_pun));
					printf("recibi %d\n",quantumKernel);
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
	solicitud = crear_paquete(lectura,payload,sizeof(t_pun)*3);
	enviar_paquete(solicitud,socketUMV);
	destruir_paquete(solicitud);
	solicitud = recibir_paquete(socketUMV);
	memcpy(&err,solicitud->payload,sizeof(int32_t));
	if(err == -1){
		notificarError_kernel("Segmentation Fault");
		//exit(1); no tiene que terminar la cpu
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
	sol->buffer = malloc(tamanio);
	memcpy(sol->buffer, buffer,tamanio);

	char* payload = serializarSolicitudEscritura(sol);

	paquete = crear_paquete(escritura,payload,sizeof(t_pun)*3 + tamanio);
	enviar_paquete(paquete,socketUMV);
	destruir_paquete(paquete);
	paquete = recibir_paquete(socketUMV);
	memcpy(&err,paquete->payload,sizeof(int32_t));
	if(err == -1){
		notificarError_kernel("Segmentation Fault");
		//exit(1); no tiene que terminar la cpu
	}
}
