/*
 * cpu.h
 *
 *  Created on: 19/06/2014
 *      Author: utnso
 */

#ifndef CPU_H_
#define CPU_H_

#include <stdio.h>
#include <stdlib.h>
#include <sockets.h>
#include <commons/config.h>
#include <commons/log.h>
#include <obtener_config.h>
#include <serializadores.h>
#include <paquetes.h>
#include <parser/sintax.h>
#include <parser/parser.h>
#include <parser/metadata_program.h>
#include <colas.h>
#include <string.h>
#include <signal.h>
#include <stdarg.h>
#define TAMANIO_INSTRUCCION 8
#define TAMANIO_ID_VAR 1
#define TAMANIO_VAR 4

void *cargar_diccionarioVariables(int32_t);
void notificar_kernel(t_paquete);
//Defino variables globales a la CPU
t_dictionary *diccionarioVariables;
t_dictionary *diccionarioEtiquetas;
t_PCB *pcb;
int socketKernel, socketUMV;
int desconectarse = false;
AnSISOP_funciones *primitivas;
int32_t quantumKernel;
t_log* logger;


t_puntero GameOfThread_definirVariable(t_nombre_variable identificador_variable);
t_puntero GameOfThread_obtenerPosicionVariable(t_nombre_variable identificador_variable);
t_valor_variable GameOfThread_dereferenciar(t_puntero direccion_variable);
void GameOfThread_asignar(t_puntero direccion_variable, t_valor_variable valor);
t_valor_variable GameOfThread_obtenerValorCompartida(t_nombre_compartida variable);
t_valor_variable GameOfThread_asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor);
t_puntero_instruccion GameOfThread_irAlLabel(t_nombre_etiqueta t_nombre_etiqueta);
void GameOfThread_llamarSinRetorno(t_nombre_etiqueta etiqueta);
void GameOfThread_llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar);
void GameOfThread_finalizar(void);
void GameOfThread_retornar(t_valor_variable retorno);
void GameOfThread_imprimir(t_valor_variable valor_mostrar);
void GameOfThread_imprimirTexto(char* texto);
void GameOfThread_entradaSalida(t_nombre_dispositivo dispositivo, int tiempo);
void GameOfThread_wait(t_nombre_semaforo identificador_semaforo);
void GameOfThread_signal(t_nombre_semaforo identificador_semaforo);


typedef struct{
	int32_t inicio;
	int32_t longitud;
}t_datoSentencia;

void rutina(int n);
void noticar_kernel(t_paquete);
void handshake_kernel(void);
package *Leer(t_pun base,t_pun offset,t_pun tamanio);
package *Escribir(t_pun base, t_pun offset, t_pun tamanio, char* buffer);


#endif /* CPU_H_ */
