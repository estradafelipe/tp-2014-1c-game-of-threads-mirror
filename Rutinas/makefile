RM=rm -rf
CC=gcc

C_SRCS=$(shell find . -iname "*.c" | tr '\n' ' ')
H_SRCS=$(shell find . -iname "*.h" | tr '\n' ' ')
OBJS:= build/obtener_config.o build/sockets.o build/serializadores.o build/paquetes.o
HDRS:= obtener_config.h sockets.h serializadores.h paquetes.h
# Clean and compile .so
all: build/libRutinas.so

create-dirs:
	mkdir -p build

build/libRutinas.so: create-dirs $(OBJS) 
	$(CC) -shared -o "build/libRutinas.so" $(OBJS)

build/obtener_config.o: obtener_config.c obtener_config.h
	$(CC) -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"

build/sockets.o: sockets.c sockets.h
	$(CC) -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"

build/serializadores.o: serializadores.c serializadores.h
	$(CC) -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"

build/paquetes.o: paquetes.c paquetes.h
	$(CC) -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"

build/colas.o: colas.c colas.h
	$(CC) -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"

# Add debug parameters and compile
debug: CC += -DDEBUG -g
debug: all

# Clean build files
clean:
	$(RM) build

install:all
	cp -u build/libRutinas.so /usr/lib
	cp --parents -u $(HDRS) /usr/include

uninstall:
	rm -f /usr/lib/libRutinas.so
	rm -rf /usr/include/Rutinas

.PHONY: all clean install uninstall