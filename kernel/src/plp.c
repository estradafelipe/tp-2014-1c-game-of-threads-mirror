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
#include <commons/collections/dictionary.h>
#include <sockets.h>
#include <colas.h>
#include <semaphore.h>
#include <pthread.h>


t_dictionary *programas;
extern t_kernel *kernel;
extern int ultimoid;
extern t_cola *cola_ready;
extern t_cola *cola_exit;
t_list *cola_new;
extern sem_t *sem_exit;
sem_t *sem_multiprogramacion;
sem_t *sem_new;

pthread_mutex_t mutex_new = PTHREAD_MUTEX_INITIALIZER;

void destruirSegmentos(int pcbid){

	char *payload = string_from_format("%d",pcbid);
	package *paquete = crear_paquete(creacionSegmentos,payload,strlen(payload)+1);
	enviar_paquete(paquete,kernel->fd_UMV);

}
t_puntero recibirSegmento(){
	//package *paquete_recibido = recibir_paquete(kernel.fd_UMV);
	t_puntero segmento = 3;
	return segmento;
}
t_puntero solicitarSegmento(t_crearSegmentoUMV *segmento){
	char *payload;
	size_t payload_size=sizeof(t_puntero)*2;
	payload = serializarSolicitudSegmento(segmento);
	package *paquete = crear_paquete(creacionSegmentos,payload,payload_size);
	int resu = enviar_paquete(paquete,kernel->fd_UMV);
	free(paquete);
	free(payload);
	if (resu==-1) return -1;
	sleep(3); //prueba
	return recibirSegmento();
}

//base,offset,size,buffer
int enviarBytesUMV(t_puntero base, t_puntero size, void * buffer){
	t_solicitudEscritura *envioBytes = malloc(sizeof(t_solicitudEscritura));
	// Segmento de Codigo
	envioBytes->base = base;
	envioBytes->offset = 0;
	envioBytes->tamanio = size;
	envioBytes->buffer = buffer;

	char * payload = serializarSolicitudEscritura(envioBytes);
	size_t payload_size = (sizeof(t_puntero)*3) + strlen(buffer) +1;
	package *paquete = crear_paquete(escritura,payload,payload_size);
	int resu = enviar_paquete(paquete,kernel->fd_UMV);
	free(paquete);
	free(payload);
	free(envioBytes);
	if (resu==-1) return -1;
	sleep(3);
	// esperar respuesta
	return 0;
}
bool solicitarSegmentosUMV(char *codigo,t_medatada_program *programa, t_PCB *pcb){

	t_puntero dirSegmento,codigoSize;
	t_crearSegmentoUMV *crearSegmento = malloc(sizeof(t_crearSegmentoUMV));
	codigoSize = strlen(codigo)+1;
	// Segmento de Codigo
	crearSegmento->programid = pcb->id;
	crearSegmento->size = codigoSize;
	dirSegmento = solicitarSegmento(crearSegmento);
	if(dirSegmento == -1){
		destruirSegmentos(pcb->id);
		return false;
	} else pcb->segmentoCodigo = dirSegmento;

	// Segmento de Stack
	crearSegmento->programid = pcb->id;
	crearSegmento->size = kernel->sizeStack;
	dirSegmento = solicitarSegmento(crearSegmento);
	if(dirSegmento == -1){
		destruirSegmentos(pcb->id);
		return false;
	} else pcb->segmentoStack = dirSegmento;

	//Indice de Etiquetas
	crearSegmento->programid = pcb->id;
	crearSegmento->size = programa->etiquetas_size;
	dirSegmento = solicitarSegmento(crearSegmento);
	if(dirSegmento == -1){
		destruirSegmentos(pcb->id);
		return false;
	} else pcb->indiceEtiquetas = dirSegmento;

	//Indice de Codigo
	crearSegmento->programid = pcb->id;
	crearSegmento->size = programa->instrucciones_size;
	dirSegmento = solicitarSegmento(crearSegmento);
	if(dirSegmento == -1){
		destruirSegmentos(pcb->id);
		return false;
	} else pcb->indiceCodigo = dirSegmento;

	pcb->cursorStack = 0; //offset
	pcb->programcounter = programa->instruccion_inicio;

	int rta;
	rta = enviarBytesUMV(pcb->segmentoCodigo,codigoSize,codigo); // Segmento de Codigo
	if (rta==-1){
		destruirSegmentos(pcb->id);
		return false;
	}

	rta = enviarBytesUMV(pcb->segmentoStack,kernel->sizeStack,string_new()); // Segmento de Stack
	if (rta==-1){
		destruirSegmentos(pcb->id);
		return false;
	}
	rta = enviarBytesUMV(pcb->indiceEtiquetas,programa->etiquetas_size,programa->etiquetas); // Segmento de Etiquetas
	if (rta==-1){
		destruirSegmentos(pcb->id);
		return false;
	}

	rta = enviarBytesUMV(pcb->indiceCodigo,programa->instrucciones_size,programa->instrucciones_serializado); // Segmento de Etiquetas
	if (rta==-1){
		destruirSegmentos(pcb->id);
		return false;
	}
	free(crearSegmento);
	return true;
}

