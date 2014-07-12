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

//t_dictionary *cpus;
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
	destruir_paquete(paquete);
}

void enviarQuantum(uint32_t fd){
	char * quantum_cadena = malloc(sizeof(int));
	memcpy(quantum_cadena, &kernel->quantum, sizeof(int));
	package *paquete = crear_paquete(handshakeKernelCPU,quantum_cadena,sizeof(int));
	enviar_paquete(paquete,fd);
	destruir_paquete(paquete);
}

void imprimirMensajeDeCPU(char * payload, uint32_t longitud){
	printf("Pedido inicio conexión CPU: %s\n", payload);
}

void opHandshakeKernelCPU(uint32_t fd, char * payload, uint32_t longitudMensaje){
	//printf("Mensaje de CPU: %d\n", fd);
	//imprimirMensajeDeCPU(payload, longitudMensaje);
	enviarQuantum(fd);
}

t_CPU *crearEstructuraCPU(uint32_t fd){
	t_CPU *cpu=malloc(sizeof(t_CPU));
	cpu->fd=fd;
	poner_cpu_no_disponible(cpu);
	return cpu;
}

void poner_cpu_no_disponible(t_CPU *cpu){
//	printf("pone cpu no disponible\n");
	pthread_mutex_lock(&kernel->mutex_cpus);
	cpu->pcb=NULL;
	cpu->estado=CPU_NO_DISPONIBLE;
	pthread_mutex_unlock(&kernel->mutex_cpus);
}

void opRecibiACKDeCPU(uint32_t fd, char * payload, uint32_t longitudMensaje){
	imprimirMensajeDeCPU(payload, longitudMensaje);
	t_CPU * cpu=crearEstructuraCPU(fd);
	char * clave = string_from_format("%d",cpu->fd);
	pthread_mutex_lock(&kernel->mutex_cpus);
	dictionary_put(kernel->cpus, clave, cpu);
	pthread_mutex_unlock(&kernel->mutex_cpus);
}

void opRespuestaCPU(uint32_t fd, char * payload, uint32_t longitudMensaje){
	if (!longitudMensaje)
		printf("Error al enviar PCB a CPU: %d\n", fd); //Si tamaño de payload == 0 => ERROR
		// que hacemos en este caso?


}

int enviar_pcb_a_cpu(t_PCB *pcb,t_CPU *cpu){
	//printf("Enviar PCB a CPU\n");
	printf("Envio de Programa %d a Cpu %d\n",pcb->id, cpu->fd);
	char * payload = serializarPCB(pcb);
	uint16_t payload_size = sizeof(t_pun)*9;
	package *paquete = crear_paquete(enviarPCBACPU, payload, payload_size);
	int resu = enviar_paquete(paquete,cpu->fd);
	destruir_paquete(paquete);
	if (resu==-1){
		printf("Error en envio de PCB a CPU: %d", cpu->fd);
		return ERROR_ENVIO_CPU;
	}
	// hay que asignar el pcb y poner la cpu ocupada aunque aun no haya respondido
	// para no mandarle otro pcb
	pthread_mutex_lock(&kernel->mutex_cpus);
	cpu->pcb=pcb; //Poner en diccionario de PCBs que esta en ejecucion VER CON SILVINA
	cpu->estado=CPU_CON_PROCESO;
	//printf("guarde en la cpu %d, el pcb %d. Estado %d\n",cpu->fd,cpu->pcb->id,cpu->estado);
	pthread_mutex_unlock(&kernel->mutex_cpus);
	return EXITO_ENVIO_PCB_A_CPU;
}

