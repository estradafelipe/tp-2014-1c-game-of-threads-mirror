/*
 * primitivas.h
 *
 *  Created on: 27/06/2014
 *      Author: utnso
 */

#ifndef PRIMITIVAS_H_
#define PRIMITIVAS_H_

#include "cpu.h"

AnSISOP_funciones *primitivas;

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


#endif /* PRIMITIVAS_H_ */
