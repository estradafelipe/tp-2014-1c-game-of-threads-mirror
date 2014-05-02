
#include "sockets.h"
#include <string.h>
#define SOY_NIVEL 1
#define SOY_PERSONAJE 0

// Crear un socket:
// AF_INET: Socket de internet IPv4
// SOCK_STREAM: Orientado a la conexion, TCP
// 0: Usar protocolo por defecto para AF_INET-SOCK_STREAM: Protocolo TCP/IPv4
int abrir_socket(){
	int descriptor;
	int optval = 1;
	if ((descriptor = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}
// Hacer que el SO libere el puerto inmediatamente luego de cerrar el socket.
	setsockopt(descriptor, SOL_SOCKET, SO_REUSEADDR, &optval,sizeof(optval));

	return(descriptor);
}

// Vincular el socket con una direccion de red almacenada en 'socketInfo'.
int vincular_socket(int descriptor, int port){
	int vinculo = 0;
	struct sockaddr_in socketInfo;

	socketInfo.sin_family = AF_INET;
	socketInfo.sin_port = htons(port);
	socketInfo.sin_addr.s_addr = INADDR_ANY;
	memset(&(socketInfo.sin_zero), '\0', 8);

	if (bind(descriptor, (struct sockaddr*) &socketInfo, sizeof(struct sockaddr)) == -1){
		perror("bind");
		exit(1);
	}

	return(vinculo);
}


// Escuchar nuevas conexiones entrantes.
int escuchar_socket(int descriptor){
	int escucha = 0;
	if (listen(descriptor, 10) == -1) {
		perror("listen");
		exit(1);
	}
	printf("Escuchando Nuesvas Conexiones Entrantes.\n");
	return(escucha);
}

// Aceptar una nueva conexion entrante. Se genera un nuevo socket con la nueva conexion.
int aceptar_conexion(int descriptorEscucha){
	int descriptor;
	struct sockaddr_in socketInfo;
	unsigned int sin_size = sizeof(struct sockaddr_in);
	if((descriptor = accept(descriptorEscucha, (struct sockaddr*)&socketInfo, &sin_size)) == -1){
		perror("accept");
	}

	return(descriptor);
}

int conectar_socket(int descriptor, char* ip, int port){
	int conexion = 0;
	struct sockaddr_in suAddr;

	suAddr.sin_family = AF_INET;
	suAddr.sin_addr.s_addr = inet_addr(ip);
	suAddr.sin_port = htons(port);

	if (connect(descriptor, (struct sockaddr *)&suAddr,sizeof(struct sockaddr)) == -1) {
			perror("connect");
			exit(1);
	}
	printf("Conectado!\n");
	return(conexion);

}

struct sockaddr_in obtener_datos_socket (int descriptor){
	struct sockaddr_in suAddr;
	socklen_t addrlen = sizeof(suAddr);

	if ((getpeername( descriptor, (struct sockaddr *)&suAddr, &addrlen))==-1){
		printf("error del getpeername\n");
		}

	return suAddr;
}

int identificarme(int descriptor,void* estructura ,int whoisthat){
	package* paquete;

	switch(whoisthat){
		case SOY_NIVEL: {
				t_presentacionNivel* nivel;
				nivel = malloc(sizeof(t_presentacionNivel));
				nivel = (t_presentacionNivel*) estructura;
				char* nivelSerializado = serializarPresNivel(nivel);
				printf("Recibi paquete\n");
				paquete= crear_paquete(presentacionNivel,nivelSerializado,sizeof(nivelSerializado));
				enviar_paquete(paquete,descriptor);
				}
		case SOY_PERSONAJE: {
				t_presentacionPersonaje* pj;
				pj = malloc(sizeof(t_presentacionPersonaje));
				pj = (t_presentacionPersonaje*)estructura;
				char* personajeSerializado = serializarPresentacionPersonaje(pj);
				paquete = crear_paquete(presentacionPersonaje,personajeSerializado,sizeof(personajeSerializado));
				enviar_paquete(paquete,descriptor);
				}
	}
return 0;
}

