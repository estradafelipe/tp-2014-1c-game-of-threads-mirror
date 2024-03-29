/*
 * paquetes.h
 *
 *  Created on: 01/11/2013
 *      Author: utnso
 */

#ifndef PAQUETES_H_
#define PAQUETES_H_

#include <stdint.h>

// Cabecera paquetes

typedef enum nipc_type{
	handshakeKernelCPU,
	retornoCPUQuantum,
	recibiACKDeCPU,
	solicitarValorVariableCompartida,
	asignarValorVariableCompartida,
	liberarSemaforo,						// signal de la CPU al kernel
	tomarSemaforo,							// wait de la CPU al Kernel
	imprimirTexto,
	imprimirValor,
	retornoCPUExcepcion,
	retornoCPUFin,
	retornoCPUPorES,
	retornoCPUBloqueado,
	estoyDisponible,
	respuestaCPU,
	bloquearProgramaCPU,					// bloquear programa de kernel a CPU
	semaforolibre,							// via libre kernel a CPU como respuesta a un wait
	enviarPCBACPU,
	envioPCBES,
	handshakeCpuUmv,						//01
	creacionSegmentos,						//02 - solicitar creacion de segmentos a la UMV
	destruccionSegmentos,					//03 - solicitar destruccion de segmentos a la UMV
	lectura,								//04 - solicitud de lectura a la UMV
	escritura,								//05 - solicitud de escritura a la UMV
	cambioProcesoActivo,					//06 - cambio de proceso activo
	respuestaUmv,							//07 - respuesta de la UMV a una solicitud
	handshakeProgKernel,					//08
	programaNuevo,							//09
	rechazoPrograma,						//10 - respuesta del Kernel de rechazo al programa
	handshakeKernelUmv,						//11
	finPrograma,							//
	violacionSegmento,
	entrada_salida,
	error_label,							//No se encuentra la instruccion asociada a la etiqueta
	cpuDesconectada,
}t_paquete;


// Tipo paquete
typedef struct paquete{
		t_paquete type;
        uint16_t payloadLength;
        char *payload;
}__attribute__((packed)) package;

#define SIZE_HEADER ( sizeof(t_paquete) + sizeof(uint16_t) )

package	*crear_paquete(t_paquete type, char *payload, uint16_t payloadLength);
int 	enviar_paquete(package *package, int socket);

package *recibir_paquete(uint32_t socket);
int32_t recibir(uint32_t descriptor, package* paquete);

void	destruir_paquete(package *package);


#endif /* PAQUETES_H_ */
