#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#endif

#include <GL/freeglut.h>
#include <GL/gl.h>
#include <GL/glu.h>

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// Constantes de Anisotropia (definidas manualmente para evitar problemas de cabecera)
#ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#endif
#ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif

// Definiciones de tipos para el estado
enum GeometryType { GEOM_CUBO, GEOM_ESFERA, GEOM_PLANO };
enum TextureType { TEX_GRID, TEX_BRICK, TEX_WOOD, TEX_STONE, TEX_CUSTOM, TEX_NONE };
enum WrapMode { WRAP_REPEAT, WRAP_CLAMP };
enum MinFilter { MIN_NEAREST, MIN_LINEAR, MIN_NEAREST_MIPMAP_NEAREST, MIN_NEAREST_MIPMAP_LINEAR, MIN_LINEAR_MIPMAP_NEAREST, MIN_LINEAR_MIPMAP_LINEAR };
enum MagFilter { MAG_NEAREST, MAG_LINEAR };

struct AppState {
    GeometryType geometry = GEOM_CUBO;
    TextureType texture = TEX_GRID;
    bool textureActive = true;
    WrapMode wrapMode = WRAP_REPEAT;
    float repeatX = 1.0f;
    float repeatY = 1.0f;
    MinFilter minFilter = MIN_LINEAR_MIPMAP_LINEAR;
    MagFilter magFilter = MAG_LINEAR;
    bool generateMipmaps = true;
    float anisotropy = 1.0f;
    float baseColor[3] = {0.23f, 0.51f, 0.96f}; // #3b82f6 (Azul Slate/Neon)
};

// Variables globales
AppState state;
int windowWidth = 1280;
int windowHeight = 720;
GLuint textureIds[5] = {0}; // IDs para GRID, BRICK, WOOD, STONE, CUSTOM
float maxAnisotropy = 1.0f;

// Variables de camara (Coordenadas esfericas)
float cameraAngleX = 0.8f;   // Rotacion yaw
float cameraAngleY = 0.5f;   // Rotacion pitch
float cameraDistance = 5.0f; // Distancia al centro
int lastMouseX = 0;
int lastMouseY = 0;
int activeMouseButton = -1;

// Rutas de archivos de textura
const char* texturePaths[4] = {
    "textures/uv_grid.png",
    "textures/brick.png",
    "textures/wood.png",
    "textures/stone.png"
};

// Carga de textura individual
GLuint loadTexture(const char* filepath) {
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    int width, height, nrChannels;
    // Forzamos 4 canales (RGBA) para evitar problemas de alineacion
    unsigned char *data = stbi_load(filepath, &width, &height, &nrChannels, 4);
    if (data) {
        // Subir imagen base y construir mipmaps automaticamente
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
        
        stbi_image_free(data);
        std::cout << "Textura cargada con exito: " << filepath << " (" << width << "x" << height << ")" << std::endl;
    } else {
        std::cerr << "Error al cargar textura: " << filepath << std::endl;
    }
    return texID;
}

// Cargar imagen personalizada usando el selector de archivos de Windows (OpenFileName)
void selectAndLoadCustomTexture() {
#ifdef _WIN32
    OPENFILENAME ofn;
    char szFile[260] = { 0 };

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "Imagenes (PNG, JPG, BMP)\0*.png;*.jpg;*.jpeg;*.bmp\0Todos los archivos\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileName(&ofn) == TRUE) {
        std::string filepath = szFile;
        int width, height, nrChannels;
        unsigned char* data = stbi_load(filepath.c_str(), &width, &height, &nrChannels, 4);
        if (data) {
            // Eliminar textura personalizada anterior si existe para liberar memoria
            if (textureIds[4] != 0) {
                glDeleteTextures(1, &textureIds[4]);
            }
            
            glGenTextures(1, &textureIds[4]);
            glBindTexture(GL_TEXTURE_2D, textureIds[4]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
            
            stbi_image_free(data);
            std::cout << "Textura personalizada cargada con exito: " << filepath << std::endl;
            
            state.textureActive = true;
            state.texture = TEX_CUSTOM;
        } else {
            std::cerr << "Error al cargar textura personalizada: " << filepath << std::endl;
        }
    }
#else
    std::cout << "La seleccion de archivos locales solo esta soportada en Windows." << std::endl;
#endif
}

