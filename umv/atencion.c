
#include "atencion.h"
#include <paquetes.h>

#define BUFFERSIZE 128
#define WORSTFIT 0
#define FIRSTFIT 1

void* atenderNuevaConexion(void* parametro){
	int socketCliente = (int) parametro;
	printf("Esperando handshake...\n");
	package* paquete = recibir_paquete(socketCliente);
	int bytesRecibidos = paquete->payloadLength;
	t_paquete tipo = paquete->type;
	if(bytesRecibidos>0){
		printf("Se recibio algo\n");
		switch(tipo){
			case handshakeKernelUmv:
				printf("Hola kernel\n");
				atenderKernel(socketCliente);
				break;
			
			case handshakeCpuUmv:
				printf("Hola cpu\n");
				atenderCpu(socketCliente);
				break;
			
			default:
				printf("No anda el tipo de paquete :(\n");
				break;
		}
	}
	//TODO Hacer handshake
	//Ver que tipo de paquete se usara en el handshake
}

// TODO Ver si combiene o no hacer dos funciones separadas
/* Atiende solicitudes del kernel */
/* quedarse esperando solicitudes de creacion  o eliminacion de segmentos de programas */
int atenderKernel(int socket){
	printf("Atendiendo al Kernel\n");
	package* paquete = recibir_paquete(socket);
	int bytesRecibidos = paquete->payloadLength;
	t_paquete tipo = paquete->type;
	if(bytesRecibidos>0){
		printf("Se recibio algo\n");
		switch(tipo){
			default:
				printf("Tipo de mensaje invalido\n");
		}
	}
	return 0;
}

/* Atiende solicitudes de la cpu */
/* primero debo saber que programa esta activo
 * luego esperar solicitudes de escritura y lectura en los segmentos de dicho programa,
 * o el cambio del programa activo
 */
int atenderCpu(int socket){
	printf("Atendiendo a una Cpu\n");	
	package* paquete = recibir_paquete(socket);
	int bytesRecibidos = paquete->payloadLength;
	t_paquete tipo = paquete->type;
	if(bytesRecibidos>0){
		printf("Se recibio algo\n");
		switch(tipo){
			case cambioProcesoActivo:
				//hacer algo
				break;
			case lectura:
				//hacer algo
				break;
			case escritura:
				//hacer algo
				break;
			default:
				printf("Tipo de mensaje invalido\n");
		}	
	}
	return 0;
}

