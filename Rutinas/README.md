Instalación:

- Ubicados en la carpeta Rutinas:
  - primero ejecutar make
  - luego ejecutar sudo make install

- Desinstalación:
  - Ubicados en la carpeta Rutinas ejecutar sudo make uninstall

- Referencia en eclipse:
  - Ir a propiedades del proyecto/ C/C++ Build /Settings 
  - pestaña Tool Settings/GCC C Linker/Libraries
  - agregar Rutinas en Libraries(-l) 

- Verificar si esta instalada:
  - Ejecutar: ls -al /usr/lib | grep Rutinas*
    - Si no devuelve nada es porque no esta instalada
  - Para las commons: ls -al /usr/lib | grep commons*
