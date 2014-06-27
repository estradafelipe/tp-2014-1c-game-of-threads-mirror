
#include "atencion.h"
#include <paquetes.h>
#include <serializadores.h>

#define BUFFERSIZE 128
#define WORSTFIT 0
#define FIRSTFIT 1

void* atenderNuevaConexion(void* parametro){
	int socketCliente = (int) parametro;
	package* respuesta;
	printf("Esperando handshake...\n");
	package* paquete = recibir_paquete(socketCliente);
	int bytesRecibidos = paquete->payloadLength;
	t_paquete tipo = paquete->type;
	if(bytesRecibidos>0){
		printf("Se recibio algo\n");
		switch(tipo){
			case handshakeKernelUmv:
				log_info(logger, "Bienvenido KERNEL");
				respuesta = crear_paquete(handshakeKernelUmv,"Hola Kernel",strlen("Hola Kernel")+1);
				enviar_paquete(respuesta,socketCliente);
				destruir_paquete(respuesta);
				atenderKernel(socketCliente);
				break;
			
			case handshakeCpuUmv:
				log_info(logger, "Bienvenida CPU");
				respuesta = crear_paquete(handshakeCpuUmv,"Hola Cpu",strlen("Hola Cpu")+1);
				enviar_paquete(respuesta,socketCliente);
				destruir_paquete(respuesta);
				atenderCpu(socketCliente);
				break;
			
			default:
				printf("No anda el tipo de paquete :(\n");
				break;
		}
		destruir_paquete(paquete);
	}
	if (bytesRecibidos==-1) {
		log_debug(logger, "BytesRecibidos == -1, Error al recibir datos");
		perror("Error al recibir datos");
		destruir_paquete(paquete);
	} else if (bytesRecibidos==0) {	//Se desconecto
		log_debug(logger, "Se desconecto y no se pudo realizar el handshake");
		destruir_paquete(paquete);
		exit(0);
	}
	return (void*)socketCliente;// para que no rompa las bolas con que la funcion no retorna un void*
}

