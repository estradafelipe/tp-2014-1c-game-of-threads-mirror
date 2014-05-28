
#include "atencion.h"
#include <paquetes.h>
#include <serializadores.h>

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
				printf("Bienvenido kernel\n");
				atenderKernel(socketCliente);
				break;
			
			case handshakeCpuUmv:
				printf("Bienvenida cpu\n");
				atenderCpu(socketCliente);
				break;
			
			default:
				printf("No anda el tipo de paquete :(\n");
				break;
		}
	}
}

/* Atiende solicitudes del kernel */
/* quedarse esperando solicitudes de creacion  o eliminacion de segmentos de programas */
int atenderKernel(int socket){
	printf("Atendiendo al Kernel\n");
	package* paquete;
	package* respuesta;
	char* answer = malloc(sizeof(t_puntero));
	int bytesRecibidos;
	t_paquete tipo;
	int procesoActivo;
	int id_programa = -1; // programa que me pasan para destruir sus segmentos
	t_solicitudLectura* solicitudLectura;
	t_solicitudEscritura* solicitudEscritura;
	t_crearSegmentoUMV* creacionSegmento;
	char* datos; //datos de la lectura
	int resultado; //resultado de la escritura
	while(1){
		paquete = recibir_paquete(socket);
		bytesRecibidos = paquete->payloadLength;
		tipo = paquete->type;
		if(bytesRecibidos>0){
			printf("Se recibio algo\n");
			switch(tipo){
				case cambioProcesoActivo: // es posible que este de mas!!!
					//deserializar para obtener el id_programa
					//procesoActivo = id_programa;
					break;
				case creacionSegmentos:
					sleep(retardo);
					//desserializar estructura con id_programa y tamaño del segmento
					creacionSegmento = deserializarSolicitudSegmento(paquete->payload);
					//guardo el proceso activo (para la escritura de los segmentos)
					procesoActivo = creacionSegmento->programid;
					pthread_mutex_lock(mutexSegmentos);
					resultado = crear_segmento(creacionSegmento);
					pthread_mutex_unlock(mutexSegmentos);
					if(resultado>=0){
						printf("El segmento del %d de tamaño %d se creo correctamente\n",creacionSegmento->programid,creacionSegmento->size);
						memcpy(answer, &resultado, sizeof(t_puntero));
						respuesta = crear_paquete(respuestaUmv,answer,sizeof(t_puntero));
						enviar_paquete(respuesta,socket);
					} else {
						printf("No hubo espacio suficiente, se realizara la compactacion y se volvera a intentar\n");
						pthread_mutex_lock(mutexSegmentos);
						compactar();
						pthread_mutex_unlock(mutexSegmentos);
						pthread_mutex_lock(mutexSegmentos);
						resultado = crear_segmento(creacionSegmento);
						pthread_mutex_unlock(mutexSegmentos);
						if(resultado>=0){
							printf("El segmento del %d de tamaño %d se creo correctamente\n",creacionSegmento->programid,creacionSegmento->size);
							memcpy(answer, &resultado, sizeof(t_puntero));
							respuesta = crear_paquete(respuestaUmv,answer,sizeof(t_puntero));
							enviar_paquete(respuesta,socket);
						} else {
							printf("No hubo espacio suficiente\n");
							memcpy(answer, &resultado, sizeof(t_puntero));
							respuesta = crear_paquete(respuestaUmv,answer,sizeof(t_puntero));
							enviar_paquete(respuesta,socket);
						}
					}
					break;
					//validar si se pudo crear el segmento y responder al kernel
				case destruccionSegmentos:
					sleep(retardo);
					//desserializar para obtener id_programa

					pthread_mutex_lock(mutexSegmentos);
					resultado = destruir_segmentos(id_programa);
					pthread_mutex_unlock(mutexSegmentos);
					//validar si hay segmentation fault por no existir segmentos del programa
					if(resultado == -1){
						//No existen segmentos de ese programa
						//Violacion de segmento???
					}

					break;
				case lectura:
					sleep(retardo);
					//desserializar estructura con la base,offset y tamaño
					solicitudLectura = desserializarSolicitudLectura(paquete->payload);
					pthread_mutex_lock(mutexSegmentos);
					datos = leer(procesoActivo,solicitudLectura);
					pthread_mutex_unlock(mutexSegmentos);
					//validar si hay segmentation fault
					if(datos!=NULL){
						printf("Los datos obtenidos son: %s\n",datos);
						// responder a la cpu
						respuesta = crear_paquete(respuestaUmv,datos,solicitudLectura->tamanio);
						enviar_paquete(respuesta,socket);
					} else {
						printf("Violación de segmento!!!\n");
						printf("Solicitud lectura:\n");
						printf("Id_programa: %d, base: %d, offset: %d y tamaño: %d\n",procesoActivo,solicitudLectura->base,solicitudLectura->offset,solicitudLectura->tamanio);
						resultado = -1;
						memcpy(answer, &resultado, sizeof(t_puntero));
						respuesta = crear_paquete(respuestaUmv,answer,sizeof(t_puntero));
						enviar_paquete(respuesta,socket);
					}
					break;
				case escritura:
					sleep(retardo);
					//desserializar estructura con la base,offset,tamaño y buffer
					solicitudEscritura = desserializarSolicitudEscritura(paquete->payload);
					pthread_mutex_lock(mutexSegmentos);
					//TODO revisar como pasar el valor del entero!!!!!
					resultado = escribir(procesoActivo,solicitudEscritura);
					pthread_mutex_unlock(mutexSegmentos);
					//validar si hay segmentation fault
					if(resultado >= 0){
						printf("La operacion de escritura termino correctamente\n");
						memcpy(answer, &resultado, sizeof(t_puntero));
						respuesta = crear_paquete(respuestaUmv,answer,sizeof(t_puntero));
						enviar_paquete(respuesta,socket);
					} else {
						printf("Violación de segmento!!!\n");
						printf("Solicitud escritura:\n");
						printf("Id_programa: %d, base: %d, offset: %d y tamaño: %d\n",procesoActivo,solicitudEscritura->base,solicitudEscritura->offset,solicitudEscritura->tamanio);
						resultado = -1;
						memcpy(answer, &resultado, sizeof(t_puntero));
						respuesta = crear_paquete(respuestaUmv,answer,sizeof(t_puntero));
						enviar_paquete(respuesta,socket);
					}
					break;
				default:
					printf("Tipo de mensaje invalido\n");
					//notificar al kernel que el mensaje es invalido???
			}
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
	package* paquete;
	package* respuesta;
	char* answer = malloc(sizeof(t_puntero));
	int bytesRecibidos;
	t_paquete tipo;
	int procesoActivo = -1;
	t_solicitudLectura* solicitudLectura;
	t_solicitudEscritura* solicitudEscritura;
	char* datos; //datos de la lectura
	int resultado; //resultado de la escritura
	while(1){
		paquete = recibir_paquete(socket);
		bytesRecibidos = paquete->payloadLength;
		tipo = paquete->type;
		if(bytesRecibidos>0){
			printf("Se recibio algo\n");
			switch(tipo){
				case cambioProcesoActivo:
					//deserializar para obtener el id_programa
					//procesoActivo = id_programa;
					break;
				case lectura:
					sleep(retardo);
					//desserializar estructura con la base,offset y tamaño
					solicitudLectura = desserializarSolicitudLectura(paquete->payload);
					pthread_mutex_lock(mutexSegmentos);
					datos = leer(procesoActivo,solicitudLectura);
					pthread_mutex_unlock(mutexSegmentos);
					//validar si hay segmentation fault
					if(datos!=NULL){
						printf("Los datos obtenidos son: %s\n",datos);
						// responder a la cpu
						respuesta = crear_paquete(respuestaUmv,datos,solicitudLectura->tamanio);
						enviar_paquete(respuesta,socket);
					} else {
						printf("Violación de segmento!!!\n");
						printf("Solicitud lectura:\n");
						printf("Id_programa: %d, base: %d, offset: %d y tamaño: %d\n",procesoActivo,solicitudLectura->base,solicitudLectura->offset,solicitudLectura->tamanio);
						resultado = -1;
						memcpy(answer, &resultado, sizeof(t_puntero));
						respuesta = crear_paquete(respuestaUmv,answer,sizeof(t_puntero));
						enviar_paquete(respuesta,socket);
					}
					break;
				case escritura:
					sleep(retardo);
					//desserializar estructura con la base,offset,tamaño y buffer
					solicitudEscritura = desserializarSolicitudEscritura(paquete->payload);
					pthread_mutex_lock(mutexSegmentos);
					resultado = escribir(procesoActivo,solicitudEscritura);
					pthread_mutex_unlock(mutexSegmentos);
					//validar si hay segmentation fault
					if(resultado >= 0){
						printf("La operacion de escritura termino correctamente\n");
						memcpy(answer, &resultado, sizeof(t_puntero));
						respuesta = crear_paquete(respuestaUmv,answer,sizeof(t_puntero));
						enviar_paquete(respuesta,socket);
					} else {
						printf("Violación de segmento!!!\n");
						printf("Solicitud escritura:\n");
						printf("Id_programa: %d, base: %d, offset: %d y tamaño: %d\n",procesoActivo,solicitudEscritura->base,solicitudEscritura->offset,solicitudEscritura->tamanio);
						resultado = -1;
						memcpy(answer, &resultado, sizeof(t_puntero));
						respuesta = crear_paquete(respuestaUmv,answer,sizeof(t_puntero));
						enviar_paquete(respuesta,socket);
					}
					break;
				default:
					printf("Tipo de mensaje invalido\n");
					//notificar a la cpu que el mensaje es invalido???
			}	
		}
	}	
	return 0;
}

void* atenderConsola(){
	char buffer[BUFFERSIZE];
	int id_programa;
	int offset;
	int tamanio;
	char* datos;
	int estado;
	t_crearSegmentoUMV* crear;
	t_solicitudLectura* reader;
	t_solicitudEscritura* writer;
	printf("Hilo que atiende la consola esperando entrada por teclado...\n");
	while(1){
		printf("Ingrese un comando\n");
		/* Leo que se ingreso por consola */
		fgets(buffer,BUFFERSIZE,stdin);
		char** palabras = string_split(buffer, " ");
		/* la ultima palabra del comando queda con el salto de linea por eso hay que tener
		 * en cuenta el \n si es la ultima palabra */

		if(strcmp(palabras[0],"operacion")==0){
			// validar  que operacion es
			id_programa = atoi(palabras[2]);
			if (strcmp(palabras[1],"lectura")==0){
				// es un pedido de lectura
				reader = malloc(sizeof(t_solicitudLectura));
				reader->base = atoi(palabras[3]);
				reader->offset = atoi(palabras[4]);
				reader->tamanio = atoi(palabras[5]);
				printf("Leyendo %d bytes en el segmento del programa %d con base %d\n",reader->tamanio,id_programa,reader->base);
				pthread_mutex_lock(mutexSegmentos);
				datos = leer(id_programa,reader);
				pthread_mutex_unlock(mutexSegmentos);
				if(datos!=NULL){
					printf("Los datos obtenidos son: %s\n",datos);
				} else {
					printf("Violación de segmento!!!\n");
				}
				free(reader);
			} else if (strcmp(palabras[1],"escritura")==0){
				// es un pedido de escritura
				writer = malloc(sizeof(t_solicitudEscritura));
				writer->base = atoi(palabras[3]);
				writer->offset = atoi(palabras[4]);
				writer->tamanio = atoi(palabras[5]);
				writer->buffer = malloc(writer->tamanio);
				memcpy(writer->buffer,palabras[6],writer->tamanio);
				printf("Escribiendo %d bytes en el segmento del programa %d con base %d\n",writer->tamanio,id_programa,writer->base);
				printf("El buffer es %s\n",writer->buffer);
				pthread_mutex_lock(mutexSegmentos);
				estado = escribir(id_programa,writer);
				pthread_mutex_unlock(mutexSegmentos);
				if(estado >= 0){
					printf("La operacion de escritura termino correctamente\n");
				} else {
					printf("Violación de segmento!!!\n");
				}
				free(writer);
			} else if (strcmp(palabras[1],"crear_seg")==0){
				// es un pedido de creacion de segmento
				crear = malloc(sizeof(t_crearSegmentoUMV));
				crear->programid = atoi(palabras[2]);
				crear->size = atoi(palabras[3]);
				pthread_mutex_lock(mutexSegmentos);
				estado = crear_segmento(crear);
				pthread_mutex_unlock(mutexSegmentos);
				//TODO informar si habia espacio suficiente y se pudo crear correctamente
				if(estado>=0){
					printf("El segmento del %d de tamaño %d se creo correctamente\n",crear->programid,crear->size);
				} else {
					printf("No hubo espacio suficiente, se realizara la compactacion y se volvera a intentar\n");
					pthread_mutex_lock(mutexSegmentos);
					compactar();
					pthread_mutex_unlock(mutexSegmentos);
					pthread_mutex_lock(mutexSegmentos);
					estado = crear_segmento(crear);
					pthread_mutex_unlock(mutexSegmentos);
					if(estado>=0){
						printf("El segmento del %d de tamaño %d se creo correctamente\n",crear->programid,crear->size);
					} else {
						printf("Violación de segmento!!!\n");
					}
				}
			} else if (strcmp(palabras[1],"destruir_seg")==0){
				// es un pedido de destruccion de segmentos
				id_programa = atoi(palabras[2]);
				pthread_mutex_lock(mutexSegmentos);
				estado = destruir_segmentos(id_programa);
				pthread_mutex_unlock(mutexSegmentos);
				if(estado >= 0){
					printf("Los segmentos del programa %d se borraron correctamente\n",id_programa);
				} else {
					printf("No habia segmentos del programa\n");
				}
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
			pthread_mutex_lock(mutexSegmentos);
			compactar();
			pthread_mutex_unlock(mutexSegmentos);
			printf("Compactación terminada\n");

		} else if (strcmp(palabras[0],"dump")==0){
			// imprimir reporte
			if (strcmp(palabras[1],"estructuras")==0){
			// imprimir tabla de segmentos por proceso en memoria
				id_programa = atoi(palabras[2]);
				pthread_mutex_lock(mutexSegmentos);
				imprimir_estructuras(id_programa);
				pthread_mutex_unlock(mutexSegmentos);
			} else if (strcmp(palabras[1],"memoria\n")==0){
			// imprimir segmentos de la memoria incluyendo espacios libres
				pthread_mutex_lock(mutexSegmentos);
				imprimir_segmentos_memoria();
				pthread_mutex_unlock(mutexSegmentos);
			} else if (strcmp(palabras[1],"contenido")==0){
			// imprimir contenido dado un offset y una cantidad de bytes
				offset = atoi(palabras[2]);
				tamanio = atoi(palabras[3]);
				pthread_mutex_lock(mutexSegmentos);
				imprimir_contenido(offset,tamanio);
				pthread_mutex_unlock(mutexSegmentos);
			} else {
				printf("Comando no reconocido, intente de nuevo\n");
			}
		} else {
			printf("Comando no reconocido, intente de nuevo\n");
		}
	}
}
/* Devuelve si el segmento siguiente tiene un id_programa mayor */
int _menor_id_programa(t_segmento *seg, t_segmento *segMayor) {
	return seg->id_programa < segMayor->id_programa;
}
/* Devuelve si el segmento siguiente es de mayor tamaño */
int _mayor_tamanio(t_segmento *seg, t_segmento *segMayor) {
	return seg->tamanio >= segMayor->tamanio;
}
/* Devuelve si esta vacio el segmento */
int _esta_vacio(t_segmento* seg){
	return seg->id_programa == -1;
}

t_segmento* _existe_algun_seg(int id_programa){
	t_segmento* seg_aux;
	int i;
	int cant_seg=list_size(segmentos);
	for(i=0;i<cant_seg;i++){
		seg_aux = list_get(segmentos,i);
		if(seg_aux->id_programa == id_programa){
			return seg_aux;
			break;
		}
	}
	return NULL;
}

/* Devuelve el segmento del programa y base indicado
 * de no encontrar el segmento devuelve NULL*/
t_segmento* buscarSegmento(int id_programa,int base){
	t_segmento* seg_aux;
	int i;
	int cant_seg=list_size(segmentos);
	for(i=0;i<cant_seg;i++){
		seg_aux = list_get(segmentos,i);
		if(seg_aux->base_logica == base && seg_aux->id_programa == id_programa){
			return seg_aux;
			break;
		}
	}
	return NULL;
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
char* leer(int id_programa,t_solicitudLectura* solicitud){
	int paraLeer;
	char* datos = malloc(solicitud->tamanio);
	t_segmento* segmentoBuscado = buscarSegmento(id_programa,solicitud->base);
	if(segmentoBuscado!=NULL){
		paraLeer = solicitud->base + solicitud->offset + solicitud->tamanio;
		int maximo = segmentoBuscado->tamanio + segmentoBuscado->base_logica;
		if(segmentoBuscado->base_logica <= paraLeer && paraLeer <= maximo){
			memcpy(datos,segmentoBuscado->base+solicitud->offset,solicitud->tamanio);
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
int escribir(int id_programa,t_solicitudEscritura* solicitud){
	int paraEscribir;
	t_segmento* segmentoBuscado = buscarSegmento(id_programa,solicitud->base);
	if(segmentoBuscado!=NULL){
		paraEscribir = solicitud->base + solicitud->offset + solicitud->tamanio;
		int maximo = segmentoBuscado->tamanio + segmentoBuscado->base_logica;
		if(segmentoBuscado->base_logica <= paraEscribir && paraEscribir <= maximo ){
			memcpy(segmentoBuscado->base+solicitud->offset,solicitud->buffer,solicitud->tamanio);
			return solicitud->tamanio;
		} else {
			return -1;
		}
	} else {
		return -1;
	}
}

/* Dado un id_programa y un tamaño debe validar que haya espacio suficiente segun el algoritmo actual
 * Si hay espacio debe crear el segmento y retornar un valor >= 0
 * Caso contrario devuelve -1
 */
int crear_segmento(t_crearSegmentoUMV* datos){
	int resultado;
	switch(algoritmo){
		case WORSTFIT:
			printf("Creando segmento del programa %d con tamaño %d con algoritmo Worst-Fit\n",datos->programid,datos->size);
			resultado = worst_fit(segmentos,datos->programid,datos->size);
			break;
		case FIRSTFIT:
			printf("Creando segmento del programa %d con tamaño %d con algoritmo First-Fit\n",datos->programid,datos->size);
			resultado = first_fit(segmentos,datos->programid,datos->size);
			break;
		default:
			printf("El algoritmo seteado no es valido\n");
	}
	return resultado;
}

/* Destruye todos los segmentos de un programa
 * En caso de que no exista ningun segmento de ese programa devuelve -1
 * Caso contrario devuelve 1
 */
int destruir_segmentos(int id_programa){
	t_segmento* seg_aux =_existe_algun_seg(id_programa);
	if(seg_aux==NULL){
		return -1; //no hay ningun segmento de ese programa
	} else {
		int i;
		t_segmento* aux;
		int cant_seg=list_size(segmentos);
		for(i=0;i<cant_seg;i++){
			aux = list_get(segmentos,i);
			if(aux->id_programa == id_programa){
				printf("Segmento del programa %d encontrado de un tamaño %d\n",id_programa,aux->tamanio);
				aux->base_logica = 0;
				aux->id_programa = -1;
			}
		}
		return 1;
	}
}

int compactar(){
	int i;
	t_segmento* aux;
	t_segmento* libre = malloc(sizeof(t_segmento));
	libre->id_programa = -1;
	libre->base = 0;
	libre->tamanio = 0;
	int cant_seg=list_size(segmentos);
	printf("Cantidad total de segmentos: %d\n",cant_seg);
	//sumo el espacio libre total
	for(i=0;i<cant_seg;i++){
		aux = list_get(segmentos,i);
		if(aux->id_programa == -1){
			libre->tamanio += aux->tamanio;
		}
	}
	printf("El espacio vacio total es %d\n",libre->tamanio);
	//elimino todos los segmentos vacios
	for(i=0;i<cant_seg;i++){
		aux =  list_remove_by_condition(segmentos, (void*) _esta_vacio);
		free(aux);
		cant_seg = list_size(segmentos);
	}
	printf("Segmentos vacios eliminados\n");
	printf("Cantidad total de segmentos: %d\n",list_size(segmentos));
	int offsetAcumulado = 0;
	char* baseNueva;
	for(i=0;i<list_size(segmentos);i++){
		aux = list_get(segmentos,i);
		printf("Copiando datos del segmento %d a partir del offset %d con un tamaño de %d\n",i,offsetAcumulado,aux->tamanio);
		baseNueva = bloqueDeMemoria + offsetAcumulado;
		memcpy(baseNueva,aux->base,aux->tamanio);
		aux->base = baseNueva;
		offsetAcumulado += aux->tamanio;
		printf("El offset siguiente es %d\n",offsetAcumulado);
	}
	list_add(segmentos,libre);
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
int first_fit(t_list* lista,int id_programa,int tamanio){
	int i;
	int resultado = -1;
	t_segmento* aux;
	t_segmento* vacio = malloc(sizeof(t_segmento));
	int cant_seg=list_size(lista);
	for(i=0;i<cant_seg;i++){
		aux = list_get(lista,i);
		if(_esta_vacio(aux)){
			printf("Se encontro un segmento vacio\n");
			if(aux->tamanio >= tamanio){
				printf("Tiene un tamaño mayor o igual que el solicitado\n");
				//si el tamanio es igual al solicitado lo reemplazo directamente
				//sino debe ser reemplazado por dos segmentos
				//el usado y el espacio que queda libre
				aux->id_programa = id_programa;
				aux->base_logica = calcularBaseLogica(id_programa);
				if(aux->tamanio == tamanio){
					printf("Era igual al tamaño solicitado\n");
					list_replace(lista,i,aux);
				} else {
					printf("Era mas grande\n");
					vacio->base = aux->base + tamanio;
					vacio->id_programa = -1;
					vacio->tamanio = aux->tamanio - tamanio;
					vacio->base_logica = 0;
					aux->tamanio = tamanio;
					list_replace(lista,i,aux);
					list_add_in_index(lista,i+1,vacio);
				}
				resultado = aux->base_logica;
			}
		}
	}
	printf("La base logica del segmento creado es %d\n",resultado);
	return resultado;
}

/* Crea un nuevo segmento con el algoritmo WORST-FIT
 * Devuelve -1 en caso de no encontrar espacio suficiente para el segmento
 * Caso contrario devuelve la posicion en la lista
 */
int worst_fit(t_list* lista,int id_programa,int tamanio){
	t_list* listaOrdenada = list_take(lista,list_size(segmentos));// revisar si se hace una copia o es la misma lista!!!!
	//ordeno la lista por tamanio descendente
	list_sort(listaOrdenada,(void*)_mayor_tamanio);
	int i,j;
	int resultado = -1;
	t_segmento* auxOrd;
	t_segmento* aux;
	t_segmento* vacio = malloc(sizeof(t_segmento));
	int cant_seg=list_size(listaOrdenada);
	for(i=0;i<cant_seg;i++){
		auxOrd = list_get(listaOrdenada,i);
		if(auxOrd->id_programa == -1){
			if(auxOrd->tamanio >= tamanio){
				//busco el segmento en la lista original
				for(j=0;j<=cant_seg;j++){
					aux = list_get(lista,j);
					if(aux->id_programa==-1 && aux->base==auxOrd->base){
						//si el tamanio es igual al solicitado lo reemplazo directamente
						//sino debe ser reemplazado por dos segmentos
						//el usado y el espacio que queda libre
						aux->id_programa = id_programa;
						aux->base_logica = calcularBaseLogica(id_programa);
						if(aux->tamanio == tamanio){
							list_replace(lista,j,aux);
						} else {
							vacio->base = aux->base + tamanio;
							vacio->id_programa = -1;
							vacio->tamanio = aux->tamanio - tamanio;
							aux->tamanio = tamanio;
							list_replace(lista,j,aux);
							list_add_in_index(lista,j+1,vacio);
						}
						resultado = aux->base_logica;
					}
				}
			}
		}
	}
	printf("La base logica del segmento creado es %d\n",resultado);
	return resultado;

}

t_puntero calcularBaseLogica(int id_programa){
	int i;
	t_segmento* aux;
	t_puntero resultado;
	int baseMaxima = 0;
	int tamanioMaximo = 0;
	int cant_seg=list_size(segmentos);
	for(i=0;i<cant_seg;i++){
		aux = list_get(segmentos,i);
		if(aux->id_programa == id_programa && aux->base_logica >= baseMaxima){
			baseMaxima = aux->base_logica;
			tamanioMaximo = aux->tamanio;
		}
	}
	if(baseMaxima > 0 && tamanioMaximo >0){
		resultado = baseMaxima + tamanioMaximo;
	} else {
		resultado = 0;
	}
	return resultado;
}

