/*
 * plp.c
 *
 *  Created on: 02/05/2014
 *      Author: utnso
 */

#include <stdio.h>
#include "plp.h"
#include <stdlib.h>
#include <parser/metadata_program.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "paquetes.h"

#include "sockets.h"

extern t_kernel *kernel;
//extern pthread_mutex_t mutex_fin;
extern int ultimoid;
extern t_list *cola_exec;
extern t_list *cola_ready;
t_list *cola_new;

bool solicitarSegmentosUMV(t_medatada_program *programa){
	return true;
}

bool cumplemultiprogramacion(){
	int cntexec = list_size(cola_exec);
	int cntready = list_size(cola_ready);
	int cnttotal = cntexec + cntready;
	return (cnttotal<kernel->multiprogramacion);
}




void encolar(t_list *cola, void *element){
	list_add(cola,element);
}
void pasarAReady(t_PCB *element){
	//hacer SJN
	int i;
	int cntready = list_size(cola_ready);
	bool entro=false;
	if(!list_is_empty(cola_ready)){
		for (i=0; i<cntready; i++) {
			t_PCB *p = list_get(cola_ready,i);
			if (p->peso > element->peso){
				list_add_in_index(cola_ready, i, element);
				entro = true;
				break;
			}
		}
	}

	if (!entro) encolar(cola_ready,element);
}

void siguienteSJN(t_list *cola_new){
	int i,menori;
	int cntnew = list_size(cola_new);
	t_PCB *element;
	t_PCB *menorElement=list_get(cola_new,0);
	menori = 0;
	for (i=1;i<cntnew;i++){
		element = list_get(cola_new,i);
		if (element->peso<menorElement->peso){
			menori = i;
			menorElement = element;
		}
	}
	pasarAReady(menorElement);
	list_remove(cola_new,menori);
	//buscar el menor de la cola de new
	//pasarAReady(element);
	//list_remove()
}

void *desencolar(t_list *cola){
	t_PCB *element =list_get(cola,0);
	pasarAReady(element);
	list_remove(cola,0);
	return element;
}


void rechazarPrograma(){

}
t_medatada_program* preprocesar(char *buffer){
	t_medatada_program *programa = metadatada_desde_literal(buffer);
	printf("Cantidad de Etiquetas:%d\n",programa->cantidad_de_etiquetas);
	printf("Cantidad de Funciones:%d\n",programa->cantidad_de_funciones);
	printf("Cantidad de Instrucciones:%d\n",programa->instrucciones_size);
	return programa;
}

int tamanioJob(int etiquetas, int funciones, int lineasCodigo){
	return ((5*etiquetas)+(3*funciones) + lineasCodigo);
}