/* Atiende solicitudes del kernel */
/* quedarse esperando solicitudes de creacion  o eliminacion de segmentos de programas */
int atenderKernel(int fd){
	package* paquete;
	package* respuesta;
	char* answer = malloc(sizeof(t_pun));
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
		paquete = recibir_paquete(fd);
		bytesRecibidos = paquete->payloadLength;
		tipo = paquete->type;
		if(bytesRecibidos>0){
			log_debug(logger, "Se recibio algo del KERNEL");
			switch(tipo){
				case creacionSegmentos:
					sleep(retardo);
					log_info(logger, "Es un pedido de creacion de segmento");
					//desserializar estructura con id_programa y tamaño del segmento
					creacionSegmento = deserializarSolicitudSegmento(paquete->payload);
					//guardo el proceso activo (para la escritura de los segmentos)
					procesoActivo = creacionSegmento->programid;
					pthread_rwlock_wrlock(&lockSegmentos);
					resultado = crear_segmento(creacionSegmento);
					pthread_rwlock_unlock(&lockSegmentos);
					if(resultado>=0){ // se creo el segmento correctamente
						log_info(logger, "El segmento del %d de tamaño %d se creo correctamente",creacionSegmento->programid,creacionSegmento->size);
						memcpy(answer, &resultado, sizeof(t_pun));
						respuesta = crear_paquete(respuestaUmv,answer,sizeof(t_pun));
						enviar_paquete(respuesta,fd);
					} else {
						log_info(logger, "No hubo espacio suficiente, se realizara la compactacion y se volvera a intentar");

						pthread_rwlock_wrlock(&lockSegmentos);
						pthread_rwlock_wrlock(&lockMemoria);
						compactar();
						pthread_rwlock_unlock(&lockMemoria);
						pthread_rwlock_unlock(&lockSegmentos);

						log_debug(logger, "Volviendo a intentar la creación del segmento");

						pthread_rwlock_wrlock(&lockSegmentos);
						resultado = crear_segmento(creacionSegmento);
						pthread_rwlock_unlock(&lockSegmentos);

						if(resultado>=0){//se creo el segmento correctamente
							log_info(logger, "El segmento del %d de tamaño %d se creo correctamente",creacionSegmento->programid,creacionSegmento->size);
							memcpy(answer, &resultado, sizeof(t_pun));
							respuesta = crear_paquete(respuestaUmv,answer,sizeof(t_pun));
							enviar_paquete(respuesta,fd);
						} else {
							log_info(logger, "No hubo espacio suficiente, Memory Overload!!");
							memcpy(answer, &resultado, sizeof(t_pun));
							respuesta = crear_paquete(respuestaUmv,answer,sizeof(t_pun));
							enviar_paquete(respuesta,fd);
						}
					}
					destruir_paquete(respuesta);
					break;
				case destruccionSegmentos:
					sleep(retardo);
					//obtengo id_programa del paquete
					memcpy(&id_programa,paquete->payload,sizeof(t_pun));
					log_info(logger, "Es un pedido de destruccion de segmentos del programa %d",id_programa);
					pthread_rwlock_wrlock(&lockSegmentos);
					resultado = destruir_segmentos(id_programa);
					pthread_rwlock_unlock(&lockSegmentos);
					memcpy(answer, &resultado, sizeof(t_pun));
					respuesta = crear_paquete(respuestaUmv,answer,sizeof(t_pun));
					enviar_paquete(respuesta,fd);
					destruir_paquete(respuesta);

					break;
				case lectura:
					sleep(retardo);
					//desserializar estructura con la base,offset y tamaño
					solicitudLectura = desserializarSolicitudLectura(paquete->payload);
					log_info(logger, "Es un pedido de lectura con id_programa: %d, base: %d, offset: %d y tamaño: %d",procesoActivo,solicitudLectura->base,solicitudLectura->offset,solicitudLectura->tamanio);
					pthread_rwlock_rdlock(&lockSegmentos);
					datos = leer(procesoActivo,solicitudLectura);
					pthread_rwlock_unlock(&lockSegmentos);
					//validar si hay segmentation fault
					if(datos!=NULL){
						log_info(logger, "El pedido de lectura fue valido");
						log_debug(logger, "Los datos obtenidos son: %s",generarHexa(datos,solicitudLectura->tamanio));
						// responder a la cpu
						respuesta = crear_paquete(respuestaUmv,datos,solicitudLectura->tamanio);
						enviar_paquete(respuesta,fd);
					} else {
						log_info(logger, "Violación de segmento!");
						resultado = -1;
						memcpy(answer, &resultado, sizeof(t_pun));
						respuesta = crear_paquete(respuestaUmv,answer,sizeof(t_pun));
						enviar_paquete(respuesta,fd);
					}
					destruir_paquete(respuesta);
					break;
				case escritura:
					sleep(retardo);
					//desserializar estructura con la base,offset,tamaño y buffer
					solicitudEscritura = desserializarSolicitudEscritura(paquete->payload);
					log_info(logger, "Es un pedido de escritura con id_programa: %d, base: %d, offset: %d, tamaño: %d, buffer: %x",procesoActivo,solicitudEscritura->base,solicitudEscritura->offset,solicitudEscritura->tamanio,*solicitudEscritura->buffer);
					pthread_rwlock_rdlock(&lockSegmentos);
					resultado = escribir(procesoActivo,solicitudEscritura);
					pthread_rwlock_unlock(&lockSegmentos);
					//validar si hay segmentation fault
					if(resultado >= 0){
						log_info(logger, "La operacion de escritura termino correctamente");
						memcpy(answer, &resultado, sizeof(t_pun));
						respuesta = crear_paquete(respuestaUmv,answer,sizeof(t_pun));
						enviar_paquete(respuesta,fd);
					} else {
						log_info(logger, "Violación de segmento!");
						resultado = -1;
						memcpy(answer, &resultado, sizeof(t_pun));
						respuesta = crear_paquete(respuestaUmv,answer,sizeof(t_pun));
						enviar_paquete(respuesta,fd);
					}
					destruir_paquete(respuesta);
					break;
				default:
					log_debug(logger, "Tipo de mensaje invalido");
					//notificar al kernel que el mensaje es invalido???
			}
			destruir_paquete(paquete);
		}
		if (bytesRecibidos==-1) {
			printf("BytesRecibidos == -1\n ");
			perror("Error al recibir datos");
			destruir_paquete(paquete);
		} else if (bytesRecibidos==0) {	//Se desconecto
			log_info(logger, "Se desconecto el Kernel");
			destruir_paquete(paquete);
			exit(0);
		}
	}
	log_debug(logger, "Hilo que atiende al KERNEL termino");
	free(answer);
	return 0;
}

/* Atiende solicitudes de la cpu */
/* primero debo saber que programa esta activo
 * luego esperar solicitudes de escritura y lectura en los segmentos de dicho programa,
 * o el cambio del programa activo
 */
