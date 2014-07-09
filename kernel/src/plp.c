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
#include <commons/log.h>
#include <sockets.h>
#include <colas.h>
#include <semaphore.h>
#include <pthread.h>


extern t_kernel *kernel;
extern int ultimoid;
extern t_cola *cola_ready;
extern t_cola *cola_exit;
extern t_log *logger;
t_list *cola_new;
t_dictionary *programasxfd;
extern sem_t *sem_exit;
extern sem_t *sem_multiprogramacion;
sem_t *sem_new;
extern sem_t *sem_estado_listo;
pthread_mutex_t mutex_new = PTHREAD_MUTEX_INITIALIZER;


void actualizarExit_Code(int pcbid,int exit_code, char * mensajeFIN){
	char *key = string_from_format("%d",pcbid);
	if (dictionary_has_key(kernel->programas,key)){
		pthread_mutex_lock(&kernel->mutex_programas);
		t_programa * programa = dictionary_get(kernel->programas,key);
		programa->exit_code = exit_code;
		programa->mensajeFIN = mensajeFIN;
		pthread_mutex_unlock(&kernel->mutex_programas);
	}
}

void destruirSegmentos(int pcbid){

	char *payload = malloc(sizeof(int));
	memcpy(payload,&pcbid,sizeof(int));
	package *paquete = crear_paquete(destruccionSegmentos,payload,sizeof(int));
	enviar_paquete(paquete,kernel->fd_UMV);

}

t_puntero recibirSegmento(){
	t_puntero segmento;
	package *paquete_recibido = recibir_paquete(kernel->fd_UMV);
	if (paquete_recibido->type == respuestaUmv){
		memcpy(&segmento,paquete_recibido->payload, sizeof(t_puntero));
		if (segmento!=-1) log_debug(logger,string_from_format("Base del nuevo segmento: %d\n",segmento));
		else log_debug(logger,string_from_format("No hubo espacio suficiente para el segmento"));
	}
	destruir_paquete(paquete_recibido);
	return segmento;
}
t_puntero recibirRespuestaEscritura(){
	t_puntero bytesEscritos;
	package *paquete_recibido = recibir_paquete(kernel->fd_UMV);
	if (paquete_recibido->type == respuestaUmv){
		memcpy(&bytesEscritos,paquete_recibido->payload, sizeof(t_puntero));
		if (bytesEscritos==-1) log_debug(logger,string_from_format("Error al escribir en el segmento (UMV)\n"));
		else log_debug(logger,string_from_format("Escritura en el segmento: %d\n",bytesEscritos));
	}
	destruir_paquete(paquete_recibido);
	return bytesEscritos;
}
//base,offset,size,buffer
int enviarBytesUMV(t_puntero base, t_puntero size, void * buffer){
	log_debug(logger,string_from_format("solicito escritura en segmento %d, tamanio: %d\n ",base,size));
	t_solicitudEscritura *envioBytes = malloc(sizeof(t_solicitudEscritura));
	// Segmento de Codigo
	envioBytes->base = base;
	envioBytes->offset = 0;
	envioBytes->tamanio = size;
	envioBytes->buffer = buffer;

	char * payload = serializarSolicitudEscritura(envioBytes);
	size_t payload_size = (sizeof(t_puntero)*3) + size;
	package *paquete = crear_paquete(escritura,payload,payload_size);
	int resu = enviar_paquete(paquete,kernel->fd_UMV);
	destruir_paquete(paquete);
	free(envioBytes);
	if (resu==-1) return -1;
	return recibirRespuestaEscritura();
}

t_puntero solicitarSegmento(t_crearSegmentoUMV *segmento){
	char *payload;
	size_t payload_size=sizeof(t_puntero)*2;
	payload = serializarSolicitudSegmento(segmento);
	package *paquete = crear_paquete(creacionSegmentos,payload,payload_size);
	int resu = enviar_paquete(paquete,kernel->fd_UMV);
	destruir_paquete(paquete);
	if (resu==-1) return -1;
	return recibirSegmento();
}

t_puntero solicitudSegmento(t_puntero id, t_puntero size){
	t_puntero dirSegmento;
	t_crearSegmentoUMV *crearSegmento = malloc(sizeof(t_crearSegmentoUMV));
	crearSegmento->programid = id;
	crearSegmento->size = size;
	dirSegmento = solicitarSegmento(crearSegmento);
	if(dirSegmento == -1){
		destruirSegmentos(id);
	}
	free(crearSegmento);
	return dirSegmento;
}

