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
#include <string.h>

#define TAMANIO_SEG 8
#define TAMANIO_ID_VAR 1
#define TAMANIO_VAR 4

void *recuperar_diccionario(int32_t);

t_dictionary *diccionario;
t_PCB *pcb;
int socketKernel, socketUMV;

int main(int argc, char **argv){
	diccionario = dictionary_create();

	package* packagePCB = malloc(sizeof(package));
	package* paq = malloc(sizeof(package));

	int32_t programcounter, segmentoCodigo, indiceCodigo;

	t_solicitudLectura *sol = malloc(sizeof(t_solicitudLectura));

	t_config *configKernel = config_create((char*)argv[1]);
	t_config *configUMV = config_create((char*)argv[1]);

	t_confKernel kernel;
	t_ip umvIP;

	pcb = malloc(sizeof(t_PCB));




	kernel.ip = malloc((sizeof(char))*15);
	kernel.port = obtenerPuerto(configKernel);
	kernel.ip = obtenerIP(configKernel);
	kernel.tamanioStack = obtenerTamanioStack(configKernel);

	umvIP.ip = malloc((sizeof(char))*15);
	umvIP.ip = obtenerIP(configUMV);
	umvIP.port = obtenerPuerto(configKernel);

	socketKernel = abrir_socket();
	conectar_socket(socketKernel, kernel.ip, (int)kernel.port); // me conecto al kernel (hacer handshake)

	socketUMV = abrir_socket();
	conectar_socket(socketUMV,umvIP.ip, (int)umvIP.port); // me conecto a la UMV


	while(1){ //para recibir los PCB

			packagePCB = recibir_paquete(socketKernel);
			pcb = desserializarPCB(packagePCB->payload);
			int quantumPrograma = 0;
			recuperar_diccionario(pcb->sizeContext);

			while(quantumPrograma<20/*quantumKernel*/){ // falta obtener el quantum del kernel

				programcounter = pcb->programcounter;
				programcounter++;
				indiceCodigo = pcb->indiceCodigo;

				sol->base = pcb->indiceCodigo;
				sol->offset = programcounter*8;
				sol->tamanio = TAMANIO_SEG;

				char* payloadSerializado = serializarSolicitudLectura(sol);
				package* handShakeUMV_CPU = crear_paquete(handshakeCpuUmv,payloadSerializado,sizeof(t_puntero)*3);

				//TODO: verificar que enviar y recibir lo haya hecho con exito
				enviar_paquete(handShakeUMV_CPU, socketUMV);
				paq = recibir_paquete(socketUMV);

				segmentoCodigo = pcb->segmentoCodigo;

				t_solicitudLectura* respuesta = desserializarSolicitudLectura(paq->payload);
				sol->base = segmentoCodigo;
				sol->offset = respuesta->offset;
				sol->tamanio = respuesta->tamanio;

				//ver con julian como manejamos la respuesta de la umv (offset del segmento de codigo y la longitud de la proxima instruccion a ejecutar)

				payloadSerializado = serializarSolicitudLectura(sol);
				package* solicitudLectura;
				solicitudLectura = crear_paquete(lectura,payloadSerializado,sizeof(t_puntero)*3);
				//TODO: verificar que enviar y recibir lo haya hecho con exito
				enviar_paquete(solicitudLectura, socketUMV);
				paq = recibir_paquete(socketUMV);
				respuesta = desserializarSolicitudLectura(paq->payload); //Esto es la sentencia a ejecutar posta

				// Ejecutar parser

				quantumPrograma ++;
		}
			dictionary_clean(diccionario); //limpia el diccionario de variables
	}
	return 0;
}

