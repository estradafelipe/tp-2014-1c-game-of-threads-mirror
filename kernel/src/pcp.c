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
#include <paquetes.h>
#include "colas.h"
#include <semaphore.h>
#include <pthread.h>
#include "hilos.h"
#include <commons/log.h>
#include <serializadores.h>

/* Crear cola ejecutando y disponibles para CPUs*/

t_dictionary *cpus;
t_cola *cpus_disponibles; // contendra los fd de las cpus con el id del PCB e id de cpu
sem_t *sem_cpu_disponible;
extern sem_t *sem_exit;
extern sem_t *sem_multiprogramacion;
extern sem_t *sem_estado_listo;
extern t_cola *cola_ready;
extern t_cola *cola_exit;
extern t_kernel *kernel;
//extern t_cola *cpus_disponibles; // contendra los fd de las cpus con el id del PCB e id de cpu
extern t_log *logger;

//t_cola *cpus_en_ejecucion=cola_create();

/*void integrarCPU(int fd){
	pthread_t * cpu = malloc(sizeof(pthread_t));
	if(!pthread_create( cpu, NULL, (void*)interactuarConCPU, fd))
		printf("Se creo el hilo q controla CPU lo mas bien\n");
	else printf("no se pudo crear el hilo de CPU\n");
}

void interactuarConCPU(){

}*/

void integrarCPU(uint32_t fd, uint32_t *fdMasGrande, fd_set *grupoFD){

	FD_SET(fd, grupoFD); // Añade al maestro, ¿lo agrega al conjunto del select read_fds?
	if (fd > *fdMasGrande)     // Actualiza máximo
		*fdMasGrande = fd;
}

void saludarCPU(uint32_t fd){
	char * payload = "Hola CPU";
	package *paquete = crear_paquete(handshakeKernelCPU,payload,strlen(payload)+1);
	enviar_paquete(paquete,fd);
	free(paquete);
	printf("CPU. Envie Hola\n");
}

void enviarQuantum(uint32_t fd){
	char * quantum_cadena = malloc(sizeof(int));
	memcpy(quantum_cadena, &kernel->quantum, sizeof(int));
	package *paquete = crear_paquete(handshakeKernelCPU,quantum_cadena,sizeof(int));
	enviar_paquete(paquete,fd);
	destruir_paquete(paquete);
	printf("CPU. Envie Quantum\n");
}

void imprimirMensajeDeCPU(char * payload, uint32_t longitud){
	//char * mensaje = deserializar_mensaje_excepcion(payload, longitud);
	printf("Pedido inicio conexión CPU: %s\n", payload);
	//free(mensaje);
}

void opHandshakeKernelCPU(uint32_t fd, char * payload, uint32_t longitudMensaje){
	printf("Mensaje de CPU: %d\n", fd);
	imprimirMensajeDeCPU(payload, longitudMensaje);
	//saludarCPU(fd);
	enviarQuantum(fd);
}

t_CPU *crearEstructuraCPU(uint32_t fd){
	t_CPU *cpu=malloc(sizeof(t_CPU));
	cpu->fd=fd;
	poner_cpu_no_disponible(cpu);
	return cpu;
}

void poner_cpu_no_disponible(t_CPU *cpu){
	printf("pone cpu no disponible\n");
	pthread_mutex_lock(&kernel->mutex_cpus);
	cpu->pcb=NULL;
	cpu->estado=CPU_NO_DISPONIBLE;
	pthread_mutex_unlock(&kernel->mutex_cpus);
}

void opRecibiACKDeCPU(uint32_t fd, char * payload, uint32_t longitudMensaje){
	printf("Recibi ACK de CPU\n");
	imprimirMensajeDeCPU(payload, longitudMensaje);
	t_CPU * cpu=crearEstructuraCPU(fd);
	char * clave = string_from_format("%d",cpu->fd);
	pthread_mutex_lock(&kernel->mutex_cpus);
	dictionary_put(cpus, clave, cpu);
	pthread_mutex_unlock(&kernel->mutex_cpus);
}

void opRespuestaCPU(uint32_t fd, char * payload, uint32_t longitudMensaje){
	if (!longitudMensaje)
		printf("Error al enviar PCB a CPU: %d\n", fd); //Si tamaño de payload == 0 => ERROR
		// que hacemos en este caso?
	else
		printf("Respuesta de CPU id %d\n", fd);

}

