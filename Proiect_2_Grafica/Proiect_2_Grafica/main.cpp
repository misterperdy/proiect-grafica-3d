// ================================================
// | Main.cpp - Scena de baza: Ground + Camera FPS|
// ================================================

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/freeglut.h>

#include "loadShaders.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

// INCLUDEM CAMERA (Asigura-te ca ai creat Camera.h)
#include "Camera.h"

using namespace std;

// --- VARIABILE GLOBALE ---

// Identificatori OpenGL
GLuint
VaoId,
VboId,
EboId,
ProgramId,
viewLocation,
projLocation,
lightColorLoc,
lightPosLoc,
viewPosLoc,
codColLocation,
fogColorLoc,
fogDensityLoc,
modelLocation,
matrUmbraLocation;

glm::mat4 matrUmbra;

// --- CAMERA SETTINGS ---
// Pozitionam camera la inaltime (Y=10) si putin in spate (Z=50)
Camera myCamera(glm::vec3(0.0f, 10.0f, 50.0f));

float width = 1200, height = 900;
float znear = 0.1f;

// Timing (pentru miscare lina)
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Input Mouse
float lastX = 600, lastY = 450; // Centrul ecranului
bool firstMouse = true;

// Input Tastatura (Vector de stari)
bool keys[1024];

// Iluminare & Ceata
// Atentie: Lumina e acum sus pe Y (200.0f)
glm::vec3 lightPos = glm::vec3(100.0f, 200.0f, 100.0f);
glm::vec3 fogColor = glm::vec3(0.5f, 0.7f, 1.0f); // Un albastru deschis (cer de zi)

// Matrice
glm::mat4 view, projection;

// --- FUNCTII CALLBACK INPUT ---

// Apasare tasta
void processNormalKeys(unsigned char key, int x, int y)
{
    if (key == 27) exit(0); // ESC iesire
    // Actualizam starea tastei in vector
    if (key >= 0 && key < 1024) keys[key] = true;
}

// Ridicare tasta
void processNormalKeysUp(unsigned char key, int x, int y)
{
    if (key >= 0 && key < 1024) keys[key] = false;
}

// Miscare Mouse stil joc fps
void processMouseMovement(int x, int y)
{
    // Centrul ferestrei
    int centerX = (int)width / 2;
    int centerY = (int)height / 2;

    if (firstMouse)
    {
        // La prima detectare, nu rotim camera.
        // Doar teleportam cursorul la centru si marcam ca am terminat initializarea.
        glutWarpPointer(centerX, centerY);
        firstMouse = false;
        return; // Iesim din functie fara sa modificam unghiurile
    }

    // Daca mouse-ul e deja in centru (teleportat de noi), ignoram
    if (x == centerX && y == centerY) return;

    // Calculam cat s-a miscat
    float xoffset = (float)(x - centerX);
    float yoffset = (float)(centerY - y);

    // Aplicam miscarea camerei
    myCamera.ProcessMouseMovement(xoffset, yoffset);

    // Teleportam cursorul inapoi in centru
    glutWarpPointer(centerX, centerY);
}

// Logica de miscare continua (apelata in RenderFunction)
void DoMovement()
{
    if (keys['w'] || keys['W']) myCamera.ProcessKeyboard(FORWARD, deltaTime);
    if (keys['s'] || keys['S']) myCamera.ProcessKeyboard(BACKWARD, deltaTime);
    if (keys['a'] || keys['A']) myCamera.ProcessKeyboard(LEFT, deltaTime);
    if (keys['d'] || keys['D']) myCamera.ProcessKeyboard(RIGHT, deltaTime);
}

// --- SETUP GEOMETRIE (DOAR SOLUL) ---
void CreateVBO(void)
{
    // Definim un plan mare pe XZ (Y=0)
    // Coordonate (X, Y, Z, W) | Culoare (R, G, B, A) | Normala (X, Y, Z) - Sus pe Y
    GLfloat Vertices[] = {
        // Stanga-Spate
        -100.0f, 0.0f, -100.0f, 1.0f,   0.2f, 0.6f, 0.2f, 1.0f,   0.0f, 1.0f, 0.0f,
        // Dreapta-Spate
         100.0f, 0.0f, -100.0f, 1.0f,   0.2f, 0.6f, 0.2f, 1.0f,   0.0f, 1.0f, 0.0f,
         // Dreapta-Fata
          100.0f, 0.0f,  100.0f, 1.0f,   0.2f, 0.6f, 0.2f, 1.0f,   0.0f, 1.0f, 0.0f,
          // Stanga-Fata
          -100.0f, 0.0f,  100.0f, 1.0f,   0.2f, 0.6f, 0.2f, 1.0f,   0.0f, 1.0f, 0.0f,
    };

    // Indici (2 triunghiuri ce formeaza patratul)
    GLuint Indices[] = {
        0, 1, 2,
        0, 2, 3
    };

    // Generare Buffere
    glGenVertexArrays(1, &VaoId);
    glBindVertexArray(VaoId);

    glGenBuffers(1, &VboId);
    glBindBuffer(GL_ARRAY_BUFFER, VboId);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices), Vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &EboId);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EboId);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Indices), Indices, GL_STATIC_DRAW);

    // Atribute (Layout-ul din Shader)
    // 0: Pozitie (vec4)
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    // 1: Culoare (vec4)
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(4 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    // 2: Normala (vec3)
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(8 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);
}