bool solicitarSegmentosUMV(char *codigo, uint16_t codigoSize, t_medatada_program *programa, t_PCB *pcb){

	t_puntero dirSegmento;
	t_crearSegmentoUMV *crearSegmento = malloc(sizeof(t_crearSegmentoUMV));


	// Segmento de Codigo
	dirSegmento = solicitudSegmento(pcb->id,codigoSize);
	if (dirSegmento==-1)
		return false;
	else pcb->segmentoCodigo = dirSegmento;


	// Segmento de Stack
	dirSegmento = solicitudSegmento(pcb->id,kernel->sizeStack);
	if(dirSegmento == -1){
		return false;
	} else pcb->segmentoStack = dirSegmento;

	if (programa->etiquetas_size>0){
		//Indice de Etiquetas
		dirSegmento = solicitudSegmento(pcb->id,programa->etiquetas_size);
		if(dirSegmento == -1){
			return false;
		} else {
			pcb->indiceEtiquetas = dirSegmento;
			pcb->sizeIndexLabel = programa->etiquetas_size;
		}
	}

	//Indice de Codigo
	dirSegmento = solicitudSegmento(pcb->id,(programa->instrucciones_size*8));
	if(dirSegmento == -1){
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

	if (programa->etiquetas_size>0){
		rta = enviarBytesUMV(pcb->indiceEtiquetas,programa->etiquetas_size,programa->etiquetas); // Segmento de Etiquetas
		if (rta==-1){
			destruirSegmentos(pcb->id);
			return false;
		}
	}

	rta = enviarBytesUMV(pcb->indiceCodigo,(programa->instrucciones_size*8),programa->instrucciones_serializado); // Segmento de Etiquetas
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
	destruir_paquete(paquete);
	if (resu ==-1) return 0;

	kernel->fd_UMV = descriptor;
	package *paquete_recibido = recibir_paquete(descriptor);

	if (paquete_recibido->type ==handshakeKernelUmv && paquete_recibido->payloadLength>0)
		resu= 1;
	else resu=0;
	destruir_paquete(paquete_recibido);
	return resu;
}

void agregarProgramaNuevo(t_list *cola_new, t_PCB *element){
	pthread_mutex_lock(&mutex_new);
	list_add(cola_new,element);
	pthread_mutex_unlock(&mutex_new);
}

void pasarAReady(t_PCB *element){
	cola_push(cola_ready,element);
	log_debug(logger,string_from_format("Pasa a READY: Programa %d\n",element->id));
	sem_post(sem_estado_listo);
}

void siguienteSJN(t_list *cola_new){

	int i,menori,cntnew;
	t_PCB *element;
	t_programa *menorPrograma;
	t_programa *programa;
	pthread_mutex_lock(&mutex_new);
	pthread_mutex_lock(&kernel->mutex_programas);
	cntnew = list_size(cola_new);
	t_PCB *menorElement=list_get(cola_new,0);
	menorPrograma = dictionary_get(kernel->programas,string_from_format("%d",menorElement->id));
	menori = 0;
	for (i=1;i<cntnew;i++){
		element = list_get(cola_new,i);
		programa = dictionary_get(kernel->programas,string_from_format("%d",element->id));
		if (programa->peso<menorPrograma->peso){
			menori = i;
			menorElement = element;
			menorPrograma = programa;
		}
	}
	pthread_mutex_unlock(&kernel->mutex_programas);
	pasarAReady(menorElement);
	list_remove(cola_new,menori);
	pthread_mutex_unlock(&mutex_new);
}


void enviarMsgPrograma(int fd, char *msg){
	char *string = strdup(msg);
	package *paquete = crear_paquete(rechazoPrograma,string,strlen(string)+1);
	enviar_paquete(paquete,fd);
	destruir_paquete(paquete);
}
void finalizarPrograma(int pcbid, int fd, int exit_code, char * mensajeFIN){
	//armamos payload con cod error
	if ((exit_code!=PROGRAM_DISCONNECT)&&(exit_code!=PROGRAM_SEGSIZE_FAULT)){
		package * paquete = crear_paquete(finPrograma,mensajeFIN,strlen(mensajeFIN)+1);
		enviar_paquete(paquete,fd);
	}
}
void eliminarProgramaTabla(int id){
	//#define FIN_SUCCESS 0
	//#define SEG_FAULT -1
	log_debug(logger,string_from_format("Elimina programa de los dictionary\n"));
	char * key = string_from_format("%d",id);
	if (dictionary_has_key(kernel->programas,key)){
		pthread_mutex_lock(&kernel->mutex_programas);
		t_programa *programa = dictionary_get(kernel->programas,key);
		int exit_code = programa->exit_code;
		int fd = programa->fd;
		char * mensajeFin = programa->mensajeFIN;
		dictionary_remove(programasxfd,string_from_format("%d",programa->fd));
		dictionary_remove(kernel->programas,key);
		pthread_mutex_unlock(&kernel->mutex_programas);
		finalizarPrograma(id,fd,exit_code,mensajeFin);
	}
}


void rechazarPrograma(int fd){
	enviarMsgPrograma(fd,"No hay recursos suficientes para procesar el programa");
}

t_medatada_program* preprocesar(char *buffer){
	t_medatada_program *programa = metadata_desde_literal(buffer);
	log_debug(logger,string_from_format("Cantidad de Etiquetas:%d\n",programa->cantidad_de_etiquetas));
	log_debug(logger,string_from_format("Cantidad de Funciones:%d\n",programa->cantidad_de_funciones));
	log_debug(logger,string_from_format("Cantidad de Instrucciones:%d\n",(int)programa->instrucciones_size));
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
	programa->estado = 1;
	char * key = string_from_format("%d",pcb->id);
	pthread_mutex_lock(&kernel->mutex_programas);
	dictionary_put(kernel->programas,key, programa);
	pthread_mutex_unlock(&kernel->mutex_programas);
	dictionary_put(programasxfd,string_from_format("%d",fd),&pcb->id);
	log_debug(logger,string_from_format("Creo PCB: Programa %d, Peso %d\n",programa->id,programa->peso));
	return pcb;
}

void elementosCola(t_PCB *pcb ){
	t_programa *prog = dictionary_get(kernel->programas, string_from_format("%d",pcb->id));
	printf("%d   |   %d\n",pcb->id, prog->peso);
}

void loggeo(){

	t_PCB *parm;
	t_programa *prog;
	printf("***IMPRIMO LA COLA DE NEW***\n");
		int i = 0;
		printf("ID  |  Peso\n");
		printf("************\n");
		pthread_mutex_lock(&kernel->mutex_programas);
		while(i < list_size(cola_new)){
			parm = list_get(cola_new,i);
			prog = dictionary_get(kernel->programas, string_from_format("%d",parm->id));
			printf("%d   |   %d\n",parm->id, prog->peso);
			i++;
		}
		pthread_mutex_unlock(&kernel->mutex_programas);
		printf("************\n");

		printf("***IMPRIMO LA COLA DE READY***\n");
		printf("ID  |  Peso\n");
		printf("************\n");
		//armar una simil en la libreria de colas.
		pthread_mutex_lock(&kernel->mutex_programas);
		list_iterate(cola_ready->queue->elements,(void*)elementosCola);
		pthread_mutex_unlock(&kernel->mutex_programas);
		printf("************\n");

		printf("***IMPRIMO LA COLA DE EXIT***\n");
		printf("ID  |  Peso\n");
		printf("************\n");
		//armar una simil en la libreria de colas.
		pthread_mutex_lock(cola_exit->mutexCola);
		list_iterate(cola_exit->queue->elements,(void*)elementosCola);
		pthread_mutex_unlock(cola_exit->mutexCola);
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
		log_debug(logger,string_from_format("Hilo Exit, se libero el semaforo\n"));
		printf("PASO ANTES DE SACAR DE COLA\n");
		t_PCB *programa = cola_pop(cola_exit);
		printf("PASO luego DE SACAR DE COLA antes de liberar recursos umv\n");
		liberarRecursosUMV(programa);
		printf("PASO luego DE liberar recursos umv antes de eliminar programa de tabla\n");
		eliminarProgramaTabla(programa->id);
		printf("luego de liberar programa de tabla\n");
		//free(programa);
		//sem_post(sem_multiprogramacion); este semaforo se incrementa cuando pasa a Exit!!!!
	}
}

