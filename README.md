# Laboratorio Interactivo de Mapeo de Texturas 3D

Este es un proyecto educativo e interactivo desarrollado como material de aprendizaje visual para la asignatura de **Computación Gráfica (Computación Visual)**. Permite explorar y comparar en tiempo real las diferencias críticas de renderizado al mapear imágenes sobre superficies 3D.

---

## 🛠️ Tecnologías Usadas
* **HTML5**: Estructura semántica del laboratorio y controles informativos.
* **JavaScript (ES6+)**: Lógica interactiva de control de texturas y sincronización de estado.
* **Three.js**: Biblioteca WebGL para la renderización de la escena 3D, cámaras, iluminación y materiales.
* **Vite**: Bundler rápido para desarrollo local y compilación optimizada.
* **Sass (SCSS)**: Diseño visual personalizado y responsivo con tema oscuro (glassmorphism).

---

## 🚀 Cómo Instalar y Ejecutar

Sigue estos pasos en tu terminal para ejecutar la aplicación:

### 1. Instalar Dependencias
Navega a la carpeta del proyecto y ejecuta el instalador de paquetes:
```bash
npm install
```

### 2. Iniciar Servidor de Desarrollo
Inicia el servidor local de desarrollo con recarga en caliente (hot reload):
```bash
npm run dev
```
La terminal imprimirá un enlace local (ej. `http://localhost:5173`). Haz Ctrl+Clic para abrirlo en tu navegador.

---

## 📖 Conceptos de Computación Gráfica Demostrados

### 1. ¿Qué es el Mapeo de Texturas?
El **mapeo de texturas** es una técnica que consiste en envolver la superficie de un objeto tridimensional con una imagen bidimensional (textura) para añadirle detalle visual, color y realismo (como ladrillo, madera o piedra) sin incrementar la complejidad geométrica (número de polígonos) del modelo.

### 2. ¿Qué son las Coordenadas UV?
Las **coordenadas UV** son un par de valores numéricos $(u, v)$ asignados a cada vértice de un objeto 3D. Estos valores están normalizados en el rango $[0.0, 1.0]$. La coordenada $u$ representa la posición horizontal en la textura 2D, y la coordenada $v$ representa la posición vertical. Permiten que la GPU conozca con precisión qué píxel (téctel) de la textura corresponde a cada punto sobre la superficie del modelo 3D.

### 3. ¿Qué demuestra RepeatWrapping vs ClampToEdgeWrapping?
Define el comportamiento del direccionador de texturas de la GPU cuando las coordenadas UV superan el intervalo $[0.0, 1.0]$ (por ejemplo, cuando la escala/repetición de la textura es de 3x3):
* **RepeatWrapping**: Repite la textura en mosaico de forma continua, ideal para patrones repetitivos como paredes de ladrillo o pisos.
* **ClampToEdgeWrapping**: "Abraza" o clampa la textura al borde, estirando los píxeles de las orillas (los bordes de U y V) hasta el infinito, resultando en líneas estiradas.

### 4. ¿Qué demuestra NearestFilter vs LinearFilter?
Se aplican cuando ocurre **magnificación** (el objeto está muy cerca y un píxel de pantalla es más pequeño que un téctel de la textura):
* **NearestFilter**: Elige el téctel más cercano al centro del píxel. Mantiene la textura nítida pero produce un aspecto cuadriculado o pixelado (retro).
* **LinearFilter**: Realiza una interpolación bilineal tomando el promedio de los 4 técteles más cercanos. Suaviza la textura difuminando los bordes de los píxeles.

### 5. ¿Qué demuestra Mipmapping?
El **Mipmapping** genera versiones precalculadas de la textura a resoluciones menores (ej. 512x512, 256x256...). Cuando el objeto se aleja, WebGL utiliza la versión más pequeña y adecuada. Esto evita el **aliasing óptico** (un ruido molesto de alta frecuencia que causa un parpadeo o parpadeo del píxel al mover la cámara, conocido como *shimmering*). Si se desactiva, las texturas lejanas se ven inestables y ruidosas.

### 6. ¿Qué demuestra el Filtrado Anisotrópico (Anisotropy)?
Los mipmaps estándar asumen que la textura se escala uniformemente en todas las direcciones. Al ver una superficie plana (como el suelo) desde un ángulo rasante u oblicuo, el área proyectada de la textura es alargada y trapezoidal. El **Filtrado Anisotrópico** toma muestras direccionales adicionales según el ángulo de inclinación, manteniendo la nitidez de la textura en la lejanía inclinada sin emborronarla.

---

## 📷 Capturas Recomendadas para el Informe Técnico

Para tu reporte o informe de laboratorio, se recomienda capturar las siguientes configuraciones utilizando el botón **📷 Capturar Vista (PNG)** integrado en la app:

1. **Distorsión UV Esférica**:
   * Geometría: **Esfera 3D**
   * Textura: **UV Grid**
   * *Muestra cómo el mapeo esférico concentra y deforma los cuadrados de la textura en los polos.*
2. **Comparación de Wrapping**:
   * Geometría: **Cubo 3D** ➔ Textura: **Ladrillo** ➔ Repetición: **3x3**
   * Captura 1: Modo **RepeatWrapping** (mosaico continuo).
   * Captura 2: Modo **ClampToEdgeWrapping** (ladrillos estirados en las caras laterales).
3. **Magnificación de Filtros**:
   * Geometría: **Cubo 3D** ➔ Textura: **UV Grid** ➔ Zoom de cerca
   * Captura 1: Filtro **NearestFilter** (píxeles gigantes cuadrados perfectos).
   * Captura 2: Filtro **LinearFilter** (suavizado borroso).
4. **Aliasing Lejano (Mipmaps Off)**:
   * Geometría: **Plano 2D** ➔ Textura: **UV Grid** ➔ Mipmaps: **Desactivado** ➔ Aléjate con el scroll.
   * *Muestra el ruido visual de moiré y parpadeo al mover la cámara.*
5. **Nitidez de Suelo (Anisotropía)**:
   * Geometría: **Plano 2D** ➔ Textura: **Piedra** ➔ Repetición: **8x8** ➔ Vista rasante inclinada
   * Captura 1: **Anisotropía = 1x** (fondo borroso).
   * Captura 2: **Anisotropía = Máxima** (fondo nítido y detallado).

---

## 🎓 Créditos
Este proyecto se desarrolló utilizando como base un template de Three.js/Vite diseñado para agilizar la codificación en WebGL. La aplicación ha sido rediseñada, adaptada y ampliada como una herramienta didáctica e interactiva por estudiantes de Computación Gráfica (Computación Visual) para ejemplificar de manera práctica la tubería (pipeline) de texturizado.