void Cleanup(void)
{
    glDeleteBuffers(1, &VboId);
    glDeleteBuffers(1, &EboId);
    glDeleteVertexArrays(1, &VaoId);
    glDeleteProgram(ProgramId);
}

void CreateShaders(void)
{
    ProgramId = LoadShaders("11_02_Shader.vert", "11_02_Shader.frag");
    glUseProgram(ProgramId);
}

void Initialize(void)
{
    // Fundalul are culoarea cetii (albastru deschis de zi)
    glClearColor(fogColor.r, fogColor.g, fogColor.b, 1.0f);

    CreateShaders();
    CreateVBO();

    // Locatii Uniforme
    viewLocation = glGetUniformLocation(ProgramId, "view");
    projLocation = glGetUniformLocation(ProgramId, "projection");
    modelLocation = glGetUniformLocation(ProgramId, "model");
    matrUmbraLocation = glGetUniformLocation(ProgramId, "matrUmbra");

    lightColorLoc = glGetUniformLocation(ProgramId, "lightColor");
    lightPosLoc = glGetUniformLocation(ProgramId, "lightPos");
    viewPosLoc = glGetUniformLocation(ProgramId, "viewPos");

    codColLocation = glGetUniformLocation(ProgramId, "codCol");
    fogColorLoc = glGetUniformLocation(ProgramId, "fogColor");
    fogDensityLoc = glGetUniformLocation(ProgramId, "fogDensity");

    // matrUmbraLocation = ... (Momentan nu avem umbre, o lasam neinitializata sau comentata)

    // Ascundem cursorul si il punem in centru
    glutSetCursor(GLUT_CURSOR_NONE);
    glutWarpPointer((int)width / 2, (int)height / 2);
}

void RenderFunction(void)
{
    // 1. Calcul Delta Time
    float currentFrame = glutGet(GLUT_ELAPSED_TIME) * 0.001f;
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    // 2. Procesare Input (Miscare)
    DoMovement();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    // 3. Setare Matrici (View & Projection)
    view = myCamera.GetViewMatrix();
    glUniformMatrix4fv(viewLocation, 1, GL_FALSE, &view[0][0]);

    projection = glm::infinitePerspective(myCamera.Zoom, width / height, znear);
    glUniformMatrix4fv(projLocation, 1, GL_FALSE, &projection[0][0]);

    // 4. Setare Iluminare si Ceata
    glUniform3f(lightColorLoc, 1.0f, 1.0f, 1.0f); // Lumina Alba
    glUniform3f(lightPosLoc, lightPos.x, lightPos.y, lightPos.z);
    glUniform3f(viewPosLoc, myCamera.Position.x, myCamera.Position.y, myCamera.Position.z);

    glUniform3f(fogColorLoc, fogColor.r, fogColor.g, fogColor.b);
    glUniform1f(fogDensityLoc, 0.005f); // Ceata densa la orizont

    // --- CALCUL MATRICE UMBRA (Pentru Y-Up) ---
    float D = -lightPos.y; // D = -Inaltimea Luminii (pentru planul Y=0)

    // Resetam matricea la 0
    matrUmbra = glm::mat4(0.0f);

    // Formula matematica pentru proiectie pe planul Y=0
    // Aceasta pastreaza X si Z, dar "striveste" Y-ul in functie de pozitia luminii

    matrUmbra[0][0] = lightPos.y + D;
    matrUmbra[0][1] = -lightPos.x;
    // [0][2] si [0][3] sunt 0

    // Linia 1 (Axa Y) - Aici se intampla "strivirea"
    // matrUmbra[1][...] ramane 0 pentru ca Y devine 0 pe plan

    matrUmbra[2][1] = -lightPos.z;
    matrUmbra[2][2] = lightPos.y + D;

    matrUmbra[3][1] = D;
    matrUmbra[3][3] = lightPos.y;

    // Trimitem matricea la shader
    glUniformMatrix4fv(matrUmbraLocation, 1, GL_FALSE, &matrUmbra[0][0]);

    // 5. Desenare SOL (Ground)
    // Matricea model este Identity (Solul sta pe loc la 0,0,0)
    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));

    glUniform1i(codColLocation, 0); // Mod normal de desenare (nu umbra)

    glBindVertexArray(VaoId);
    // Avem 6 indici (2 triunghiuri)
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    glutSwapBuffers();
    glFlush();
}

int main(int argc, char* argv[])
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
    glutInitWindowPosition(100, 100);
    glutInitWindowSize((int)width, (int)height);
    glutCreateWindow("Proiect - Camera FPS & Ground");

    glewInit();
    Initialize();

    glutDisplayFunc(RenderFunction);
    glutIdleFunc(RenderFunction); // Randare continua

    // Callback-uri
    glutKeyboardFunc(processNormalKeys);
    glutKeyboardUpFunc(processNormalKeysUp);
    glutPassiveMotionFunc(processMouseMovement); // Miscare mouse fara click

    glutCloseFunc(Cleanup);
    glutMainLoop();

    return 0;
}