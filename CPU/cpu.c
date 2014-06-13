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

typedef u_int32_t t_valor_variable;
t_dictionary *diccionario;
t_PCB *pcb;
int socketKernel, socketUMV;

int main(int argc, char **argv){
	diccionario = dictionary_create();
	package* packagePCB;
	t_config *configKernel = config_create((char*)argv[1]);
	t_config *configUMV = config_create((char*)argv[1]);
	t_ip kernelIP;
	t_ip umvIP;
	pcb = malloc(sizeof(t_PCB));
	uint32_t programcounter, indiceCodigo, segmentoCodigo, sizeContext;
	t_solicitudLectura *sol;
	package* paquete, respuesta;

	kernelIP.ip = malloc((sizeof(char))*15);
	kernelIP.port = obtenerPuerto(configKernel);
	kernelIP.ip = obtenerIP(configKernel);
	umvIP.ip = malloc((sizeof(char))*15);
	umvIP.ip = obtenerIP(configUMV);
	umvIP.port = obtenerPuerto(configKernel);

	socketKernel = abrir_socket();
	conectar_socket(socketKernel, kernelIP.ip, (int)kernelIP.port); // me conecto al kernel (hacer handshake)

	socketUMV = abrir_socket();
	conectar_socket(socketUMV,umvIP.ip, (int)umvIP.port); // me conecto a la UMV


	while(1){ //para recibir los PCB

			packagePCB = recibir_paquete(socketKernel);
			pcb = desserializarPCB(packagePCB->payload);
			int quantumPrograma = 0;

			recrearDiccionario();

			while(quantumPrograma<20/*quantumKernel*/){ // falta obtener el quantum del kernel

				programcounter = pcb->programcounter;
				programcounter++;
				indiceCodigo = pcb->indiceCodigo;

				sol->base = pcb->indiceCodigo;
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

				segmentoCodigo = pcb->segmentoCodigo;

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
			dictionary_clean(diccionario); //limpia el diccionario de variables
	}
	return 0;
}

void recrearDiccionario(){
	uint32_t cant_var, cursorStack;
	t_solicitudLectura *sol;
	char* payloadSerializado;
	package* solicitudLectura, paquete;
	int bytesRecibidos;
	char* var;
	t_puntero* puntero;
	t_paquete tipo;

	cant_var = pcb->sizeContext;

	while(cant_var >0){
		cursorStack = pcb->cursorStack;

		sol->base = cursorStack;
		sol->offset = (cant_var - 1) * 5;
		sol->tamanio = TAMANIO_SEG; // aca leo el nombre de la variable.. nose si es TAMANIO_SEG

		payloadSerializado = serializarSolicitudLectura(sol);
		solicitudLectura = crear_paquete(lectura,payloadSerializado,sizeof(t_puntero)*3);
		enviar_paquete(solicitudLectura, socketUMV);

		paquete = recibir_paquete(socketUMV);
		bytesRecibidos = paquete->payloadLength;
		tipo = paquete->type;


		var = desserializarSolicitudLectura(paquete->payload);
		puntero = sol->base + sol->offset;

		dictionary_put(diccionario, var, puntero);
		cant_var --;
	}

}

t_puntero definirVariable(t_nombre_variable identificador_variable){
	char* var = (char*)identificador_variable; //Aca casteamos.. verificar los tipos (es la key)
	uint32_t sizeContext = pcb->sizeContext;
	t_solicitudEscritura *sol;
	package* escritura = malloc(sizeof(package));
	package* respuesta = malloc(sizeof(package));
	t_puntero *puntero;

	sol->base = pcb->segmentoStack;
	sol->offset = pcb->cursorStack + sizeContext*5;
	sol->tamanio = TAMANIO_ID_VAR;
	sol->buffer = identificador_variable;

	puntero = sol->offset;

	char* payloadSerializado = serializarSolicitudEscritura(sol);

	escritura = crear_paquete(escritura,payloadSerializado,sizeof(t_puntero)*3 + strlen(sol->buffer));
	enviar_paquete(escritura,socketUMV);

	recibir_paquete(respuesta,socketUMV);

	if (respuesta->payload == -1){
		printf("Fallo la escritura\n");
		exit();
		//TODO: ver como notificamos al kernel;
	}

	dictionary_put(diccionario, var, puntero);

	pcb->sizeContext++;

	return puntero;
}

t_puntero obtenerPosicionVariable(t_nombre_variable identificador_variable ){
	char* key = (char*)identificador_variable;
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
	sol->tamanio = TAMANIO_SEG;

	char* payloadSerializado = serializarSolicitudLectura(sol);
	solicitudLectura = crear_paquete(lectura,payloadSerializado,sizeof(t_puntero)*3);
	enviar_paquete(solicitudLectura, socketUMV);

	respuesta = recibir_paquete(socketUMV);
	valorVariable = (t_valor_variable)desserializarSolicitudLectura(respuesta->payload);
	return valorVariable;
}

void asignar(t_puntero direccion_variable, t_valor_variable valor){
	package* solicitudEscritura = malloc(sizeof(package));;
	package* respuesta = malloc(sizeof(package));;
	t_solicitudEscritura* sol;
	sol->base = pcb->segmentoStack;
	sol->offset = dirreccion_variable + 1;
	sol->tamanio = TAMANIO_SEG;
	sol->buffer = valor; //TODO: casteo de buffer-valor (casteo char* - u_int32_t)

	char* payloadSerializado = serializarSolicitudEscritura(sol);
	solicitudEscritura = crear_paquete(escritura, payloadSerializado, sizeof((t_puntero)*3 + strlen(buffer)));
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
	//TODO
}