int atenderCpu(int fd){
	package* paquete;
	package* respuesta;
	char* answer = malloc(sizeof(t_pun));
	int bytesRecibidos;
	t_paquete tipo;
	int procesoActivo = -1;
	t_solicitudLectura* solicitudLectura;
	t_solicitudEscritura* solicitudEscritura;
	char* datos; //datos de la lectura
	int resultado; //resultado de la escritura
	while(1){
		paquete = recibir_paquete(fd);
		bytesRecibidos = paquete->payloadLength;
		tipo = paquete->type;
		if(bytesRecibidos>0){
			log_debug(logger, "Se recibio algo de una CPU");
			switch(tipo){
				case cambioProcesoActivo:
					//deserializar para obtener el id_programa
					memcpy(&resultado,paquete->payload,sizeof(t_pun));
					procesoActivo = resultado;
					log_debug(logger, "Cambio de proceso activo. Ahora es el programa %d",procesoActivo);
					respuesta = crear_paquete(respuestaUmv,"Cambio ok",strlen("Cambio ok") + 1);
					enviar_paquete(respuesta,fd);
					destruir_paquete(respuesta);
					break;
				case lectura:
					sleep(retardo);
					//desserializar estructura con la base,offset y tamaño
					solicitudLectura = desserializarSolicitudLectura(paquete->payload);
					log_debug(logger, "Es un pedido de lectura con id_programa: %d, base: %d, offset: %d y tamaño: %d",procesoActivo,solicitudLectura->base,solicitudLectura->offset,solicitudLectura->tamanio);
					pthread_rwlock_rdlock(&lockSegmentos);
					datos = leer(procesoActivo,solicitudLectura);
					pthread_rwlock_unlock(&lockSegmentos);
					//validar si hay segmentation fault
					if(datos!=NULL){
						log_debug(logger, "El pedido de lectura fue valido");
						log_debug(logger, "Los datos obtenidos son: %s",generarHexa(datos,solicitudLectura->tamanio));
						// responder a la cpu
						respuesta = crear_paquete(respuestaUmv,datos,solicitudLectura->tamanio);
						enviar_paquete(respuesta,fd);
					} else {
						log_debug(logger, "Violación de segmento!");
						resultado = -1;
						memcpy(answer, &resultado, sizeof(t_pun));
						respuesta = crear_paquete(respuestaUmv,answer,sizeof(t_pun));
						enviar_paquete(respuesta,fd);
					}
					destruir_paquete(respuesta);
					break;
				case escritura:
					sleep(retardo);
					//desserializar estructura con la base,offset,tamaño y buffer
					solicitudEscritura = desserializarSolicitudEscritura(paquete->payload);
					log_debug(logger, "Es un pedido de escritura con id_programa: %d, base: %d, offset: %d, tamaño: %d, buffer: %x",procesoActivo,solicitudEscritura->base,solicitudEscritura->offset,solicitudEscritura->tamanio,*solicitudEscritura->buffer);
					pthread_rwlock_rdlock(&lockSegmentos);
					resultado = escribir(procesoActivo,solicitudEscritura);
					pthread_rwlock_unlock(&lockSegmentos);
					//validar si hay segmentation fault
					if(resultado >= 0){
						log_debug(logger, "La operacion de escritura termino correctamente");
						memcpy(answer, &resultado, sizeof(t_pun));
						respuesta = crear_paquete(respuestaUmv,answer,sizeof(t_pun));
						enviar_paquete(respuesta,fd);
					} else {
						log_debug(logger, "Violación de segmento!");
						resultado = -1;
						memcpy(answer, &resultado, sizeof(t_pun));
						respuesta = crear_paquete(respuestaUmv,answer,sizeof(t_pun));
						enviar_paquete(respuesta,fd);
					}
					destruir_paquete(respuesta);
					break;
				default:
					log_debug(logger, "Tipo de mensaje invalido");
					//notificar a la cpu que el mensaje es invalido???
			}
			destruir_paquete(paquete);
		}
		if (bytesRecibidos==-1) {
			printf("BytesRecibidos == -1\n ");
			perror("Error al recibir datos");
			destruir_paquete(paquete);
		} else if (bytesRecibidos==0) {	//Se desconecto
			log_debug(logger, "Se desconecto una CPU");
			destruir_paquete(paquete);
			exit(0);
		}
	}
	log_debug(logger, "Hilo que atiende una CPU termino");
	free(answer);
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
	if(logConsola){
		log_info(loggerConsola,"Hilo que atiende la consola esperando entrada por teclado");
	}
	while(1){
		printf("Ingrese un comando\n");
		/* Leo que se ingreso por consola */
		fgets(buffer,BUFFERSIZE,stdin);
		char** palabras = string_split(buffer, " ");
		/* la ultima palabra del comando queda con el salto de linea por eso hay que tener
		 * en cuenta el \n si es la ultima palabra */
		if(logConsola){
			log_debug(loggerConsola,"Se ingreso un comando\n");
		}
		if(strcmp(palabras[0],"operacion")==0){
			// validar  que operacion es
			id_programa = atoi(palabras[2]);
			if (strcmp(palabras[1],"lectura")==0){
				// es un pedido de lectura
				if(logConsola){
					log_debug(loggerConsola,"Es un pedido de lectura");
				}
				reader = malloc(sizeof(t_solicitudLectura));
				reader->base = atoi(palabras[3]);
				reader->offset = atoi(palabras[4]);
				reader->tamanio = atoi(palabras[5]);
				printf("Leyendo %d bytes en el segmento del programa %d con base %d\n",reader->tamanio,id_programa,reader->base);
				if(logConsola){
					log_debug(loggerConsola,"Leyendo %d bytes en el segmento del programa %d con base %d",reader->tamanio,id_programa,reader->base);
				}
				pthread_rwlock_rdlock(&lockSegmentos);
				datos = leer(id_programa,reader);
				pthread_rwlock_unlock(&lockSegmentos);
				if(datos!=NULL){
					printf("Los datos obtenidos son: %s\n",generarHexa(datos,reader->tamanio));
					if(logConsola){
						log_debug(loggerConsola,"Los datos obtenidos son: %s",generarHexa(datos,reader->tamanio));
					}
				} else {
					printf("Violación de segmento!!!\n");
					if(logConsola){
						log_debug(loggerConsola,"Violación de segmento!!!\n");
					}
				}
				free(reader);
			} else if (strcmp(palabras[1],"escritura")==0){
				// es un pedido de escritura
				if(logConsola){
					log_debug(loggerConsola,"Es un pedido de escritura");
				}
				writer = malloc(sizeof(t_solicitudEscritura));
				writer->base = atoi(palabras[3]);
				writer->offset = atoi(palabras[4]);
				writer->tamanio = atoi(palabras[5]);
				writer->buffer = malloc(writer->tamanio);
				memcpy(writer->buffer,palabras[6],writer->tamanio);
				printf("Escribiendo %d bytes en el segmento del programa %d con base %d\n",writer->tamanio,id_programa,writer->base);
				if(logConsola){
					log_debug(loggerConsola,"Escribiendo %d bytes en el segmento del programa %d con base %d",writer->tamanio,id_programa,writer->base);
				}
				pthread_rwlock_rdlock(&lockSegmentos);
				estado = escribir(id_programa,writer);
				pthread_rwlock_unlock(&lockSegmentos);
				if(estado >= 0){
					printf("La operacion de escritura termino correctamente\n");
					if(logConsola){
						log_debug(loggerConsola,"La operacion de escritura termino correctamente\n");
					}
				} else {
					printf("Violación de segmento!!!\n");
					if(logConsola){
						log_debug(loggerConsola,"Violación de segmento!!!\n");
					}
				}
				free(writer);
			} else if (strcmp(palabras[1],"crear_seg")==0){
				// es un pedido de creacion de segmento
				if(logConsola){
					log_debug(loggerConsola,"Es un pedido de creación de segmento");
				}
				crear = malloc(sizeof(t_crearSegmentoUMV));
				crear->programid = atoi(palabras[2]);
				crear->size = atoi(palabras[3]);
				pthread_rwlock_wrlock(&lockSegmentos);
				estado = crear_segmento(crear);
				pthread_rwlock_unlock(&lockSegmentos);
				if(estado>=0){
					printf("El segmento del %d de tamaño %d se creo correctamente\n",crear->programid,crear->size);
					if(logConsola){
						log_debug(loggerConsola,"El segmento del %d de tamaño %d se creo correctamente\n",crear->programid,crear->size);
					}
				} else {
					printf("No hubo espacio suficiente, se realizara la compactacion y se volvera a intentar\n");
					if(logConsola){
						log_debug(loggerConsola,"No hubo espacio suficiente, se realizara la compactacion y se volvera a intentar");
					}
					pthread_rwlock_wrlock(&lockSegmentos);
					pthread_rwlock_wrlock(&lockMemoria);
					compactar();
					pthread_rwlock_unlock(&lockMemoria);
					pthread_rwlock_unlock(&lockSegmentos);

					pthread_rwlock_wrlock(&lockSegmentos);
					estado = crear_segmento(crear);
					pthread_rwlock_unlock(&lockSegmentos);

					if(estado>=0){
						printf("El segmento del %d de tamaño %d se creo correctamente\n",crear->programid,crear->size);
						if(logConsola){
							log_debug(loggerConsola,"El segmento del %d de tamaño %d se creo correctamente\n",crear->programid,crear->size);
						}
					} else {
						printf("Memory overload!!!\n");
						if(logConsola){
							log_debug(loggerConsola,"Memory overload!!!\n");
						}
					}
				}
			} else if (strcmp(palabras[1],"destruir_seg")==0){
				// es un pedido de destruccion de segmentos
				if(logConsola){
					log_debug(loggerConsola,"Es un pedido de destrucción de segmentos");
				}
				id_programa = atoi(palabras[2]);

				pthread_rwlock_wrlock(&lockSegmentos);
				estado = destruir_segmentos(id_programa);
				pthread_rwlock_unlock(&lockSegmentos);

				if(estado >= 0){
					printf("Los segmentos del programa %d se borraron correctamente\n",id_programa);
					if(logConsola){
						log_debug(loggerConsola,"Los segmentos del programa %d se borraron correctamente\n",id_programa);
					}
				} else {
					printf("No habia segmentos del programa\n");
					if(logConsola){
						log_debug(loggerConsola,"No habia segmentos del programa\n");
					}
				}
			} else {
				printf("Comando no reconocido, intente de nuevo\n");
				if(logConsola){
					log_debug(loggerConsola,"Comando no reconocido, intente de nuevo\n");
				}
			}

		} else if (strcmp(palabras[0],"retardo")==0){
			if(palabras[1]!=NULL){
				// modificar el retardo (en milisegundos)
				printf("El retardo paso de %d a ",retardo*1000);
				retardo = atoi(palabras[1])/1000;
				printf("%d milisegundos\n",retardo*1000);
				if(logConsola){
					log_debug(loggerConsola,"El retardo ha sido modificado, actualmente es %d milisegundos\n",retardo*1000);
				}
			}

		} else if (strcmp(palabras[0],"algoritmo")==0){
			// modificar el algoritmo de ubicacion de segmentos
			// worst-fit o first-fit
			//mutex del algoritmo
			if(palabras[1]!=NULL){
			pthread_rwlock_wrlock(&lockAlgoritmo);
				if (strcmp(palabras[1],"WORSTFIT\n")==0){
					algoritmo = WORSTFIT;
					pthread_rwlock_unlock(&lockAlgoritmo);
					printf("Se modifico el algortimo correctamente. El algoritmo actual es Worst-Fit\n");
					if(logConsola){
						log_debug(loggerConsola,"Se modifico el algortimo correctamente. El algoritmo actual es Worst-Fit\n");
					}
				} else if (strcmp(palabras[1],"FIRSTFIT\n")==0){
					algoritmo = FIRSTFIT;
					pthread_rwlock_unlock(&lockAlgoritmo);
					printf("Se modifico el algortimo correctamente. El algoritmo actual es First-Fit\n");
					if(logConsola){
						log_debug(loggerConsola,"Se modifico el algortimo correctamente. El algoritmo actual es First-Fit\n");
					}
				} else {
					printf("Comando no reconocido, intente de nuevo\n");
					if(logConsola){
						log_debug(loggerConsola,"Comando no reconocido, intente de nuevo\n");
					}
				}
			}

		} else if (strcmp(palabras[0],"compactacion\n")==0){
			// hacer compactacion
			printf("Compactando...\n");
			pthread_rwlock_wrlock(&lockSegmentos);
			pthread_rwlock_wrlock(&lockMemoria);
			compactar();
			pthread_rwlock_unlock(&lockMemoria);
			pthread_rwlock_unlock(&lockSegmentos);
			printf("Compactación terminada\n");
			if(logConsola){
				log_debug(loggerConsola,"Compactación realizada\n");
			}

		} else if (strcmp(palabras[0],"dump")==0){
			// imprimir reporte
			if (strcmp(palabras[1],"estructuras")==0){
			// imprimir tabla de segmentos por proceso en memoria
				id_programa = atoi(palabras[2]);
				pthread_rwlock_rdlock(&lockSegmentos);
				imprimir_estructuras(id_programa);
				pthread_rwlock_unlock(&lockSegmentos);
				printf("Revisar archivo loggerConsola.log\n");
			} else if (strcmp(palabras[1],"memoria\n")==0){
			// imprimir segmentos de la memoria incluyendo espacios libres
				pthread_rwlock_rdlock(&lockSegmentos);
				imprimir_segmentos_memoria();
				pthread_rwlock_unlock(&lockSegmentos);
				printf("Revisar archivo loggerConsola.log\n");
			} else if (strcmp(palabras[1],"contenido")==0){
			// imprimir contenido dado un offset y una cantidad de bytes
				offset = atoi(palabras[2]);
				tamanio = atoi(palabras[3]);
				pthread_rwlock_rdlock(&lockMemoria);
				imprimir_contenido(offset,tamanio);
				pthread_rwlock_unlock(&lockMemoria);
				printf("Revisar archivo loggerConsola.log\n");
			} else {
				printf("Comando no reconocido, intente de nuevo\n");
				if(logConsola){
					log_debug(loggerConsola,"Comando no reconocido, intente de nuevo\n");
				}
			}
		} else if (strcmp(palabras[0],"script\n")==0){// borrar estooo!!!!
			scriptCreacionSegmentos();

		} else {
			printf("Comando no reconocido, intente de nuevo\n");
			if(logConsola){
				log_debug(loggerConsola,"Comando no reconocido, intente de nuevo\n");
			}
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

/* Busca el segmento del programa y lee el tamaño indicado:
 * Si encuentra el semgento y el acceso es dentro del mismo devuelve los datos solicitados
 * Caso contrario devuelve NULL
 */
char* leer(int id_programa,t_solicitudLectura* solicitud){
	int paraLeer;
	char* datos = malloc(solicitud->tamanio);
	t_segmento* segmentoBuscado = buscarSegmento(id_programa,solicitud->base);
	if(segmentoBuscado!=NULL){
		paraLeer = solicitud->base + solicitud->offset + solicitud->tamanio - 1;
		int maximo = segmentoBuscado->tamanio + segmentoBuscado->base_logica - 1;
		if(segmentoBuscado->base_logica <= paraLeer && paraLeer <= maximo){
			pthread_rwlock_rdlock(&lockMemoria);
			memcpy(datos,segmentoBuscado->base+solicitud->offset,solicitud->tamanio);
			pthread_rwlock_unlock(&lockMemoria);
			return datos;
		} else {
			return NULL;
		}
	} else {
		return NULL;
	}

}

/*Busca el segmento del programa y escribe los datos:
 * Si encuentra el segmento y el acceso es dentro del mismo devuelve 1
 * Caso contrario devuelve -1
 */
int escribir(int id_programa,t_solicitudEscritura* solicitud){
	int paraEscribir;
	t_segmento* segmentoBuscado = buscarSegmento(id_programa,solicitud->base);
	if(segmentoBuscado!=NULL){
		paraEscribir = solicitud->base + solicitud->offset + solicitud->tamanio - 1;
		int maximo = segmentoBuscado->tamanio + segmentoBuscado->base_logica - 1;
		if(segmentoBuscado->base_logica <= paraEscribir && paraEscribir <= maximo ){
			pthread_rwlock_wrlock(&lockMemoria);
			memcpy(segmentoBuscado->base+solicitud->offset,solicitud->buffer,solicitud->tamanio);
			pthread_rwlock_unlock(&lockMemoria);
			return solicitud->tamanio;
		} else {
			printf("Violacion de segmento\n");
			return -1;
		}
	} else {
		return -1;
	}
}

/* Dado un id_programa y un tamaño debe validar que haya espacio suficiente segun el algoritmo actual
 * Si hay espacio debe crear el segmento y retornar la dir logica del segmento
 * Caso contrario devuelve -1
 */
int crear_segmento(t_crearSegmentoUMV* datos){
	int resultado = -1;
	pthread_rwlock_rdlock(&lockAlgoritmo);
	switch(algoritmo){
		case WORSTFIT:
			pthread_rwlock_unlock(&lockAlgoritmo);
			log_debug(logger, "Creando segmento del programa %d con tamaño %d con algoritmo Worst-Fit",datos->programid,datos->size);
			resultado = worst_fit(segmentos,datos->programid,datos->size);
			break;
		case FIRSTFIT:
			pthread_rwlock_unlock(&lockAlgoritmo);
			log_debug(logger, "Creando segmento del programa %d con tamaño %d con algoritmo First-Fit",datos->programid,datos->size);
			resultado = first_fit(segmentos,datos->programid,datos->size);
			break;
		default:
			log_debug(logger, "El algoritmo seteado no es valido");
	}
	return resultado;
}

/* Destruye todos los segmentos de un programa
 * En caso de que no exista ningun segmento de ese programa devuelve -1
 * Caso contrario devuelve 1
 */
int destruir_segmentos(int id_programa){
	int i;
	t_segmento* aux;
	int cant_seg=list_size(segmentos);
	for(i=0;i<cant_seg;i++){
		aux = list_get(segmentos,i);
		if(aux->id_programa == id_programa){
			log_debug(logger, "Segmento del programa %d encontrado de un tamaño %d",id_programa,aux->tamanio);
			aux->base_logica = 0;
			aux->id_programa = -1;
		}
	}
	return 1;
}

/* Realiza la compactacion */
int compactar(){
	log_debug(logger, "Realizando compactacion");
	int i, flag = 1;
	t_segmento* aux;
	t_segmento* libre = malloc(sizeof(t_segmento));
	libre->id_programa = -1;
	libre->base = 0;
	libre->tamanio = 0;
	libre->base = bloqueDeMemoria;
	int cant_seg=list_size(segmentos);
	//sumo el espacio libre total
	for(i=0;i<cant_seg;i++){
		aux = list_get(segmentos,i);
		if(aux->id_programa == -1){
			libre->tamanio += aux->tamanio;
		}
	}
	log_debug(logger, "Tamaño libre total %d",libre->tamanio);
	//elimino todos los segmentos vacios
	while(flag == 1){
		aux =  list_remove_by_condition(segmentos, (void*) _esta_vacio);
		if(aux != NULL){
			free(aux);
		} else {
			flag = 0;
		}
	}
	log_debug(logger, "Segmentos vacios eliminados");
	int offsetAcumulado = 0;
	char* baseNueva;

	for(i=0;i<list_size(segmentos);i++){
		aux = list_get(segmentos,i);
		baseNueva = bloqueDeMemoria + offsetAcumulado;
		memcpy(baseNueva,aux->base,aux->tamanio);
		aux->base = baseNueva;
		offsetAcumulado += aux->tamanio;
	}

	log_debug(logger, "Offset acumulado %d",offsetAcumulado);
	libre->base += offsetAcumulado;
	log_debug(logger, "Bases físicas actualizadas");
	list_add(segmentos,libre);
	log_debug(logger, "Segmento vacío agregado al final");
	return -1;
}

/* Crea un nuevo segmento con el algoritmo FIRST-FIT
 * Devuelve -1 en caso de no encontrar espacio suficiente para el segmento
 * Caso contrario devuelve la dir logica del segmento
 */
int first_fit(t_list* lista,int id_programa,int tamanio){
	int i;
	int resultado = -1;
	t_segmento* aux;
	t_segmento* vacio = malloc(sizeof(t_segmento));
	int cant_seg=list_size(lista);
	for(i=0;i<cant_seg;i++){
		aux = list_get(lista,i);
		if(aux->id_programa == -1){
			if(aux->tamanio >= tamanio){
				//si el tamanio es igual al solicitado lo reemplazo directamente
				//sino debe ser reemplazado por dos segmentos
				//el usado y el espacio que queda libre
				aux->base_logica = calcularBaseLogica(id_programa, tamanio);
				aux->id_programa = id_programa;
				if(aux->tamanio > tamanio){
					vacio->base = aux->base + tamanio;
					vacio->id_programa = -1;
					vacio->tamanio = aux->tamanio - tamanio;
					vacio->base_logica = 0;
					aux->tamanio = tamanio;
					list_add_in_index(lista,i+1,vacio);
				}
				resultado = aux->base_logica;
				return(resultado);
				break;
			}
		}
	}
	return(resultado);
}

/* Crea un nuevo segmento con el algoritmo WORST-FIT
 * Devuelve -1 en caso de no encontrar espacio suficiente para el segmento
 * Caso contrario devuelve la dir logica del segmento
 */
int worst_fit(t_list* lista,int id_programa,int tamanio){
	t_list* listaOrdenada = list_take(lista,list_size(segmentos));
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
						aux->base_logica = calcularBaseLogica(id_programa,tamanio);
						aux->id_programa = id_programa;
						if(aux->tamanio > tamanio){
							vacio->base = aux->base + tamanio;
							vacio->id_programa = -1;
							vacio->tamanio = aux->tamanio - tamanio;
							aux->tamanio = tamanio;
							list_add_in_index(lista,j+1,vacio);
						}
						resultado = aux->base_logica;
						return(resultado);
						break;
					}
				}
			}
		}
	}
	return(resultado);
}