// Inicializar texturas y anisotropia
void initTextures() {
    // Configurar stb_image para invertir verticalmente las imagenes en la carga.
    // Esto corrige que las texturas se muestren al reves/de cabeza en OpenGL.
    stbi_set_flip_vertically_on_load(true);

    for (int i = 0; i < 4; i++) {
        textureIds[i] = loadTexture(texturePaths[i]);
    }
    textureIds[4] = 0;

    // Obtener la anisotropia maxima del hardware
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);
    std::cout << "Anisotropia maxima soportada por la GPU: " << maxAnisotropy << "x" << std::endl;
}

// Aplicar los parametros de envoltura, filtrado y anisotropia en tiempo real
void applyTextureState() {
    if (!state.textureActive || state.texture == TEX_NONE) {
        glDisable(GL_TEXTURE_2D);
        return;
    }

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textureIds[state.texture]);

    // 1. Envoltura (Wrapping)
    GLint wrapVal = (state.wrapMode == WRAP_REPEAT) ? GL_REPEAT : GL_CLAMP_TO_EDGE;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapVal);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapVal);

    // 2. Filtro de Minificacion (Min Filter)
    GLint minFilterVal;
    if (state.generateMipmaps) {
        switch (state.minFilter) {
            case MIN_NEAREST: minFilterVal = GL_NEAREST; break;
            case MIN_LINEAR: minFilterVal = GL_LINEAR; break;
            case MIN_NEAREST_MIPMAP_NEAREST: minFilterVal = GL_NEAREST_MIPMAP_NEAREST; break;
            case MIN_NEAREST_MIPMAP_LINEAR: minFilterVal = GL_NEAREST_MIPMAP_LINEAR; break;
            case MIN_LINEAR_MIPMAP_NEAREST: minFilterVal = GL_LINEAR_MIPMAP_NEAREST; break;
            case MIN_LINEAR_MIPMAP_LINEAR: minFilterVal = GL_LINEAR_MIPMAP_LINEAR; break;
        }
    } else {
        // Si mipmaps esta apagado, forzar fallback a Nearest o Linear no-mipmapeado
        if (state.minFilter == MIN_NEAREST || 
            state.minFilter == MIN_NEAREST_MIPMAP_NEAREST || 
            state.minFilter == MIN_NEAREST_MIPMAP_LINEAR) {
            minFilterVal = GL_NEAREST;
        } else {
            minFilterVal = GL_LINEAR;
        }
    }

    // Filtro de Magnificacion (Mag Filter)
    GLint magFilterVal = (state.magFilter == MAG_NEAREST) ? GL_NEAREST : GL_LINEAR;

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilterVal);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilterVal);

    // 3. Anisotropia
    if (maxAnisotropy > 1.0f) {
        float safeAniso = std::min(state.anisotropy, maxAnisotropy);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, safeAniso);
    }
}

