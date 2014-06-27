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
#include "colas.h"
#include <semaphore.h>
#include <pthread.h>

/* Crear cola ejecutando y disponibles para CPUs*/

t_dictionary *cpus;
extern t_cola *cola_ready;
extern t_cola *cola_exit;
extern t_kernel *kernel;
t_cola *cpus_disponibles=cola_create();
//t_cola *cpus_en_ejecucion=cola_create();

/*  Hilo que recibe los cpu (select) */
void recibirCPU(void){
	int nuevofd,nbytes;
	int32_t i;
	fd_set master;  // Conjunto maestro de descriptores de fichero
	fd_set read_fds;// Conjunto temporal de descriptores de fichero para select()
	FD_ZERO(&master); // Vacia el descriptor maestro
	FD_ZERO(&read_fds); // Vacia el descriptor temporal
	struct sockaddr_in direccion_cliente; // Dirección del cliente

	void (*tabla_operaciones[])(int, char *) = {
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
					if ((nuevofd = accept(socket_escucha, (struct sockaddr *)&direccion_cliente, &addrlen)) == -1)
						perror("accept");
					else {
						integrarCPU(nuevofd, fdmax, &master); //Utilizando un solo Hilo con un Select
						//integrarCPU(nuevofd); //Utilizando un Hilo para cada CPU
						printf("selectserver: new connection from %s on "
								"socket %d\n", inet_ntoa(direccion_cliente.sin_addr), nuevofd);
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
						(tabla_operaciones[paquete->type])(i, paquete->payload, nbytes);
					}
					free(paquete);
				}
			}
		} // for
	} // while
}

/*void integrarCPU(int fd){
	pthread_t * cpu = malloc(sizeof(pthread_t));
	if(!pthread_create( cpu, NULL, (void*)interactuarConCPU, fd))
		printf("Se creo el hilo q controla CPU lo mas bien\n");
	else printf("no se pudo crear el hilo de CPU\n");
}

void interactuarConCPU(){

}*/

void integrarCPU(int fd, int *fdMasGrande, fd_set *grupoFD){
	FD_SET(fd, grupoFD); // Añade al maestro, ¿lo agrega al conjunto del select read_fds?
	if (fd > *fdMasGrande)     // Actualiza máximo
		*fdMasGrande = fd;
}

void saludarCPU(int fd){
	char * payload = "Hola CPU";
	package *paquete = crear_paquete(handshakeKernelCPU,payload,strlen(payload)+1);
	enviar_paquete(paquete,fd);
	free(paquete);
}

void enviarQuantum(int fd){
	package *paquete = crear_paquete(handshakeKernelCPU,kernel->quantum,sizeof(kernel->quantum));
	enviar_paquete(paquete,fd);
	free(paquete);
}

void imprimirMensajeDeCPU(char * payload, int longitud){
	char * mensaje=malloc(longitud +1);
	memcpy(mensaje, payload, longitud);
	memcpy(mensaje+longitud, "\0", 1);
	printf("Pedido inicio conexión CPU: %s\n", mensaje);
	free(mensaje);
}

void opHandshakeKernelCPU(int fd, char * payload, int longitudMensaje){
	printf("Mensaje de CPU: %d\n", fd);
	imprimirMensajeDeCPU(payload, longitudMensaje);
	saludarCPU(fd);
	enviarQuantum(fd);
}

t_CPU *crearEstructuraCPU(int fd){
	t_CPU *cpu=malloc(sizeof(t_CPU));
	cpu->fd=fd;
	poner_cpu_no_disponible(cpu);
	return cpu;
}

void poner_cpu_no_disponible(t_CPU cpu){
	cpu->pcb=NULL;
	cpu->estado=CPU_NO_DISPONIBLE;
}

void opRecibiACKDeCPU(int fd, char * payload, int longitudMensaje){
	printf("Recibi ACK de CPU\n");
	imprimirMensajeDeCPU(payload, longitudMensaje);
	t_CPU cpu=crearEstructuraCPU(fd);
	char * clave = string_from_format("%d",cpu->fd);
	dictionary_put(cpus, clave, cpu);
}

