/*
 * sockets.h
 *
 *  Created on: 11/06/2013
 *      Author: utnso
 */

#ifndef SOCKETS_H_
#define SOCKETS_H_

#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <commons/config.h>
#include "paquetes.h"
#include "serializadores.h"
#include "obtener_config.h"
typedef struct t_accept{
	struct sockaddr_in socketInfo;
	int desc_fichero;
} t_cliente;

int abrir_socket();
int vincular_socket(int descriptor, int port);
int escuchar_socket(int descriptor);
int aceptar_conexion(int descriptorEscucha);
int conectar_socket(int descriptor, char* ip, int port);
int identificarme(int descriptor,void* estructura ,int whoisthat);
int identificar(int descriptor, char* buffer);
struct sockaddr_in obtener_datos_socket (int descriptor);
#endif /* SOCKETS_H_ */