int enviar_pcb_a_cpu(t_PCB *pcb,t_CPU *cpu){
	printf("Enviar PCB a CPU\n");
	printf("envio paquete a cpu %d\n",cpu->fd);
	char * payload = serializarPCB(pcb);
	int payload_size = sizeof(t_pun)*9;
	package *paquete = crear_paquete(enviarPCBACPU, payload, payload_size);

	if (enviar_paquete(paquete,cpu->fd)==-1){
		printf("Error en envio de PCB a CPU: %d", cpu->fd);
		return ERROR_ENVIO_CPU;
	}
	//int resu = recibir_respuesta_envio_pcb_a_cpu(cpu);
	//if (!resu){
	//if (!recibir_respuesta_envio_pcb_a_cpu(cpu)){
	// habria que asignar el pcb y poner la cpu ocupada aunque aun no haya respondido
	// para no mandarle otro pcb
		cpu->pcb=pcb; //Poner en diccionario de PCBs que esta en ejecucion VER CON SILVINA
		cpu->estado=CPU_CON_PROCESO;
		return EXITO_ENVIO_PCB_A_CPU;
	//}
	destruir_paquete(paquete);

	return ERROR_RESPUESTA_CPU;
}

int recibir_respuesta_envio_pcb_a_cpu(t_CPU * cpu){
	package *paquete_recibido = recibir_paquete(cpu->fd);
	printf("recibir respuesta paquete cpu\n");
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

void modificarPCB(t_PCB *pcb, t_iPCBaCPU *datosPCB){
	pcb->indiceEtiquetas = datosPCB->indiceEtiquetas;
	pcb->programcounter = datosPCB->programcounter;
	pcb->sizeContext = datosPCB->sizeContext;
	pcb->cursorStack = datosPCB->cursorStack;
}

void opRetornoCPUQuantum(uint32_t fd, char * payload, uint32_t longitudMensaje){
	printf("Retorno de CPU por Quantum\n");
	t_iPCBaCPU *datosPCB = deserializarRetornoPCBdeCPU(payload);
	pthread_mutex_lock(&kernel->mutex_cpus);
	t_CPU *cpu = dictionary_get(cpus, string_from_format("%d",fd));
	pthread_mutex_unlock(&kernel->mutex_cpus);
	modificarPCB(cpu->pcb, datosPCB);
	printf("actualice el pcb\n");
	//AGREGAR SEMAFOROS en version con varios threads
	//pasarACola(cola_ready, cpu->pcb);
	cola_push(cola_ready, cpu->pcb);
	printf("pase a ready el pcb\n");
	poner_cpu_no_disponible(cpu);
	sem_post(sem_estado_listo);

}

void opEstoyDisponible(uint32_t fd, char * payload, uint32_t longitudMensaje){
	printf("Recibi EstoyDisponbile de la CPU\n");
	if (dictionary_has_key(cpus,string_from_format("%d",fd))){
		pthread_mutex_lock(&kernel->mutex_cpus);
		t_CPU *cpu = dictionary_get(cpus, string_from_format("%d",fd));
		cpu->estado=CPU_DISPONIBLE;
		pthread_mutex_unlock(&kernel->mutex_cpus);
		pasarACola(cpus_disponibles, cpu);
		//cola_push(cpus_disponibles,cpu);
		//sem_post(&cpus_disponibles->contador);
		sem_post(sem_cpu_disponible);
	}else printf("No guardo en el dictionary el fd de la cpu!\n");
}

t_iPCBaCPU * recibir_pcb_de_cpu(uint32_t fd){
	package *paquete_recibido = recibir_paquete(fd);
	if (paquete_recibido->type == respuestaCPU){
		if (paquete_recibido->payloadLength){
			printf("Error al enviar PCB a CPU: %d\n", fd); //Si tamaño de payload == 0 => ERROR
		}
		else{
			printf("Respuesta de CPU id %d\n", fd);
		}
	}
	t_iPCBaCPU *datosPCB = deserializarRetornoPCBdeCPU(paquete_recibido->payload);
	free(paquete_recibido);
	return datosPCB;
}

void mandarA_ES(t_entradasalida *dispositivo, t_PCB * pcb, int tiempo){
	t_progIO *es = malloc(sizeof(t_progIO));
	es->PCB = pcb;
	es->unidadesTiempo = tiempo;
	pasarACola(dispositivo->cola, es);
	sem_post(dispositivo->semaforo_IO); // Despierto el hilo
	free(es);
}

void opRetornoCPUPorES(uint32_t fd, char * payload, uint32_t longitudMensaje){
	printf("Retorno de CPU por E/S\n");
	t_iESdeCPU * datosES = deserializar_mensaje_ES(payload);
	t_iPCBaCPU * datosPCB = recibir_pcb_de_cpu(fd);
	t_CPU *cpu = dictionary_get(cpus, string_from_format("%d",fd));
	modificarPCB(cpu->pcb, datosPCB);
	poner_cpu_no_disponible(cpu);
	t_entradasalida * ES = dictionary_get(kernel->entradasalida, datosES->id);
	mandarA_ES(ES, cpu->pcb, datosES->tiempo);
}

void opRetornoCPUFin(uint32_t fd, char * payload, uint32_t longitudMensaje){
	printf("Retorno de CPU por Finalizacion\n");
	t_iPCBaCPU * datosPCB = recibir_pcb_de_cpu(fd);
	pthread_mutex_lock(&kernel->mutex_cpus);
	t_CPU *cpu = dictionary_get(cpus, string_from_format("%d",fd));
	pthread_mutex_unlock(&kernel->mutex_cpus);
	modificarPCB(cpu->pcb, datosPCB);
	pasarACola(cola_exit, cpu->pcb);
	poner_cpu_no_disponible(cpu);
	sem_post(sem_exit);
}

void opRetornoCPUExcepcion(uint32_t fd, char * payload, uint32_t longitudMensaje){
	printf("Retorno de CPU por Excepcion logica\n");
	//char * excepcion = deserializar_mensaje_excepcion(payload, longitudMensaje);
	pthread_mutex_lock(&kernel->mutex_cpus);
	t_CPU *cpu = dictionary_get(cpus, string_from_format("%d",fd));
	pthread_mutex_unlock(&kernel->mutex_cpus);
	printf("tengo cpu, tiene asignado el pcb: %d\n",cpu->pcb->id);
	if (dictionary_has_key(kernel->programas,string_from_format("%d",cpu->pcb->id))){
		pthread_mutex_lock(&kernel->mutex_programas);
		t_programa * programa = dictionary_get(kernel->programas, string_from_format("%d",cpu->pcb->id));
		pthread_mutex_unlock(&kernel->mutex_programas);

		//programa->mensajeFIN = malloc(sizeof(longitudMensaje));
		programa->mensajeFIN = payload;
		poner_cpu_no_disponible(cpu);
		cola_push(cola_exit,cpu->pcb);
		printf("pase a exit el pcb");
		sem_post(sem_exit);

	}
	else
		printf("no estaba en el diccionario WTF\n");

}

void opExcepcionCPUHardware(uint32_t fd){
	printf("EX HARD Retorno de CPU por Excepcion Hardware\n");
	if (dictionary_has_key(cpus,string_from_format("%d",fd))){
		pthread_mutex_lock(&kernel->mutex_cpus);
		t_CPU * cpu = dictionary_remove(cpus, string_from_format("%d",fd));
		pthread_mutex_unlock(&kernel->mutex_cpus);
		printf("EX HARD Quite cpu del diccionario cpu\n");
		if (cpu->estado == CPU_CON_PROCESO){
			if (dictionary_has_key(kernel->programas,string_from_format("%d",cpu->pcb->id))){
				pthread_mutex_lock(&kernel->mutex_programas);
				t_programa * programa = dictionary_get(kernel->programas, string_from_format("%d",cpu->pcb->id));
				pthread_mutex_unlock(&kernel->mutex_programas);
				programa->mensajeFIN="Error CPU"; // DONDE se pone el mensaje de finalizacion de programa SILVINA
				printf("EX HARD actualice el programa\n");
				pasarACola(cola_exit, cpu->pcb);
				printf("EX HARD pase pcb a exit\n");
				sem_post(sem_exit);
			}
		}
	}
}

void pasar_dato_a_imprimir(char * texto, int longitudTexto, uint32_t fd, t_paquete tipoPaquete){
	package *paquete = crear_paquete(tipoPaquete, texto, longitudTexto); //agregar a enum de tipos de mensaje
	if (enviar_paquete(paquete,fd)==-1){
		printf("Error en envio de Texto a imprimir: %d", fd);
	}
	free(paquete);
}

void opImprimirValor(uint32_t fd, char * payload, uint32_t longitudMensaje){
	printf("Imprimir Valor \n");
	t_CPU *cpu = dictionary_get(cpus, string_from_format("%d",fd));
	t_programa * programa = dictionary_get(kernel->programas, string_from_format("%d",cpu->pcb->id));
	int valor;
	memcpy(&valor,payload,sizeof(int32_t));
	printf("valor %d\n",valor);


	pasar_dato_a_imprimir(payload, longitudMensaje, programa->fd, imprimirValor);// VER Con Silvina ¿imprime el PCP?
}

void opImprimirTexto(uint32_t fd, char * payload, uint32_t longitudMensaje){
	printf("Imprimir Texto\n");
	t_CPU *cpu = dictionary_get(cpus, string_from_format("%d",fd));
	t_programa * programa = dictionary_get(kernel->programas, string_from_format("%d", cpu->pcb->id));
	pasar_dato_a_imprimir(payload, longitudMensaje, programa->fd, imprimirTexto);// VER Con Silvina ¿imprime el PCP?
}

void opTomarSemaforo(uint32_t fd, char * payload, uint32_t longitudMensaje){
	printf("Tomar semaforo\n");
	char * nombre_semaforo = deserializar_nombre_recurso(payload, longitudMensaje);
	wait_semaforo(nombre_semaforo, fd);
}

void opLiberarSemaforo(uint32_t fd, char * payload, uint32_t longitudMensaje){
	printf("Liberar semaforo\n");
	char * nombre_semaforo = deserializar_nombre_recurso(payload, longitudMensaje);
	signal_semaforo(nombre_semaforo);
}

char * serializar_valor_variable_compartida(uint32_t valor){
    char *stream = malloc(sizeof(uint32_t));
    memcpy(stream, &valor, sizeof(uint32_t));
    return stream;
}

void opSolicitarValorVariableCompartida(uint32_t fd, char * payload, uint32_t longitudMensaje){
	printf("Solicitud de variable\n");
	char * nombre_variable = deserializar_nombre_recurso(payload, longitudMensaje);
	t_variable_compartida * variable_compartida = dictionary_get(kernel->variables_compartidas, string_from_format("%d",nombre_variable));
	pthread_mutex_lock(variable_compartida->mutex);
	char * valor_variable_compartida = serializar_valor_variable_compartida(variable_compartida->valor);
	package *paquete = crear_paquete(solicitarValorVariableCompartida, valor_variable_compartida, sizeof(uint32_t)); //agregar a enum de tipos de mensaje
	if (enviar_paquete(paquete,fd)==-1){
		printf("Error en envio de Valor a imprimir: %d", fd);
	} else {
		printf("Envio de Valor a imprimir: %d", fd);
	}
	pthread_mutex_lock(variable_compartida->mutex);
	free(paquete);
}

void opAsignarValorVariableCompartida(uint32_t fd, char * payload, uint32_t longitudMensaje){
	printf("Guardar valor en variable\n");
	t_iVARCOM * variable = deserializar_datos_variable(payload, longitudMensaje);
	t_variable_compartida * variable_compartida = dictionary_get(kernel->variables_compartidas, string_from_format("%d",variable->nombre));
	pthread_mutex_lock(variable_compartida->mutex);
	variable_compartida->valor = variable->valor;
	char * valor_variable_compartida = serializar_valor_variable_compartida(variable_compartida->valor);
	package *paquete = crear_paquete(asignarValorVariableCompartida, valor_variable_compartida, sizeof(uint32_t)); //agregar a enum de tipos de mensaje
	if (enviar_paquete(paquete,fd)==-1){
		printf("Error en envio de Valor a imprimir: %d", fd);
	} else {
		printf("Envio de Valor a imprimir: %d", fd);
	}
	pthread_mutex_lock(variable_compartida->mutex);
	free(paquete);
}

void pasarACola(t_cola* cola, void *element){
	cola_push(cola,element);
}

void (*tabla_operaciones[])(uint32_t, char *, uint32_t) = {
		opHandshakeKernelCPU,
		opRetornoCPUQuantum,
		opRecibiACKDeCPU,
		opSolicitarValorVariableCompartida,
		opAsignarValorVariableCompartida,
		opLiberarSemaforo,
		opTomarSemaforo,
		opImprimirTexto,
		opImprimirValor,
		opRetornoCPUExcepcion,
		opRetornoCPUFin,
		opRetornoCPUPorES,
		opEstoyDisponible,
		opRespuestaCPU,
	};

/*  Hilo que recibe los cpu (select) */
void recibirCPU(void){
	uint32_t nuevofd,nbytes;
	int32_t i;
	fd_set master;  // Conjunto maestro de descriptores de fichero
	fd_set read_fds;// Conjunto temporal de descriptores de fichero para select()
	FD_ZERO(&master); // Vacia el descriptor maestro
	FD_ZERO(&read_fds); // Vacia el descriptor temporal
	struct sockaddr_in direccion_cliente; // Dirección del cliente

	printf("HILO recibirCPUs\n");

	int socket_escucha = abrir_socket();
	vincular_socket(socket_escucha,kernel->puertocpu);
	escuchar_socket(socket_escucha);
	FD_SET(socket_escucha, &master); // Agrega socket_escucha al conjunto maestro de FD
	uint32_t fdmax = socket_escucha; // Inicializa fdmax, numero de FD mas grande,
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
						integrarCPU(nuevofd, &fdmax, &master); //Utilizando un solo Hilo con un Select
						//integrarCPU(nuevofd); //Utilizando un Hilo para cada CPU
						printf("selectserver: new connection from %s on "
								"socket %d\n", inet_ntoa(direccion_cliente.sin_addr), nuevofd);
					} // accept
				} else {
	                // gestionar datos de un cliente
					paquete = recibir_paquete(i);
					nbytes = paquete->payloadLength;
					printf("Hilo que recibe CPU! tipo de paquete %d, mensaje %s\n",paquete->type,paquete->payload);

					if (nbytes == 0) {
						// conexión cerrada por el cliente
						printf("selectserver: socket %d hung up\n", i);
						close(i);
						FD_CLR(i, &master); // Elimina del conjunto maestro, ¿saca de la copia del read_fs?
						opExcepcionCPUHardware(i);
					}
					else {
						if (dictionary_has_key(cpus,string_from_format("%d",i))) {
							pthread_mutex_lock(&kernel->mutex_cpus);
							t_CPU *cpu = dictionary_get(cpus, string_from_format("%d",i));
							pthread_mutex_unlock(&kernel->mutex_cpus);
							if (cpu->pcb!=NULL){
								t_programa * programa = dictionary_get(kernel->programas, string_from_format("%d",cpu->pcb->id));
								if (!programa->estado){ //Verifico  si el programa esta activo
									pasarACola(cola_exit, cpu->pcb);
									sem_post(sem_exit);
								} else {
									(tabla_operaciones[paquete->type])(i, paquete->payload, nbytes);
								}
							} else (tabla_operaciones[paquete->type])(i, paquete->payload, nbytes);
						} else (tabla_operaciones[paquete->type])(i, paquete->payload, nbytes);
					}
					free(paquete);
				}
			}
		} // for
	} // while
}