/* Calcula la base logica del segmento aleatoriamente */
t_pun calcularBaseLogica(int id_programa, int tamanio){
	int validacion = -1;
	t_pun resultado;
	while(validacion == -1){
		resultado = rand()%1000000;
		validacion = validarBaseLogica(id_programa,tamanio, resultado);
	}
	return resultado;
}

/* Verifica si la base calculada es valida */
int validarBaseLogica(int id, int tamanio, t_pun aleatorio){
	int i;
	int retorno = -1;
	t_segmento* aux;
	t_segmento* seg_aux =_existe_algun_seg(id);;
	int base;
	int final;
	int finalCalculado = aleatorio + tamanio;
	int cant_seg=list_size(segmentos);
	if(seg_aux != NULL){
		for(i=0;i<cant_seg;i++){
			aux = list_get(segmentos,i);
			if(aux->id_programa == id){
				base = aux->base_logica;
				final = aux->base_logica + aux->tamanio - 1;
				if((aleatorio >= base && aleatorio <= final) || (finalCalculado >= base && finalCalculado <= final) ){
					retorno = -1;
				} else {
					retorno = 1;
				}
			}
		}
	} else {
		retorno = 1;
	}
	return retorno;
}

/* Imprime la tabla de segmentos del programa que se le pase por parametro
 * o de todos los programas si se le pasa -1
 */
