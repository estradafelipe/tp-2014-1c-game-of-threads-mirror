/*
 * hilos.c
 *
 *  Created on: 12/06/2014
 *      Author: utnso
 */
#include "hilos.h"

extern t_kernel *kernel;
extern t_cola *cola_block;
extern t_log *logger;
char *pathconfig;

/* Envia mensaje a cpu de que el programa se debe bloquear
 * como respuesta a un WAIT */
void bloqueo_por_semaforo(t_CPU *cpu){
	char *payload = "bloquear"; // por mandar algo
	int size = strlen(payload) +1;
	package *paquete = crear_paquete(bloquearProgramaCPU,payload,size);
	enviar_paquete(paquete,cpu->fd);
}

/* Envia mensaje a cpu de que el programa puede seguir,
 * el semaforo esta libre! (respuesta a un WAIT) */
void semaforo_libre(t_CPU *cpu){
	char *payload = "freedom!"; // por mandar algo
	int size = strlen(payload) +1;
	package *paquete = crear_paquete(semaforolibre,payload,size);
	enviar_paquete(paquete,cpu->fd);
}

/* Crea las tablas de semaforos y de I/O */
void crea_tablasSitema(){
	int i;
	kernel->semaforos = malloc(sizeof(t_dictionary));
	kernel->entradasalida = malloc(sizeof(t_dictionary));
	kernel->cpus = malloc(sizeof(t_dictionary));
	kernel->semaforos = dictionary_create();
	kernel->entradasalida = dictionary_create();
	kernel->cpus = dictionary_create();
	// Crea tabla de semaforos
	i = 0;
	while (1) {
		if (kernel->semaforosid[i]!='\0'){
			t_semaforo *SEM = malloc(sizeof(t_semaforo));
			SEM->id = kernel->semaforosid[i];
			SEM->valor = atoi(kernel->semaforosvalor[i]);
			SEM->cola = cola_create();
			SEM->mutex = malloc(sizeof(pthread_mutex_t));
			pthread_mutex_init(SEM->mutex,NULL);
			dictionary_put(kernel->semaforos,kernel->semaforosid[i],SEM);
			i++;
		}else break;

	}

	// Crea tabla de IO
	i = 0;
	while (1) {
		if (kernel->entradasalidaid[i]!='\0'){

			t_entradasalida *IO = malloc(sizeof(t_entradasalida));
			IO->id = kernel->entradasalidaid[i];
			IO->retardo = atoi(kernel->entradasalidaret[i]);
			IO->semaforo_IO = malloc(sizeof(sem_t));
			IO->cola = cola_create();
			sem_init(IO->semaforo_IO,0,0); //inicializo semaforo del hilo en 0
			dictionary_put(kernel->entradasalida,IO->id,IO);
			i++;
		}else break;
	}

}

void wait_semaforo(char *semaforo,uint32_t fd){

	if (dictionary_has_key(kernel->semaforos,semaforo)){
		t_semaforo *SEM = dictionary_get(kernel->semaforos,semaforo);
		t_CPU *cpu = dictionary_get(kernel->cpus,string_from_format("%d",fd));
		pthread_mutex_lock(SEM->mutex);
		log_debug(logger, string_from_format("El semaforo %s tenia el valor %d ",semaforo,SEM->valor));
		SEM->valor --;
		log_debug(logger, string_from_format(" y ahora pasa a tener el valor %d ",SEM->valor));
		if (SEM->valor<0){
			bloqueo_por_semaforo(cpu);
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
			t_iPCBaCPU *datosPCB = deserializarRetornoPCBdeCPU(paquete_recibido->payload);
			t_CPU *cpu = dictionary_get(cpus, string_from_format("%d",fd));
			t_PCB *pcb = modificarPCB(cpu->pcb, datosPCB);
			poner_cpu_no_disponible(cpu);
			cola_push(SEM->cola, cpu->pcb);
			free(paquete_recibido);
		}else semaforo_libre(cpu);

		pthread_mutex_unlock(SEM->mutex);
	}
	else printf("el semaforo no existe!\n");
}

void signal_semaforo(char *semaforo){
	t_semaforo *SEM = dictionary_get(kernel->semaforos,semaforo);
	pthread_mutex_lock(SEM->mutex);
	SEM->valor ++;
	log_debug(logger, string_from_format("signal al semaforo %s, valor final: %d",semaforo,SEM->valor));
	if (SEM->valor<=0){
		t_PCB *PCB = cola_pop(SEM->cola);
		cola_push(cola_ready, PCB);	// sacar a un programa de la cola
		// envia una confirmacion?
	}

	pthread_mutex_unlock(SEM->mutex);
}


void hiloIO(t_entradasalida *IO){
	// hilo de entrada salida
	while(1){
		sem_wait(IO->semaforo_IO); // cuando deba ejecutar este hilo, darle signal
		t_progIO *elemento = cola_pop(IO->cola);
		// controlar que este activo el programa para no desperdiciar recurso
		int retardo = IO->retardo * elemento->unidadesTiempo;
		usleep(retardo);
		// cola de bloqueados intermedia para pasar de block->ready
		cola_push(cola_block,elemento->PCB);
		free(elemento);
	}
}

void crea_hilosIO(char* key, t_entradasalida *IO){
	int thr;
	pthread_t * IOthr = malloc(sizeof(pthread_t));
	thr = pthread_create( IOthr, NULL, (void*)hiloIO, IO);

	if (thr== 0)
		log_debug(logger,string_from_format("Se creo el hilo de IO %s\n",IO->id));
	else log_debug(logger,string_from_format("no se pudo crear el hilo IO %s\n",IO->id));



}

void imprimepantalla(char * key, t_entradasalida *IO){
	log_debug(logger,string_from_format("%s: %d\n",key,IO->retardo));
}


