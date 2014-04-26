/*
 * cliente.c
 *
 *  Created on: 28/07/2013
 *      Author: utnso
 */

#include <stdlib.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "sockets.h"
//#include "../misc/msj.h"
#include "string.h"
#include "cliente.h"


//Conecta el socket cliente hacia una ip.
t_cliente cliente_iniciar_conexion(t_cliente cliente,t_ip ip)
{
	cliente.ip.ip      = ip.ip;
	cliente.ip.puerto  = ip.puerto;

	cliente.socket     = iniciar_conexion(cliente.socket,ip);

	return cliente;
}

//Cierra la conexion del socket.
void cliente_cerrar_conexion(t_cliente cliente)
{
	socket_cerrar(cliente.socket.descriptor);
}

//Enviar mensaje al servidor.
int cliente_enviar_msj(t_cliente cliente,char *msj,t_ip serverIP)
{
	//Envio el msj por el socket, otengo el resultado.
	int resu  = socket_enviar(cliente.socket.descriptor,msj);

	//Si se puedo enviar lo informo.
	if (resu>0)
		return 1;
	else
		return 0;
}