void* atenderConsola(){
	char buffer[BUFFERSIZE];
	int id_programa;
	int base;
	int offset;
	int tamanio;
	printf("Hilo que atiende la consola esperando entrada por teclado...\n");
	while(1){
		printf("Ingrese un comando\n");
		/* Leo que se ingreso por consola */
		fgets(buffer,BUFFERSIZE,stdin);
		char** palabras = string_split(buffer, " ");
		/* la ultima palabra del comando queda con el salto de linea por eso hay que tener
		 * en cuenta el \n si es la ultima palabra */

		if(strcmp(palabras[0],"operacion")==0){
			// validar  que operacion es operacion
			id_programa = atoi(palabras[2]);
			if (strcmp(palabras[1],"lectura")==0){
				// es un pedido de lectura
				base = atoi(palabras[3]);
				offset = atoi(palabras[4]);
				tamanio = atoi(palabras[5]);
				printf("Leyendo %d bytes en el segmento del programa %d con base %d\n",tamanio,id_programa,base);
				char* datos = leer(id_programa,base,offset,tamanio);
				if(datos!=NULL){
					printf("Los datos obtenidos son: %s\n",datos);
				} else {
					printf("Violación de segmento!!!\n");
				}
			} else if (strcmp(palabras[1],"escritura")==0){
				// es un pedido de escritura
				base = atoi(palabras[3]);
				offset = atoi(palabras[4]);
				tamanio = atoi(palabras[5]);
				char* buffer = malloc(tamanio);
				memcpy(buffer,palabras[6],tamanio);
				printf("Escribiendo %d bytes en el segmento del programa %d con base %d\n",tamanio,id_programa,base);
				printf("El buffer es %s\n",buffer);
				int estado = escribir(id_programa,base,offset,tamanio,buffer);
				if(estado >= 0){
					printf("La operacion de escritura termino correctamente\n");
				} else {
					printf("Violación de segmento!!!\n");
				}
			} else if (strcmp(palabras[1],"crear_seg")==0){
				// es un pedido de creacion de segmentos
				crear_segmentos(id_programa);
				//TODO informar si habia espacio suficiente y se pudo crear correctamente
			} else if (strcmp(palabras[1],"destruir_seg")==0){
				// es un pedido de destruccion de segmentos
				destruir_segmentos(id_programa);
			} else {
				printf("Comando no reconocido, intente de nuevo\n");
			}

		} else if (strcmp(palabras[0],"retardo")==0){
			// modificar el retardo (en milisegundos)
			retardo = atoi(palabras[1]);

		} else if (strcmp(palabras[0],"algoritmo")==0){
			// modificar el algoritmo de ubicacion de segmentos
			// worst-fit o first-fit
			if (strcmp(palabras[1],"WORSTFIT\n")==0){
				algoritmo = WORSTFIT;
				printf("Se modifico el algortimo correctamente. El algoritmo actual es Worst-Fit\n");
			} else if (strcmp(palabras[1],"FIRSTFIT\n")==0){
				algoritmo = FIRSTFIT;
				printf("Se modifico el algortimo correctamente. El algoritmo actual es First-Fit\n");
			} else {
				printf("Comando no reconocido, intente de nuevo\n");
			}

		} else if (strcmp(palabras[0],"compactacion\n")==0){
			// hacer compactacion
			printf("Compactando...\n");
			compactar();
			printf("Compactación terminada\n");

		} else if (strcmp(palabras[0],"dump")==0){
			// imprimir reporte
			if (strcmp(palabras[1],"estructuras")==0){
			// imprimir tabla de segmentos por proceso en memoria
				id_programa = atoi(palabras[2]);
				imprimir_estructuras(id_programa);
			} else if (strcmp(palabras[1],"memoria\n")==0){
			// imprimir segmentos de la memoria incluyendo espacios libres
				imprimir_segmentos_memoria();
			} else if (strcmp(palabras[1],"contenido")==0){
			// imprimir contenido dado un offset y una cantidad de bytes
				offset = atoi(palabras[2]);
				tamanio = atoi(palabras[3]);
				imprimir_contenido(offset,tamanio);
			} else {
				printf("Comando no reconocido, intente de nuevo\n");
			}

		} else {
			printf("Comando no reconocido, intente de nuevo\n");
		}
	}
}

int _menor_id_programa(t_segmento *seg, t_segmento *segMayor) {
	return seg->id_programa < segMayor->id_programa;
}

int _mayor_tamanio(t_segmento *seg, t_segmento *segMayor) {
	return seg->tamanio < segMayor->tamanio;
}
/* Devuelve si el segmento es el buscado */
int _es_el_buscado(t_segmento* seg) {
	return seg->id_programa == id_prog_buscado && seg->base_logica == base_buscada;
}
/* Devuelve si esta vacio el segmento */
int _esta_vacio(t_segmento* seg){
	return seg->id_programa == -1;
}

int _existe_algun_seg(t_segmento* seg){
	return seg->id_programa == id_prog_buscado;
}

/* Devuelve el segmento del programa y base indicado */
t_segmento* buscarSegmento(int id_programa,int base){
	t_segmento* seg_aux =(void*)  list_find(segmentos, (void*) _es_el_buscado);
	return seg_aux;
}

/* Devuelve la posicion de un segmento vacio */
int buscarSegmentoVacio(int tamanio){
	int i;
	t_segmento* aux;
	int cant_seg=list_size(segmentos);
	for(i=0;i<cant_seg;i++){
		aux = list_get(segmentos,i);
		if(_esta_vacio(aux)){
			return i;
		}
	}
	return -1;
}