void hiloMultiprogramacion(){

	while(1){
		sem_wait(sem_multiprogramacion);
		sem_wait(sem_new);
		log_debug(logger,string_from_format("paso el sem de multiprogramacion y de new\n"));
		siguienteSJN(cola_new);
		// informar NEW -> Programa X -> READY

	}
}
void saludarPrograma(int fd){
	char * payload = "Hola Programa";
	package *paquete = crear_paquete(handshakeProgKernel,payload,strlen(payload)+1);
	enviar_paquete(paquete,fd);
	destruir_paquete(paquete);
}

void gestionarDatos(int fd, package *paquete){


	if (paquete->type == handshakeProgKernel){
		log_debug(logger,string_from_format("Recibi handshake del progrmaa\n"));
		saludarPrograma(fd);
	}


	if (paquete->type == programaNuevo){

		t_medatada_program *programa = preprocesar(paquete->payload);
		t_PCB *PCB = crearPCB(fd,programa);
		if (solicitarSegmentosUMV(paquete->payload,paquete->payloadLength,programa,PCB)) {
			agregarProgramaNuevo(cola_new,PCB);
			log_debug(logger,string_from_format("Pasa a NEW: Programa %d\n",PCB->id));
			sem_post(sem_new);
		}
		else {
			log_debug(logger,string_from_format("PROGRAMA RECHAZADO: Programa %d\n",PCB->id));
			actualizarExit_Code(PCB->id,PROGRAM_SEGSIZE_FAULT,"No hay espacio suficiente para procesar este Programa");
			eliminarProgramaTabla(PCB->id);
			rechazarPrograma(fd); // informa por pantalla
		}
	}

}


