/*
 ============================================================================
 Name        : Pruebas.c
 Author      : silvina
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <commons/string.h>
#include <commons/config.h>
#include "socket/cliente.h"

t_config* config;
char * path_config;
char * ip;
int puerto;

void leerconfiguracion(){
	path_config =getenv("ANSISOP_CONFIG"); // levanto una variable de entorno
	config = config_create(path_config);
	if (config_has_property(config, "IP"))
		ip = config_get_string_value(config,"IP");
	if (config_has_property(config, "Puerto"))
		puerto = config_get_int_value(config, "Puerto");
  }

int main(int argc, char **argv) {

	leerconfiguracion();

	//** este pedazo de codigo es de debug
	int index;
	for (index=0; index<argc;index++){
		printf(" Parametro %d: %s\n", index, argv[index]);
	}
	//**

	char * path = argv[1]; // path del script ansisop
	struct stat stat_file;
	stat(path, &stat_file);

	FILE* file = NULL;
	file = fopen(path,"r");
	if (file==NULL)
		printf("no se puede abrir el archivo\n");

	else {
		char* buffer = calloc(1, stat_file.st_size + 1);
		fread(buffer, stat_file.st_size, 1, file); // levanto el archivo en buffer
		printf("%s",buffer);
		printf("tamanio:%d\n",(int)stat_file.st_size);

		//** Prueba basica envio por socket
		t_cliente cliente;
		cliente = cliente_iniciar_conexion(cliente,nueva_ip(ip,puerto));

		int *descriptor = malloc(sizeof(int));
		*descriptor = cliente.socket.descriptor;
		//Enviar msj al server
		int resu  = socket_enviar(cliente.socket.descriptor,buffer);
		if (resu ==-1)
			printf("No se pudo enviar el archivo\n");
		char *msj2 =  socket_recibir(cliente.socket.descriptor);
		printf("Nos dijeron: %s\n",msj2);
		free(buffer);
		fclose(file);
	}
	return EXIT_SUCCESS;
}
