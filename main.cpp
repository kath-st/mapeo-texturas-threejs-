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
#ifndef GL_MIRRORED_REPEAT
#define GL_MIRRORED_REPEAT 0x8370
#endif
#ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#endif
#ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif

#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <sstream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

enum GeometryType {
    GEOM_CUBO,
    GEOM_ESFERA,
    GEOM_PLANO,
    GEOM_CILINDRO,
    GEOM_CONO,
    GEOM_TETERA,
    GEOM_OBJ
};

enum TextureType {
    TEX_GRID,
    TEX_BRICK,
    TEX_WOOD,
    TEX_STONE,
    TEX_CHECKER,
    TEX_CUSTOM,
    TEX_NONE
};

enum WrapMode {
    WRAP_REPEAT,
    WRAP_CLAMP,
    WRAP_MIRRORED_REPEAT
};

enum DemoMode {
    DEMO_LIBRE,
    DEMO_INTERPOLACION_UV,
    DEMO_FILTRADO,
    DEMO_MIPMAPPING,
    DEMO_WRAP,
    DEMO_PROYECCIONES,
    DEMO_OBJ_UV
};

enum MinFilter {
    MIN_NEAREST,
    MIN_LINEAR,
    MIN_NEAREST_MIPMAP_NEAREST,
    MIN_NEAREST_MIPMAP_LINEAR,
    MIN_LINEAR_MIPMAP_NEAREST,
    MIN_LINEAR_MIPMAP_LINEAR
};

enum MagFilter {
    MAG_NEAREST,
    MAG_LINEAR
};

// Estructuras para carga de archivos OBJ
struct Vec2 { float u, v; };
struct Vec3 { float x, y, z; };

struct OBJVertex {
    int vIndex;
    int vtIndex;
    int vnIndex;
};

struct OBJTriangle {
    OBJVertex a, b, c;
};

struct OBJMesh {
    std::vector<Vec3> positions;
    std::vector<Vec2> texcoords;
    std::vector<Vec3> normals;
    std::vector<OBJTriangle> triangles;
    bool loaded = false;
    bool hasTexcoords = false;
    bool hasNormals = false;
    std::string filename;
};

struct AppState {
    DemoMode demoMode = DEMO_LIBRE;
    GeometryType geometry = GEOM_CUBO;
    TextureType texture = TEX_GRID;
    bool textureActive = true;
    WrapMode wrapMode = WRAP_REPEAT;
    float repeatX = 1.0f;
    float repeatY = 1.0f;
    MinFilter minFilter = MIN_LINEAR_MIPMAP_LINEAR;
    MagFilter magFilter = MAG_LINEAR;
    bool generateMipmaps = true; // "Uso de mipmaps" en el HUD
    float anisotropy = 1.0f;
    float baseColor[3] = {0.23f, 0.51f, 0.96f}; // #3b82f6
    bool wireframe = false;
    bool hudVisible = true;
    bool objFlipV = false;
    OBJMesh objMeshes[4];
    int activeObjIndex = 0;
};

// Variables globales
AppState state;
int windowWidth = 1280;
int windowHeight = 720;
GLuint textureIds[7] = {0}; // IDs para GRID, BRICK, WOOD, STONE, CHECKER, CUSTOM, NONE
float maxAnisotropy = 1.0f;

// Variables de camara (Coordenadas esfericas)
float cameraAngleX = 0.8f;   // Rotacion yaw
float cameraAngleY = 0.5f;   // Rotacion pitch
float cameraDistance = 5.0f; // Distancia al centro
int lastMouseX = 0;
int lastMouseY = 0;
int activeMouseButton = -1;

// Rutas de archivos de textura predeterminados
const char* texturePaths[4] = {
    "textures/uv_grid.png",
    "textures/brick.png",
    "textures/wood.png",
    "textures/stone.png"
};

// Rutas de archivos OBJ predeterminados
const char* objPaths[4] = {
    "models/teapot.obj",
    "models/cube.obj",
    "models/sphere.obj",
    "models/minion.obj"
};

// Prototipos de funciones
void drawText3D(float x, float y, float z, const std::string& text, void* font = GLUT_BITMAP_8_BY_13);
void applyTextureState(GLuint customTexId = 0);
void selectAndLoadCustomTexture();
bool loadOBJ(const std::string& path, OBJMesh& mesh);
void drawActiveGeometry();
void drawHUD();

// Carga de textura individual
GLuint loadTexture(const char* filepath) {
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    int width, height, nrChannels;
    unsigned char *data = stbi_load(filepath, &width, &height, &nrChannels, 4);
    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
        
        stbi_image_free(data);
        std::cout << "Textura cargada con exito: " << filepath << " (" << width << "x" << height << ")" << std::endl;
    } else {
        std::cerr << "Error al cargar textura: " << filepath << std::endl;
    }
    return texID;
}

// Genera un patron checkerboard de alto contraste
GLuint generateCheckerTexture() {
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    
    const int w = 64;
    const int h = 64;
    std::vector<unsigned char> data(w * h * 4);
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            bool white = ((x / 8) + (y / 8)) % 2 == 0;
            unsigned char val = white ? 255 : 0;
            int idx = (y * w + x) * 4;
            data[idx + 0] = val; // R
            data[idx + 1] = val; // G
            data[idx + 2] = val; // B
            data[idx + 3] = 255; // A
        }
    }
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, w, h, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
    
    std::cout << "Textura Checkerboard generada en memoria." << std::endl;
    return texID;
}

// Inicializar texturas y anisotropia
void initTextures() {
    stbi_set_flip_vertically_on_load(true);

    for (int i = 0; i < 4; i++) {
        textureIds[i] = loadTexture(texturePaths[i]);
    }
    textureIds[4] = generateCheckerTexture();
    textureIds[5] = 0; // Custom
    textureIds[6] = 0; // None

    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);
    std::cout << "Anisotropia maxima soportada por la GPU: " << maxAnisotropy << "x" << std::endl;
}

// Auxiliar para dibujar quads usando triangulos (para ver diagonales en wireframe)
void drawQuadAsTriangles(
    float x1, float y1, float z1, float u1, float v1, float nx1, float ny1, float nz1,
    float x2, float y2, float z2, float u2, float v2, float nx2, float ny2, float nz2,
    float x3, float y3, float z3, float u3, float v3, float nx3, float ny3, float nz3,
    float x4, float y4, float z4, float u4, float v4, float nx4, float ny4, float nz4
) {
    glBegin(GL_TRIANGLES);
    // Triangulo 1
    glNormal3f(nx1, ny1, nz1); glTexCoord2f(u1, v1); glVertex3f(x1, y1, z1);
    glNormal3f(nx2, ny2, nz2); glTexCoord2f(u2, v2); glVertex3f(x2, y2, z2);
    glNormal3f(nx3, ny3, nz3); glTexCoord2f(u3, v3); glVertex3f(x3, y3, z3);
    // Triangulo 2
    glNormal3f(nx1, ny1, nz1); glTexCoord2f(u1, v1); glVertex3f(x1, y1, z1);
    glNormal3f(nx3, ny3, nz3); glTexCoord2f(u3, v3); glVertex3f(x3, y3, z3);
    glNormal3f(nx4, ny4, nz4); glTexCoord2f(u4, v4); glVertex3f(x4, y4, z4);
    glEnd();
}

// Dibujar Cubo con triangulos
void drawTexturedCube(float size, float repeatX, float repeatY) {
    float h = size / 2.0f;
    
    // Frente
    drawQuadAsTriangles(
        -h, -h,  h, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
         h, -h,  h, repeatX, 0.0f, 0.0f, 0.0f, 1.0f,
         h,  h,  h, repeatX, repeatY, 0.0f, 0.0f, 1.0f,
        -h,  h,  h, 0.0f, repeatY, 0.0f, 0.0f, 1.0f
    );
    // Atras
    drawQuadAsTriangles(
         h, -h, -h, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f,
        -h, -h, -h, repeatX, 0.0f, 0.0f, 0.0f, -1.0f,
        -h,  h, -h, repeatX, repeatY, 0.0f, 0.0f, -1.0f,
         h,  h, -h, 0.0f, repeatY, 0.0f, 0.0f, -1.0f
    );
    // Arriba
    drawQuadAsTriangles(
        -h,  h,  h, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
         h,  h,  h, repeatX, 0.0f, 0.0f, 1.0f, 0.0f,
         h,  h, -h, repeatX, repeatY, 0.0f, 1.0f, 0.0f,
        -h,  h, -h, 0.0f, repeatY, 0.0f, 1.0f, 0.0f
    );
    // Abajo
    drawQuadAsTriangles(
        -h, -h, -h, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f,
         h, -h, -h, repeatX, 0.0f, 0.0f, -1.0f, 0.0f,
         h, -h,  h, repeatX, repeatY, 0.0f, -1.0f, 0.0f,
        -h, -h,  h, 0.0f, repeatY, 0.0f, -1.0f, 0.0f
    );
    // Derecha
    drawQuadAsTriangles(
         h, -h, -h, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
         h, -h,  h, repeatX, 0.0f, 1.0f, 0.0f, 0.0f,
         h,  h,  h, repeatX, repeatY, 1.0f, 0.0f, 0.0f,
         h,  h, -h, 0.0f, repeatY, 1.0f, 0.0f, 0.0f
    );
    // Izquierda
    drawQuadAsTriangles(
        -h, -h,  h, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
        -h, -h, -h, repeatX, 0.0f, -1.0f, 0.0f, 0.0f,
        -h,  h, -h, repeatX, repeatY, -1.0f, 0.0f, 0.0f,
        -h,  h,  h, 0.0f, repeatY, -1.0f, 0.0f, 0.0f
    );
}

