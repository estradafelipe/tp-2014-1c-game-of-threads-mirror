/*
 * cpu.c
 *
 *  Created on: 03/05/2014
 *      Author: utnso
 */

#include <stdio.h>
#include <stdlib.h>
#include <sockets.h>
#include <commons/config.h>
#include <obtener_config.h>
#include <serializadores.h>
#include <paquetes.h>

#define TAMANIO_SEG 8

int main(int argc, char **argv){


	package* packagePCB;
	t_config *configKernel = config_create((char*)argv[1]);
	t_config *configUMV = config_create((char*)argv[1]);
	t_ip kernelIP;
	t_ip umvIP;
	t_PCB *PCB =  malloc(sizeof(t_PCB));
	uint32_t programCounter, indiceCodigo;


	kernelIP.ip = malloc((sizeof(char))*15);
	kernelIP.port = obtenerPuerto(configKernel);
	kernelIP.ip = obtenerIP(configKernel);
	umvIP.ip = malloc((sizeof(char))*15);
	umvIP.ip = obtenerIP(configUMV);
	umvIP.port = obtenerPuerto(configKernel);


	int socketKernel, socketUMV;
	socketKernel = abrir_socket();
	conectar_socket(socketKernel, kernelIP.ip, (int)kernelIP.port);
	packagePCB = recibir_paquete(socketKernel);
	PCB = desserializarPCB(packagePCB->payload);

	programCounter = PCB->programcounter;
	programCounter++;
	indiceCodigo = PCB->indiceCodigo;


	  t_solicitudLectura sol = malloc(sizeof(t_solicitudLectura));
	  sol->base = indiceCodigo;
	  sol->offset = (programCounter*8)
	  sol->tamanio = TAMANIO_SEG
	  char* payloadSerializado = serializarSolicitudLectura(sol);
	  package* handShakeUMV_CPU = crear_paquete(handshakeCpuUmv,payloadSerializado,sizeof(t_puntero)*3);
	  enviar_paquete(socketUMV);


	socketUMV = abrir_socket();
	conectar_socket(socketUMV,umvIP.ip, (int)umvIP.port);





	return 0;
}