t_PCB *crearPCB(t_medatada_program *element){
	t_PCB *pcb =  malloc(sizeof(t_PCB));
	ultimoid++;

	pcb->id = ultimoid;
	pcb->peso = tamanioJob(element->cantidad_de_etiquetas,element->cantidad_de_funciones,element->instrucciones_size);
	pcb->programcounter = 0;
	// faltan campos
	return pcb;
}
void loggeo(){

	t_PCB *parm=malloc(sizeof(t_PCB));
	printf("***IMPRIMO LA COLA DE NEW***\n");
		int i = 0;
		printf("ID  |  Peso\n");
		printf("************\n");

		while(i < list_size(cola_new)){
			parm = list_get(cola_new,i);
			printf("%d   |   %d\n",parm->id, parm->peso);
			i++;
		}
		printf("************\n");

		printf("***IMPRIMO LA COLA DE READY***\n");
		i = 0;
		printf("ID  |  Peso\n");
		printf("************\n");

		while(i < list_size(cola_ready)){
			parm = list_get(cola_ready,i);
			printf("%d   |   %d\n",parm->id, parm->peso);
			i++;
		}
		printf("************\n");
		free(parm);
}
void gestionarDatos(char *buffer){
	t_medatada_program *programa = preprocesar(buffer);
	if (solicitarSegmentosUMV(programa)) {
		t_PCB *PCB = crearPCB(programa);
		encolar(cola_new,PCB);
		// loguear: informar Programa X -> NEW
		loggeo();
		if (cumplemultiprogramacion()){
			siguienteSJN(cola_new);
			// informar NEW -> Programa X -> READY
			loggeo();
		}
	}
	else
		rechazarPrograma(); // informa por pantalla

}
void recibirProgramas(void){
	int i,newfd,nbytes;
	fd_set master;  // conjunto maestro de descriptores de fichero
	fd_set read_fds;// conjunto temporal de descriptores de fichero para select()
	FD_ZERO(&master); // borra los conjuntos maestro y temporal
	FD_ZERO(&read_fds);
	struct sockaddr_in remoteaddr; // dirección del cliente
	printf("HILO PLP\n");

	int listensocket = abrir_socket();
	vincular_socket(listensocket,kernel->puertoprog);
	escuchar_socket(listensocket);
	FD_SET(listensocket, &master); // añadir listenningSocket al conjunto maestro
	int fdmax = listensocket; //seguir la pista del descriptor de fichero mayor,por ahora es éste

	printf("Listening Socket:%d",listensocket);

	package *paquete;
	// bucle principal
	while(1) {
		read_fds = master; // cópialo
		if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
			perror("select");
			exit(1);
		}

		// analizamos las conexiones existentes en busca de datos que leer
		for(i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &read_fds)) { // ¡¡tenemos datos!!
				if (i == listensocket) {
					// gestionar nuevas conexiones
					int addrlen = sizeof(remoteaddr);
					if ((newfd = accept(listensocket, (struct sockaddr *)&remoteaddr, &addrlen)) == -1)
						perror("accept");
					else {
						FD_SET(newfd, &master); // añadir al conjunto maestro
						if (newfd > fdmax)     // actualizar el máximo
							fdmax = newfd;
						printf("selectserver: new connection from %s on "
												"socket %d\n", inet_ntoa(remoteaddr.sin_addr), newfd);
					} // accept
				} else {
	                // gestionar datos de un cliente
					paquete = recibir_paquete(i);
					nbytes = paquete->payloadLength;
					if (nbytes == 0) {
						// conexión cerrada por el cliente
						printf("selectserver: socket %d hung up\n", i);
						close(i);
						FD_CLR(i, &master); // eliminar del conjunto maestro
					}
					else {

						if (paquete->type == handshake)
							printf("Handshake del socket %d, tamanio:%d,mensaje:%s\n",i,paquete->payloadLength,paquete->payload);
						else
							printf("Mensaje del socket %d, tamanio:%d,mensaje:%s\n",i,paquete->payloadLength,paquete->payload);
							gestionarDatos(paquete->payload);
						// tenemos datos del cliente i

						if (send(i, "Lo recibi noma", nbytes, 0) == -1) {
							perror("send");
						}
					}
				}
			}
		} // for
	} // while
}

void hiloPLP(){

	cola_new = list_create();
	// todo esto va a estar en un while(1)

	/*
	- para el plp llega el programa
	- pasa el programa por el preprocesador
	- pide los segmentos de memoria
	- crea su pcb
	- los pone en la cola de new
		* yo pondria en new el pcb con la memoria ya reservada, porque
			 * si no me dan esos segmentos de memoria debo rechazar el programa
			 * y sacarlo de la cola de new en ese caso.
			 * Mejor encolarlos en new cuando ya se que van

	- controla grado de multiprogramacion
			* el grado de multiprogramacion es la cantidad de programas que pueden
			* estar encolados en ready block y exec

	- si no da el grado de multiprogramacion los deja en la cola de new
	- si da, saca de new y pone en ready segun sjn

	 */

	recibirProgramas();
/*
	struct stat stat_file;
	stat(path, &stat_file);
	char* buffer = calloc(1, stat_file.st_size + 1);
	FILE* file = NULL;
	file = fopen(path,"r");
	if (file==NULL)
		printf("no se puede abrir el archivo\n");

	else {
		fread(buffer, stat_file.st_size, 1, file); // levanto el archivo en buffer
	}


	t_medatada_program *programa = preprocesar(buffer);
	if (solicitarSegmentosUMV(programa)) {
		t_PCB *PCB = crearPCB(programa);
		encolar(cola_new,PCB);
		// loguear: informar Programa X -> NEW
		if (cumplemultiprogramacion()){
			siguienteSJN(cola_new);
			// informar NEW -> Programa X -> READY
		}
	}
	else
		rechazarPrograma(); // informa por pantalla
*/
}