// Dibujar Plano Horizontal con triangulos
void drawTexturedPlane(float size, float repeatX, float repeatY) {
    float h = size / 2.0f;
    drawQuadAsTriangles(
        -h, 0.0f,  h, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
         h, 0.0f,  h, repeatX, 0.0f, 0.0f, 1.0f, 0.0f,
         h, 0.0f, -h, repeatX, repeatY, 0.0f, 1.0f, 0.0f,
        -h, 0.0f, -h, 0.0f, repeatY, 0.0f, 1.0f, 0.0f
    );
}

// Dibujar Esfera Matematica triangulada con normales e UV correctos
void drawTexturedSphere(float radius, int slices, int stacks, float repeatX, float repeatY) {
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < slices; i++) {
        float theta1 = i * 2.0f * M_PI / slices;
        float theta2 = (i + 1) * 2.0f * M_PI / slices;
        
        float cosT1 = cos(theta1), sinT1 = sin(theta1);
        float cosT2 = cos(theta2), sinT2 = sin(theta2);

        for (int j = 0; j < stacks; j++) {
            float phi1 = j * M_PI / stacks;
            float phi2 = (j + 1) * M_PI / stacks;

            float sinP1 = sin(phi1), cosP1 = cos(phi1);
            float sinP2 = sin(phi2), cosP2 = cos(phi2);

            // Vertices del quad de la esfera
            // Superior izquierdo
            float x00 = radius * sinP1 * cosT1;
            float y00 = radius * cosP1;
            float z00 = radius * sinP1 * sinT1;

            // Superior derecho
            float x10 = radius * sinP1 * cosT2;
            float y10 = radius * cosP1;
            float z10 = radius * sinP1 * sinT2;

            // Inferior izquierdo
            float x01 = radius * sinP2 * cosT1;
            float y01 = radius * cosP2;
            float z01 = radius * sinP2 * sinT1;

            // Inferior derecho
            float x11 = radius * sinP2 * cosT2;
            float y11 = radius * cosP2;
            float z11 = radius * sinP2 * sinT2;

            // UVs - Corregido orientacion V (1.0f - ...)
            float u00 = (float)i / slices * repeatX;
            float v00 = (1.0f - (float)j / stacks) * repeatY;

            float u10 = (float)(i + 1) / slices * repeatX;
            float v10 = (1.0f - (float)j / stacks) * repeatY;

            float u01 = (float)i / slices * repeatX;
            float v01 = (1.0f - (float)(j + 1) / stacks) * repeatY;

            float u11 = (float)(i + 1) / slices * repeatX;
            float v11 = (1.0f - (float)(j + 1) / stacks) * repeatY;

            // Triangulo 1
            glNormal3f(x00 / radius, y00 / radius, z00 / radius);
            glTexCoord2f(u00, v00);
            glVertex3f(x00, y00, z00);

            glNormal3f(x01 / radius, y01 / radius, z01 / radius);
            glTexCoord2f(u01, v01);
            glVertex3f(x01, y01, z01);

            glNormal3f(x10 / radius, y10 / radius, z10 / radius);
            glTexCoord2f(u10, v10);
            glVertex3f(x10, y10, z10);

            // Triangulo 2
            glNormal3f(x10 / radius, y10 / radius, z10 / radius);
            glTexCoord2f(u10, v10);
            glVertex3f(x10, y10, z10);

            glNormal3f(x01 / radius, y01 / radius, z01 / radius);
            glTexCoord2f(u01, v01);
            glVertex3f(x01, y01, z01);

            glNormal3f(x11 / radius, y11 / radius, z11 / radius);
            glTexCoord2f(u11, v11);
            glVertex3f(x11, y11, z11);
        }
    }
    glEnd();
}

// Dibujar Cilindro con mapeo lateral analitico y tapas planares
void drawTexturedCylinder(float radius, float height, int slices, int stacks, float repeatX, float repeatY) {
    float halfH = height / 2.0f;
    
    // 1. Cara lateral
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < slices; i++) {
        float theta1 = i * 2.0f * M_PI / slices;
        float theta2 = (i + 1) * 2.0f * M_PI / slices;
        
        float cos1 = cos(theta1), sin1 = sin(theta1);
        float cos2 = cos(theta2), sin2 = sin(theta2);

        for (int j = 0; j < stacks; j++) {
            float y1 = -halfH + ((float)j / stacks) * height;
            float y2 = -halfH + ((float)(j + 1) / stacks) * height;

            float x1 = radius * cos1;
            float z1 = radius * sin1;
            float x2 = radius * cos2;
            float z2 = radius * sin2;

            float u1 = (float)i / slices * repeatX;
            float u2 = (float)(i + 1) / slices * repeatX;
            float v1 = (float)j / stacks * repeatY;
            float v2 = (float)(j + 1) / stacks * repeatY;

            // Triangulo 1
            glNormal3f(cos1, 0.0f, sin1); glTexCoord2f(u1, v1); glVertex3f(x1, y1, z1);
            glNormal3f(cos1, 0.0f, sin1); glTexCoord2f(u1, v2); glVertex3f(x1, y2, z1);
            glNormal3f(cos2, 0.0f, sin2); glTexCoord2f(u2, v1); glVertex3f(x2, y1, z2);

            // Triangulo 2
            glNormal3f(cos2, 0.0f, sin2); glTexCoord2f(u2, v1); glVertex3f(x2, y1, z2);
            glNormal3f(cos1, 0.0f, sin1); glTexCoord2f(u1, v2); glVertex3f(x1, y2, z1);
            glNormal3f(cos2, 0.0f, sin2); glTexCoord2f(u2, v2); glVertex3f(x2, y2, z2);
        }
    }
    glEnd();

    // 2. Tapa Superior (Planar)
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < slices; i++) {
        float theta1 = i * 2.0f * M_PI / slices;
        float theta2 = (i + 1) * 2.0f * M_PI / slices;

        float x1 = radius * cos(theta1);
        float z1 = radius * sin(theta1);
        float x2 = radius * cos(theta2);
        float z2 = radius * sin(theta2);

        float u1 = (0.5f + x1 / (2.0f * radius)) * repeatX;
        float v1 = (0.5f + z1 / (2.0f * radius)) * repeatY;
        float u2 = (0.5f + x2 / (2.0f * radius)) * repeatX;
        float v2 = (0.5f + z2 / (2.0f * radius)) * repeatY;
        float uc = 0.5f * repeatX;
        float vc = 0.5f * repeatY;

        glNormal3f(0.0f, 1.0f, 0.0f);
        glTexCoord2f(uc, vc); glVertex3f(0.0f, halfH, 0.0f);
        glTexCoord2f(u1, v1); glVertex3f(x1, halfH, z1);
        glTexCoord2f(u2, v2); glVertex3f(x2, halfH, z2);
    }
    glEnd();

    // 3. Tapa Inferior (Planar)
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < slices; i++) {
        float theta1 = i * 2.0f * M_PI / slices;
        float theta2 = (i + 1) * 2.0f * M_PI / slices;

        float x1 = radius * cos(theta1);
        float z1 = radius * sin(theta1);
        float x2 = radius * cos(theta2);
        float z2 = radius * sin(theta2);

        float u1 = (0.5f + x1 / (2.0f * radius)) * repeatX;
        float v1 = (0.5f + z1 / (2.0f * radius)) * repeatY;
        float u2 = (0.5f + x2 / (2.0f * radius)) * repeatX;
        float v2 = (0.5f + z2 / (2.0f * radius)) * repeatY;
        float uc = 0.5f * repeatX;
        float vc = 0.5f * repeatY;

        glNormal3f(0.0f, -1.0f, 0.0f);
        glTexCoord2f(uc, vc); glVertex3f(0.0f, -halfH, 0.0f);
        glTexCoord2f(u2, v2); glVertex3f(x2, -halfH, z2);
        glTexCoord2f(u1, v1); glVertex3f(x1, -halfH, z1);
    }
    glEnd();
}

