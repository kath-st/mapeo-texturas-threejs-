# Laboratorio Interactivo de Mapeo de Texturas 3D

Este es un proyecto educativo e interactivo desarrollado en C++ y OpenGL utilizando FreeGLUT para la asignatura de Computacion Grafica (Computacion Visual). Permite explorar y comparar en tiempo real las diferencias de renderizado al mapear imagenes sobre superficies 3D.

## Caracteristicas del Laboratorio

- Geometrias Soportadas: Cubo 3D, Esfera calculada matematicamente con coordenadas de textura y normales correctas, y un Plano 2D horizontal en los ejes X y Z.
- Control de Texturas: UV Grid de mapeo, Patron de Ladrillos, Madera organica, Piedra rugosa y opcion de desactivar texturas (color plano).
- Modos de Envoltura (Wrapping): Comparacion entre repeticion (GL_REPEAT) y abrazado de bordes (GL_CLAMP_TO_EDGE).
- Filtros de Textura: Alternancia entre Nearest, Linear, y las 4 variantes de filtrado con Mipmaps (GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR_MIPMAP_LINEAR).
- Filtrado Anisotropico: Ajuste de nivel en tiempo real desde 1.0x hasta el maximo soportado por el hardware de tu GPU.
- Carga de Imagenes Propias: Permite cargar cualquier imagen PNG o JPG desde tu computadora mediante el cuadro de dialogo de archivos nativo de Windows.
- Captura de Pantalla: Guarda el estado del renderizado en un archivo BMP en el directorio local.
- HUD Informativo: Panel lateral izquierdo translucido que muestra el estado de todos los parametros y la guia de controles.
- Gizmo de Ejes: Indicador en la esquina superior derecha que muestra la orientacion en 3D de los ejes X (Rojo), Y (Verde) y Z (Azul) sincronizado con el giro de camara.

## Requisitos e Instalación

Para compilar y ejecutar este proyecto en Windows, solo necesitas tener instalado el compilador **GCC/G++** (por ejemplo, a través de **MinGW**).

> [!NOTE]
> Las dependencias de **FreeGLUT** (archivos de cabecera `.h`, librerías estáticas `.a` y la librería dinámica `.dll`) ya vienen incluidas dentro del repositorio para facilitar su configuración:
> - `include/GL/`: Archivos de cabecera para FreeGLUT (`freeglut.h`, etc.).
> - `lib/`: Librerías estáticas de FreeGLUT (`libfreeglut.a`, `libfreeglut.dll.a`).
> - `libfreeglut.dll`: Librería dinámica requerida en tiempo de ejecución (ubicada en el directorio raíz).

### Compilación

Navega a la carpeta del proyecto y ejecuta el archivo por lotes en la terminal/CMD:
```bash
build.bat
```
Esto compilará el archivo `main.cpp` enlazando las librerías locales, y ejecutará la aplicación de forma automática.


## Controles e Interaccion

### Control de Camara con el Raton

- Arrastrar con Clic Izquierdo: Rotar camara (Pitch y Yaw).
- Arrastrar con Clic Derecho / Rueda del Raton: Zoom (acercar/alejar).

### Atajos de Teclado

- G: Cambiar geometria (Cubo, Esfera, Plano).
- T: Ciclar texturas fijas (Grid, Ladrillo, Madera, Piedra, Personalizada o Sin Textura).
- U: Cargar imagen propia (Abre el explorador de archivos para elegir un archivo local).
- W: Cambiar modo de envoltura (GL_REPEAT vs GL_CLAMP_TO_EDGE).
- + / -: Aumentar o disminuir la repeticion UV.
- M: Activar o desactivar mipmapping.
- F: Cambiar filtro de minificacion (Lejos).
- N: Cambiar filtro de magnificacion (Cerca).
- A / D: Duplicar o reducir a la mitad el nivel de anisotropia.
- S: Tomar captura de pantalla (BMP).
- ESC: Salir de la aplicacion.