/* Busca el segmento del programa y lee el tamaño indicado:
 * Si encuentra el semgento y el acceso es dentro del mismo devuelve los datos solicitados
 * Caso contrario devuelve NULL
 */
char* leer(int id_programa,int base,int offset,int tamanio){
	int desplazamiento;
	char* datos = malloc(tamanio);
	t_segmento* segmentoBuscado = buscarSegmento(id_programa,base);
	if(segmentoBuscado!=NULL){
		desplazamiento = base + offset + tamanio;
		if(desplazamiento <= (segmentoBuscado->tamanio + segmentoBuscado->base_logica)){
			memcpy(datos,segmentoBuscado->base+offset,tamanio);
			return datos;
		} else {
			return NULL;
		}
	} else {
		return NULL;
	}

}

/*Busca el segmento del programa y escribe los datos:
 * Si encuentra el semgento y el acceso es dentro del mismo devuelve 1
 * Caso contrario devuelve -1
 */
int escribir(int id_programa,int base,int offset,int tamanio,char* buffer){
	int desplazamiento;
	t_segmento* segmentoBuscado = buscarSegmento(id_programa,base);
	if(segmentoBuscado!=NULL){
		desplazamiento = base + offset + tamanio;
		if(desplazamiento <= (segmentoBuscado->tamanio + segmentoBuscado->base_logica)){
			memcpy(segmentoBuscado->base+offset,buffer,tamanio);
			return 1;
		} else {
			return -1;
		}
	} else {
		return -1;
	}
}


int crear_segmentos(int id_programa){
return -1;
}

/* Destruye todos los segmentos de un programa
 * En caso de que no exista ningun segmento de ese programa devuelve -1
 * Caso contrario devuelve 1
 */
int destruir_segmentos(int id_programa){
	t_segmento* seg_aux =(void*)  list_find(segmentos, (void*) _existe_algun_seg);
	if(seg_aux==NULL){
		return -1; //no hay ningun segmento de ese programa (segmentation fault)
	} else {
		int i;
		t_segmento* aux;
		t_segmento* vacio = malloc(sizeof(t_segmento));
		vacio->id_programa = -1;
		int cant_seg=list_size(segmentos);
		for(i=0;i<cant_seg;i++){
			aux = list_get(segmentos,i);
			if(aux->id_programa == id_programa){
				vacio->base = aux->base;
				vacio->base_logica = aux->base_logica;
				vacio->tamanio = aux->tamanio;
				list_replace(segmentos,i,vacio);
			}
		}
		return 1;
	}
}

int compactar(){
	return -1;
}

/* Imprime la tabla de segmentos del programa que se le pase por parametro
 * o de todos los programas si se le pasa -1
 */
int imprimir_estructuras(int id_programa){
	int i;
	t_segmento* aux;
	int cant_seg=list_size(segmentos);
	if(id_programa < 0){ //todos los procesos
		t_list* listaOrdenada = segmentos; // lista ordenada por id_programa
		list_sort(listaOrdenada,(void*)_menor_id_programa);
		for(i=0;i<cant_seg;i++){
			aux = list_get(listaOrdenada,i);
			if(!_esta_vacio(aux)){
				printf("***********************************\n");
				printf("Id_programa: %d\n",aux->id_programa);
				printf("Base logica: %d\n", aux->base_logica);
				printf("Base real: %p\n",aux->base);
				printf("Tamaño: %d\n",aux->tamanio);
			}
		}
	} else if (id_programa >= 0) { //de un proceso en particular
		for(i=0;i<cant_seg;i++){
			aux = list_get(segmentos,i);
			if(aux->id_programa == id_programa){
				printf("***********************************\n");
				printf("Id_programa: %d\n",aux->id_programa);
				printf("Base logica: %d\n", aux->base_logica);
				printf("Base real: %p\n",aux->base);
				printf("Tamaño: %d\n",aux->tamanio);
			}
		}
	}
	return 0;
}