// Dibujar Cono con mapeo lateral analitico, normales reales y base planar
void drawTexturedCone(float baseRadius, float height, int slices, int stacks, float repeatX, float repeatY) {
    float halfH = height / 2.0f;

    // 1. Cara lateral
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < slices; i++) {
        float theta1 = i * 2.0f * M_PI / slices;
        float theta2 = (i + 1) * 2.0f * M_PI / slices;

        float cos1 = cos(theta1), sin1 = sin(theta1);
        float cos2 = cos(theta2), sin2 = sin(theta2);

        // Normales analiticas
        float nx1 = cos1 * height;
        float ny1 = baseRadius;
        float nz1 = sin1 * height;
        float len1 = sqrt(nx1*nx1 + ny1*ny1 + nz1*nz1);
        nx1 /= len1; ny1 /= len1; nz1 /= len1;

        float nx2 = cos2 * height;
        float ny2 = baseRadius;
        float nz2 = sin2 * height;
        float len2 = sqrt(nx2*nx2 + ny2*ny2 + nz2*nz2);
        nx2 /= len2; ny2 /= len2; nz2 /= len2;

        for (int j = 0; j < stacks; j++) {
            float h1 = (float)j / stacks;
            float h2 = (float)(j + 1) / stacks;

            float y1 = -halfH + h1 * height;
            float y2 = -halfH + h2 * height;

            float r1 = baseRadius * (1.0f - h1);
            float r2 = baseRadius * (1.0f - h2);

            float x1_theta1 = r1 * cos1;
            float z1_theta1 = r1 * sin1;
            float x1_theta2 = r1 * cos2;
            float z1_theta2 = r1 * sin2;

            float x2_theta1 = r2 * cos1;
            float z2_theta1 = r2 * sin1;
            float x2_theta2 = r2 * cos2;
            float z2_theta2 = r2 * sin2;

            float u1 = (float)i / slices * repeatX;
            float u2 = (float)(i + 1) / slices * repeatX;
            float v1 = h1 * repeatY;
            float v2 = h2 * repeatY;

            // Triangulo 1
            glNormal3f(nx1, ny1, nz1); glTexCoord2f(u1, v1); glVertex3f(x1_theta1, y1, z1_theta1);
            glNormal3f(nx1, ny1, nz1); glTexCoord2f(u1, v2); glVertex3f(x2_theta1, y2, z2_theta1);
            glNormal3f(nx2, ny2, nz2); glTexCoord2f(u2, v1); glVertex3f(x1_theta2, y1, z1_theta2);

            // Triangulo 2
            glNormal3f(nx2, ny2, nz2); glTexCoord2f(u2, v1); glVertex3f(x1_theta2, y1, z1_theta2);
            glNormal3f(nx1, ny1, nz1); glTexCoord2f(u1, v2); glVertex3f(x2_theta1, y2, z2_theta1);
            glNormal3f(nx2, ny2, nz2); glTexCoord2f(u2, v2); glVertex3f(x2_theta2, y2, z2_theta2);
        }
    }
    glEnd();

    // 2. Base (Planar)
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < slices; i++) {
        float theta1 = i * 2.0f * M_PI / slices;
        float theta2 = (i + 1) * 2.0f * M_PI / slices;

        float x1 = baseRadius * cos(theta1);
        float z1 = baseRadius * sin(theta1);
        float x2 = baseRadius * cos(theta2);
        float z2 = baseRadius * sin(theta2);

        float u1 = (0.5f + x1 / (2.0f * baseRadius)) * repeatX;
        float v1 = (0.5f + z1 / (2.0f * baseRadius)) * repeatY;
        float u2 = (0.5f + x2 / (2.0f * baseRadius)) * repeatX;
        float v2 = (0.5f + z2 / (2.0f * baseRadius)) * repeatY;
        float uc = 0.5f * repeatX;
        float vc = 0.5f * repeatY;

        glNormal3f(0.0f, -1.0f, 0.0f);
        glTexCoord2f(uc, vc); glVertex3f(0.0f, -halfH, 0.0f);
        glTexCoord2f(u2, v2); glVertex3f(x2, -halfH, z2);
        glTexCoord2f(u1, v1); glVertex3f(x1, -halfH, z1);
    }
    glEnd();
}

// Cargador OBJ en C++ standard
bool loadOBJ(const std::string& path, OBJMesh& mesh) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Error: No se pudo abrir el archivo OBJ: " << path << std::endl;
        mesh.loaded = false;
        return false;
    }

    mesh.positions.clear();
    mesh.texcoords.clear();
    mesh.normals.clear();
    mesh.triangles.clear();
    mesh.loaded = false;
    mesh.hasTexcoords = false;
    mesh.hasNormals = false;
    mesh.filename = path.substr(path.find_last_of("/\\") + 1);

    std::string line;
    while (std::getline(file, line)) {
        size_t start = line.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) continue;
        line = line.substr(start);
        if (line.empty() || line[0] == '#') continue;

        std::stringstream ss(line);
        std::string prefix;
        ss >> prefix;

        if (prefix == "v") {
            Vec3 pos;
            ss >> pos.x >> pos.y >> pos.z;
            mesh.positions.push_back(pos);
        } else if (prefix == "vt") {
            Vec2 tc;
            ss >> tc.u >> tc.v;
            mesh.texcoords.push_back(tc);
            mesh.hasTexcoords = true;
        } else if (prefix == "vn") {
            Vec3 norm;
            ss >> norm.x >> norm.y >> norm.z;
            mesh.normals.push_back(norm);
            mesh.hasNormals = true;
        } else if (prefix == "f") {
            std::vector<OBJVertex> faceVertices;
            std::string token;
            while (ss >> token) {
                int v = 0, vt = 0, vn = 0;
                std::stringstream tokStream(token);
                std::string part;

                if (std::getline(tokStream, part, '/')) {
                    if (!part.empty()) v = std::stoi(part);
                }
                if (std::getline(tokStream, part, '/')) {
                    if (!part.empty()) vt = std::stoi(part);
                }
                if (std::getline(tokStream, part, '/')) {
                    if (!part.empty()) vn = std::stoi(part);
                }

                OBJVertex vert;
                vert.vIndex = (v > 0) ? (v - 1) : ((v < 0) ? (int)mesh.positions.size() + v : -1);
                vert.vtIndex = (vt > 0) ? (vt - 1) : ((vt < 0) ? (int)mesh.texcoords.size() + vt : -1);
                vert.vnIndex = (vn > 0) ? (vn - 1) : ((vn < 0) ? (int)mesh.normals.size() + vn : -1);
                faceVertices.push_back(vert);
            }

            if (faceVertices.size() >= 3) {
                for (size_t i = 1; i < faceVertices.size() - 1; i++) {
                    OBJTriangle tri;
                    tri.a = faceVertices[0];
                    tri.b = faceVertices[i];
                    tri.c = faceVertices[i + 1];
                    mesh.triangles.push_back(tri);
                }
            }
        }
    }

    file.close();
    if (mesh.positions.empty()) {
        mesh.loaded = false;
        return false;
    }
    mesh.loaded = true;
    std::cout << "Modelo OBJ cargado correctamente: " << path << std::endl;
    std::cout << "  Vertices: " << mesh.positions.size() << " | Triangulos: " << mesh.triangles.size() << std::endl;
    return true;
}

// Dibujar OBJ model cargado
void drawOBJModel(const OBJMesh& mesh, float repeatX, float repeatY, bool flipV) {
    if (!mesh.loaded) return;

    glBegin(GL_TRIANGLES);
    for (const auto& tri : mesh.triangles) {
        // Vertice A
        if (mesh.hasNormals && tri.a.vnIndex >= 0 && tri.a.vnIndex < (int)mesh.normals.size()) {
            const auto& n = mesh.normals[tri.a.vnIndex];
            glNormal3f(n.x, n.y, n.z);
        } else {
            Vec3 vA = mesh.positions[tri.a.vIndex];
            Vec3 vB = mesh.positions[tri.b.vIndex];
            Vec3 vC = mesh.positions[tri.c.vIndex];
            float ux = vB.x - vA.x, uy = vB.y - vA.y, uz = vB.z - vA.z;
            float vx = vC.x - vA.x, vy = vC.y - vA.y, vz = vC.z - vA.z;
            float nx = uy*vz - uz*vy, ny = uz*vx - ux*vz, nz = ux*vy - uy*vx;
            float len = sqrt(nx*nx + ny*ny + nz*nz);
            if (len > 0.0f) { nx /= len; ny /= len; nz /= len; }
            glNormal3f(nx, ny, nz);
        }
        if (mesh.hasTexcoords && tri.a.vtIndex >= 0 && tri.a.vtIndex < (int)mesh.texcoords.size()) {
            const auto& tc = mesh.texcoords[tri.a.vtIndex];
            glTexCoord2f(tc.u * repeatX, (flipV ? 1.0f - tc.v : tc.v) * repeatY);
        }
        glVertex3f(mesh.positions[tri.a.vIndex].x, mesh.positions[tri.a.vIndex].y, mesh.positions[tri.a.vIndex].z);

        // Vertice B
        if (mesh.hasNormals && tri.b.vnIndex >= 0 && tri.b.vnIndex < (int)mesh.normals.size()) {
            const auto& n = mesh.normals[tri.b.vnIndex];
            glNormal3f(n.x, n.y, n.z);
        } else {
            Vec3 vA = mesh.positions[tri.a.vIndex];
            Vec3 vB = mesh.positions[tri.b.vIndex];
            Vec3 vC = mesh.positions[tri.c.vIndex];
            float ux = vB.x - vA.x, uy = vB.y - vA.y, uz = vB.z - vA.z;
            float vx = vC.x - vA.x, vy = vC.y - vA.y, vz = vC.z - vA.z;
            float nx = uy*vz - uz*vy, ny = uz*vx - ux*vz, nz = ux*vy - uy*vx;
            float len = sqrt(nx*nx + ny*ny + nz*nz);
            if (len > 0.0f) { nx /= len; ny /= len; nz /= len; }
            glNormal3f(nx, ny, nz);
        }
        if (mesh.hasTexcoords && tri.b.vtIndex >= 0 && tri.b.vtIndex < (int)mesh.texcoords.size()) {
            const auto& tc = mesh.texcoords[tri.b.vtIndex];
            glTexCoord2f(tc.u * repeatX, (flipV ? 1.0f - tc.v : tc.v) * repeatY);
        }
        glVertex3f(mesh.positions[tri.b.vIndex].x, mesh.positions[tri.b.vIndex].y, mesh.positions[tri.b.vIndex].z);

        // Vertice C
        if (mesh.hasNormals && tri.c.vnIndex >= 0 && tri.c.vnIndex < (int)mesh.normals.size()) {
            const auto& n = mesh.normals[tri.c.vnIndex];
            glNormal3f(n.x, n.y, n.z);
        } else {
            Vec3 vA = mesh.positions[tri.a.vIndex];
            Vec3 vB = mesh.positions[tri.b.vIndex];
            Vec3 vC = mesh.positions[tri.c.vIndex];
            float ux = vB.x - vA.x, uy = vB.y - vA.y, uz = vB.z - vA.z;
            float vx = vC.x - vA.x, vy = vC.y - vA.y, vz = vC.z - vA.z;
            float nx = uy*vz - uz*vy, ny = uz*vx - ux*vz, nz = ux*vy - uy*vx;
            float len = sqrt(nx*nx + ny*ny + nz*nz);
            if (len > 0.0f) { nx /= len; ny /= len; nz /= len; }
            glNormal3f(nx, ny, nz);
        }
        if (mesh.hasTexcoords && tri.c.vtIndex >= 0 && tri.c.vtIndex < (int)mesh.texcoords.size()) {
            const auto& tc = mesh.texcoords[tri.c.vtIndex];
            glTexCoord2f(tc.u * repeatX, (flipV ? 1.0f - tc.v : tc.v) * repeatY);
        }
        glVertex3f(mesh.positions[tri.c.vIndex].x, mesh.positions[tri.c.vIndex].y, mesh.positions[tri.c.vIndex].z);
    }
    glEnd();
}