int conectarConUMV(){
	t_puntero descriptor;
	char * payload = "HolaUMV";
	descriptor = abrir_socket();
	conectar_socket(descriptor,kernel->ip_umv,kernel->puertoumv);
	package *paquete = crear_paquete(handshakeKernelUmv,payload,strlen(payload)+1);
	int resu = enviar_paquete(paquete,descriptor);
	free(paquete);
	if (resu ==-1) return 0;

	kernel->fd_UMV = descriptor;
	package *paquete_recibido = recibir_paquete(descriptor);
	if (paquete_recibido->type ==handshakeKernelUmv && paquete_recibido->payloadLength>0)
		resu= 1;
	else resu=0;
	free(paquete_recibido);
	return resu;
}

void agregarProgramaNuevo(t_list *cola_new, t_PCB *element){
	pthread_mutex_lock(&mutex_new);
	list_add(cola_new,element);
	pthread_mutex_unlock(&mutex_new);
}

void pasarAReady(t_PCB *element){
	cola_push(cola_ready,element);
}

void siguienteSJN(t_list *cola_new){

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

	pasarAReady(menorElement);
	list_remove(cola_new,menori);
	pthread_mutex_unlock(&mutex_new);
}


/*
void enviarMsgPrograma(int id,char * msg){
	char * key = string_from_format("%d",id);
	int * item = dictionary_get(programas,key);
	int fd = *item;
	int nbytes;
	if (send(fd, msg, nbytes, 0) == -1) {
		perror("send");
	}
}
*/

void rechazarPrograma(int id, int fd){
	// eliminar programa de la tabla
	char * key = string_from_format("%d",id);
	dictionary_remove(programas,key);

	// enviar un mensaje por consola
	// Cambiar para que envie un paquete
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

	// crea programa
	programa->fd = fd;
	programa->id = pcb->id;
	programa->peso = tamanioJob(element->cantidad_de_etiquetas,element->cantidad_de_funciones,element->instrucciones_size);
	char * key = string_from_format("%d",pcb->id);
	dictionary_put(programas,key, programa);
	return pcb;
}

void elementosCola(t_PCB *pcb ){
	t_programa *prog = dictionary_get(programas, string_from_format("%d",pcb->id));
	printf("%d   |   %d\n",pcb->id, prog->peso);
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
		printf("ID  |  Peso\n");
		printf("************\n");
		//armar una simil en la libreria de colas.
		list_iterate(cola_ready->queue->elements,(void*)elementosCola);
		printf("************\n");
}

void liberarRecursosUMV(t_PCB *programa){
	destruirSegmentos(programa->id);
}

/*
 * Hilo que se encarga de liberar los segmentos de memoria reservados
 * por los programas a la UMV
 * */
void hiloSacaExit(){

	while(1){
		sem_wait(sem_exit);
		printf("Hilo Exit, se libero el semaforo");
		t_PCB *programa = cola_pop(cola_exit);
		liberarRecursosUMV(programa);
		free(programa);
		sem_post(sem_multiprogramacion);
	}
}

void hiloMultiprogramacion(){

	while(1){
		sem_wait(sem_multiprogramacion);
		sem_wait(sem_new);
		sleep(5); // pongo el sleep para poder "ver"
		siguienteSJN(cola_new);
		// informar NEW -> Programa X -> READY
		loggeo();
	}
}
void saludarPrograma(int fd){
	char * payload = "Hola Programa";
	package *paquete = crear_paquete(handshakeProgKernel,payload,strlen(payload)+1);
	enviar_paquete(paquete,fd);
	free(paquete);
}

void gestionarDatos(int fd, package *paquete){
	char * buffer = paquete->payload;

	if (paquete->type == handshakeProgKernel){
		printf("Recibi handshake del progrmaa\n");
		saludarPrograma(fd);
	}


	if (paquete->type == programaNuevo){
		t_medatada_program *programa = preprocesar(buffer);
		t_PCB *PCB = crearPCB(fd,programa);
		if (solicitarSegmentosUMV(buffer,programa,PCB)) {
			agregarProgramaNuevo(cola_new,PCB);
			pthread_mutex_unlock(&mutex_new);
			sem_post(sem_new);
			// loguear: informar Programa X -> NEW

			loggeo();

		}
		else
			rechazarPrograma(PCB->id,fd); // informa por pantalla
	}

}
/*  Hilo que recibe los programas (select) */
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

	printf("Listening Socket:%d\n",listensocket);

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
					unsigned int addrlen = sizeof(remoteaddr);
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

						if (paquete->type == handshakeProgKernel)
							printf("Handshake del socket %d, tamanio:%d",i,paquete->payloadLength);
						else
							printf("Mensaje del socket %d, tamanio:%d\n",i,paquete->payloadLength);
						gestionarDatos(i,paquete);
					}
					free(paquete);
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


	// Conectarse a la UMV
	int resconn = conectarConUMV();
	if (resconn==0)
		printf("No se pudo conectar con la UMV"); //ver como tratamos errores.

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
