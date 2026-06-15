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


## Controles e Interacción

### Control de Cámara con el Ratón

- Arrastrar con Clic Izquierdo: Rotar cámara (Pitch y Yaw).
- Arrastrar con Clic Derecho / Rueda del Ratón: Zoom orbital (acercar/alejar con paso adaptativo y límites extendidos de `0.35` a `60.0`).

### Atajos de Teclado (Controles Globales)

- **[0] a [6]**: Cambiar el **Modo Didáctico Activo**:
  - `[0]`: **Modo Libre**: Permite control total y personalización de todos los parámetros.
  - `[1]`: **Interpolación UV**: Demuestra la interpolación lineal de coordenadas UV sobre triángulos. Muestra la línea de costura roja en la esfera/cilindro.
  - `[2]`: **Filtrado de Textura**: Renderiza una comparación lado a lado con el filtrado `GL_NEAREST` (izq.) vs `GL_LINEAR` (der.).
  - `[3]`: **Mipmapping**: Muestra dos planos texturizados en la distancia para comparar el aliasing/centelleo `Sin Mipmaps` (izq.) vs `Con Mipmaps` (der.).
  - `[4]`: **Modos de Repetición (Wrap)**: Muestra una comparativa simultánea de tres figuras usando `GL_REPEAT` (izq.), `GL_CLAMP_TO_EDGE` (centro) y `GL_MIRRORED_REPEAT` (der.).
  - `[5]`: **Proyecciones Analíticas**: Demuestra el mapeo planar, esférico, cilíndrico y cónico sobre geometrías analíticas.
  - `[6]`: **Desenvuelto UV / OBJ**: Renderiza un modelo `.obj` importado que demuestra coordenadas de mapeo UV complejas (`vt`) exportadas desde software de modelado.
- **[Z]**: Alternar modo visual entre **Sólido Texturizado** y **Wireframe puro** (dibuja la malla triangulada en color cian sin iluminación ni texturas).
- **[H]**: Mostrar / Ocultar el panel de la interfaz **HUD**. Al ocultarlo, la escena 3D se expande para ocupar toda la pantalla.
- **[V]**: Invertir la orientación de la coordenada vertical **V** únicamente para el modelo OBJ cargado.
- **[S]**: Guardar una **captura de pantalla** en formato BMP en la raíz del proyecto.
- **[ESC]**: Salir de la aplicación.

### Controles de Parámetros (Modo Libre)

- **[G]**: Ciclar geometría activa: *Cubo -> Esfera -> Plano -> Cilindro Analítico -> Cono Analítico -> Tetera (GLUT con Gen Auto) -> Tetera (OBJ con UV Unwrap)*.
- **[T]**: Ciclar textura activa: *Grid UV -> Ladrillos -> Madera -> Piedra -> Tablero Checker (alto contraste) -> Imagen Personalizada -> Sin textura (Color plano)*.
- **[U]**: Cargar una imagen de textura personalizada (.png, .jpg, .bmp) desde tu computadora.
- **[W]**: Ciclar modo de envoltura (wrapping): *GL_REPEAT -> GL_CLAMP_TO_EDGE -> GL_MIRRORED_REPEAT*.
- **[+] / [-]**: Aumentar o disminuir la escala de repetición de textura (rango `0.5x` a `10.0x`).
- **[M]**: Activar o desactivar el uso de Mipmaps en la minificación.
- **[F]**: Ciclar filtro de minificación (Lejos): *GL_NEAREST, GL_LINEAR, y las 4 variantes de Mipmapping*.
- **[N]**: Alternar filtro de magnificación (Cerca): *GL_NEAREST vs GL_LINEAR*.
- **[A] / [D]**: Duplicar o reducir a la mitad el nivel de anisotropía en el filtrado de textura.