// Cargar imagen personalizada usando el explorador de Windows (OpenFileName)
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
            if (textureIds[5] != 0) {
                glDeleteTextures(1, &textureIds[5]);
            }
            
            glGenTextures(1, &textureIds[5]);
            glBindTexture(GL_TEXTURE_2D, textureIds[5]);
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



// Aplicar los parametros de envoltura, filtrado y anisotropia en tiempo real
void applyTextureState(GLuint customTexId) {
    GLuint activeTexId = (customTexId != 0) ? customTexId : textureIds[state.texture];

    if (!state.textureActive || activeTexId == 0 || state.texture == TEX_NONE) {
        glDisable(GL_TEXTURE_2D);
        return;
    }

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, activeTexId);

    // 1. Envoltura (Wrapping)
    GLint wrapVal = GL_REPEAT;
    if (state.wrapMode == WRAP_CLAMP) {
        wrapVal = GL_CLAMP_TO_EDGE;
    } else if (state.wrapMode == WRAP_MIRRORED_REPEAT) {
        wrapVal = GL_MIRRORED_REPEAT;
    }
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
        if (state.minFilter == MIN_NEAREST || 
            state.minFilter == MIN_NEAREST_MIPMAP_NEAREST || 
            state.minFilter == MIN_NEAREST_MIPMAP_LINEAR) {
            minFilterVal = GL_NEAREST;
        } else {
            minFilterVal = GL_LINEAR;
        }
    }

    GLint magFilterVal = (state.magFilter == MAG_NEAREST) ? GL_NEAREST : GL_LINEAR;

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilterVal);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilterVal);

    // 3. Anisotropia
    if (maxAnisotropy > 1.0f) {
        float safeAniso = std::min(state.anisotropy, maxAnisotropy);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, safeAniso);
    }
}

// Dibujar geometria seleccionada
void drawActiveGeometry() {
    switch (state.geometry) {
        case GEOM_CUBO:
            drawTexturedCube(2.0f, state.repeatX, state.repeatY);
            break;
        case GEOM_ESFERA:
            drawTexturedSphere(1.3f, 64, 64, state.repeatX, state.repeatY);
            break;
        case GEOM_PLANO:
            drawTexturedPlane(2.5f, state.repeatX, state.repeatY);
            break;
        case GEOM_CILINDRO:
            drawTexturedCylinder(1.0f, 2.2f, 64, 32, state.repeatX, state.repeatY);
            break;
        case GEOM_CONO:
            drawTexturedCone(1.0f, 2.2f, 64, 32, state.repeatX, state.repeatY);
            break;
        case GEOM_TETERA:
            if (state.objMeshes[0].loaded) {
                glPushMatrix();
                glScalef(1.2f, 1.2f, 1.2f);
                drawOBJModel(state.objMeshes[0], state.repeatX, state.repeatY, state.objFlipV);
                glPopMatrix();
            } else {
                glEnable(GL_TEXTURE_GEN_S);
                glEnable(GL_TEXTURE_GEN_T);
                GLfloat sGenParams[] = {1.0f, 0.0f, 0.0f, 0.0f};
                GLfloat tGenParams[] = {0.0f, 1.0f, 0.0f, 0.0f};
                glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
                glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
                glTexGenfv(GL_S, GL_OBJECT_PLANE, sGenParams);
                glTexGenfv(GL_T, GL_OBJECT_PLANE, tGenParams);
                
                glutSolidTeapot(1.2);
                
                glDisable(GL_TEXTURE_GEN_S);
                glDisable(GL_TEXTURE_GEN_T);
            }
            break;
        case GEOM_OBJ:
            if (state.objMeshes[state.activeObjIndex].loaded) {
                glPushMatrix();
                // Centrado y escalado automatico del OBJ
                const auto& activeMesh = state.objMeshes[state.activeObjIndex];
                float minX = 1e9, minY = 1e9, minZ = 1e9;
                float maxX = -1e9, maxY = -1e9, maxZ = -1e9;
                for (const auto& pos : activeMesh.positions) {
                    minX = std::min(minX, pos.x); maxX = std::max(maxX, pos.x);
                    minY = std::min(minY, pos.y); maxY = std::max(maxY, pos.y);
                    minZ = std::min(minZ, pos.z); maxZ = std::max(maxZ, pos.z);
                }
                float cx = (minX + maxX) / 2.0f;
                float cy = (minY + maxY) / 2.0f;
                float cz = (minZ + maxZ) / 2.0f;
                float sizeX = maxX - minX;
                float sizeY = maxY - minY;
                float sizeZ = maxZ - minZ;
                float maxSize = std::max({sizeX, sizeY, sizeZ});
                float scale = 2.0f / (maxSize > 0.0f ? maxSize : 1.0f);
                
                glScalef(scale, scale, scale);
                glTranslatef(-cx, -cy, -cz);
                
                drawOBJModel(activeMesh, state.repeatX, state.repeatY, state.objFlipV);
                glPopMatrix();
            }
            break;
    }
}

// Dibujar GridHelper
void drawGridHelper(float size, int divisions) {
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glLineWidth(1.0f);

    float step = size / divisions;
    float halfSize = size / 2.0f;

    glBegin(GL_LINES);
    for (int i = 0; i <= divisions; i++) {
        float pos = -halfSize + i * step;

        if (std::abs(pos) < 0.001f) {
            glColor3f(0.31f, 0.27f, 0.90f); // Azul Neon
        } else {
            glColor3f(0.15f, 0.15f, 0.16f); // Gris oscuro
        }

        glVertex3f(pos, -1.5f, -halfSize);
        glVertex3f(pos, -1.5f, halfSize);

        glVertex3f(-halfSize, -1.5f, pos);
        glVertex3f(halfSize, -1.5f, pos);
    }
    glEnd();
    glEnable(GL_LIGHTING);
}

// Dibujar gizmo de ejes en esquina del viewport
void drawViewportGizmo() {
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    int gizmoSize = 100;
    glViewport(viewport[0] + viewport[2] - gizmoSize - 20, viewport[1] + viewport[3] - gizmoSize - 20, gizmoSize, gizmoSize);

    glClear(GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluPerspective(50.0, 1.0, 0.1, 10.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    float gcx = 3.0f * cos(cameraAngleY) * sin(cameraAngleX);
    float gcy = 3.0f * sin(cameraAngleY);
    float gcz = 3.0f * cos(cameraAngleY) * cos(cameraAngleX);
    float upY = (cos(cameraAngleY) >= 0.0f) ? 1.0f : -1.0f;
    gluLookAt(gcx, gcy, gcz, 0.0f, 0.0f, 0.0f, 0.0f, upY, 0.0f);

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    glLineWidth(3.0f);
    glBegin(GL_LINES);
    glColor3f(1.0f, 0.2f, 0.2f); glVertex3f(0.0f, 0.0f, 0.0f); glVertex3f(1.0f, 0.0f, 0.0f); // X
    glColor3f(0.2f, 1.0f, 0.2f); glVertex3f(0.0f, 0.0f, 0.0f); glVertex3f(0.0f, 1.0f, 0.0f); // Y
    glColor3f(0.2f, 0.2f, 1.0f); glVertex3f(0.0f, 0.0f, 0.0f); glVertex3f(0.0f, 0.0f, 1.0f); // Z
    glEnd();

    glColor3f(1.0f, 0.2f, 0.2f);
    glRasterPos3f(1.2f, 0.0f, 0.0f); glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, 'X');
    glColor3f(0.2f, 1.0f, 0.2f);
    glRasterPos3f(0.0f, 1.2f, 0.0f); glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, 'Y');
    glColor3f(0.2f, 0.2f, 1.0f);
    glRasterPos3f(0.0f, 0.0f, 1.2f); glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, 'Z');

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    glEnable(GL_LIGHTING);
}

// Dibujar texto en pantalla 2D
void drawText(float x, float y, const std::string& text, void* font = GLUT_BITMAP_8_BY_13) {
    glRasterPos2f(x, y);
    for (char c : text) {
        glutBitmapCharacter(font, c);
    }
}

// Dibujar texto en pantalla 3D
void drawText3D(float x, float y, float z, const std::string& text, void* font) {
    glRasterPos3f(x, y, z);
    for (char c : text) {
        glutBitmapCharacter(font, c);
    }
}

// Iniciar HUD 2D
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

