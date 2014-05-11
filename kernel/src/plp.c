/*
 * plp.c
 *
 *  Created on: 02/05/2014
 *      Author: utnso
 */

#include <stdio.h>
#include "plp.h"
#include "kernel.h"
#include <stdlib.h>
#include <parser/metadata_program.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "paquetes.h"
#include <commons/collections/dictionary.h>
#include "sockets.h"
#include <semaphore.h>
#include <pthread.h>


t_dictionary *programas;
extern t_kernel *kernel;
extern int ultimoid;
extern t_list *cola_ready;
extern t_list *cola_exit;
t_list *cola_new;
extern sem_t *sem_exit;
sem_t *sem_multiprogramacion;
sem_t *sem_new;
pthread_mutex_t mutex_new = PTHREAD_MUTEX_INITIALIZER;
extern pthread_mutex_t mutex_ready;
bool solicitarSegmentosUMV(char *codigo,t_medatada_program *programa){
	//solicito segmento de codigo = strlen(buffer)
	// indice de codigo=
	// indice de etiquetas=
	// stack = kernel->sizeStack
	//programa->etiquetas_size
	//programa->instrucciones_size
	return true;
}


void encolar(t_list *cola, t_PCB *element){
	printf("funcion encolar\n");
	list_add(cola,element);
}

void pasarAReady(t_PCB *element){
	pthread_mutex_lock(&mutex_ready);
	list_add(cola_ready,element);
	pthread_mutex_unlock(&mutex_ready);
	/*
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
	*/
}

void siguienteSJN(t_list *cola_new){
	printf("funcion siguienteSJN");
	int i,menori,cntnew;
	t_PCB *element;
	t_programa *menorPrograma;
	t_programa *programa;
	pthread_mutex_lock(&mutex_new);

	cntnew = list_size(cola_new);
	t_PCB *menorElement=list_get(cola_new,0);
	menorPrograma = dictionary_get(programas,string_from_format("%d",menorElement->id));
	menori = 0;
	for (i=1;i<cntnew;i++){
		element = list_get(cola_new,i);
		programa = dictionary_get(programas,string_from_format("%d",element->id));
		if (programa->peso<menorPrograma->peso){
			menori = i;
			menorElement = element;
			menorPrograma = programa;
		}
	}
	pthread_mutex_unlock(&mutex_new);
	printf("menor elemento:%d, peso:%d\n",menorPrograma->id,menorPrograma->peso);
	pasarAReady(menorElement);
	list_remove(cola_new,menori);
}

t_PCB *desencolar(t_list *cola){
	t_PCB *element =list_get(cola,0);
	list_remove(cola,0);
	return element;
}

void enviarMsgPrograma(int id,char * msg){
	char * key = string_from_format("%d",id);
	int * item = dictionary_get(programas,key);
	int fd = *item;
	int nbytes;
	if (send(fd, msg, nbytes, 0) == -1) {
		perror("send");
	}
}