// Dibujar Cubo con normales e UV
void drawTexturedCube(float size, float repeatX, float repeatY) {
    float h = size / 2.0f;
    glBegin(GL_QUADS);

    // Frente
    glNormal3f(0.0f, 0.0f, 1.0f);
    glTexCoord2f(0.0f, 0.0f);       glVertex3f(-h, -h,  h);
    glTexCoord2f(repeatX, 0.0f);    glVertex3f( h, -h,  h);
    glTexCoord2f(repeatX, repeatY); glVertex3f( h,  h,  h);
    glTexCoord2f(0.0f, repeatY);    glVertex3f(-h,  h,  h);

    // Atras
    glNormal3f(0.0f, 0.0f, -1.0f);
    glTexCoord2f(0.0f, 0.0f);       glVertex3f( h, -h, -h);
    glTexCoord2f(repeatX, 0.0f);    glVertex3f(-h, -h, -h);
    glTexCoord2f(repeatX, repeatY); glVertex3f(-h,  h, -h);
    glTexCoord2f(0.0f, repeatY);    glVertex3f( h,  h, -h);

    // Arriba
    glNormal3f(0.0f, 1.0f, 0.0f);
    glTexCoord2f(0.0f, 0.0f);       glVertex3f(-h,  h,  h);
    glTexCoord2f(repeatX, 0.0f);    glVertex3f( h,  h,  h);
    glTexCoord2f(repeatX, repeatY); glVertex3f( h,  h, -h);
    glTexCoord2f(0.0f, repeatY);    glVertex3f(-h,  h, -h);

    // Abajo
    glNormal3f(0.0f, -1.0f, 0.0f);
    glTexCoord2f(0.0f, 0.0f);       glVertex3f(-h, -h, -h);
    glTexCoord2f(repeatX, 0.0f);    glVertex3f( h, -h, -h);
    glTexCoord2f(repeatX, repeatY); glVertex3f( h, -h,  h);
    glTexCoord2f(0.0f, repeatY);    glVertex3f(-h, -h,  h);

    // Derecha
    glNormal3f(1.0f, 0.0f, 0.0f);
    glTexCoord2f(0.0f, 0.0f);       glVertex3f( h, -h, -h);
    glTexCoord2f(repeatX, 0.0f);    glVertex3f( h, -h,  h);
    glTexCoord2f(repeatX, repeatY); glVertex3f( h,  h,  h);
    glTexCoord2f(0.0f, repeatY);    glVertex3f( h,  h, -h);

    // Izquierda
    glNormal3f(-1.0f, 0.0f, 0.0f);
    glTexCoord2f(0.0f, 0.0f);       glVertex3f(-h, -h,  h);
    glTexCoord2f(repeatX, 0.0f);    glVertex3f(-h, -h, -h);
    glTexCoord2f(repeatX, repeatY); glVertex3f(-h,  h, -h);
    glTexCoord2f(0.0f, repeatY);    glVertex3f(-h,  h,  h);

    glEnd();
}

// Dibujar Plano Horizontal con normales e UV
void drawTexturedPlane(float size, float repeatX, float repeatY) {
    float h = size / 2.0f;
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 1.0f, 0.0f);
    glTexCoord2f(0.0f, 0.0f);       glVertex3f(-h, 0.0f,  h);
    glTexCoord2f(repeatX, 0.0f);    glVertex3f( h, 0.0f,  h);
    glTexCoord2f(repeatX, repeatY); glVertex3f( h, 0.0f, -h);
    glTexCoord2f(0.0f, repeatY);    glVertex3f(-h, 0.0f, -h);
    glEnd();
}

// Dibujar Esfera Matematica con normales e UV correctos
void drawTexturedSphere(float radius, int slices, int stacks, float repeatX, float repeatY) {
    for (int i = 0; i < slices; i++) {
        float theta1 = i * 2.0f * M_PI / slices;
        float theta2 = (i + 1) * 2.0f * M_PI / slices;

        glBegin(GL_QUAD_STRIP);
        for (int j = 0; j <= stacks; j++) {
            float phi = j * M_PI / stacks;

            // Vertice 1
            float x1 = radius * sin(phi) * cos(theta1);
            float y1 = radius * cos(phi);
            float z1 = radius * sin(phi) * sin(theta1);
            float u1 = (float)i / slices * repeatX;
            float v1 = (float)j / stacks * repeatY;

            glNormal3f(x1 / radius, y1 / radius, z1 / radius);
            glTexCoord2f(u1, v1);
            glVertex3f(x1, y1, z1);

            // Vertice 2
            float x2 = radius * sin(phi) * cos(theta2);
            float y2 = radius * cos(phi);
            float z2 = radius * sin(phi) * sin(theta2);
            float u2 = (float)(i + 1) / slices * repeatX;
            float v2 = (float)j / stacks * repeatY;

            glNormal3f(x2 / radius, y2 / radius, z2 / radius);
            glTexCoord2f(u2, v2);
            glVertex3f(x2, y2, z2);
        }
        glEnd();
    }
}

// Dibujar GridHelper similar al de Three.js
void drawGridHelper(float size, int divisions) {
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glLineWidth(1.0f);

    float step = size / divisions;
    float halfSize = size / 2.0f;

    glBegin(GL_LINES);
    for (int i = 0; i <= divisions; i++) {
        float pos = -halfSize + i * step;

        // Color mas intenso para los ejes centrales
        if (std::abs(pos) < 0.001f) {
            glColor3f(0.31f, 0.27f, 0.90f); // Azul Neon (#4f46e5)
        } else {
            glColor3f(0.15f, 0.15f, 0.16f); // Gris oscuro (#27272a)
        }

        // Lineas paralelas al eje Z
        glVertex3f(pos, -1.5f, -halfSize);
        glVertex3f(pos, -1.5f, halfSize);

        // Lineas paralelas al eje X
        glVertex3f(-halfSize, -1.5f, pos);
        glVertex3f(halfSize, -1.5f, pos);
    }
    glEnd();
    glEnable(GL_LIGHTING);
}

