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
	handshakeKernelCPU,						//00
	retornoCPUQuantum,						//01
	recibiACKDeCPU,							//02
	solicitarValorVariableCompartida,		//03
	asignarValorVariableCompartida,			//04
	liberarSemaforo,						//05
	tomarSemaforo,							//06
	imprimirTexto,							//07
	imprimirValor,							//08
	retornoCPUExcepcion,					//09
	retornoCPUFin,							//10
	retornoCPUPorES,						//11
	retornoCPUBloqueado,					//12
	estoyDisponible,						//13
	respuestaCPU,							//14
	bloquearProgramaCPU,					//15 bloquear programa de kernel a CPU
	semaforolibre,							//16 via libre kernel a CPU como respuesta a un wait
	enviarPCBACPU,							//17
	envioPCBES,								//18
	handshakeCpuUmv,						//19
	creacionSegmentos,						//20 - solicitar creacion de segmentos a la UMV
	destruccionSegmentos,					//21 - solicitar destruccion de segmentos a la UMV
	lectura,								//22 - solicitud de lectura a la UMV
	escritura,								//23 - solicitud de escritura a la UMV
	cambioProcesoActivo,					//24 - cambio de proceso activo
	respuestaUmv,							//25 - respuesta de la UMV a una solicitud
	handshakeProgKernel,					//26
	programaNuevo,							//27
	rechazoPrograma,						//28 - respuesta del Kernel de rechazo al programa
	handshakeKernelUmv,						//29
	finPrograma,							//30
	violacionSegmento,						//31
	entrada_salida,							//32
	error_label,							//33 No se encuentra la instruccion asociada a la etiqueta
	cpuDesconectada,						//34
	mandaPCB,								//35
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
