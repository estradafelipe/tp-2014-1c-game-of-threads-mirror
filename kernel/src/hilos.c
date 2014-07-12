/*
 * hilos.c
 *
 *  Created on: 12/06/2014
 *      Author: utnso
 */
#include "hilos.h"

extern t_kernel *kernel;
extern sem_t *sem_estado_listo;
extern sem_t *sem_exit;
extern sem_t *sem_multiprogramacion;
extern t_cola *cola_ready;
extern t_log *logger;
extern t_cola *cola_exit;
char *pathconfig;

/* Envia mensaje a cpu de que el programa se debe bloquear
 * como respuesta a un WAIT */
void bloqueo_por_semaforo(uint32_t fd){
	char *payload = "bloquear"; // por mandar algo
	int size = strlen(payload) +1;
	package *paquete = crear_paquete(bloquearProgramaCPU,payload,size);
	enviar_paquete(paquete,fd);
	destruir_paquete(paquete);
}

/* Envia mensaje a cpu de que el programa puede seguir,
 * el semaforo esta libre! (respuesta a un WAIT) */
void semaforo_libre(t_CPU *cpu){
	char *payload = "freedom!"; // por mandar algo
	int size = strlen(payload) +1;
	package *paquete = crear_paquete(semaforolibre,payload,size);
	enviar_paquete(paquete,cpu->fd);
	destruir_paquete(paquete);
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
	kernel->variables_compartidas = dictionary_create();
	// Crea tabla de semaforos
	i = 0;
	while (1) {
		if (kernel->semaforosid[i]!='\0'){
			pthread_mutex_t *mutex = malloc(sizeof(pthread_mutex_t));
			pthread_mutex_init(mutex,NULL);
			t_semaforo *SEM = malloc(sizeof(t_semaforo));
			char * nombre = kernel->semaforosid[i];
			//SEM->id = strdup(kernel->semaforosid[i]);
			//memcpy(SEM->id,nombre,strlen(nombre)+1);
			SEM->id = string_duplicate(nombre);
			SEM->valor = atoi(kernel->semaforosvalor[i]);
			SEM->cola = cola_create();
			SEM->mutex = mutex;
			//printf("Se crea semaforo %s, valor %d strlen %d\n",SEM->id, SEM->valor,strlen(SEM->id));
			dictionary_put(kernel->semaforos,SEM->id,SEM);

			i++;
		}else break;

	}
	// Crea tabla variables compartidas
	if (kernel->compartidasid!=NULL){
		i = 0;
		while (1) {
			if (kernel->compartidasid[i]!='\0'){
				pthread_mutex_t *mutex = malloc(sizeof(pthread_mutex_t));
				pthread_mutex_init(mutex,NULL);
				t_variable_compartida *VAR = malloc(sizeof(t_variable_compartida));
				VAR->nombre = kernel->compartidasid[i];
				VAR->valor = 0; // esto esta bien?
				VAR->mutex = mutex;
				dictionary_put(kernel->variables_compartidas,VAR->nombre,VAR);

				i++;
			}else break;
		}
	}
	// Crea tabla de IO
	if (kernel->entradasalidaid!=NULL){
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
				//printf("Se crea IO %s, retardo %d strlen %d\n",IO->id, IO->retardo,strlen(IO->id));
			}else break;
		}
	}
}

void wait_semaforo(char *semaforo,uint32_t fd){
	//printf("wait al semaforo %s\n", semaforo);

	if (dictionary_has_key(kernel->semaforos,semaforo)){
		t_semaforo *SEM = dictionary_get(kernel->semaforos,semaforo);
		t_CPU *cpu = dictionary_get(kernel->cpus,string_from_format("%d",fd));
		pthread_mutex_lock(SEM->mutex);
		log_debug(logger, string_from_format("El semaforo %s tenia el valor %d ",semaforo,SEM->valor));
		SEM->valor --;
		log_debug(logger, string_from_format(" y ahora pasa a tener el valor %d ",SEM->valor));
		if (SEM->valor<0){

			bloqueo_por_semaforo(fd);
			//printf("Envio mensaje a la cpu");
			cola_push(SEM->cola, cpu->pcb);
			printf("Programa %d, pasa a cola de Bloqueados\n",cpu->pcb->id);
		}else semaforo_libre(cpu);

		pthread_mutex_unlock(SEM->mutex);
	}
	else printf("el semaforo no existe!\n");
}

void signal_semaforo(char *semaforo){
	//printf("Signal al semaforo %s strlen %d\n",semaforo,strlen(semaforo));
	if (dictionary_has_key(kernel->semaforos,semaforo)){
		t_semaforo *SEM = dictionary_get(kernel->semaforos,semaforo);
		pthread_mutex_lock(SEM->mutex);
		SEM->valor ++;
		log_debug(logger, string_from_format("signal al semaforo %s, valor final: %d",semaforo,SEM->valor));
		if (SEM->valor<=0){
			t_PCB *PCB = cola_pop(SEM->cola);
			pthread_mutex_lock(&kernel->mutex_programas);
			t_programa * programa = dictionary_get(kernel->programas, string_from_format("%d",PCB->id));
			pthread_mutex_unlock(&kernel->mutex_programas);
			if (!programa->estado){ 				//Verifico  si el programa esta activo
				pasarACola(cola_exit, PCB);
				sem_post(sem_exit);
				sem_post(sem_multiprogramacion);
			} else {
				printf("Programa %d pasa de Bloqueado a Ready\n",PCB->id);
				cola_push(cola_ready, PCB);
				sem_post(sem_estado_listo);
			}
		}
		pthread_mutex_unlock(SEM->mutex);
		}else printf("no existe el semaforo!");
}


void hiloIO(t_entradasalida *IO){
	// hilo de entrada salida
	while(1){
		sem_wait(IO->semaforo_IO); // cuando deba ejecutar este hilo, darle signal
		//printf("Va a ejecutar el dispositivo: %s",IO->id);
		t_progIO *elemento = cola_pop(IO->cola);
		// controlar que este activo el programa para no desperdiciar recurso
		int retardo = IO->retardo * elemento->unidadesTiempo;
		usleep(retardo);
		pthread_mutex_lock(&kernel->mutex_programas);
		t_programa * programa = dictionary_get(kernel->programas, string_from_format("%d",elemento->PCB->id));
		pthread_mutex_unlock(&kernel->mutex_programas);
		if (!programa->estado){ 				//Verifico  si el programa esta activo
			pasarACola(cola_exit, elemento->PCB);
			sem_post(sem_exit);
			sem_post(sem_multiprogramacion);
		} else {
			printf("Programa %d pasa de Bloqueado por Dispositivo a Ready\n",elemento->PCB->id);
			cola_push(cola_ready,elemento->PCB);
			sem_post(sem_estado_listo);
		}
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
