/*
 * varcom.h
 *
 *  Created on: 02/05/2014
 *      Author: utnso
 */

#ifndef VARCOM_H_
#define VARCOM_H_

typedef struct
{
	char *valor;
	pthread_mutex_t* mutex;;
}t_variable_compartida;


#endif /* VARCOM_H_ */