void rechazarPrograma(int fd){
	char * msg = "No hay recursos suficientes para procesar el programa";
	if (send(fd, msg, strlen(msg), 0) == -1) {
		perror("send");
	}

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



t_PCB *crearPCB(int fd, t_medatada_program *element){
	t_PCB *pcb =  malloc(sizeof(t_PCB));
	t_programa *programa = malloc(sizeof(t_programa));

	ultimoid++;
	pcb->id = ultimoid;
	pcb->programcounter = element->instruccion_inicio;
	// faltan campos

	// crea programa
	programa->fd = fd;
	programa->id = pcb->id;
	programa->peso = tamanioJob(element->cantidad_de_etiquetas,element->cantidad_de_funciones,element->instrucciones_size);
	char * key = string_from_format("%d",pcb->id);
	dictionary_put(programas,key, programa);
	return pcb;
}

void loggeo(){

	t_PCB *parm;
	t_programa *prog;
	printf("***IMPRIMO LA COLA DE NEW***\n");
		int i = 0;
		printf("ID  |  Peso\n");
		printf("************\n");

		while(i < list_size(cola_new)){
			parm = list_get(cola_new,i);
			prog = dictionary_get(programas, string_from_format("%d",parm->id));
			printf("%d   |   %d\n",parm->id, prog->peso);
			i++;
		}
		printf("************\n");

		printf("***IMPRIMO LA COLA DE READY***\n");
		i = 0;
		printf("ID  |  Peso\n");
		printf("************\n");

		while(i < list_size(cola_ready)){
			parm = list_get(cola_ready,i);
			prog = dictionary_get(programas, string_from_format("%d",parm->id));
			printf("%d   |   %d\n",parm->id, prog->peso);
			i++;
		}
		printf("************\n");

}
void liberarRecursosUMV(t_PCB *programa){

}

/*
 * Hilo que se encarga de liberar los segmentos de memoria reservados
 * por los programas a la UMV
 * */
void hiloSacaExit(){
	printf("Hilo exit");
	while(1){
		sem_wait(sem_exit);
		printf("Hilo Exit, se libero el semaforo");
		t_PCB *programa = desencolar(cola_exit);
		liberarRecursosUMV(programa);
		sem_post(sem_multiprogramacion);

	}

}
void hiloMultiprogramacion(){
	printf("hilo multiprogramacion");
	while(1){
		sem_wait(sem_multiprogramacion);
		sem_wait(sem_new);
		sleep(10);
		siguienteSJN(cola_new);
		// informar NEW -> Programa X -> READY
		loggeo();
	}
}
void gestionarDatos(int fd, char *buffer){
	t_medatada_program *programa = preprocesar(buffer);
	if (solicitarSegmentosUMV(buffer,programa)) {
		t_PCB *PCB = crearPCB(fd,programa);
		pthread_mutex_lock(&mutex_new);
		encolar(cola_new,PCB);
		pthread_mutex_unlock(&mutex_new);
		sem_post(sem_new);
		// loguear: informar Programa X -> NEW

		loggeo();

	}
	else
		rechazarPrograma(fd); // informa por pantalla

}
void recibirProgramas(void){
	int newfd,nbytes;
	int32_t i;
	fd_set master;  // conjunto maestro de descriptores de fichero
	fd_set read_fds;// conjunto temporal de descriptores de fichero para select()
	FD_ZERO(&master); // borra los conjuntos maestro y temporal
	FD_ZERO(&read_fds);
	struct sockaddr_in remoteaddr; // dirección del cliente
	printf("HILO recibirProgramas\n");

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
						// eliminar del dictionary

					}
					else {

						if (paquete->type == handshake)
							printf("Handshake del socket %d, tamanio:%d,mensaje:%s\n",i,paquete->payloadLength,paquete->payload);
						else
							printf("Mensaje del socket %d, tamanio:%d,mensaje:%s\n",i,paquete->payloadLength,paquete->payload);
						gestionarDatos(i,paquete->payload);
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
	programas = dictionary_create(); // contendra los fd de los programas con el id del PCB
	sem_multiprogramacion = malloc(sizeof(sem_t));
	sem_new = malloc(sizeof(sem_t));
	sem_init(sem_multiprogramacion,0,kernel->multiprogramacion);
	sem_init(sem_new,0,0);

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

	// un hilo que controle el grado de multiprogramacion
	// un hilo que controle la cola de exit
	// un hilo que recibe programas.

	int thr;
	pthread_t * progthr = malloc(sizeof(pthread_t)); // hilo q recibe programas
	thr = pthread_create( progthr, NULL, (void*)recibirProgramas, NULL);
	if (thr== 0)
		printf("Se creo el hilo q recibe programas lo mas bien\n");//se pudo crear el hilo
	else printf("no se pudo crear el hilo\n");//no se pudo crear el hilo

	pthread_t * multithr = malloc(sizeof(pthread_t)); // hilo q recibe programas
	thr = pthread_create( multithr, NULL, (void*)hiloMultiprogramacion, NULL);
	if (thr== 0)
		printf("Se creo el hilo q controla multiprogramacion lo mas bien\n");
	else printf("no se pudo crear el hilo\n");

	pthread_t * exitthr = malloc(sizeof(pthread_t)); // hilo q recibe programas
	thr = pthread_create( exitthr, NULL, (void*)hiloSacaExit, NULL);
	if (thr== 0)
		printf("Se creo el hilo q saca de exit lo mas bien\n");//se pudo crear el hilo
	else printf("no se pudo crear el hilo\n");//no se pudo crear el hilo


}