int imprimir_estructuras(int id_programa){
	int i;
	t_segmento* aux;
	int cant_seg=list_size(segmentos);
	log_debug(loggerConsola,"**********************************************");
	if(id_programa < 0){ //todos los procesos
		t_list* listaOrdenada = list_take(segmentos,list_size(segmentos)); // lista ordenada por id_programa
		list_sort(listaOrdenada,(void*)_menor_id_programa);
		for(i=0;i<cant_seg;i++){
			aux = list_get(listaOrdenada,i);
			if(!_esta_vacio(aux)){
				log_debug(loggerConsola,"Id_programa: %d, Base logica: %d, Base real: %p, Tamaño: %d",aux->id_programa, aux->base_logica,aux->base,aux->tamanio);
			}
		}
	} else if (id_programa >= 0) { //de un proceso en particular
		for(i=0;i<cant_seg;i++){
			aux = list_get(segmentos,i);
			if(aux->id_programa == id_programa){
				log_debug(loggerConsola,"Id_programa: %d, Base logica: %d, Base real: %p, Tamaño: %d",aux->id_programa, aux->base_logica,aux->base,aux->tamanio);
			}
		}
	}
	log_debug(loggerConsola,"**********************************************\n");
	return 0;
}

/* Imprime la tabla de todos los segmentos en memoria (incluidos los vacios) */
int imprimir_segmentos_memoria(){
	int i;
	t_segmento* aux;
	int cant_seg=list_size(segmentos);
	log_debug(loggerConsola,"**********************************************");
	for(i=0;i<cant_seg;i++){
		aux = list_get(segmentos,i);
		if(aux->id_programa != -1){
			log_debug(loggerConsola,"Id_programa: %d, Base logica: %d, Base real: %p, Tamaño: %d",aux->id_programa, aux->base_logica,aux->base,aux->tamanio);
		} else {
			log_debug(loggerConsola,"Id_programa: %d (VACÍO), Base logica: %d, Base real: %p, Tamaño: %d",aux->id_programa, aux->base_logica,aux->base,aux->tamanio);
		}
	}
	log_debug(loggerConsola,"**********************************************\n");
	return 0;
}