// Dibujar un gizmo de orientacion de ejes (viewport pequeno en la esquina superior derecha)
void drawViewportGizmo() {
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    int gizmoSize = 100;
    // Situado en la esquina superior derecha
    glViewport(viewport[2] - gizmoSize - 20, viewport[3] - gizmoSize - 20, gizmoSize, gizmoSize);

    glClear(GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluPerspective(50.0, 1.0, 0.1, 10.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // Orientar la camara del gizmo igual que la camara principal
    float gcx = 3.0f * cos(cameraAngleY) * sin(cameraAngleX);
    float gcy = 3.0f * sin(cameraAngleY);
    float gcz = 3.0f * cos(cameraAngleY) * cos(cameraAngleX);
    float upY = (cos(cameraAngleY) >= 0.0f) ? 1.0f : -1.0f;
    gluLookAt(gcx, gcy, gcz, 0.0f, 0.0f, 0.0f, 0.0f, upY, 0.0f);

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    glLineWidth(3.0f);
    glBegin(GL_LINES);
    // Eje X (Rojo)
    glColor3f(1.0f, 0.2f, 0.2f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(1.0f, 0.0f, 0.0f);

    // Eje Y (Verde)
    glColor3f(0.2f, 1.0f, 0.2f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 1.0f, 0.0f);

    // Eje Z (Azul)
    glColor3f(0.2f, 0.2f, 1.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 1.0f);
    glEnd();

    // Dibujar las etiquetas X, Y, Z
    glColor3f(1.0f, 0.2f, 0.2f);
    glRasterPos3f(1.2f, 0.0f, 0.0f);
    glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, 'X');

    glColor3f(0.2f, 1.0f, 0.2f);
    glRasterPos3f(0.0f, 1.2f, 0.0f);
    glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, 'Y');

    glColor3f(0.2f, 0.2f, 1.0f);
    glRasterPos3f(0.0f, 0.0f, 1.2f);
    glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, 'Z');

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    // Restaurar el viewport principal
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    glEnable(GL_LIGHTING);
}

// Auxiliar para dibujar texto
void drawText(float x, float y, const std::string& text, void* font = GLUT_BITMAP_8_BY_13) {
    glRasterPos2f(x, y);
    for (char c : text) {
        glutBitmapCharacter(font, c);
    }
}

// Iniciar proyeccion 2D para HUD
void beginHUD() {
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, windowWidth, 0, windowHeight);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

