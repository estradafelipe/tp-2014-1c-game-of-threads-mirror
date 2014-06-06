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
#define TAMANIO_ID_VAR 1
#define TAMANIO_VAR 4

typedef struct{
	t_nombre_variable identificador_variable;
	uint32_t direccion;
}t_datosvariable;

t_list *diccionario;
t_PCB *PCB;
int socketKernel, socketUMV;

int main(int argc, char **argv){
	diccionario = list_create();
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
	t_datosvariable *var;
	uint32_t sizeContext = PCB->sizeContext;
	t_solicitudEscritura *sol;
	package* escritura;
	package* respuesta;
	t_puntero *puntero;

	sol->base = PCB->segmentoStack;
	sol->offset = PCB->cursorStack + sizeContext;
	sol->tamanio = TAMANIO_ID_VAR ;
	sol->buffer = identificador_variable;

	puntero = sol->offset;

	char* payloadSerializado = serializarSolicitudEscritura(sol);

	escritura = crear_paquete(escritura,payloadSerializado,sizeof(t_puntero)*3 + strlen(sol->buffer));
	enviar_paquete(escritura,socketUMV);

	recibir_paquete(respuesta,socketUMV);

	puntero = respuesta->payload; //TODO: VERIFICAR SI PIPE ME ESTA PASANDO EL PUNTERO A LA ID DE LA VARIABLE

	if (respuesta->payload == -1){
		printf("Fallo la escritura\n");
		exit();
		//TODO: ver como notificamos al kernel;
	}


	var->identificador_variable = identificador_variable;
	var->direccion = puntero;
	list_add(diccionario,var);

	PCB->sizeContext++;

	return puntero;
}

t_puntero obtenerPosicionVariable(t_nombre_variable identificador_variable ){
	t_datosvariable *var;

	int tamanio = list_size(diccionario);
	int i;
	for(i=0;i<=tamanio;i++){ //TODO: ESTA HECHO CON list_get() , VER SI SE PUEDE HACER CON list_find()

		var = (t_datosvariable*) list_get(diccionario,i);
		if (var->identificador_variable == identificador_variable){
			return var->direccion;
		}
		else{
			i++;
		}

	}

	return -1;
}