/* Imprime la tabla de todos los segmentos en memoria (incluidos los vacios) */
int imprimir_segmentos_memoria(){
	int i;
	t_segmento* aux;
	int cant_seg=list_size(segmentos);
	for(i=0;i<cant_seg;i++){
		aux = list_get(segmentos,i);
		printf("***********************************\n");
		if(aux->id_programa != -1){
			printf("Id_programa: %d\n",aux->id_programa);
		} else {
			printf("Id_programa: %d (VACÍO)\n",aux->id_programa);
		}
		printf("Base logica: %d\n", aux->base_logica);
		printf("Base real: %p\n",aux->base);
		printf("Tamaño: %d\n",aux->tamanio);
	}
	return 0;
}

/* Imprime el contenido de la memoria a partir de un offset y tamanio */
int imprimir_contenido(int offset, int tamanio){
	char* datos = malloc(tamanio);
	memcpy(datos,bloqueDeMemoria+offset,tamanio);
	printf("El contenido a partir de la posicion %d y tamaño %d es: %s\n", offset, tamanio, datos);
	return 0;
}

/* Crea un nuevo segmento con el algoritmo FIRST-FIT
 * Devuelve -1 en caso de no encontrar espacio suficiente para el segmento
 * Caso contrario devuelve la posicion en la lista
 */
int first_fit(int id_programa,int tamanio){
	int i;
	t_segmento* aux;
	t_segmento* vacio = malloc(sizeof(t_segmento));
	int cant_seg=list_size(segmentos);
	for(i=0;i<cant_seg;i++){
		aux = list_get(segmentos,i);
		if(_esta_vacio(aux)){
			if(aux->tamanio >= tamanio){
				//si el tamanio es igual al solicitado lo reemplazo directamente
				//sino debe ser reemplazado por dos segmentos
				//el usado y el espacio que queda libre
				aux->id_programa = id_programa;
				if(aux->tamanio == tamanio){
					//TODO semaforos!!
					list_replace(segmentos,i,aux);
				} else {
					vacio->base = aux->base + tamanio;
					vacio->id_programa = -1;
					vacio->tamanio = aux->tamanio - tamanio;
					aux->tamanio = tamanio;
					list_replace(segmentos,i,aux);
					list_add_in_index(segmentos,i+1,vacio);
				}
				return i;
			}
		}
	}
	return -1;
}

/* Crea un nuevo segmento con el algoritmo WORST-FIT
 * Devuelve -1 en caso de no encontrar espacio suficiente para el segmento
 * Caso contrario devuelve la posicion en la lista
 */
int worst_fit(int id_programa,int tamanio){
	t_list* listaOrdenada = segmentos;// revisar si se hace una copia o es la misma lista!!!!
	//ordeno la lista por tamanio descendente
	list_sort(listaOrdenada,(void*)_mayor_tamanio);
	int i,j;
	t_segmento* auxOrd;
	t_segmento* aux;
	t_segmento* vacio = malloc(sizeof(t_segmento));
	int cant_seg=list_size(listaOrdenada);
	for(i=0;i<cant_seg;i++){
		auxOrd = list_get(listaOrdenada,i);
		if(_esta_vacio(aux)){
			if(auxOrd->tamanio >= tamanio){
				//busco el segmento en la lista original
				for(j=0;j<=cant_seg;j++){
					aux = list_get(segmentos,j);
					if(aux->id_programa==-1 && aux->base==auxOrd->base){
						//si el tamanio es igual al solicitado lo reemplazo directamente
						//sino debe ser reemplazado por dos segmentos
						//el usado y el espacio que queda libre
						if(aux->tamanio == tamanio){
							aux->id_programa = id_programa;
							//TODO semaforos!!
							list_replace(segmentos,j,aux);
						} else {
							vacio->base = aux->base + tamanio;
							vacio->id_programa = -1;
							vacio->tamanio = aux->tamanio - tamanio;
							aux->id_programa = id_programa;
							aux->tamanio = tamanio;
							list_replace(segmentos,j,aux);
							list_add_in_index(segmentos,j+1,vacio);
						}
						return j;
					}
				}
			}
		}
	}
	return -1;

}

