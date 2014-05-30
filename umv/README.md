- Requerimientos:
=================
  - Tener instaladas las commons
  - Tener instalada ultima version de Rutinas

- Para compilar por consola:
============================
  - -lpthread -lcommons -lRutinas
  
- Para eclipse:
===============
  - agregar librerias: pthread, commons y Rutinas

- Comandos por consola reconocidos:
===================================
(entre comillas son los parametros)
  - Operaciones:
    - operacion crear_seg "id_programa" "tamaño"
    - operacion destruir_seg "id_programa"
    - operacion lectura "id_programa" "base" "offset" "size"
    - operacion escritura "id_programa" "base" "offset" "size" "buffer"
  - Modificar retardo:
    - retardo "time_miliseg"
  - Modificar algoritmo:
    - algoritmo WORSTFIT
    - algoritmo FIRSTFIT
  - Compactar:
    - compactacion
  - Dump:
    - dump estructuras "id_programa" (si id_programa = -1 -> todos los programas)
    - dump memoria
    - dump contenido "offset" "tamaño"