// Terminar HUD 2D
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

    // Fondo translucido
    glColor4f(0.07f, 0.07f, 0.08f, 0.92f);
    glBegin(GL_QUADS);
    glVertex2f(15.0f, 15.0f);
    glVertex2f(445.0f, 15.0f);
    glVertex2f(445.0f, windowHeight - 15.0f);
    glVertex2f(15.0f, windowHeight - 15.0f);
    glEnd();

    // Borde elegante
    glColor4f(0.23f, 0.51f, 0.96f, 0.70f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(15.0f, 15.0f);
    glVertex2f(445.0f, 15.0f);
    glVertex2f(445.0f, windowHeight - 15.0f);
    glVertex2f(15.0f, windowHeight - 15.0f);
    glEnd();

    float y = windowHeight - 45.0f;

    // Titulo
    glColor3f(1.0f, 1.0f, 1.0f);
    drawText(30.0f, y, "LABORATORIO DE TEXTURAS 3D", GLUT_BITMAP_HELVETICA_18);
    y -= 30.0f;

    // Linea Separadora
    glColor4f(1.0f, 1.0f, 1.0f, 0.1f);
    glBegin(GL_LINES);
    glVertex2f(30.0f, y);
    glVertex2f(430.0f, y);
    glEnd();
    y -= 20.0f;

    // SECCION: MODO DIDACTICO ACTIVO
    glColor3f(0.23f, 0.51f, 0.96f);
    drawText(30.0f, y, "MODO DIDACTICO ACTIVO:", GLUT_BITMAP_HELVETICA_12);
    y -= 22.0f;

    glColor3f(1.0f, 0.85f, 0.3f);
    std::string modeStr = "";
    switch (state.demoMode) {
        case DEMO_LIBRE: modeStr = "[0] Modo Libre (Configuracion Total)"; break;
        case DEMO_INTERPOLACION_UV: modeStr = "[1] Interpolacion de Coordenadas UV"; break;
        case DEMO_FILTRADO: modeStr = "[2] Filtrado (Nearest vs Linear)"; break;
        case DEMO_MIPMAPPING: modeStr = "[3] Mipmapping (Sin vs Con Mipmap)"; break;
        case DEMO_WRAP: modeStr = "[4] Modos de Repeticion (Wrap Modes)"; break;
        case DEMO_PROYECCIONES: modeStr = "[5] Proyecciones Analiticas"; break;
        case DEMO_OBJ_UV: modeStr = "[6] Desenvuelto UV / Carga de OBJ"; break;
    }
    drawText(30.0f, y, modeStr, GLUT_BITMAP_HELVETICA_12);
    y -= 25.0f;

    // Explicacion contextual del modo activo
    glColor3f(0.8f, 0.8f, 0.8f);
    if (state.demoMode == DEMO_INTERPOLACION_UV) {
        drawText(30.0f, y, "Cada vertice tiene coordenadas UV. OpenGL las"); y -= 16.0f;
        drawText(30.0f, y, "interpola en cada triangulo y muestrea la textura."); y -= 16.0f;
        drawText(30.0f, y, "La linea roja muestra la costura de textura (1 -> 0).");
        y -= 25.0f;
    } else if (state.demoMode == DEMO_FILTRADO) {
        drawText(30.0f, y, "El filtrado define como se obtiene el color cuando"); y -= 16.0f;
        drawText(30.0f, y, "una coordenada UV cae entre texeles o cuando la"); y -= 16.0f;
        drawText(30.0f, y, "textura se amplia (magnifica) o reduce (minifica).");
        y -= 25.0f;
    } else if (state.demoMode == DEMO_MIPMAPPING) {
        drawText(30.0f, y, "Los Mipmaps reducen el aliasing (centelleo y"); y -= 16.0f;
        drawText(30.0f, y, "ruido visual) a distancias lejanas mediante el"); y -= 16.0f;
        drawText(30.0f, y, "uso de texturas pre-escaladas a menor resolucion.");
        y -= 25.0f;
    } else if (state.demoMode == DEMO_WRAP) {
        drawText(30.0f, y, "Define como responde OpenGL cuando las coords UV"); y -= 16.0f;
        drawText(30.0f, y, "salen del rango [0.0, 1.0]. Compara REPEAT, CLAMP"); y -= 16.0f;
        drawText(30.0f, y, "y MIRRORED_REPEAT en simultaneo.");
        y -= 25.0f;
    } else if (state.demoMode == DEMO_PROYECCIONES) {
        drawText(30.0f, y, "Metodos geometricos para calcular coords UV:"); y -= 16.0f;
        drawText(30.0f, y, "Planar (deriva de X/Z), Esferica (longitud/latitud),"); y -= 16.0f;
        drawText(30.0f, y, "Cilindrica (angulo/altura) o Conica (angulo/altura radial).");
        y -= 25.0f;
    } else if (state.demoMode == DEMO_OBJ_UV) {
        drawText(30.0f, y, "El modelo OBJ contiene coordenadas de textura 'vt'"); y -= 16.0f;
        drawText(30.0f, y, "exportadas desde un editor 3D. Esto representa"); y -= 16.0f;
        drawText(30.0f, y, "un desenvuelto (unwrap) UV real de una malla compleja.");
        y -= 25.0f;
    }

    // Linea Separadora
    glColor4f(1.0f, 1.0f, 1.0f, 0.1f);
    glBegin(GL_LINES);
    glVertex2f(30.0f, y);
    glVertex2f(430.0f, y);
    glEnd();
    y -= 20.0f;

    // SECCION: ESTADO DEL RENDERIZADO
    glColor3f(0.23f, 0.51f, 0.96f);
    drawText(30.0f, y, "ESTADO DEL RENDERIZADO:", GLUT_BITMAP_HELVETICA_12);
    y -= 22.0f;

    glColor3f(1.0f, 1.0f, 1.0f);

    std::string geomStr = "Geometria: ";
    switch (state.geometry) {
        case GEOM_CUBO: geomStr += "Cubo 3D"; break;
        case GEOM_ESFERA: geomStr += "Esfera 3D (Corregida)"; break;
        case GEOM_PLANO: geomStr += "Plano 2D"; break;
        case GEOM_CILINDRO: geomStr += "Cilindro Analitico"; break;
        case GEOM_CONO: geomStr += "Cono Analitico"; break;
        case GEOM_TETERA: geomStr += (state.objMeshes[0].loaded) ? "Tetera OBJ (Con UV)" : "Tetera GLUT (Gen Auto)"; break;
        case GEOM_OBJ: geomStr += state.objMeshes[state.activeObjIndex].loaded ? ("OBJ: " + state.objMeshes[state.activeObjIndex].filename) : "OBJ No Cargado"; break;
    }
    drawText(30.0f, y, geomStr);
    y -= 18.0f;

    std::string texStr = "Textura: ";
    if (!state.textureActive) texStr += "NINGUNA (Color Plano)";
    else {
        switch (state.texture) {
            case TEX_GRID: texStr += "Grid UV (Mapeo)"; break;
            case TEX_BRICK: texStr += "Ladrillos (Patron)"; break;
            case TEX_WOOD: texStr += "Madera (Organico)"; break;
            case TEX_STONE: texStr += "Piedra (Rugoso)"; break;
            case TEX_CHECKER: texStr += "Tablero Checker (Contraste)"; break;
            case TEX_CUSTOM: texStr += "Personalizada (Cargada)"; break;
            case TEX_NONE: texStr += "NINGUNA"; break;
        }
    }
    drawText(30.0f, y, texStr);
    y -= 18.0f;

    std::string visualStr = "Modo Visual: ";
    visualStr += state.wireframe ? "Wireframe (Triangulos)" : "Solido Texturizado";
    drawText(30.0f, y, visualStr);
    y -= 18.0f;

    if (state.demoMode != DEMO_WRAP) {
        std::string wrapStr = "Envoltura (Wrap): ";
        switch (state.wrapMode) {
            case WRAP_REPEAT: wrapStr += "GL_REPEAT"; break;
            case WRAP_CLAMP: wrapStr += "GL_CLAMP_TO_EDGE"; break;
            case WRAP_MIRRORED_REPEAT: wrapStr += "GL_MIRRORED_REPEAT"; break;
        }
        drawText(30.0f, y, wrapStr);
        y -= 18.0f;
    }

    if (state.demoMode != DEMO_WRAP) {
        char repBuf[64];
        sprintf(repBuf, "Repeticion UV: %.1fx * %.1fy", state.repeatX, state.repeatY);
        drawText(30.0f, y, repBuf);
        y -= 18.0f;
    }

    if (state.demoMode != DEMO_MIPMAPPING) {
        std::string mipStr = "Uso de Mipmaps: ";
        mipStr += state.generateMipmaps ? "SI (Habilitado)" : "NO (Deshabilitado)";
        drawText(30.0f, y, mipStr);
        y -= 18.0f;
    }

    if (state.demoMode != DEMO_FILTRADO) {
        std::string minStr = "Filtro Min (Lejos): ";
        switch (state.minFilter) {
            case MIN_NEAREST: minStr += "GL_NEAREST"; break;
            case MIN_LINEAR: minStr += "GL_LINEAR"; break;
            case MIN_NEAREST_MIPMAP_NEAREST: minStr += "GL_NEAREST_MIPMAP_NEAREST"; break;
            case MIN_NEAREST_MIPMAP_LINEAR: minStr += "GL_NEAREST_MIPMAP_LINEAR"; break;
            case MIN_LINEAR_MIPMAP_NEAREST: minStr += "GL_LINEAR_MIPMAP_NEAREST"; break;
            case MIN_LINEAR_MIPMAP_LINEAR: minStr += "GL_LINEAR_MIPMAP_LINEAR"; break;
        }
        drawText(30.0f, y, minStr);
        y -= 18.0f;

        std::string magStr = "Filtro Mag (Cerca): ";
        magStr += (state.magFilter == MAG_NEAREST) ? "GL_NEAREST" : "GL_LINEAR";
        drawText(30.0f, y, magStr);
        y -= 18.0f;
    }

    if (state.demoMode != DEMO_FILTRADO && state.demoMode != DEMO_MIPMAPPING) {
        char anisBuf[64];
        sprintf(anisBuf, "Filtrado Anisotropico: %.1fx", state.anisotropy);
        drawText(30.0f, y, anisBuf);
        y -= 25.0f;
    } else {
        y -= 7.0f;
    }

    // Informacion especifica del OBJ si esta activo
    if (state.geometry == GEOM_OBJ || state.demoMode == DEMO_OBJ_UV) {
        const auto& activeMesh = state.objMeshes[state.activeObjIndex];
        if (!activeMesh.loaded) {
            glColor3f(1.0f, 0.3f, 0.3f);
            std::string errStr = "ERROR: " + std::string(objPaths[state.activeObjIndex]) + " no encontrado.";
            drawText(30.0f, y, errStr);
            y -= 20.0f;
        } else {
            glColor3f(0.3f, 1.0f, 0.3f);
            drawText(30.0f, y, "OBJ cargado correctamente."); y -= 16.0f;
            if (!activeMesh.hasTexcoords) {
                glColor3f(1.0f, 0.3f, 0.3f);
                drawText(30.0f, y, "ATENCION: Este modelo no tiene coords vt."); y -= 16.0f;
                drawText(30.0f, y, "No se puede demostrar desenvuelto UV.");
                y -= 20.0f;
            } else {
                glColor3f(0.6f, 0.8f, 1.0f);
                char infoBuf[80];
                sprintf(infoBuf, "Malla: %s | Triangulos: %d", activeMesh.filename.c_str(), (int)activeMesh.triangles.size());
                drawText(30.0f, y, infoBuf); y -= 16.0f;
                std::string flipStr = "Invertir V del OBJ: ";
                flipStr += state.objFlipV ? "SI (Habilitado)" : "NO (Deshabilitado)";
                drawText(30.0f, y, flipStr);
                y -= 20.0f;
            }
        }
    }

    // Linea Separadora
    glColor4f(1.0f, 1.0f, 1.0f, 0.1f);
    glBegin(GL_LINES);
    glVertex2f(30.0f, y);
    glVertex2f(430.0f, y);
    glEnd();
    y -= 20.0f;

    // SECCION: CONTROLES DISPONIBLES (Contextuales)
    glColor3f(0.23f, 0.51f, 0.96f);
    drawText(30.0f, y, "CONTROLES DISPONIBLES:", GLUT_BITMAP_HELVETICA_12);
    y -= 22.0f;

    glColor3f(0.9f, 0.9f, 0.9f);
    
    // Controles globales siempre presentes
    drawText(30.0f, y, "[0]-[6] Cambiar Modo Didactico"); y -= 16.0f;
    drawText(30.0f, y, "[Z] Alternar Solido / Wireframe"); y -= 16.0f;
    drawText(30.0f, y, "[H] Mostrar/Ocultar HUD"); y -= 16.0f;

    if (state.demoMode == DEMO_LIBRE) {
        drawText(30.0f, y, "[G] Ciclar Geometria"); y -= 16.0f;
        if (state.geometry == GEOM_OBJ) {
            drawText(30.0f, y, "[O] Ciclar Modelo (Tetera/Cubo/Esf/Minion)"); y -= 16.0f;
        }
        drawText(30.0f, y, "[T] Ciclar Texturas"); y -= 16.0f;
        drawText(30.0f, y, "[U] Cargar Textura Local"); y -= 16.0f;
        drawText(30.0f, y, "[W] Ciclar Wrap (Repeat/Clamp/Mirror)"); y -= 16.0f;
        drawText(30.0f, y, "[+] / [-] Modificar Repeticion UV"); y -= 16.0f;
        drawText(30.0f, y, "[M] Activar/Desactivar Mipmapping"); y -= 16.0f;
        drawText(30.0f, y, "[F] Ciclar Filtro Minificacion"); y -= 16.0f;
        drawText(30.0f, y, "[N] Alternar Filtro Magnificacion"); y -= 16.0f;
        drawText(30.0f, y, "[A] / [D] Modificar Anisotropia"); y -= 16.0f;
    } else {
        if (state.demoMode == DEMO_INTERPOLACION_UV) {
            drawText(30.0f, y, "[G] Alternar Esfera / Cilindro"); y -= 16.0f;
            drawText(30.0f, y, "[Z] Activar Wireframe para ver triangulos"); y -= 16.0f;
        } else if (state.demoMode == DEMO_FILTRADO) {
            drawText(30.0f, y, "[T] Ciclar Texturas (Se sugiere Checker)"); y -= 16.0f;
            drawText(30.0f, y, "[M] Alternar Mipmapping en lado derecho"); y -= 16.0f;
            drawText(30.0f, y, "* Usa Zoom para acercarte y ver pixeles."); y -= 16.0f;
        } else if (state.demoMode == DEMO_MIPMAPPING) {
            drawText(30.0f, y, "[T] Ciclar Texturas"); y -= 16.0f;
            drawText(30.0f, y, "* Aleja la camara para ver aliasing a la izq."); y -= 16.0f;
        } else if (state.demoMode == DEMO_WRAP) {
            drawText(30.0f, y, "[G] Cambiar Geometria de comparacion"); y -= 16.0f;
            drawText(30.0f, y, "[T] Ciclar Texturas"); y -= 16.0f;
        } else if (state.demoMode == DEMO_PROYECCIONES) {
            drawText(30.0f, y, "[G] Cambiar Geometria (Plano/Esf/Cil/Cono)"); y -= 16.0f;
            drawText(30.0f, y, "[T] Ciclar Texturas (Grid recomendado)"); y -= 16.0f;
            drawText(30.0f, y, "[W] Cambiar Wrap Mode"); y -= 16.0f;
            drawText(30.0f, y, "[+] / [-] Modificar Repeticion"); y -= 16.0f;
        } else if (state.demoMode == DEMO_OBJ_UV) {
            drawText(30.0f, y, "[O] Ciclar Modelo (Tetera/Cubo/Esf/Minion)"); y -= 16.0f;
            drawText(30.0f, y, "[V] Invertir orientacion vertical V"); y -= 16.0f;
            drawText(30.0f, y, "[T] Ciclar Texturas"); y -= 16.0f;
        }
    }

    drawText(30.0f, y, "[S] Guardar Captura de Pantalla (BMP)"); y -= 16.0f;
    drawText(30.0f, y, "[ESC] Salir");

    endHUD();
}