void pasarListosAEjecucion(void){
	while(1){
		//sem_wait(&cpus_disponibles->contador);
		sem_wait(sem_cpu_disponible);
		sem_wait(sem_estado_listo);
		log_debug(logger,string_from_format("Hilo pasa PCB a Ejecución\n"));
		t_PCB *pcb = cola_pop(cola_ready);
		t_programa * programa = dictionary_get(kernel->programas, string_from_format("%d",pcb->id));

		if (!programa->estado){
			log_debug(logger,string_from_format("Hilo pasa PCB a Ejecución, No existe el programa no va a ejecucion\n"));
			pasarACola(cola_exit, pcb);
			sem_post(sem_exit);
			sem_post(sem_multiprogramacion);
			//sem_post(&cpus_disponibles->contador);
			sem_post(sem_cpu_disponible);
		} else {
			log_debug(logger,string_from_format("Hilo pasa PCB a Ejecución, Existe el programa VA a ejecucion\n"));
			t_CPU *cpu = cola_pop(cpus_disponibles);
			enviar_pcb_a_cpu(pcb,cpu);
		}
	}
}

void hiloPCP(){
	int thr;
	cpus =dictionary_create();
	cpus_disponibles = cola_create();
	sem_cpu_disponible=malloc(sizeof(sem_t));
	sem_init(sem_cpu_disponible,0,0);
	pthread_t * administra_cpus_thr = malloc(sizeof(pthread_t)); // Administra CPUs
	thr = pthread_create( administra_cpus_thr, NULL, (void*)recibirCPU, NULL);
	if (thr== 0)
		log_debug(logger,string_from_format("Hilo que recibe CPUs creado correctamente\n"));
	else log_debug(logger,string_from_format("Hilo que recibe CPUs no se pudo crear\n"));

	pthread_t * pasa_a_ejecucion_thr = malloc(sizeof(pthread_t)); // hilo q recibe programas
	thr = pthread_create( pasa_a_ejecucion_thr, NULL, (void*)pasarListosAEjecucion, NULL);
	if (thr== 0)
		log_debug(logger,string_from_format("Hilo que pone en ejecucion PCBs creado correctamente\n"));
	else log_debug(logger,string_from_format("Hilo que pone en ejecucion PCBs no se pudo crear\n"));
}