// Finalizar proyeccion 2D y restaurar estado 3D
void endHUD() {
    glDisable(GL_BLEND);
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

// Dibujar Panel HUD lateral interactivo
void drawHUD() {
    beginHUD();

    // Fondo oscuro translucido (Glassmorphism effect)
    glColor4f(0.07f, 0.07f, 0.08f, 0.90f);
    glBegin(GL_QUADS);
    glVertex2f(15.0f, 15.0f);
    glVertex2f(455.0f, 15.0f);
    glVertex2f(455.0f, windowHeight - 15.0f);
    glVertex2f(15.0f, windowHeight - 15.0f);
    glEnd();

    // Borde elegante color azul
    glColor4f(0.23f, 0.51f, 0.96f, 0.60f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(15.0f, 15.0f);
    glVertex2f(455.0f, 15.0f);
    glVertex2f(455.0f, windowHeight - 15.0f);
    glVertex2f(15.0f, windowHeight - 15.0f);
    glEnd();

    float y = windowHeight - 45.0f;

    // Titulo (Quitando el subtitulo)
    glColor3f(1.0f, 1.0f, 1.0f);
    drawText(30.0f, y, "LABORATORIO DE TEXTURAS 3D", GLUT_BITMAP_HELVETICA_18);
    y -= 30.0f;

    // Linea Separadora
    glColor4f(1.0f, 1.0f, 1.0f, 0.1f);
    glBegin(GL_LINES);
    glVertex2f(30.0f, y);
    glVertex2f(440.0f, y);
    glEnd();
    y -= 20.0f;

    // Seccion: Estado
    glColor3f(0.23f, 0.51f, 0.96f);
    drawText(30.0f, y, "ESTADO DEL RENDERIZADO:", GLUT_BITMAP_HELVETICA_12);
    y -= 22.0f;

    glColor3f(1.0f, 1.0f, 1.0f);
    std::string geomStr = "Geometria: ";
    if (state.geometry == GEOM_CUBO) geomStr += "Cubo 3D";
    else if (state.geometry == GEOM_ESFERA) geomStr += "Esfera 3D";
    else geomStr += "Plano 2D (XZ)";
    drawText(30.0f, y, geomStr);
    y -= 18.0f;

    std::string texStr = "Textura activa: ";
    if (!state.textureActive) texStr += "NINGUNA (Color Plano)";
    else {
        if (state.texture == TEX_GRID) texStr += "UV Grid (Mapeo)";
        else if (state.texture == TEX_BRICK) texStr += "Ladrillo (Patron)";
        else if (state.texture == TEX_WOOD) texStr += "Madera (Organica)";
        else if (state.texture == TEX_STONE) texStr += "Piedra (Rugosa)";
        else if (state.texture == TEX_CUSTOM) texStr += "Personalizada (Subida)";
        else texStr += "Desconocida";
    }
    drawText(30.0f, y, texStr);
    y -= 18.0f;

    std::string wrapStr = "Modo Envoltura (Wrap): ";
    wrapStr += (state.wrapMode == WRAP_REPEAT) ? "GL_REPEAT" : "GL_CLAMP_TO_EDGE";
    drawText(30.0f, y, wrapStr);
    y -= 18.0f;

    char repBuf[64];
    sprintf(repBuf, "Repeticion UV (Escala): %.1fx * %.1fy", state.repeatX, state.repeatY);
    drawText(30.0f, y, repBuf);
    y -= 18.0f;

    std::string mipStr = "Generar Mipmaps: ";
    mipStr += state.generateMipmaps ? "SI (Habilitado)" : "NO (Deshabilitado)";
    drawText(30.0f, y, mipStr);
    y -= 18.0f;

    std::string minStr = "Filtro Min (Lejos): ";
    switch (state.minFilter) {
        case MIN_NEAREST: minStr += "GL_NEAREST"; break;
        case MIN_LINEAR: minStr += "GL_LINEAR"; break;
        case MIN_NEAREST_MIPMAP_NEAREST: minStr += "GL_NEAREST_MIPMAP_NEAREST"; break;
        case MIN_NEAREST_MIPMAP_LINEAR: minStr += "GL_NEAREST_MIPMAP_LINEAR"; break;
        case MIN_LINEAR_MIPMAP_NEAREST: minStr += "GL_LINEAR_MIPMAP_NEAREST"; break;
        case MIN_LINEAR_MIPMAP_LINEAR: minStr += "GL_LINEAR_MIPMAP_LINEAR"; break;
    }
    if (!state.generateMipmaps) {
        minStr += " (Fallback: ";
        if (state.minFilter == MIN_NEAREST || state.minFilter == MIN_NEAREST_MIPMAP_NEAREST || state.minFilter == MIN_NEAREST_MIPMAP_LINEAR)
            minStr += "NEAREST)";
        else
            minStr += "LINEAR)";
    }
    drawText(30.0f, y, minStr);
    y -= 18.0f;

    std::string magStr = "Filtro Mag (Cerca): ";
    magStr += (state.magFilter == MAG_NEAREST) ? "GL_NEAREST" : "GL_LINEAR";
    drawText(30.0f, y, magStr);
    y -= 18.0f;

    char anisBuf[64];
    sprintf(anisBuf, "Filtrado Anisotropico: %.1fx (Max %.1fx)", state.anisotropy, maxAnisotropy);
    drawText(30.0f, y, anisBuf);
    y -= 25.0f;

    // Linea Separadora
    glColor4f(1.0f, 1.0f, 1.0f, 0.1f);
    glBegin(GL_LINES);
    glVertex2f(30.0f, y);
    glVertex2f(440.0f, y);
    glEnd();
    y -= 20.0f;

    // Seccion: Controles
    glColor3f(0.23f, 0.51f, 0.96f);
    drawText(30.0f, y, "CONTROLES POR TECLADO:", GLUT_BITMAP_HELVETICA_12);
    y -= 22.0f;

    glColor3f(0.9f, 0.9f, 0.9f);
    drawText(30.0f, y, "[G] Cambiar de Geometria"); y -= 16.0f;
    drawText(30.0f, y, "[T] Ciclar Texturas Fijas / Apagar"); y -= 16.0f;
    drawText(30.0f, y, "[U] Cargar Imagen Personalizada (PNG/JPG)"); y -= 16.0f;
    drawText(30.0f, y, "[W] Alternar Wrap (REPEAT / CLAMP)"); y -= 16.0f;
    drawText(30.0f, y, "[+] / [-] Aumentar/Reducir Repeticion"); y -= 16.0f;
    drawText(30.0f, y, "[M] Activar/Desactivar Mipmaps"); y -= 16.0f;
    drawText(30.0f, y, "[F] Cambiar Filtro Minificacion (Min)"); y -= 16.0f;
    drawText(30.0f, y, "[N] Cambiar Filtro Magnificacion (Mag)"); y -= 16.0f;
    drawText(30.0f, y, "[A] / [D] Subir/Bajar Anisotropia"); y -= 16.0f;
    drawText(30.0f, y, "[S] Guardar Captura de Pantalla (BMP)"); y -= 16.0f;
    drawText(30.0f, y, "[ESC] Salir"); y -= 25.0f;

    // Linea Separadora
    glColor4f(1.0f, 1.0f, 1.0f, 0.1f);
    glBegin(GL_LINES);
    glVertex2f(30.0f, y);
    glVertex2f(440.0f, y);
    glEnd();
    y -= 20.0f;

    // Raton
    glColor3f(0.5f, 0.6f, 0.7f);
    drawText(30.0f, y, "Control de Camara (Raton):", GLUT_BITMAP_HELVETICA_12);
    y -= 18.0f;
    glColor3f(0.85f, 0.85f, 0.85f);
    drawText(30.0f, y, "- Clic Izq + Arrastrar: Rotar Camara"); y -= 16.0f;
    drawText(30.0f, y, "- Clic Der + Arrastrar / Rueda: Zoom");

    endHUD();
}

// Capturar el framebuffer y guardarlo como un archivo BMP
void captureScreenshot(const std::string& filename) {
    int width = glutGet(GLUT_WINDOW_WIDTH);
    int height = glutGet(GLUT_WINDOW_HEIGHT);
    std::vector<unsigned char> pixels(width * height * 3);
    
    // Leer los pixeles RGB de la pantalla
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    FILE* f = fopen(filename.c_str(), "wb");
    if (!f) {
        std::cerr << "Error: No se pudo escribir la captura en " << filename << std::endl;
        return;
    }

    // Cabeceras BMP (54 bytes)
    unsigned char fileHeader[14] = {
        'B', 'M',           // Firma
        0, 0, 0, 0,         // Tamano de archivo (rellenado abajo)
        0, 0, 0, 0,         // Reservado
        54, 0, 0, 0         // Offset a los pixeles
    };
    unsigned char infoHeader[40] = {
        40, 0, 0, 0,        // Tamano de esta cabecera
        0, 0, 0, 0,         // Ancho (rellenado abajo)
        0, 0, 0, 0,         // Alto (rellenado abajo)
        1, 0,               // Planos
        24, 0,              // Bits por pixel (24 bits = RGB)
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 // Resto en 0
    };

    int imageSize = width * height * 3;
    int fileSize = 54 + imageSize;

    fileHeader[2] = (unsigned char)(fileSize);
    fileHeader[3] = (unsigned char)(fileSize >> 8);
    fileHeader[4] = (unsigned char)(fileSize >> 16);
    fileHeader[5] = (unsigned char)(fileSize >> 24);

    infoHeader[4] = (unsigned char)(width);
    infoHeader[5] = (unsigned char)(width >> 8);
    infoHeader[6] = (unsigned char)(width >> 16);
    infoHeader[7] = (unsigned char)(width >> 24);

    infoHeader[8] = (unsigned char)(height);
    infoHeader[9] = (unsigned char)(height >> 8);
    infoHeader[10] = (unsigned char)(height >> 16);
    infoHeader[11] = (unsigned char)(height >> 24);

    fwrite(fileHeader, 1, 14, f);
    fwrite(infoHeader, 1, 40, f);

    // Guardar en formato BGR que espera el archivo BMP
    for (int i = 0; i < width * height; i++) {
        unsigned char r = pixels[i * 3 + 0];
        unsigned char g = pixels[i * 3 + 1];
        unsigned char b = pixels[i * 3 + 2];
        fputc(b, f);
        fputc(g, f);
        fputc(r, f);
    }

    fclose(f);
    std::cout << "Captura de pantalla guardada con exito en " << filename << std::endl;
}

// Funcion principal de renderizado
void display() {
    glClearColor(0.07f, 0.07f, 0.08f, 1.0f); // Slate oscuro matching scene.js
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Configurar matriz de vista (Camara)
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Calcular la posicion de la camara a partir de las coordenadas esfericas
    float cx = cameraDistance * cos(cameraAngleY) * sin(cameraAngleX);
    float cy = cameraDistance * sin(cameraAngleY);
    float cz = cameraDistance * cos(cameraAngleY) * cos(cameraAngleX);
    
    // Controlar el vector UP para evitar giros bruscos en los polos
    float upY = (cos(cameraAngleY) >= 0.0f) ? 1.0f : -1.0f;
    gluLookAt(cx, cy, cz, 0.0f, 0.0f, 0.0f, 0.0f, upY, 0.0f);

    // Dibujar el Grid Helper
    drawGridHelper(10.0f, 10);

    // Configurar Luces
    glEnable(GL_LIGHTING);
    
    // Luz 0: Luz ambiental y direccional (Sol)
    float lightAmbient[] = {0.4f, 0.4f, 0.4f, 1.0f};
    float lightDiffuse[] = {0.8f, 0.8f, 0.8f, 1.0f};
    float lightPosition[] = {5.0f, 8.0f, 5.0f, 0.0f}; // Direccional
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
    glEnable(GL_LIGHT0);

    // Luz 1: Luz puntual (relleno)
    float light1Diffuse[] = {0.3f, 0.3f, 0.3f, 1.0f};
    float light1Position[] = {-4.0f, 2.0f, -4.0f, 1.0f}; // Posicional
    glLightfv(GL_LIGHT1, GL_DIFFUSE, light1Diffuse);
    glLightfv(GL_LIGHT1, GL_POSITION, light1Position);
    glEnable(GL_LIGHT1);

    // Configurar texturas
    applyTextureState();

    // Configurar material basico
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    
    // Si la textura esta desactivada, usamos el color base azul slate
    if (!state.textureActive || state.texture == TEX_NONE) {
        glColor3fv(state.baseColor);
    } else {
        glColor3f(1.0f, 1.0f, 1.0f); // Color blanco para que la textura brille natural
    }

    // Dibujar geometria seleccionada
    glPushMatrix();
    if (state.geometry == GEOM_CUBO) {
        drawTexturedCube(2.0f, state.repeatX, state.repeatY);
    } else if (state.geometry == GEOM_ESFERA) {
        drawTexturedSphere(1.3f, 64, 64, state.repeatX, state.repeatY);
    } else if (state.geometry == GEOM_PLANO) {
        drawTexturedPlane(2.5f, state.repeatX, state.repeatY);
    }
    glPopMatrix();

    // Dibujar la interfaz de usuario en pantalla
    // Dibujar la interfaz de usuario en pantalla
    drawHUD();

    // Dibujar el gizmo de ejes en la esquina superior derecha
    drawViewportGizmo();

    glutSwapBuffers();
}

// Redimensionar ventana
void reshape(int w, int h) {
    windowWidth = w;
    windowHeight = h;
    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(50.0, (double)w / (double)h, 0.1, 1000.0);
}

// Manejo de eventos del teclado (Atajos de control)
void keyboard(unsigned char key, int x, int y) {
    switch (key) {
        case 27: // ESC
            exit(0);
            break;
        case 'g':
        case 'G':
            state.geometry = static_cast<GeometryType>((state.geometry + 1) % 3);
            break;
        case 't':
        case 'T':
            if (!state.textureActive) {
                state.textureActive = true;
                state.texture = TEX_GRID;
            } else {
                state.texture = static_cast<TextureType>((state.texture + 1) % 6); // GRID, BRICK, WOOD, STONE, CUSTOM, NONE
                if (state.texture == TEX_CUSTOM && textureIds[4] == 0) {
                    selectAndLoadCustomTexture();
                    if (textureIds[4] == 0) {
                        state.texture = TEX_NONE;
                        state.textureActive = false;
                    }
                } else if (state.texture == TEX_NONE) {
                    state.textureActive = false;
                }
            }
            break;
        case 'u':
        case 'U':
            selectAndLoadCustomTexture();
            break;
        case 'w':
        case 'W':
            state.wrapMode = (state.wrapMode == WRAP_REPEAT) ? WRAP_CLAMP : WRAP_REPEAT;
            break;
        case 'f':
        case 'F':
            state.minFilter = static_cast<MinFilter>((state.minFilter + 1) % 6);
            break;
        case 'n':
        case 'N':
            state.magFilter = (state.magFilter == MAG_NEAREST) ? MAG_LINEAR : MAG_NEAREST;
            break;
        case 'm':
        case 'M':
            state.generateMipmaps = !state.generateMipmaps;
            break;
        case 'a':
        case 'A':
            state.anisotropy = std::min(state.anisotropy * 2.0f, maxAnisotropy);
            if (state.anisotropy < 1.0f) state.anisotropy = 1.0f;
            break;
        case 'd':
        case 'D':
            state.anisotropy = std::max(state.anisotropy / 2.0f, 1.0f);
            break;
        case '+':
        case '=':
            state.repeatX += 0.5f;
            state.repeatY += 0.5f;
            if (state.repeatX > 10.0f) {
                state.repeatX = 10.0f;
                state.repeatY = 10.0f;
            }
            break;
        case '-':
        case '_':
            state.repeatX -= 0.5f;
            state.repeatY -= 0.5f;
            if (state.repeatX < 0.5f) {
                state.repeatX = 0.5f;
                state.repeatY = 0.5f;
            }
            break;
        case 's':
        case 'S':
            captureScreenshot("captura_textura.bmp");
            break;
    }
    glutPostRedisplay();
}

// Clics del Raton
void mouse(int button, int state_mouse, int x, int y) {
    activeMouseButton = button;
    if (state_mouse == GLUT_DOWN) {
        lastMouseX = x;
        lastMouseY = y;
    }
}

// Movimiento del Raton (Arrastrar)
void motion(int x, int y) {
    if (activeMouseButton == GLUT_LEFT_BUTTON) {
        // Rotar camara
        float dx = (x - lastMouseX) * 0.005f;
        float dy = (y - lastMouseY) * 0.005f;
        cameraAngleX += dx;
        cameraAngleY += dy;

        // Limitar angulo vertical para no invertir la camara
        if (cameraAngleY > 1.4f) cameraAngleY = 1.4f;
        if (cameraAngleY < -1.4f) cameraAngleY = -1.4f;

        lastMouseX = x;
        lastMouseY = y;
    } else if (activeMouseButton == GLUT_RIGHT_BUTTON) {
        // Zoom con clic derecho
        float dy = (y - lastMouseY) * 0.05f;
        cameraDistance += dy;
        if (cameraDistance < 2.0f) cameraDistance = 2.0f;
        if (cameraDistance > 15.0f) cameraDistance = 15.0f;

        lastMouseX = x;
        lastMouseY = y;
    }
    glutPostRedisplay();
}

// Rueda del raton para Zoom (FreeGLUT)
void mouseWheel(int wheel, int direction, int x, int y) {
    if (direction > 0) {
        cameraDistance -= 0.5f;
    } else {
        cameraDistance += 0.5f;
    }
    if (cameraDistance < 2.0f) cameraDistance = 2.0f;
    if (cameraDistance > 15.0f) cameraDistance = 15.0f;
    glutPostRedisplay();
}

int main(int argc, char** argv) {
    // Inicializar GLUT
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(windowWidth, windowHeight);
    glutCreateWindow("Laboratorio Interactivo de Mapeo de Texturas 3D");

    // Habilitar profundidad y mezclas
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Configurar iluminacion base
    glEnable(GL_LIGHTING);
    glShadeModel(GL_SMOOTH);

    // Cargar todas las texturas de imagen
    initTextures();

    // Registrar Callbacks
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutMouseWheelFunc(mouseWheel); // Registro de rueda de raton para FreeGLUT

    // Iniciar bucle
    glutMainLoop();
    return 0;
}