/* Imprime el contenido de la memoria a partir de un offset y tamanio */
//TODO Preguntar como imprimir los datos
int imprimir_contenido(int offset, int tamanio){
	char* datos = malloc(tamanio);
	memcpy(datos,bloqueDeMemoria+offset,tamanio);
	printf("El contenido a partir de la posicion %d y tamaño %d es: %s\n", offset, tamanio, generarHexa(datos,tamanio));//imprimir datos en hexa??
	log_debug(loggerConsola,"**********************************************");
	log_debug(loggerConsola,"El contenido a partir de la posicion %d y tamaño %d es: %s", offset, tamanio, generarHexa(datos,tamanio));
	log_debug(loggerConsola,"**********************************************\n");
	return 0;
}

/*Genera un string con el valor en hexa de cada byte */
char* generarHexa(char* buffer,int tamanio){
	int i;
	char* logueo = malloc((3 * tamanio) + 1);
	char* hexa = malloc(4);

	for(i=0;i<tamanio;i++){
		hexa = string_from_format("%x ", (unsigned char)buffer[i]);
		if(i==0){
			strcpy(logueo,hexa);
		} else {
			strcat(logueo,hexa);
		}
	}
	return logueo;
}


/* Para facilitar las pruebas ACORDATE DE BORRARLO DESPUES!!!!! */
/* Prueba los algoritmos de creacion de segmentos */
/* Funcionan OK ;) */
int scriptCreacionSegmentos(){

	t_crearSegmentoUMV* datos = malloc(sizeof(t_crearSegmentoUMV));

	datos->programid = 0;
	datos->size = 4;
	int dir1 = crear_segmento(datos);
	printf("Segmento programa 0 y tamaño 4, dir logica: %d\n",dir1);
	datos->programid = 2;
	datos->size = 6;
	int dir3 = crear_segmento(datos);
	printf("Segmento programa 2 y tamaño 6, dir logica: %d\n",dir3);
	datos->programid = 1;
	datos->size = 8;
	int dir2 = crear_segmento(datos);
	printf("Segmento programa 1 y tamaño 8, dir logica: %d\n",dir2);


	printf("Imprimiendo lista de segmentos\n");
	imprimir_segmentos_memoria();

	sleep(3);

	printf("Destruyendo segmentos del programa 2\n");
	destruir_segmentos(2);


	printf("Imprimiendo lista de segmentos\n");
	imprimir_segmentos_memoria();

	sleep(3);

	datos->programid = 1;
	datos->size = 5;
	int dir4 = crear_segmento(datos);
	printf("Segmento programa 1 y tamaño 8, dir logica: %d\n",dir4);

	printf("Imprimiendo lista de segmentos\n");
	imprimir_segmentos_memoria();

	return 0;
}

