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

#define ERROR_ESCRITURA_CPU -1
#define EXITO_ENVIO_PCB_A_CPU 0
#define ERROR_RESPUESTA_CPU -2
#define CPU_NO_DISPONIBLE -1
#define CPU_DISPONIBLE NULL

/* Crear cola ejecutando y disponibles para CPUs*/

t_dictionary *cpus;
extern t_cola *cola_ready;
extern t_cola *cola_exit;
extern t_kernel *kernel;
t_cola *cpus_disponibles=cola_create();
//t_cola *cpus_en_ejecucion=cola_create();

char * entero_a_cadena(uint32_t numero){
        char *cadena=malloc(sizeof(uint32_t));
        sprintf(cadena, "%d", numero);
        return cadena;
}

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
						(tabla_operaciones[paquete->type])(i,paquete->payload);
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

void opHandshakeKernelCPU(int fd){
	printf("Recibi handshake de CPU\n");
	saludarCPU(fd);
	enviarQuantum(fd);
}

t_CPU *crearEstructuraCPU(int fd){
	t_CPU *cpu=malloc(sizeof(t_CPU));
/*	char * cadena=malloc(3+sizeof(uint32_t)+1);
	strcat(cadena, "cpu");
	strcat(cadena, entero_a_cadena(fd));
	cpu->id=dictionary_hash(cadena, strlen(cadena));*/
	cpu->fd=fd;
	cpu->id_pcb=NULL;
	return cpu;
}

void opRecibiACKDeCPU(int fd){
	printf("Recibi ACK de CPU\n");
	t_CPU cpu=crearEstructuraCPU(fd);
	char * clave = string_from_format("%d",cpu->fd);
	cpu->id_pcb=CPU_NO_DISPONIBLE;
	dictionary_put(cpus,clave, cpu);
}

int enviar_pcb_a_cpu(t_PCB pcb, t_CPU cpu){
	char * payload = serializar_pcb(pcb);
	int payload_size = sizeof(payload);
	package *paquete = crear_paquete(enviarPCBACPU,payload,payload_size);
	int resultado = enviar_paquete(paquete,cpu->fd);
	free(paquete);
	free(payload);
	if (resultado==-1){
		printf("Error en envio de PCB a CPU: %d", cpu->fd);
		return ERROR_ESCRITURA_CPU;
	}
	if (recibir_respuesta_envio_pcb_a_cpu(cpu)){
		cpu->id_pcb=pcb->id;
		//Poner en diccionario de PCBs que esta en ejecucion VER CON SILVINA
		return EXITO_ENVIO_PCB_A_CPU;
	}
	return ERROR_RESPUESTA_CPU;
}

int recibir_respuesta_envio_pcb_a_cpu(t_CPU cpu){
	t_puntero respuesta;
	package *paquete_recibido = recibir_paquete(cpu->fd);
	if (paquete_recibido->type == respuestaCPU){
		memcpy(&bytesEscritos,paquete_recibido->payload, sizeof(t_puntero));
		if (respuesta==-1) printf("Error al enviar PCB a CPU: %d\n", cpu->fd);
		else printf("Respuesta de CPU id %d: %d\n", cpu->fd, respuesta);
	}
	free(paquete_recibido);
	return respuesta;
}

void opRetornoCPUQuantum(int fd, char *pcb){
	printf("Retorno de CPU por Quantum\n");
	t_CPU *cpu = dictionary_get(cpus, string_from_format("%d",fd));
	cpu->id_pcb=CPU_NO_DISPONIBLE;
	//AGREGAR SEMAFOROS
	pasarACola(cola_ready, pcb);
}

void opEstoyDisponible(int fd){
	t_CPU *cpu = dictionary_get(cpus, string_from_format("%d",fd));
	cpu->id_pcb=CPU_DISPONIBLE;
	pasarACola(cpus_disponibles, cpu);
}



void opRetornoCPUPorES(int fd, char *pcb){
	printf("Retorno de CPU por E/S\n");
	mandarA_ES(paquete);
}

void opRetornoCPUFin(int fd, char *pcb){
	printf("Retorno de CPU por Finalizacion\n");
	mandarAColaSalida(paquete);
}

void opRetornoCPUExcepcion(int fd, char *pcb){
	printf("Retorno de CPU por Excepcion logica\n");
	mandarAFinalizarProgramaExcLogica(paquete);
}

void opExcepcionCPUHardware(int fd, char *pcb){
	printf("Retorno de CPU por Excepcion Hardware\n");
	mandarAFinalizarProgramaExcHardware(paquete);
}

void opImprimirValor(int fd, char *valor){
	printf("Retorno de CPU por Excepcion Hardware\n");
	mandarAPLPImprimirValorEnConsola(valor);
}

void opImprimirTexto(int fd, char *texto){
	printf("Retorno de CPU por Excepcion Hardware\n");
	mandarAPLPImprimirTextoEnConsola(texto);
}

void opTomarSemaforo(int fd, char *semaforo){
	printf("Retorno de CPU por Excepcion Hardware\n");
	mandarABloquearSemaforo(semaforo);
}

void opLiberarSemaforo(int fd, char *semaforo){
	printf("Retorno de CPU por Excepcion Hardware\n");
	mandarALiberarSemaforo(semaforo);
}

void mandarAColaSalida(package *paquete){
	desserializarPCB;
	pasarA(EXIT, element);
	sem_post(sem_exit);
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