void pasarAExit(t_PCB *pcb){
	cola_push(cola_exit,pcb);
	//sem_post(sem_estado_listo);
	log_debug(logger,string_from_format("Paso a EXIT: Programa %d\n",pcb->id));
	sem_post(sem_exit);
}

void buscarEnColaDesconectado(int id){
	t_PCB *pcb;
	pthread_mutex_lock(&mutex_new);
	int size = list_size(cola_new);
	int i =0;

	if (size > 0){
		while(i<size){
			pcb =list_get(cola_new,i);
			if(pcb->id == id){
				list_remove(cola_new,i);
				pasarAExit(pcb);
				sem_wait(sem_new);
				break;
			}
			i++;
		}
	}
	pthread_mutex_unlock(&mutex_new);
}
/* Actualiza el estado de un programa que se desconecto*/
void detectoDesconexion(int fd){

	log_debug(logger,string_from_format("detecto la desconexion, actualiza estado del programa\n"));
	if (dictionary_has_key(programasxfd,string_from_format("%d",fd))){
		int *id = dictionary_get(programasxfd,string_from_format("%d",fd));
		log_debug(logger,string_from_format("programa:%d\n",*id));
		char *key = string_from_format("%d",*id);
		if (dictionary_has_key(kernel->programas,key)){
			pthread_mutex_lock(&kernel->mutex_programas);
			t_programa * programa = dictionary_get(kernel->programas,key);
			programa->estado =0;
			programa->exit_code = PROGRAM_DISCONNECT;
			pthread_mutex_unlock(&kernel->mutex_programas);
			buscarEnColaDesconectado(*id); // busca en cola de New
		}
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
	log_debug(logger,string_from_format("HILO recibirProgramas\n"));

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
						detectoDesconexion(i);
					}
					else {

						if (paquete->type == handshakeProgKernel)
							log_debug(logger,string_from_format("Handshake del socket %d, tamanio:%d\n",i,strlen(strdup(paquete->payload))));
						else
							log_debug(logger,string_from_format("Mensaje del socket %d, tamanio:%d\n",i,paquete->payloadLength));
						gestionarDatos(i,paquete);
					}
					destruir_paquete(paquete);
				}
			}
		} // for
	} // while
}

void hiloPLP(){

	cola_new = list_create();
	programasxfd = dictionary_create();
	sem_multiprogramacion = malloc(sizeof(sem_t));
	sem_new = malloc(sizeof(sem_t));
	sem_init(sem_new,0,0);
	sem_init(sem_multiprogramacion,0,kernel->multiprogramacion);
	// Conectarse a la UMV
	int resconn = conectarConUMV();

	if (resconn==0)
		printf("No se pudo conectar con la UMV"); //ver como tratamos errores.
	else
		log_debug(logger,string_from_format("Se conecto con UMV!\n"));
	int thr;
	pthread_t * progthr = malloc(sizeof(pthread_t)); // hilo q recibe programas
	thr = pthread_create( progthr, NULL, (void*)recibirProgramas, NULL);
	if (thr== 0)
		log_debug(logger,string_from_format("Se creo el hilo q recibe programas lo mas bien\n"));//se pudo crear el hilo
	else log_debug(logger,string_from_format("no se pudo crear el hilo que recibe programas\n"));//no se pudo crear el hilo

	pthread_t * multithr = malloc(sizeof(pthread_t)); // hilo q recibe programas
	thr = pthread_create( multithr, NULL, (void*)hiloMultiprogramacion, NULL);
	if (thr== 0)
		log_debug(logger,string_from_format("Se creo el hilo q controla multiprogramacion lo mas bien\n"));
	else log_debug(logger,string_from_format("no se pudo crear el hilo de multiprogramacion\n"));

	pthread_t * exitthr = malloc(sizeof(pthread_t)); // hilo q recibe programas
	thr = pthread_create( exitthr, NULL, (void*)hiloSacaExit, NULL);
	if (thr== 0)
		log_debug(logger,string_from_format("Se creo el hilo q saca de exit lo mas bien\n"));//se pudo crear el hilo
	else log_debug(logger,string_from_format("no se pudo crear el hilo que saca de exit\n"));//no se pudo crear el hilo


}