/* Prueba la compactacion, lectura y escritura */
/* funcionan OK ;) */
/*
int scriptCreacionSegmentos(){
	int entero;
	int resultado;
	char* buffer;
	t_crearSegmentoUMV* datos = malloc(sizeof(t_crearSegmentoUMV));
	t_solicitudEscritura* sol = malloc(sizeof(t_solicitudEscritura));
	t_solicitudLectura* soli = malloc(sizeof(t_solicitudLectura));
	sol->buffer = malloc(sizeof(int));

	datos->programid = 0;
	datos->size = 4;
	int dir1 = crear_segmento(datos);
	printf("Segmento programa 0 y tamaño 4, dir logica: %d\n",dir1);
	datos->programid = 2;
	datos->size = 6;
	int dir3 = crear_segmento(datos);
	printf("Segmento programa 2 y tamaño 6, dir logica: %d\n",dir3);
	datos->programid = 1;
	datos->size = 8;
	int dir2 = crear_segmento(datos);
	printf("Segmento programa 1 y tamaño 8, dir logica: %d\n",dir2);


	printf("Imprimiendo lista de segmentos\n");
	imprimir_segmentos_memoria();

	sleep(3);

	printf("Escrituras\n");
	entero = 15;
	sol->base = dir1;
	memcpy(sol->buffer,&entero,sizeof(int));
	sol->offset = 0;
	sol->tamanio = sizeof(int);
	printf("Escribiendo en seg %d el entero %s (hexa)\n",dir1,generarHexa(sol->buffer,sizeof(int)));
	resultado = escribir(0,sol);
	printf("Resultado de escritura: %d\n",resultado);

	entero = 78;
	sol->base = dir2;
	memcpy(sol->buffer,&entero,sizeof(int));
	printf("Escribiendo en seg %d el entero %s (hexa)\n",dir2,generarHexa(sol->buffer,sizeof(int)));
	resultado = escribir(1,sol);
	printf("Resultado de escritura: %d\n",resultado);

	sleep(3);

	printf("Lecturas\n");
	soli->base = dir1;
	soli->offset = 0;
	soli->tamanio = sizeof(int);
	printf("Lectura completa del seg %d\n",dir1);
	buffer = leer(0,soli);
	if(buffer != NULL){
		printf("Los datos son: %s\n",generarHexa(buffer,sizeof(int)));
	} else {
		printf("NULL?????\n");
	}

	soli->base = dir2;
	soli->offset = 0;
	soli->tamanio = sizeof(int);
	printf("Lectura completa del seg %d\n",dir2);
	buffer = leer(1,soli);
	if(buffer != NULL){
		printf("Los datos son: %s\n",generarHexa(buffer,sizeof(int)));
	} else {
		printf("NULL?????\n");
	}

	sleep(3);

	printf("Destruyendo segmentos del programa 2\n");
	destruir_segmentos(2);


	printf("Imprimiendo lista de segmentos\n");
	imprimir_segmentos_memoria();

	sleep(3);

	printf("Compactando\n");
	compactar();
	printf("Compactacion terminada\n");

	printf("Imprimiendo lista de segmentos\n");
	imprimir_segmentos_memoria();


	sleep(3);

	printf("Lecturas otra vez\n");
	soli->base = dir1;
	soli->offset = 0;
	soli->tamanio = sizeof(int);
	printf("Lectura completa del seg %d\n",dir1);
	buffer = leer(0,soli);
	if(buffer != NULL){
		printf("Los datos son: %s\n",generarHexa(buffer,sizeof(int)));
	} else {
		printf("NULL?????\n");
	}

	soli->base = dir2;
	soli->offset = 0;
	soli->tamanio = sizeof(int);
	printf("Lectura completa del seg %d\n",dir2);
	buffer = leer(1,soli);
	if(buffer != NULL){
		printf("Los datos son: %s\n",generarHexa(buffer,sizeof(int)));
	} else {
		printf("NULL?????\n");
	}


	return 0;
}*/