int recibir_respuesta_envio_pcb_a_cpu(t_CPU * cpu){
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

void modificarPCB(t_PCB *pcb, t_iPCBaCPU *datosPCB){
	pcb->indiceEtiquetas = datosPCB->indiceEtiquetas;
	pcb->programcounter = datosPCB->programcounter;
	pcb->sizeContext = datosPCB->sizeContext;
	pcb->cursorStack = datosPCB->cursorStack;
}

void opRetornoCPUQuantum(uint32_t fd, char * payload, uint32_t longitudMensaje){
	//printf("Retorno de CPU por Quantum\n");
	t_iPCBaCPU *datosPCB = deserializarRetornoPCBdeCPU(payload);
	pthread_mutex_lock(&kernel->mutex_cpus);
	t_CPU *cpu = dictionary_get(kernel->cpus, string_from_format("%d",fd));
	pthread_mutex_unlock(&kernel->mutex_cpus);
	if(cpu->pcb!=NULL)
		modificarPCB(cpu->pcb, datosPCB);
	else
		printf("La cpu no tenia PCB WTF!!\n");
	//printf("datosPCB actualizado id %d, indice %d, pc %d, sizecontext %d, cursor %d\n",cpu->pcb->id, cpu->pcb->indiceEtiquetas, cpu->pcb->programcounter, cpu->pcb->sizeContext, cpu->pcb->cursorStack);
	cola_push(cola_ready, cpu->pcb);
	printf("Programa %d pasa a Ready\n",cpu->pcb->id);
	poner_cpu_no_disponible(cpu);
	sem_post(sem_estado_listo);
	package *respuesta = crear_paquete(respuestaCPU, "respuesta",9);
	enviar_paquete(respuesta,fd);
}

void opEstoyDisponible(uint32_t fd, char * payload, uint32_t longitudMensaje){
	//printf("Recibi EstoyDisponbile de la CPU\n");
	if (dictionary_has_key(kernel->cpus,string_from_format("%d",fd))){
		pthread_mutex_lock(&kernel->mutex_cpus);
		t_CPU *cpu = dictionary_get(kernel->cpus, string_from_format("%d",fd));
		cpu->estado=CPU_DISPONIBLE;
		pthread_mutex_unlock(&kernel->mutex_cpus);
		//printf("cambie estado de la cpu\n");
		cola_push(cpus_disponibles, cpu);
		//printf("pase a cola de disponibles\n");
		//printf("post al semaforo de cpu disponible\n");
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
	destruir_paquete(paquete_recibido);
	return datosPCB;
}

void mandarA_ES(t_entradasalida *dispositivo, t_PCB * pcb, int tiempo){
	t_progIO *es = malloc(sizeof(t_progIO));
	es->PCB = pcb;
	es->unidadesTiempo = tiempo;
	cola_push(dispositivo->cola, es);
	printf("Mando a ejecutar %s, %d de tiempo\n",dispositivo->id,tiempo);
	sem_post(dispositivo->semaforo_IO); // Despierto el hilo
	//free(es);
}

void opRetornoCPUPorES(uint32_t fd, char * payload, uint32_t longitudMensaje){
	//printf("Retorno de CPU por E/S tamanio %d\n",longitudMensaje);
	t_iESdeCPU * datosES = deserializar_mensaje_ES(payload,longitudMensaje);
	char *streamPCB=malloc(sizeof(t_pun)*5);
	int tamanioDatosES = datosES->tamanioID + sizeof(int32_t)*2;
	memcpy(streamPCB,payload+tamanioDatosES,sizeof(t_pun)*5);
	t_iPCBaCPU *datosPCB = deserializarRetornoPCBdeCPU(streamPCB);

	pthread_mutex_lock(&kernel->mutex_cpus);
	t_CPU *cpu = dictionary_get(kernel->cpus, string_from_format("%d",fd));
	pthread_mutex_unlock(&kernel->mutex_cpus);

	modificarPCB(cpu->pcb, datosPCB);

	//printf("Dispositivo: %s**\n",datosES->id);
	package *paquete;
	if (dictionary_has_key(kernel->entradasalida,datosES->id)){
		t_entradasalida * ES = dictionary_get(kernel->entradasalida, datosES->id);
		mandarA_ES(ES, cpu->pcb, datosES->tiempo);
		paquete = crear_paquete(bloquearProgramaCPU,"algo",4);
		enviar_paquete(paquete,fd);
		poner_cpu_no_disponible(cpu);
	} else
		printf("No se encontro el dispositivo\n");
}

void opRetornoCPUBloqueado(uint32_t fd, char * payload, uint32_t longitudMensaje){
		//printf("Retorno CPU x Bloqueado\n");
		pthread_mutex_lock(&kernel->mutex_cpus);
		t_CPU *cpu = dictionary_get(kernel->cpus, string_from_format("%d",fd));
		pthread_mutex_unlock(&kernel->mutex_cpus);
		t_iPCBaCPU *datosPCB = deserializarRetornoPCBdeCPU(payload);
		modificarPCB(cpu->pcb, datosPCB);
		//printf("datosPCB actualizado id %d, indice %d, pc %d, sizecontext %d, cursor %d\nSEGMENTO DE CODIGO: %d\n",cpu->pcb->id, cpu->pcb->indiceEtiquetas, cpu->pcb->programcounter, cpu->pcb->sizeContext, cpu->pcb->cursorStack,cpu->pcb->segmentoCodigo);
		poner_cpu_no_disponible(cpu);
		package *respuesta = crear_paquete(respuestaCPU, "respuesta",9);
		enviar_paquete(respuesta,fd);
}

void opRetornoCPUFin(uint32_t fd, char * payload, uint32_t longitudMensaje){
	//printf("Retorno de CPU por Finalizacion\n");
	t_iPCBaCPU * datosPCB = deserializarRetornoPCBdeCPU(payload);
	pthread_mutex_lock(&kernel->mutex_cpus);
	t_CPU *cpu = dictionary_get(kernel->cpus, string_from_format("%d",fd));
	pthread_mutex_unlock(&kernel->mutex_cpus);
	modificarPCB(cpu->pcb, datosPCB);
	//printf("datosPCB actualizado id %d, indice %d, pc %d, sizecontext %d, cursor %d\nSEGMENTO DE CODIGO: %d\n",cpu->pcb->id, cpu->pcb->indiceEtiquetas, cpu->pcb->programcounter, cpu->pcb->sizeContext, cpu->pcb->cursorStack,cpu->pcb->segmentoCodigo);

	package *respuesta = crear_paquete(respuestaCPU, "respuesta",9);
	enviar_paquete(respuesta,fd);

	pthread_mutex_lock(&kernel->mutex_programas);
	t_programa *programa = dictionary_get(kernel->programas,string_from_format("%d",cpu->pcb->id));
	programa->mensajeFIN="El programa Finalizo correctamente";
	programa->exit_code=FIN_PROGRAM_SUCCESS;
	pthread_mutex_unlock(&kernel->mutex_programas);
	printf("Programa %d pasa a Exit\n",cpu->pcb->id);
	cola_push(cola_exit, cpu->pcb);
	poner_cpu_no_disponible(cpu);
	sem_post(sem_exit);
	sem_post(sem_multiprogramacion);
}

void opRetornoCPUExcepcion(uint32_t fd, char * payload, uint32_t longitudMensaje){
	//printf("Retorno de CPU por Excepcion logica\n");
	pthread_mutex_lock(&kernel->mutex_cpus);
	t_CPU *cpu = dictionary_get(kernel->cpus, string_from_format("%d",fd));
	pthread_mutex_unlock(&kernel->mutex_cpus);
	//printf("tengo cpu, tiene asignado el pcb: %d\n",cpu->pcb->id);
	if (dictionary_has_key(kernel->programas,string_from_format("%d",cpu->pcb->id))){
		pthread_mutex_lock(&kernel->mutex_programas);
		t_programa * programa = dictionary_get(kernel->programas, string_from_format("%d",cpu->pcb->id));
		programa->mensajeFIN = malloc(longitudMensaje);
		memcpy(programa->mensajeFIN,payload,longitudMensaje);
		pthread_mutex_unlock(&kernel->mutex_programas);
		printf("Programa %d pasa a Exit\n",cpu->pcb->id);
		cola_push(cola_exit,cpu->pcb);
		poner_cpu_no_disponible(cpu);
		//printf("pase a exit el pcb");
		sem_post(sem_exit);
		sem_post(sem_multiprogramacion);
	}
	else
		printf("no estaba en el diccionario WTF\n");
}

void opExcepcionCPUHardware(uint32_t fd){
	//printf("Retorno de CPU %d por Excepcion Hardware\n",fd);
	if (dictionary_has_key(kernel->cpus,string_from_format("%d",fd))){
		pthread_mutex_lock(&kernel->mutex_cpus);
		t_CPU * cpu = dictionary_remove(kernel->cpus, string_from_format("%d",fd));
		pthread_mutex_unlock(&kernel->mutex_cpus);
		//printf("EX HARD Quite cpu del diccionario cpu\n");
		//printf("Estado de la CPU al desconectarse: %d\n",cpu->estado);
		if (cpu->estado == CPU_CON_PROCESO){
			if (dictionary_has_key(kernel->programas,string_from_format("%d",cpu->pcb->id))){
				pthread_mutex_lock(&kernel->mutex_programas);
				t_programa * programa = dictionary_get(kernel->programas, string_from_format("%d",cpu->pcb->id));
				pthread_mutex_unlock(&kernel->mutex_programas);
				programa->mensajeFIN="Error CPU";
				//printf("EX HARD actualice el programa\n");
				printf("Programa %d pasa a Exit\n",cpu->pcb->id);
				cola_push(cola_exit, cpu->pcb);
				//printf("EX HARD pase pcb a exit\n");
				sem_post(sem_exit);
				sem_post(sem_multiprogramacion);
			}
		} else {
				cpu->estado=CPU_DESCONECTADA;
				//printf("Actualizo el estado de la CPU a %d\n",cpu->estado);
		}
	}
}

void pasar_dato_a_imprimir(char * texto, int longitudTexto, uint32_t fd, t_paquete tipoPaquete){
	package *paquete = crear_paquete(tipoPaquete, texto, longitudTexto); //agregar a enum de tipos de mensaje
	if (enviar_paquete(paquete,fd)==-1){
		printf("Error en envio de Texto a imprimir: %d", fd);
	}
	destruir_paquete(paquete);
}

void opImprimirValor(uint32_t fd, char * payload, uint32_t longitudMensaje){
	//printf("Imprimir Valor \n");
	pthread_mutex_lock(&kernel->mutex_cpus);
	t_CPU *cpu = dictionary_get(kernel->cpus, string_from_format("%d",fd));
	pthread_mutex_unlock(&kernel->mutex_cpus);

	pthread_mutex_lock(&kernel->mutex_programas);
	t_programa * programa = dictionary_get(kernel->programas, string_from_format("%d",cpu->pcb->id));
	pthread_mutex_unlock(&kernel->mutex_programas);
	int valor;
	memcpy(&valor,payload,sizeof(int32_t));
	//printf("valor %d\n",valor);

	package *respuesta = crear_paquete(respuestaCPU,"Listo",6);
	enviar_paquete(respuesta,fd);
	destruir_paquete(respuesta);

	pasar_dato_a_imprimir(payload, longitudMensaje, programa->fd, imprimirValor);
}

void opImprimirTexto(uint32_t fd, char * payload, uint32_t longitudMensaje){
	//printf("Imprimir Texto\n");
	pthread_mutex_lock(&kernel->mutex_cpus);
	t_CPU *cpu = dictionary_get(kernel->cpus, string_from_format("%d",fd));
	pthread_mutex_unlock(&kernel->mutex_cpus);
	pthread_mutex_lock(&kernel->mutex_programas);
	t_programa * programa = dictionary_get(kernel->programas, string_from_format("%d", cpu->pcb->id));
	pthread_mutex_unlock(&kernel->mutex_programas);
	package *respuesta = crear_paquete(respuestaCPU,"Listo",6);
	enviar_paquete(respuesta,fd);
	destruir_paquete(respuesta);
	pasar_dato_a_imprimir(payload, longitudMensaje, programa->fd, imprimirTexto);
}

void opTomarSemaforo(uint32_t fd, char * payload, uint32_t longitudMensaje){

	//printf("Tomar semaforo %sholaquetal\n",payload);
	char * nombre_semaforo = deserializar_nombre_recurso(payload, longitudMensaje);
	//printf("semaforo deserializado %sholaquetal\n",nombre_semaforo);
	wait_semaforo(nombre_semaforo, fd);
}

void opLiberarSemaforo(uint32_t fd, char * payload, uint32_t longitudMensaje){
	//printf("Liberar semaforo\n");
	char * nombre_semaforo = deserializar_nombre_recurso(payload, longitudMensaje);
	signal_semaforo(nombre_semaforo);

	package *respuesta = crear_paquete(respuestaCPU,"Listo",6);
	enviar_paquete(respuesta,fd);
	destruir_paquete(respuesta);
}

char * serializar_valor_variable_compartida(int32_t valor){
    char *stream = malloc(sizeof(int32_t));
    memcpy(stream, &valor, sizeof(int32_t));
    return stream;
}

void opSolicitarValorVariableCompartida(uint32_t fd, char * payload, uint32_t longitudMensaje){

	char * variable = malloc(sizeof(longitudMensaje));
	memcpy(variable,payload,longitudMensaje);
	//printf("Solicitud de variable %s\n",variable);
	if (dictionary_has_key(kernel->variables_compartidas,variable)){
		t_variable_compartida * variable_compartida = dictionary_get(kernel->variables_compartidas, variable);
		pthread_mutex_lock(variable_compartida->mutex);
		char * valor_variable_compartida = serializar_valor_variable_compartida(variable_compartida->valor);
		package *paquete = crear_paquete(solicitarValorVariableCompartida, valor_variable_compartida, sizeof(int32_t));
		if (enviar_paquete(paquete,fd)==-1){
			printf("Error en envio de Valor a imprimir: %d", fd);
		}
		pthread_mutex_unlock(variable_compartida->mutex);
		destruir_paquete(paquete);
	}else printf("No existe la variable compartida!\n");
}

void opAsignarValorVariableCompartida(uint32_t fd, char * payload, uint32_t longitudMensaje){
	//printf("Guardar valor en variable\n");
	t_iVARCOM * variable = deserializar_datos_variable(payload, longitudMensaje);
	//printf("asignar valor %d a la variable %s \n",variable->valor, variable->nombre);
	if(dictionary_has_key(kernel->variables_compartidas, string_from_format("%s",variable->nombre))){

		t_variable_compartida * variable_compartida = dictionary_get(kernel->variables_compartidas, string_from_format("%s",variable->nombre));
		pthread_mutex_lock(variable_compartida->mutex);
		variable_compartida->valor = variable->valor;
		char * valor_variable_compartida = serializar_valor_variable_compartida(variable_compartida->valor);
		package *paquete = crear_paquete(asignarValorVariableCompartida, valor_variable_compartida, sizeof(uint32_t)); //agregar a enum de tipos de mensaje
		if (enviar_paquete(paquete,fd)==-1){
			printf("Error en envio de Valor a imprimir: %d", fd);
		}
		pthread_mutex_unlock(variable_compartida->mutex);
		destruir_paquete(paquete);
	}
	else printf("No encontro la avariable compartida\n");

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
		opRetornoCPUBloqueado,
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

	//printf("HILO recibirCPUs\n");

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
					//printf("Hilo que recibe CPU! tipo de paquete %d, mensaje %s\n",paquete->type,paquete->payload);

					if (nbytes == 0) {
						// conexión cerrada por el cliente
						printf("selectserver: socket %d hung up\n", i);
						close(i);
						FD_CLR(i, &master); // Elimina del conjunto maestro
						opExcepcionCPUHardware(i);
					}
					else {
						if (dictionary_has_key(kernel->cpus,string_from_format("%d",i))) {
							pthread_mutex_lock(&kernel->mutex_cpus);
							t_CPU *cpu = dictionary_get(kernel->cpus, string_from_format("%d",i));
							pthread_mutex_unlock(&kernel->mutex_cpus);
							if (cpu->pcb!=NULL){
								pthread_mutex_lock(&kernel->mutex_programas);
								t_programa * programa = dictionary_get(kernel->programas, string_from_format("%d",cpu->pcb->id));
								pthread_mutex_unlock(&kernel->mutex_programas);
								if (!programa->estado){ //Verifico  si el programa esta activo
									printf("Programa %d pasa a Exit\n",cpu->pcb->id);
									cola_push(cola_exit, cpu->pcb);
									sem_post(sem_exit);
									sem_post(sem_multiprogramacion);
								} else {
									(tabla_operaciones[paquete->type])(i, paquete->payload, nbytes);
								}
							} else (tabla_operaciones[paquete->type])(i, paquete->payload, nbytes);
						} else (tabla_operaciones[paquete->type])(i, paquete->payload, nbytes);
					}
					destruir_paquete(paquete);
				}
			}
		} // for
	} // while
}


