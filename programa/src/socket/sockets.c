#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include "sockets.h"
#include <commons/string.h>
#include <string.h>


int socket_error(int valor)
{
	if (valor==-1)
		return 1;
	else
		return 0;
}

int socket_crear()
{
	int resu = socket(AF_INET, SOCK_STREAM, 0);

	if (resu!=-1)
		return resu;
	else
		return -1;
}

int socket_sockop(int descriptor)
{
	int yes =1;
	return setsockopt(descriptor, SOL_SOCKET, SO_REUSEADDR, &yes,sizeof(int));
}

int socket_vincular(int descriptor,struct sockaddr_in local)
{
	return (bind(descriptor, (struct sockaddr *)&local, sizeof(local)));
}

int socket_escuchar(int descriptor)
{
	return listen(descriptor, 10);
}

int socket_select(int max,fd_set fd_listen)
{
	return select(max+1,&fd_listen, NULL, NULL, NULL);
}

//Devuelve una estructura con la información que le dice al socket que ip y puerto va a conectarse.
struct sockaddr_in socket_config(char *ip,int puerto)
{
	struct sockaddr_in config;

	config.sin_family      = AF_INET;
	config.sin_addr.s_addr = inet_addr(ip);
	config.sin_port        = htons(puerto);

	return config;
}

//Conectar el socket a una ip y puerto, usamos la estructura sockaddr_in que tiene estos datos.
int socket_conectar(int descriptor,struct sockaddr_in direccion)
{
	return 	connect(descriptor, (struct sockaddr*) &direccion, sizeof(direccion));
}

//Envia el texto por el socket.
int socket_enviar(int descriptor,char *texto)
{
	return send(descriptor,texto,strlen(texto),0);
}

t_stream *serializar(t_person *self)
{
	char *data = malloc(sizeof(int)+strlen(self->name)+strlen(self->lastname)+1);
	t_stream *stream = malloc(sizeof(t_stream));
	int offset   = 0;
	int tmp_size = 0;

	memcpy(data,&self->dni,tmp_size=sizeof(int));

	offset=tmp_size;
	memcpy(data+offset,self->name,tmp_size=sizeof(self->name)+1);

	offset+=tmp_size;
	memcpy(data+offset,self->lastname,tmp_size=sizeof(self->lastname)+1);

	stream->length=offset+tmp_size;
	stream->data=data;

	return stream;
}

t_person *deserializar(t_stream *stream)
{
	t_person *self = malloc(sizeof(t_person));
	int offset = 0,tmp_size = 0;

	//dni
	memcpy(&self->dni,stream->data,tmp_size=sizeof(int));

	offset=tmp_size;
	for (tmp_size=1;(stream->data+offset)[tmp_size-1]!='\0';tmp_size++);
	self->name=malloc(tmp_size);
	memcpy(self->name,stream->data+offset,tmp_size);

	offset +=tmp_size;
	for (tmp_size=1;(stream->data+offset)[tmp_size-1]!='\0';tmp_size++);
	self->lastname=malloc(tmp_size);
	memcpy(self->lastname,stream->data+offset,tmp_size);

	return self;
}

//Recibe datos del socket.
char *socket_recibir(int descriptor)
{
	 int  numbytes;
	 char buf[256];

	 memset(&buf,'\0',256); //limpiamos buffer

	 int size = 256;

	 if ((numbytes=recv(descriptor,buf,size,0)) == 0)
	 {
		  printf("Conexion cerrada por el socket \n");
		  exit(-1);
		  return NULL;
	 }
	 else
	 {
		 return string_from_format("%s",buf);
	 }
}

//Cerrar socket.
void socket_cerrar(int descriptor)
{
	close(descriptor);
}

//Inicia la conexion.
t_socket iniciar_conexion(t_socket conexion,t_ip ip)
{
	//Creo el socket.
	conexion.descriptor = socket_crear();

	//Reviso si se creo bien, sino fin de programa.
	if (socket_error(conexion.descriptor))
	{
		perror("Error al crear el socket, fin de programa");
	}

	//Obtengo info para conectar el socket al server.
	conexion.direccion = socket_config(ip.ip,ip.puerto);

	//Conectar el socket a la direccion que fijamos.
	if (socket_error(socket_conectar(conexion.descriptor,conexion.direccion)))
	{
		perror("Error al conectar socket");
	}
	else
	{
		return conexion;
	}

	return conexion;
}

//Armo la tupla IP / Puerto
t_ip   nueva_ip(char *ip,int puerto)
{
	t_ip dir;
	dir.ip = ip;
	dir.puerto=puerto;
	return dir;
}