int enviar_pcb_a_cpu(t_PCB pcb, t_CPU cpu){
	char * payload = serializar_datos_pcb_para_cpu(pcb);
	int payload_size = sizeof(payload);
	package *paquete = crear_paquete(enviarPCBACPU,payload,payload_size);
	if (enviar_paquete(paquete,cpu->fd)==-1){
		printf("Error en envio de PCB a CPU: %d", cpu->fd);
		return ERROR_ENVIO_CPU;
	}
	if (!recibir_respuesta_envio_pcb_a_cpu(cpu)){
		cpu->pcb=pcb; //Poner en diccionario de PCBs que esta en ejecucion VER CON SILVINA
		cpu->estado=1;
		return EXITO_ENVIO_PCB_A_CPU;
	}
	free(paquete);
	free(payload);
	return ERROR_RESPUESTA_CPU;
}

int recibir_respuesta_envio_pcb_a_cpu(t_CPU cpu){
	package *paquete_recibido = recibir_paquete(cpu->fd);
	int retorno;
	if (paquete_recibido->type == respuestaCPU){
		if (paquete_recibido->payloadLength){
			printf("Error al enviar PCB a CPU: %d\n", cpu->fd); //Si tamaño de payload == 0 => ERROR
			retorno = CPU_NO_CONFIRMA_RECEPCION_PCB;
		}
		else{
			printf("Respuesta de CPU id %d\n", cpu->fd);
			retorno = CPU_CONFIRMA_RECEPCION_PCB;
		}
	}
	free(paquete_recibido);
	return retorno;
}

t_PCB * modificarPCB(t_PCB pcb, t_iPCBaCPU datosPCB){
	pcb->indiceEtiquetas = datosPCB->indiceEtiquetas;
	pcb->programcounter = datosPCB->programcounter;
	return pcb;
}

void opRetornoCPUQuantum(int fd, char * payload, int longitudMensaje){
	printf("Retorno de CPU por Quantum\n");
	t_iPCBaCPU *datosPCB = deserializarRetornoPCBdeCPU(payload);
	t_CPU *cpu = dictionary_get(cpus, string_from_format("%d",fd));
	t_PCB *pcb = modificarPCB(cpu->pcb, datosPCB);
	poner_cpu_no_disponible(cpu);
	//AGREGAR SEMAFOROS en version con varios threads
	pasarACola(cola_ready, pcb);
}

void opEstoyDisponible(int fd, char * payload, int longitudMensaje){
	t_CPU *cpu = dictionary_get(cpus, string_from_format("%d",fd));
	cpu->estado=CPU_DISPONIBLE;
	pasarACola(cpus_disponibles, cpu);
}

t_iPCBaCPU * recibir_pcb_de_cpu(int fd){
	package *paquete_recibido = recibir_paquete(fd);
	int estado;
	if (paquete_recibido->type == envioPCBES){
		t_iPCBaCPU *datosPCB = deserializarRetornoPCBdeCPU(paquete_recibido->payload);
		free(paquete_recibido);
		return datosPCB;
	}
	free(paquete_recibido);
	return ERROR_RECEPCION_PCB;
}

void mandarA_ES(t_entradasalida dispositivo, t_PCB * pcb, int tiempo){
	t_progIO * es = malloc(t_progIO);
	es->PCB = pcb;
	es->unidadesTiempo = tiempo;
	pasarACola(dispositivo->cola, es);
	sem_post(&dispositivo->semaforo_IO); // Despierto el hilo
	free(es);
}

void opRetornoCPUPorES(int fd, char * payload, int longitudMensaje){
	printf("Retorno de CPU por E/S\n");
	t_iESdeCPU * datosES = deserializar_mensaje_ES(payload);
	t_iPCBaCPU * datosPCB = recibir_pcb_de_cpu(fd);
	t_CPU *cpu = dictionary_get(cpus, string_from_format("%d",fd));
	t_PCB *pcb = modificarPCB(cpu->pcb, datosPCB);
	poner_cpu_no_disponible(cpu);
	t_entradasalida * ES = dictionary_get(kernel->entradasalida, datosES->id);
	mandarA_ES(ES, pcb, datosES->tiempo);
}

