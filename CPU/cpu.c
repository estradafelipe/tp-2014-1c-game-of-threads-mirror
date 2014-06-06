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
#include <parser/sintax.h>
#include <parser/parser.h>
#include <colas.h>
#define TAMANIO_SEG 8

typedef struct{
	t_nombre_variable identificador_variable;
	uint32_t direccion;
}t_diccionario;

t_cola *diccionario;
t_PCB *PCB;

int main(int argc, char **argv){


	package* packagePCB;
	t_config *configKernel = config_create((char*)argv[1]);
	t_config *configUMV = config_create((char*)argv[1]);
	t_ip kernelIP;
	t_ip umvIP;
	PCB= malloc(sizeof(t_PCB));
	uint32_t programcounter, indiceCodigo, segmentoCodigo;
	t_solicitudLectura *sol;
	package* paquete, respuesta;

	kernelIP.ip = malloc((sizeof(char))*15);
	kernelIP.port = obtenerPuerto(configKernel);
	kernelIP.ip = obtenerIP(configKernel);
	umvIP.ip = malloc((sizeof(char))*15);
	umvIP.ip = obtenerIP(configUMV);
	umvIP.port = obtenerPuerto(configKernel);


	int socketKernel, socketUMV;
	socketKernel = abrir_socket();
	conectar_socket(socketKernel, kernelIP.ip, (int)kernelIP.port); // me conecto al kernel

	socketUMV = abrir_socket();
	conectar_socket(socketUMV,umvIP.ip, (int)umvIP.port); // me conecto a la UMV


	while(1){ //para recibir los PCB

			packagePCB = recibir_paquete(socketKernel);
			PCB = desserializarPCB(packagePCB->payload);
			int quantumPrograma = 0;

			while(quantumPrograma<20/*quantumKernel*/){ // falta obtener el quantum del kernel

								programcounter = PCB->programcounter;
								programcounter++;
								indiceCodigo = PCB->indiceCodigo;

								sol->base = PCB->indiceCodigo;
								sol->offset = programcounter*8;
								sol->tamanio = TAMANIO_SEG;

								char* payloadSerializado = serializarSolicitudLectura(sol);
								package* handShakeUMV_CPU = crear_paquete(handshakeCpuUmv,payloadSerializado,sizeof(t_puntero)*3);
								enviar_paquete(handShakeUMV_CPU, socketUMV);


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

								respuesta = desserializarSolicitudLectura(paquete->payload); //Esto es la sentencia a ejecutar posta

								// Ejecutar parser

								quantumPrograma ++;
												}
			}
	return 0;
}

t_puntero definirVariable(t_nombre_variable identificador_variable){
	diccionario = cola_create();
	t_diccionario *dicc;
	if(PCB->sizeContext == 0){ // El contexto esta vacio

		/*Decir a pipe que escriba en el stack la variable : direccion = cursor_stack
		 * solicitudEscritura(tamanio=1,identificador_variable);
		 * solicitudEscritura(tamanio=4);
		 */

		dicc->identificador_variable = identificador_variable;
		dicc->direccion	= 0 ; //Poner la direccion

		cola_push(diccionario,(void*)dicc);

		// return direccionVaraible
	}
		else{ //El contexto tiene al menos una variable

			/*Decir a pipe que escriba en el stack la variable : direccion = sizecontext*5 + 1
					 * solicitudEscritura(tamanio=1,identificador_variable);
					 * solicitudEscritura(tamanio=4);
					 */
			dicc->identificador_variable = identificador_variable;
					dicc->direccion	= 0 ; //Poner la direccion

					cola_push(diccionario,(void*)dicc);

			// return direccionVaraible

		}
}
