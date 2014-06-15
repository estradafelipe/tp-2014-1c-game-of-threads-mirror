/*
 * hilos.c
 *
 *  Created on: 12/06/2014
 *      Author: utnso
 */
#include "hilos.h"

extern t_kernel *kernel;
extern t_cola *cola_block;
char *pathconfig;

void bloqueo_por_semaforo(t_CPU *cpu){
	// enviar mensaje a cpu de que el programa se debe bloquear
	//cpu->fd
}

/* Crea las tablas de semaforos y de I/O */
void crea_tablasSitema(){
	int i;
	kernel->semaforos = malloc(sizeof(t_dictionary));
	kernel->entradasalida = malloc(sizeof(t_dictionary));
	kernel->semaforos = dictionary_create();
	kernel->entradasalida = dictionary_create();

	// Crea tabla de semaforos
	i = 0;
	while (1) {
		if (kernel->semaforosid[i]!='\0'){
			t_semaforo *SEM = malloc(sizeof(t_semaforo));
			SEM->id = kernel->semaforosid[i];
			SEM->valor = atoi(kernel->semaforosvalor[i]);
			SEM->cola = cola_create();
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

	t_semaforo *SEM = dictionary_get(kernel->semaforos,semaforo);
	t_CPU *cpu = dictionary_get(kernel->cpus,string_from_format("%d",fd));
	t_PCB *PCB = dictionary_get(kernel->programas,string_from_format("%d",cpu->id_pcb));
	pthread_mutex_lock(SEM->mutex);
	SEM->valor --;

	if (SEM->valor<0){
		cola_push(SEM->cola,PCB);
		bloqueo_por_semaforo(cpu);
	}

	pthread_mutex_unlock(SEM->mutex);

}

void signal_semaforo(char *semaforo){
	t_semaforo *SEM = dictionary_get(kernel->semaforos,semaforo);
	pthread_mutex_lock(SEM->mutex);
	SEM->valor ++;
	if (SEM->valor<=0){
		t_PCB *PCB = cola_pop(SEM->cola);
		cola_push(cola_block,PCB);	// sacar a un programa de la cola
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
		printf("Se creo el hilo de IO %s\n",IO->id);
	else printf("no se pudo crear el hilo IO %s\n",IO->id);

}

void imprimepantalla(char * key, t_entradasalida *IO){
	printf("%s: %d\n",key,IO->retardo);
}
