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
#include <sockets.h>
#include <unistd.h>
#include <string.h>

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
	printf("ip:%s,puerto:%d",ip,puerto);
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

		char * buffer = "HolaKernel";
		package *paquete_nuevo = malloc(sizeof(paquete_nuevo));

		int descriptor;
		descriptor = abrir_socket();
		conectar_socket(descriptor,ip,puerto);
		printf("me conecte, voy a enviar el saludo..\n");
		package *paquete = crear_paquete(handshakeProgKernel,buffer,strlen(buffer)+1);
		int resu = enviar_paquete(paquete,descriptor);
		printf("mande el handshake\n");
		free(paquete);
		//Enviar msj al server
		if (resu ==-1)
			printf("No se pudo enviar el mensaje\n");

		package * paquete_recibido = recibir_paquete(descriptor);
		if(paquete_recibido->type==handshakeProgKernel) {
			printf("Me dio el ok el Kernel\n");
			buffer = calloc(1, stat_file.st_size); //+1
			fread(buffer, stat_file.st_size-1, 1, file); // levanto el archivo en buffer
			fclose(file);
			package *paquete = crear_paquete(programaNuevo,buffer,stat_file.st_size);
			int resu = enviar_paquete(paquete,descriptor);
			if (resu ==-1)
				printf("No se pudo enviar el mensaje\n");
			free(paquete);
			printf("Envie el programa\n");
			paquete_nuevo = recibir_paquete(descriptor);
			// Analizar Respuestas del Kernel
			if (paquete_nuevo->type==rechazoPrograma){
				char *string = malloc(paquete_nuevo->payloadLength);
				memcpy(string,paquete_nuevo->payload,paquete_nuevo->payloadLength);
				printf("%s\n",string);
			}

			if (paquete_nuevo->type==finPrograma){
				int *exit_code = malloc(paquete_nuevo->payloadLength);
				memcpy(&exit_code,paquete_nuevo->payload,paquete_nuevo->payloadLength);
				//en funcion del codigo  mensaje por consola
				if (exit_code==0) printf("El programa Finalizo correctamente");
				// DEFINIR LOS != MENSAJES DE ERROR
			}

			if (paquete_nuevo->type==programaImprimir){
				int *valor_mostrar = malloc(paquete_nuevo->payloadLength);
				memcpy(&valor_mostrar,paquete_nuevo->payload,paquete_nuevo->payloadLength);
				printf("%d",*valor_mostrar);

			}
			if (paquete_nuevo->type==programaImprimirTexto){
				char *texto = malloc(paquete_nuevo->payloadLength);
				memcpy(texto,paquete_nuevo->payload,paquete_nuevo->payloadLength);
				printf("%s",texto);

			}


		}


		free(paquete_recibido);
		free(paquete_nuevo);
		free(buffer);
		close(descriptor);
	}
	return EXIT_SUCCESS;
}
