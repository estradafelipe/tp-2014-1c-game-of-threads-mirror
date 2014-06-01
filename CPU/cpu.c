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
	t_PCB* PCB =  malloc(sizeof(t_PCB));
	uint32_t programCounter, indiceCodigo, segmentoCodigo;


	kernelIP.ip = malloc((sizeof(char))*15);
	kernelIP.port = obtenerPuerto(configKernel);
	kernelIP.ip = obtenerIP(configKernel);
	umvIP.ip = malloc((sizeof(char))*15);
	umvIP.ip = obtenerIP(configUMV);
	umvIP.port = obtenerPuerto(configKernel);


	int socketKernel, socketUMV;

	socketKernel = abrir_socket();
	conectar_socket(socketKernel, kernelIP.ip, (int)kernelIP.port); // me conecto al kernel

	package* handshakeCPU_Kernel = crear_paquete(handshakeCpuKernel, "Hola Kernel!", sizeof(char)*12);
	enviar_paquete(handshakeCPU_Kernel);

	// Aca recibimos la rta del handshake por parte del Kernel

	socketUMV = abrir_socket();
	conectar_socket(socketUMV,umvIP.ip, (int)umvIP.port); // me conecto a la UMV

	package* handshakeCPU_UMV = crear_paquete(handshakeCpuUmv,"Hola Umv!",sizeof(char)*12);
	enviar_paquete(handshakeCPU_UMV, socketUMV);

	// Aca recibimos la rta del handshake por parte de la UMV

	while(1){ //para recibir los PCB
			printf("Esperando PCB...\n");
	// TODO: Informar al kernel que estamos libres
			packagePCB = recibir_paquete(socketKernel);
			PCB = desserializarPCB(packagePCB->payload);
			int quantumPrograma = 0;

			while(quantumPrograma<quantumKernel){ // falta obtener el quantum del kernel

								programCounter = PCB->programCounter;
								programCounter++; // ver si se aumenta ahora o despues de ejecutar
								indiceCodigo = PCB->indiceCodigo;

								t_solicitudLectura sol = malloc(sizeof(t_solicitudLectura));
								sol->base = indiceCodigo;
								sol->offset = (programCounter*8);
								sol->tamanio = TAMANIO_SEG;

								char* payloadSerializado = serializarSolicitudLectura(sol);
								package* handshakeUMV_CPU = crear_paquete(handshakeCpuUmv,payloadSerializado,sizeof(t_puntero)*3);
								enviar_paquete(handshakeUMV_CPU, socketUMV);

								package* paquete;
								int bytesRecibidos;
								t_paquete tipo;

								paquete = recibir_paquete(socketUMV);
								bytesRecibidos = paquete->payloadLength;
								tipo = paquete->type;

								segmentoCodigo = PCB->segmentoCodigo;

								t_solicitudLectura* respuesta = desserializarSolicitudLectura(paquete->payload);
								sol->base = segmentoCodigo;
								sol->offset = respuesta->offset;
								sol->tamanio = respuesta->tamanio;

								//ver con julian como manejamos la respuesta de la umv (offset del segmento de codigo y la longitud de la proxima instruccion a ejecutar)

								payloadSerializado = serializarSolicitudLectura(sol);
								package* solicitudLectura;
								solicitudLectura = crear_paquete(lectura,payloadSerializado,sizeof(t_puntero)*3);
								enviar_paquete(solicitudLectura, socketUMV);

								paquete = recibir_paquete(socketUMV);
								bytesRecibidos = paquete->payloadLength;
								tipo = paquete->type;

								respuesta = desserializarSolicitudLectura(paquete->payload);

								// Ejecutar parser

								quantumPrograma ++;
												}
	// Pasar nuevo status del PCB al kernel
			}
	return 0;
}
