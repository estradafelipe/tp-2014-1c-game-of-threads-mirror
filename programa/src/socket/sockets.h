/*
 * sockets.h
 *
 *  Created on: 28/06/2013
 *      Author: utnso
 */

#ifndef SOCKETS_H_
#define SOCKETS_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "sockets.h"
#include <commons/string.h>
//#include "string.h"
#include <stdio.h>
#include <stdlib.h>


typedef struct
{
    int dni;
    char *name;
    char *lastname;
}t_person;

typedef struct
{
	int length;
	char *data;
}t_stream;

typedef struct
{
	struct sockaddr_in direccion;
	int32_t            descriptor;
}t_socket;

typedef struct
{
	char *ip;
	int  puerto;
}t_ip;

typedef struct
{
	int  fd_id;
	t_ip ip;
}t_conexion;

t_stream *serializar(t_person *self);
t_person *deserializar(t_stream *stream);

int      socket_error(int valor);
int      socket_crear();
struct   sockaddr_in socket_config(char *ip,int puerto);
int      socket_conectar(int descriptor,struct sockaddr_in direccion);
int      socket_enviar(int descriptor,char *texto);
int 	 socket_sockop(int descriptor);
int 	 socket_vincular(int descriptor,struct sockaddr_in local);
int      socket_escuchar(int descriptor);
t_ip     nueva_ip(char *ip,int puerto);
char    *socket_recibir(int descriptor);
void 	 socket_cerrar(int descriptor);
t_socket iniciar_conexion(t_socket conexion,t_ip ip);

#endif /* SOCKETS_H_ */