void opRetornoCPUFin(int fd, char * payload, int longitudMensaje){
	printf("Retorno de CPU por Finalizacion\n");
	t_iPCBaCPU * datosPCB = recibir_pcb_de_cpu(fd);
	t_CPU *cpu = dictionary_get(cpus, string_from_format("%d",fd));
	t_PCB *pcb = modificarPCB(cpu->pcb, datosPCB);
	poner_cpu_no_disponible(cpu);
	pasarACola(cola_exit, pcb);
	sem_post(sem_exit);
}

void opRetornoCPUExcepcion(int fd, char * payload, int longitudMensaje){
	printf("Retorno de CPU por Excepcion logica\n");
	char * excepcion = deserializar_mensaje_excepcion(payload);
	imprimir_mensaje_excepcion(excepcion);// DONDE se pone el mensaje de finalizacion de programa SILVINA
	poner_cpu_no_disponible(cpu);
	pasarACola(cola_exit, cpu->pcb);
	sem_post(sem_exit);
}

void opExcepcionCPUHardware(int fd){
	printf("Retorno de CPU por Excepcion Hardware\n");
	char * excepcion = "Error CPU";
	imprimir_mensaje_excepcion(excepcion);// DONDE se pone el mensaje de finalizacion de programa SILVINA
	t_CPU *cpu = dictionary_get(cpus, string_from_format("%d",fd));
	poner_cpu_no_disponible(cpu);
	pasarACola(cola_exit, cpu->pcb);
	sem_post(sem_exit);
}

void pasar_valor_a_imprimir(char * valor, int longitudValor, int fd){
	package *paquete = crear_paquete(imprimirValor,valor,longitudValor); //agregar a enum de tipos de mensaje
	if (enviar_paquete(paquete,fd)==-1){
		printf("Error en envio de Valor a imprimir: %d", fd);
	}
	free(paquete);
}

void pasar_valor_a_imprimir(char * texto, int longitudTexto, int fd){
	package *paquete = crear_paquete(imprimirTexto,texto,longitudTexto); //agregar a enum de tipos de mensaje
	if (enviar_paquete(paquete,fd)==-1){
		printf("Error en envio de Texto a imprimir: %d", fd);
	}
	free(paquete);
}

void opImprimirValor(int fd, char * payload, int longitudMensaje){
	printf("Imprimir Valor\n");
	t_CPU *cpu = dictionary_get(cpus, string_from_format("%d",fd));
	t_PCB *pcb = cpu->pcb;
	t_programa * programa = dictionary_get(kernel->programas, string_from_format("%d",pcb->id));
	pasar_valor_a_imprimir(payload, longitudMensaje, programa->fd);// VER Con Silvina ¿imprime el PCP?
}

void opImprimirTexto(int fd, char * payload, int longitudMensaje){
	printf("Imprimir Texto\n");
	t_CPU *cpu = dictionary_get(cpus, string_from_format("%d",fd));
	t_PCB *pcb = cpu->pcb;
	t_programa * programa = dictionary_get(kernel->programas, string_from_format("%d",pcb->id));
	pasar_mensaje_a_imprimir(payload, longitudMensaje, programa->fd);// VER Con Silvina ¿imprime el PCP?
}

void opTomarSemaforo(int fd, char * payload, int longitudMensaje){
	printf("Tomar semaforo\n");
	char * nombre_semaforo = deserializar_nombre_semaforo(payload, longitudMensaje);
	wait_semaforo(nombre_semaforo, fd);
}

char * deserializar_nombre_semaforo(char * mensaje, int longitud){
	char * semaforo = malloc(longitud);
	memcpy(semaforo, mensaje, longitud);
	free(semaforo);
	return semaforo;
}

void opLiberarSemaforo(int fd, char * payload, int longitudMensaje){
	printf("Liberar semaforo\n");
	char * semaforo = deserializar_nombre_semaforo(payload, longitudMensaje);
	signal_semaforo(semaforo);
}

void opObtenerVariable(int fd, char * payload, int longitudMensaje){
	printf("Solicitud de variable\n");


}

void opGrabarVariable(int fd, char * payload, int longitudMensaje){
	printf("Guardar valor en variable\n");

}

void pasarACola(t_cola* cola, t_PCB *element){
	cola_push(cola,element);
}

void hiloPCP(){

	t_cola *cpus_disponibles = cola_create(); // contendra los fd de las cpus con el id del PCB e id de cpu


	// hilo que reciba CPU
	// hilo que pase ready->exec
	// hilo que pase exec->exit
}