// Framebuffer captura a BMP
void captureScreenshot(const std::string& filename) {
    int width = glutGet(GLUT_WINDOW_WIDTH);
    int height = glutGet(GLUT_WINDOW_HEIGHT);
    std::vector<unsigned char> pixels(width * height * 3);
    
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    FILE* f = fopen(filename.c_str(), "wb");
    if (!f) {
        std::cerr << "Error: No se pudo escribir la captura en " << filename << std::endl;
        return;
    }

    unsigned char fileHeader[14] = {
        'B', 'M',           // Firma
        0, 0, 0, 0,         // Tamano (abajo)
        0, 0, 0, 0,
        54, 0, 0, 0         // Offset
    };
    unsigned char infoHeader[40] = {
        40, 0, 0, 0,
        0, 0, 0, 0,         // Ancho
        0, 0, 0, 0,         // Alto
        1, 0,
        24, 0,              // RGB
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
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

    for (int i = 0; i < width * height; i++) {
        unsigned char r = pixels[i * 3 + 0];
        unsigned char g = pixels[i * 3 + 1];
        unsigned char b = pixels[i * 3 + 2];
        fputc(b, f);
        fputc(g, f);
        fputc(r, f);
    }

    fclose(f);
    std::cout << "Captura de pantalla guardada en " << filename << std::endl;
}

// Funcion principal de renderizado
void display() {
    glClearColor(0.07f, 0.07f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Calcular Viewport de la escena 3D y HUD
    int sceneX = state.hudVisible ? 460 : 0;
    int sceneW = windowWidth - sceneX;
    if (sceneW < 1) sceneW = 1;

    // 1. RENDERIZADO 3D DE LA ESCENA
    glViewport(sceneX, 0, sceneW, windowHeight);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(50.0, (double)sceneW / (double)windowHeight, 0.1, 1000.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Posicionamiento de camara orbital
    float cx = cameraDistance * cos(cameraAngleY) * sin(cameraAngleX);
    float cy = cameraDistance * sin(cameraAngleY);
    float cz = cameraDistance * cos(cameraAngleY) * cos(cameraAngleX);
    float upY = (cos(cameraAngleY) >= 0.0f) ? 1.0f : -1.0f;
    gluLookAt(cx, cy, cz, 0.0f, 0.0f, 0.0f, 0.0f, upY, 0.0f);

    // Dibujar GridHelper (solo en modos correspondientes)
    if (state.demoMode != DEMO_FILTRADO && state.demoMode != DEMO_MIPMAPPING && state.demoMode != DEMO_WRAP) {
        drawGridHelper(10.0f, 10);
    }

    // Luces
    glEnable(GL_LIGHTING);
    float lightAmbient[] = {0.4f, 0.4f, 0.4f, 1.0f};
    float lightDiffuse[] = {0.8f, 0.8f, 0.8f, 1.0f};
    float lightPosition[] = {5.0f, 8.0f, 5.0f, 0.0f}; // Direccional
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
    glEnable(GL_LIGHT0);

    float light1Diffuse[] = {0.3f, 0.3f, 0.3f, 1.0f};
    float light1Position[] = {-4.0f, 2.0f, -4.0f, 1.0f};
    glLightfv(GL_LIGHT1, GL_DIFFUSE, light1Diffuse);
    glLightfv(GL_LIGHT1, GL_POSITION, light1Position);
    glEnable(GL_LIGHT1);

    // Habilitar colores en materiales
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

    // Renderizar escena segun el modo didactico activo
    if (state.demoMode == DEMO_FILTRADO) {
        // Lado izquierdo: NEAREST
        glPushMatrix();
        glTranslatef(-1.6f, 0.0f, 0.0f);
        if (state.wireframe) {
            glDisable(GL_TEXTURE_2D);
            glDisable(GL_LIGHTING);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glColor3f(0.0f, 1.0f, 1.0f);
        } else {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glEnable(GL_TEXTURE_2D);
            glEnable(GL_LIGHTING);
            glBindTexture(GL_TEXTURE_2D, textureIds[state.texture]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glColor3f(1.0f, 1.0f, 1.0f);
        }
        drawActiveGeometry();
        glPopMatrix();

        // Lado derecho: LINEAR
        glPushMatrix();
        glTranslatef(1.6f, 0.0f, 0.0f);
        if (state.wireframe) {
            glDisable(GL_TEXTURE_2D);
            glDisable(GL_LIGHTING);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glColor3f(0.0f, 1.0f, 1.0f);
        } else {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glEnable(GL_TEXTURE_2D);
            glEnable(GL_LIGHTING);
            glBindTexture(GL_TEXTURE_2D, textureIds[state.texture]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, state.generateMipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glColor3f(1.0f, 1.0f, 1.0f);
        }
        drawActiveGeometry();
        glPopMatrix();

        // Restaurar estado de texturas
        if (state.textureActive && state.texture != TEX_NONE) {
            applyTextureState();
        }

        // Etiquetas 3D
        glDisable(GL_LIGHTING);
        glDisable(GL_TEXTURE_2D);
        glColor3f(1.0f, 1.0f, 1.0f);
        drawText3D(-3.0f, 1.6f, 0.0f, "NEAREST: Texel mas cercano / Pixelado", GLUT_BITMAP_HELVETICA_12);
        drawText3D(0.3f, 1.6f, 0.0f, "LINEAR: Interpolacion bilineal / Suave", GLUT_BITMAP_HELVETICA_12);
        glEnable(GL_LIGHTING);
    } 
    else if (state.demoMode == DEMO_MIPMAPPING) {
        // Dos planos largos inclinados en perspectiva para ver aliasing
        
        // Plano Izquierdo: SIN MIPMAP (GL_LINEAR)
        glPushMatrix();
        glTranslatef(-1.8f, -0.5f, 0.0f);
        if (state.wireframe) {
            glDisable(GL_TEXTURE_2D);
            glDisable(GL_LIGHTING);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glColor3f(0.0f, 1.0f, 1.0f);
        } else {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glEnable(GL_TEXTURE_2D);
            glEnable(GL_LIGHTING);
            glBindTexture(GL_TEXTURE_2D, textureIds[state.texture]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glColor3f(1.0f, 1.0f, 1.0f);
        }
        drawTexturedPlane(12.0f, 12.0f, 12.0f);
        glPopMatrix();

        // Plano Derecho: CON MIPMAP (GL_LINEAR_MIPMAP_LINEAR)
        glPushMatrix();
        glTranslatef(1.8f, -0.5f, 0.0f);
        if (state.wireframe) {
            glDisable(GL_TEXTURE_2D);
            glDisable(GL_LIGHTING);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glColor3f(0.0f, 1.0f, 1.0f);
        } else {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glEnable(GL_TEXTURE_2D);
            glEnable(GL_LIGHTING);
            glBindTexture(GL_TEXTURE_2D, textureIds[state.texture]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glColor3f(1.0f, 1.0f, 1.0f);
        }
        drawTexturedPlane(12.0f, 12.0f, 12.0f);
        glPopMatrix();

        if (state.textureActive && state.texture != TEX_NONE) {
            applyTextureState();
        }

        // Etiquetas 3D
        glDisable(GL_LIGHTING);
        glDisable(GL_TEXTURE_2D);
        glColor3f(1.0f, 1.0f, 1.0f);
        drawText3D(-4.2f, 1.0f, 0.0f, "SIN MIPMAP: Aliasing / Centelleo a distancia", GLUT_BITMAP_HELVETICA_12);
        drawText3D(0.4f, 1.0f, 0.0f, "CON MIPMAP: Mapeo suavizado y estable", GLUT_BITMAP_HELVETICA_12);
        glEnable(GL_LIGHTING);
    } 
    else if (state.demoMode == DEMO_WRAP) {
        // Comparativa simultanea de REPEAT, CLAMP y MIRRORED REPEAT
        float rX = 4.0f, rY = 4.0f;

        // Izquierda: REPEAT
        glPushMatrix();
        glTranslatef(-2.4f, 0.0f, 0.0f);
        if (state.wireframe) {
            glDisable(GL_TEXTURE_2D);
            glDisable(GL_LIGHTING);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glColor3f(0.0f, 1.0f, 1.0f);
        } else {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glEnable(GL_TEXTURE_2D);
            glEnable(GL_LIGHTING);
            glBindTexture(GL_TEXTURE_2D, textureIds[state.texture]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glColor3f(1.0f, 1.0f, 1.0f);
        }
        
        if (state.geometry == GEOM_PLANO) drawTexturedPlane(2.0f, rX, rY);
        else if (state.geometry == GEOM_ESFERA) drawTexturedSphere(1.0f, 32, 32, rX, rY);
        else if (state.geometry == GEOM_CILINDRO) drawTexturedCylinder(0.8f, 1.8f, 32, 16, rX, rY);
        else if (state.geometry == GEOM_CONO) drawTexturedCone(0.8f, 1.8f, 32, 16, rX, rY);
        else drawTexturedCube(1.5f, rX, rY);
        glPopMatrix();

        // Centro: CLAMP_TO_EDGE
        glPushMatrix();
        glTranslatef(0.0f, 0.0f, 0.0f);
        if (state.wireframe) {
            glDisable(GL_TEXTURE_2D);
            glDisable(GL_LIGHTING);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glColor3f(0.0f, 1.0f, 1.0f);
        } else {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glEnable(GL_TEXTURE_2D);
            glEnable(GL_LIGHTING);
            glBindTexture(GL_TEXTURE_2D, textureIds[state.texture]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glColor3f(1.0f, 1.0f, 1.0f);
        }
        
        if (state.geometry == GEOM_PLANO) drawTexturedPlane(2.0f, rX, rY);
        else if (state.geometry == GEOM_ESFERA) drawTexturedSphere(1.0f, 32, 32, rX, rY);
        else if (state.geometry == GEOM_CILINDRO) drawTexturedCylinder(0.8f, 1.8f, 32, 16, rX, rY);
        else if (state.geometry == GEOM_CONO) drawTexturedCone(0.8f, 1.8f, 32, 16, rX, rY);
        else drawTexturedCube(1.5f, rX, rY);
        glPopMatrix();

        // Derecha: MIRRORED_REPEAT
        glPushMatrix();
        glTranslatef(2.4f, 0.0f, 0.0f);
        if (state.wireframe) {
            glDisable(GL_TEXTURE_2D);
            glDisable(GL_LIGHTING);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glColor3f(0.0f, 1.0f, 1.0f);
        } else {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glEnable(GL_TEXTURE_2D);
            glEnable(GL_LIGHTING);
            glBindTexture(GL_TEXTURE_2D, textureIds[state.texture]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
            glColor3f(1.0f, 1.0f, 1.0f);
        }
        
        if (state.geometry == GEOM_PLANO) drawTexturedPlane(2.0f, rX, rY);
        else if (state.geometry == GEOM_ESFERA) drawTexturedSphere(1.0f, 32, 32, rX, rY);
        else if (state.geometry == GEOM_CILINDRO) drawTexturedCylinder(0.8f, 1.8f, 32, 16, rX, rY);
        else if (state.geometry == GEOM_CONO) drawTexturedCone(0.8f, 1.8f, 32, 16, rX, rY);
        else drawTexturedCube(1.5f, rX, rY);
        glPopMatrix();

        if (state.textureActive && state.texture != TEX_NONE) {
            applyTextureState();
        }

        // Etiquetas 3D
        glDisable(GL_LIGHTING);
        glDisable(GL_TEXTURE_2D);
        glColor3f(1.0f, 1.0f, 1.0f);
        drawText3D(-3.1f, 1.3f, 0.0f, "GL_REPEAT", GLUT_BITMAP_HELVETICA_12);
        drawText3D(-0.5f, 1.3f, 0.0f, "GL_CLAMP", GLUT_BITMAP_HELVETICA_12);
        drawText3D(1.6f, 1.3f, 0.0f, "GL_MIRRORED_REPEAT", GLUT_BITMAP_HELVETICA_12);
        glEnable(GL_LIGHTING);
    } 
    else {
        // Modos Standard: LIBRE, INTERPOLACION, PROYECCIONES, OBJ
        if (state.wireframe) {
            glDisable(GL_TEXTURE_2D);
            glDisable(GL_LIGHTING);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glColor3f(0.0f, 1.0f, 1.0f);
        } else {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glEnable(GL_LIGHTING);
            if (state.textureActive && state.texture != TEX_NONE) {
                applyTextureState();
                glColor3f(1.0f, 1.0f, 1.0f);
            } else {
                glDisable(GL_TEXTURE_2D);
                glColor3fv(state.baseColor);
            }
        }

        drawActiveGeometry();

        // Indicador de costura (Seam) en modos didacticos
        if (!state.wireframe && (state.demoMode == DEMO_INTERPOLACION_UV || state.demoMode == DEMO_PROYECCIONES)) {
            glDisable(GL_LIGHTING);
            glDisable(GL_TEXTURE_2D);
            glLineWidth(2.0f);
            
            if (state.geometry == GEOM_ESFERA) {
                glColor3f(1.0f, 0.2f, 0.2f);
                glBegin(GL_LINE_STRIP);
                for (int j = 0; j <= 64; j++) {
                    float phi = j * M_PI / 64;
                    float x = 1.305f * sin(phi);
                    float y = 1.305f * cos(phi);
                    float z = 0.0f;
                    glVertex3f(x, y, z);
                }
                glEnd();
                glColor3f(1.0f, 1.0f, 1.0f);
                drawText3D(1.35f, 0.0f, 0.0f, "Costura UV (U: 1.0 -> 0.0)", GLUT_BITMAP_HELVETICA_10);
            } 
            else if (state.geometry == GEOM_CILINDRO) {
                glColor3f(1.0f, 0.2f, 0.2f);
                glBegin(GL_LINES);
                glVertex3f(1.01f, -1.1f, 0.0f);
                glVertex3f(1.01f, 1.1f, 0.0f);
                glEnd();
                glColor3f(1.0f, 1.0f, 1.0f);
                drawText3D(1.05f, 0.0f, 0.0f, "Costura UV (U: 1.0 -> 0.0)", GLUT_BITMAP_HELVETICA_10);
            }
            glEnable(GL_LIGHTING);
        }
    }

    drawViewportGizmo();

    // 2. INTERFAZ DE USUARIO HUD
    if (state.hudVisible) {
        glViewport(0, 0, windowWidth, windowHeight);
        drawHUD();
    }

    glutSwapBuffers();
}

// Redimensionar ventana
void reshape(int w, int h) {
    windowWidth = w;
    windowHeight = h;
    glViewport(0, 0, w, h);
}

// Manejo de eventos del teclado (Atajos de control)
void keyboard(unsigned char key, int x, int y) {
    switch (key) {
        case 27: // ESC
            exit(0);
            break;
        case 'h':
        case 'H':
            state.hudVisible = !state.hudVisible;
            break;
        case 'z':
        case 'Z':
            state.wireframe = !state.wireframe;
            break;

        case 'v':
        case 'V':
            state.objFlipV = !state.objFlipV;
            break;
        case 'g':
        case 'G':
            if (state.demoMode == DEMO_INTERPOLACION_UV) {
                state.geometry = (state.geometry == GEOM_ESFERA) ? GEOM_CILINDRO : GEOM_ESFERA;
            } else if (state.demoMode == DEMO_PROYECCIONES) {
                if (state.geometry == GEOM_PLANO) state.geometry = GEOM_ESFERA;
                else if (state.geometry == GEOM_ESFERA) state.geometry = GEOM_CILINDRO;
                else if (state.geometry == GEOM_CILINDRO) state.geometry = GEOM_CONO;
                else state.geometry = GEOM_PLANO;
            } else {
                state.geometry = static_cast<GeometryType>((state.geometry + 1) % 7);
            }
            break;
        case 't':
        case 'T':
            if (!state.textureActive) {
                state.textureActive = true;
                state.texture = TEX_GRID;
            } else {
                state.texture = static_cast<TextureType>((state.texture + 1) % 7);
                if (state.texture == TEX_CUSTOM && textureIds[5] == 0) {
                    selectAndLoadCustomTexture();
                    if (textureIds[5] == 0) {
                        state.texture = TEX_NONE;
                        state.textureActive = false;
                    }
                } else if (state.texture == TEX_NONE) {
                    state.textureActive = false;
                }
            }
            break;
        case 'o':
        case 'O':
            state.activeObjIndex = (state.activeObjIndex + 1) % 4;
            break;
        case 'u':
        case 'U':
            selectAndLoadCustomTexture();
            break;
        case 'w':
        case 'W':
            state.wrapMode = static_cast<WrapMode>((state.wrapMode + 1) % 3);
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
        
        // Atajos para Modos Didacticos
        case '0':
            state.demoMode = DEMO_LIBRE;
            break;
        case '1':
            state.demoMode = DEMO_INTERPOLACION_UV;
            state.geometry = GEOM_ESFERA;
            state.texture = TEX_GRID;
            state.textureActive = true;
            state.repeatX = 1.0f;
            state.repeatY = 1.0f;
            state.wireframe = false;
            cameraDistance = 3.5f;
            break;
        case '2':
            state.demoMode = DEMO_FILTRADO;
            state.geometry = GEOM_PLANO;
            state.texture = TEX_CHECKER;
            state.textureActive = true;
            state.repeatX = 1.0f;
            state.repeatY = 1.0f;
            state.wireframe = false;
            cameraDistance = 3.2f;
            cameraAngleX = 0.0f;
            cameraAngleY = 0.4f;
            break;
        case '3':
            state.demoMode = DEMO_MIPMAPPING;
            state.geometry = GEOM_PLANO;
            state.texture = TEX_GRID;
            state.textureActive = true;
            state.repeatX = 10.0f;
            state.repeatY = 10.0f;
            state.wireframe = false;
            cameraDistance = 9.0f;
            cameraAngleX = 0.0f;
            cameraAngleY = 0.2f;
            break;
        case '4':
            state.demoMode = DEMO_WRAP;
            state.geometry = GEOM_PLANO;
            state.texture = TEX_GRID;
            state.textureActive = true;
            state.repeatX = 4.0f;
            state.repeatY = 4.0f;
            state.wireframe = false;
            cameraDistance = 5.0f;
            cameraAngleX = 0.0f;
            cameraAngleY = 0.5f;
            break;
        case '5':
            state.demoMode = DEMO_PROYECCIONES;
            state.geometry = GEOM_PLANO;
            state.texture = TEX_GRID;
            state.textureActive = true;
            state.repeatX = 1.0f;
            state.repeatY = 1.0f;
            state.wireframe = false;
            cameraDistance = 4.0f;
            break;
        case '6':
            state.demoMode = DEMO_OBJ_UV;
            state.geometry = GEOM_OBJ;
            state.texture = TEX_GRID;
            state.textureActive = true;
            state.wireframe = false;
            cameraDistance = 4.0f;
            for (int i = 0; i < 4; i++) {
                if (!state.objMeshes[i].loaded) {
                    loadOBJ(objPaths[i], state.objMeshes[i]);
                }
            }
            break;
    }
    glutPostRedisplay();
}

// Clics del Raton
void mouse(int button, int state_mouse, int x, int y) {
    if (state_mouse == GLUT_DOWN) {
        activeMouseButton = button;
        lastMouseX = x;
        lastMouseY = y;
    } else if (state_mouse == GLUT_UP) {
        if (activeMouseButton == button) {
            activeMouseButton = -1;
        }
    }
}

// Movimiento del Raton (Arrastrar)
void motion(int x, int y) {
    if (activeMouseButton == GLUT_LEFT_BUTTON) {
        float dx = (x - lastMouseX) * 0.012f;
        float dy = (y - lastMouseY) * 0.012f;
        cameraAngleX -= dx; // Direccion natural de arrastre
        cameraAngleY -= dy; // Direccion natural de arrastre

        if (cameraAngleY > 1.4f) cameraAngleY = 1.4f;
        if (cameraAngleY < -1.4f) cameraAngleY = -1.4f;

        lastMouseX = x;
        lastMouseY = y;
    } else if (activeMouseButton == GLUT_RIGHT_BUTTON) {
        // Zoom con clic derecho
        float speed = cameraDistance * 0.01f;
        if (speed < 0.01f) speed = 0.01f;
        float dy = (y - lastMouseY) * speed;
        cameraDistance += dy;
        if (cameraDistance < 0.35f) cameraDistance = 0.35f;
        if (cameraDistance > 60.0f) cameraDistance = 60.0f;

        lastMouseX = x;
        lastMouseY = y;
    }
    glutPostRedisplay();
}

// Rueda del raton para Zoom (FreeGLUT)
void mouseWheel(int wheel, int direction, int x, int y) {
    float step = cameraDistance * 0.1f;
    if (step < 0.1f) step = 0.1f;
    if (direction > 0) {
        cameraDistance -= step;
    } else {
        cameraDistance += step;
    }
    if (cameraDistance < 0.35f) cameraDistance = 0.35f;
    if (cameraDistance > 60.0f) cameraDistance = 60.0f;
    glutPostRedisplay();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(windowWidth, windowHeight);
    glutCreateWindow("Laboratorio Interactivo de Mapeo de Texturas 3D");

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_LIGHTING);
    glShadeModel(GL_SMOOTH);
    
    // Habilitar normalizacion automatica de normales al escalar modelos
    glEnable(GL_NORMALIZE);

    // Cargar e inicializar texturas
    initTextures();

    // Intentar precargar los modelos OBJ predeterminados
    for (int i = 0; i < 4; i++) {
        loadOBJ(objPaths[i], state.objMeshes[i]);
    }

    // Registrar Callbacks
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutMouseWheelFunc(mouseWheel);

    glutMainLoop();
    return 0;
}
