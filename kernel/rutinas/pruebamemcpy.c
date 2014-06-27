#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void main()
{
	char * origen="copiar esto";
	char * destino=malloc(strlen(origen)+1);
	int tamanio_origen=strlen(origen);
	int offset=0, tmp_size=0;
	tmp_size = tamanio_origen;
	memcpy(destino, origen, tmp_size);
	memcpy((destino)+tamanio_origen,"\0",1);
	printf("tamanio: %d origen: %s\ndestino: %s\n", tamanio_origen, origen, destino);
}