void pasarListosAEjecucion(void){
	while(1){

		sem_wait(sem_cpu_disponible);
		sem_wait(sem_estado_listo);

		log_debug(logger,string_from_format("Hilo pasa PCB a Ejecución\n"));
		t_PCB *pcb = cola_pop(cola_ready);
		printf("Programa: %d pasa de Ready a Ejecucion \n",pcb->id);
		pthread_mutex_lock(&kernel->mutex_programas);
		t_programa * programa = dictionary_get(kernel->programas, string_from_format("%d",pcb->id));
		pthread_mutex_unlock(&kernel->mutex_programas);
		if (!programa->estado){
			log_debug(logger,string_from_format("Hilo pasa PCB a Ejecución, No existe el programa no va a ejecucion\n"));
			printf("Programa %d pasa a Exit\n",programa->id);
			cola_push(cola_exit, pcb);
			sem_post(sem_exit);
			sem_post(sem_multiprogramacion);
		} else {
			log_debug(logger,string_from_format("Hilo pasa PCB a Ejecución, Existe el programa VA a ejecucion\n"));
			t_CPU *cpu = cola_pop(cpus_disponibles);
			//printf("saque de la cola, la cpu %d, con Estado %d\n",cpu->fd,cpu->estado);
			while(cpu->estado==CPU_DESCONECTADA){
				//printf("Encontre la CPU %d desconectada, la elimino\n",cpu->fd);
				free(cpu);
				sem_wait(sem_cpu_disponible);
				cpu = cola_pop(cpus_disponibles);
				//printf("saque de la cola, la cpu %d, con Estado %d\n",cpu->fd,cpu->estado);
			}
			//printf("Esta no estaba desconectada! %d\n",cpu->fd);
			enviar_pcb_a_cpu(pcb,cpu);
		}
	}
}

void hiloPCP(){
	int thr;
	//cpus =dictionary_create();
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
