/*
 * cliente.h
 *
 *  Created on: 28/07/2013
 *      Author: utnso
 */

#ifndef CLIENTE_H_
#define CLIENTE_H_

#include "sockets.h"

//Representa un simple cliente.
typedef struct
{
	t_ip 	 ip;
	t_socket socket;
}t_cliente;

t_cliente cliente_iniciar_conexion(t_cliente cliente,t_ip ip);
void 	  cliente_cerrar_conexion(t_cliente cliente);
int 	  cliente_registar_nivel(t_cliente cliente,char *nombre,t_ip serverIP);

#endif /* CLIENTE_H_ */
