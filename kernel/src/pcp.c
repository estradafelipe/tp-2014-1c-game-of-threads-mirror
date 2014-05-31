/*
 * pcp.c
 *
 *  Created on: 02/05/2014
 *      Author: utnso
 */

#include "pcp.h"
#include <stdio.h>
#include "kernel.h"
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <commons/collections/dictionary.h>
#include <sockets.h>
#include <colas.h>
#include <semaphore.h>
#include <pthread.h>

t_dictionary *cpus;
extern t_cola *cola_ready;
extern t_cola *cola_exit;


/*  Hilo que recibe los programas (select) */
void recibirCPU(void){
	int nuevofd,nbytes;
	int32_t i;
	fd_set master;  // Conjunto maestro de descriptores de fichero
	fd_set read_fds;// Conjunto temporal de descriptores de fichero para select()
	FD_ZERO(&master); // Vacia el descriptor maestro
	FD_ZERO(&read_fds); // Vacia el descriptor temporal
	struct sockaddr_in direccion_cliente; // Dirección del cliente

	void (*tabla_operaciones[])(char *) = {
		opHandshakeKernelCPU,
		opRetornoCPUQuantum,
	};

	printf("HILO recibirCPUs\n");

	int socket_escucha = abrir_socket();
	vincular_socket(socket_escucha,kernel->puertocpu);
	escuchar_socket(socket_escucha);
	FD_SET(socket_escucha, &master); // Agrega socket_escucha al conjunto maestro de FD
	int fdmax = socket_escucha; // Inicializa fdmax, numero de FD mas grande,
								// con el numero de FD del socket_escucha.

	printf("Listening Socket:%d\n",socket_escucha);

	package *paquete;
	// bucle principal
	while(1) {
		read_fds = master; // Copia el conjunto de descriptores maestro al temporal que utilizara el select
		if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
			perror("select");
			exit(1);
		}

		// analizamos las conexiones existentes en busca de datos que leer
		for(i = 0; i <= fdmax; i++) { // Recorro todos los descriptores hasta el mas grande del conjunto
			if (FD_ISSET(i, &read_fds)) { // Verifico si pertenece al conjunto
				if (i == socket_escucha) { // Hay novedades en el socket del servicio (Servidor)
					unsigned int addrlen = sizeof(direccion_cliente);
					if ((newfd = accept(socket_escucha, (struct sockaddr *)&direccion_cliente, &addrlen)) == -1)
						perror("accept");
					else {
						FD_SET(newfd, &master); // Añade al maestro, ¿lo agrega al conjunto del select read_fds?
						if (newfd > fdmax)     // Actualiza máximo
							fdmax = newfd;
						printf("selectserver: new connection from %s on "
												"socket %d\n", inet_ntoa(direccion_cliente.sin_addr), newfd);
					} // accept
				} else {
	                // gestionar datos de un cliente
					paquete = recibir_paquete(i);
					nbytes = paquete->payloadLength;

					if (nbytes == 0) {
						// conexión cerrada por el cliente
						printf("selectserver: socket %d hung up\n", i);
						close(i);
						FD_CLR(i, &master); // Elimina del conjunto maestro, ¿saca de la copia del read_fs?
						//eliminarDeTablaCPU(i);
					}
					else {

						if (paquete->type == handshakeKernelCPU)
							printf("Handshake del socket %d, tamanio:%d\n",i,strlen(strdup(paquete->payload)));
						else
							printf("Mensaje del socket %d, tamanio:%d\n",i,paquete->payloadLength);
						(tabla_operaciones[paquete->type])(i,paquete);
					}
					free(paquete);
				}
			}
		} // for
	} // while
}

void opHandshakeKernelCPU(int fd, package *paquete){
	printf("Recibi handshake de CPU\n");
	intercambioDeInformacion(fd);
}
void opRetornoCPUQuantum(int fd, package *paquete){
	printf("Retorno de CPU por Quantum\n");
	mandarAColaListos(paquete);
}
void opRetornoCPUPorES(int fd, package *paquete){
	printf("Retorno de CPU por E/S\n");
	mandarA_ES(paquete);
}
void opRetornoCPUFin(int fd, package *paquete){
	printf("Retorno de CPU por Finalizacion\n");
	mandarAColaSalida(paquete);
}
void opRetornoCPUExcepcion(int fd, package *paquete){
	printf("Retorno de CPU por Excepcion logica\n");
	mandarAFinalizarProgramaExcLogica(paquete);
}
void opExcepcionCPUHardware(int fd, package *paquete){
	printf("Retorno de CPU por Excepcion Hardware\n");
	mandarAFinalizarProgramaExcHardware(paquete);
}

void hiloPCP(){

	cpus = dictionary_create(); // contendra los fd de las cpus con el id del PCB


	// hilo que reciba CPU
	// hilo que pase ready->exec
	// hilo que pase exec->exit
}