t_puntero definirVariable(t_nombre_variable identificador_variable){
	char* var = (char*)identificador_variable; //TODO: Preguntar si esta bien.
	uint32_t sizeContext = pcb->sizeContext;
	t_solicitudEscritura *sol =  malloc(sizeof(t_solicitudEscritura));
	package* package_escritura = malloc(sizeof(package));
	package* package_respuesta = malloc(sizeof(package));
	t_puntero puntero;

	sol->base = pcb->segmentoStack;
	sol->offset = pcb->cursorStack + sizeContext*5;
	sol->tamanio = TAMANIO_ID_VAR;
	memcpy(sol->buffer,var,strlen(var)); //TODO: Preguntar si esta bien

	puntero = (uint32_t)sol->offset;

	char* payloadSerializado = serializarSolicitudEscritura(sol);

	package_escritura = crear_paquete(escritura,payloadSerializado,sizeof(t_puntero)*3 + strlen(sol->buffer));
	enviar_paquete(package_escritura,socketUMV);

	package_respuesta = recibir_paquete(socketUMV);

	if (respuesta->payload == "Segmentation Fault"){
		printf("Fallo la escritura\n");
		exit();
		//TODO: ver como notificamos al kernel;
	}

	dictionary_put(diccionario, var, puntero);

	pcb->sizeContext++;

	return puntero;
}

t_puntero obtenerPosicionVariable(t_nombre_variable identificador_variable ){
	char* key = (char*)identificador_variable; // TODO: Preguntar si esta bien.
	t_puntero posicion = dictionary_get(diccionario, key);
	return posicion;
}

t_valor_variable dereferenciar(t_puntero direccion_variable){
	t_valor_variable valorVariable;
	package* solicitudLectura = malloc(sizeof(package));;
	package* respuesta = malloc(sizeof(package));;
	t_solicitudLectura* sol;
	sol->base = pcb->segmentoStack;
	sol->offset = direccion_variable;
	sol->tamanio = TAMANIO_SEG;//TODO: Preguntar a pipe si es TAMANIO_SEG o TAMANIO_VAR

	char* payloadSerializado = serializarSolicitudLectura(sol);
	solicitudLectura = crear_paquete(lectura,payloadSerializado,sizeof(t_puntero)*3);
	enviar_paquete(solicitudLectura, socketUMV);

	respuesta = recibir_paquete(socketUMV);
	valorVariable = (t_valor_variable)respuesta->payload;
	return valorVariable;
}

void asignar(t_puntero direccion_variable, t_valor_variable valor){
	package* solicitudEscritura = malloc(sizeof(package));
	package* respuesta = malloc(sizeof(package));
	t_solicitudEscritura* sol = malloc(sizeof(t_solicitudLecutra));

	sol->base = pcb->segmentoStack;
	sol->offset = direccion_variable + 1;
	sol->tamanio = TAMANIO_VAR;
	memcpy(sol->buffer,&valor,sizeof(t_valor_variable)); //TODO: Preguntar si esta bien

	char* payloadSerializado = serializarSolicitudEscritura(sol);
	solicitudEscritura = crear_paquete(escritura, payloadSerializado, sizeof((t_pun)*3 + strlen(sol->buffer)));
	enviar_paquete(solicitudEscritura, socketUMV);

	respuesta = recibir_paquete(socketUMV);
	//Verificar rta de UMV
}

t_valor_variable obtenerValorCompartida(t_nombre_compartida variable){
	//TODO
}
t_valor_variable asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor){
	//TODO
}
t_puntero_instruccion irAlLabel(t_nombre_etiqueta etiqueta){

}


void *recuperar_diccionario(int32_t cant_var){
	diccionario = dictionary_create();
	t_solicitudLectura *sol = malloc(sizeof(t_solicitudLectura));
	int32_t cursorStack = pcb->cursorStack;
	package* solicitudLectura = malloc(sizeof(package));
	package* paq = malloc(sizeof(package));

			while(cant_var >0){

				sol->base = cursorStack;
				sol->offset = (cant_var - 1) * 5;
				sol->tamanio = TAMANIO_ID_VAR;

				char* payloadSerializado = serializarSolicitudLectura(sol);

				solicitudLectura = crear_paquete(lectura,payloadSerializado,sizeof(t_puntero)*3);
				enviar_paquete(solicitudLectura, socketUMV);
				paq = recibir_paquete(socketUMV);
				//TODO: Verificar que todo haya salido bien
				char* var = paq->payload;
				t_puntero puntero;
				puntero = sol->base + sol->offset;

				dictionary_put(diccionario, var,(void*)puntero);
				cant_var --;
					}

		}
