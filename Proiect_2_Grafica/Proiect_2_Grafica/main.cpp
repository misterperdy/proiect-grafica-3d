// ================================================
// | Main.cpp - Scena de baza: Ground + Camera FPS|
// ================================================

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <vector>
#include "stb_image.h"

#include "loadShaders.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

// INCLUDEM CAMERA (Asigura-te ca ai creat Camera.h)
#include "Camera.h"
#include "SceneObjects.h"

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
fogStartLoc,
fogEndLoc,
modelLocation,
matrUmbraLocation,
overrideColorLoc,
diffuseMap,
normalMap;

// --- CAMERA SETTINGS ---
// Pozitionam camera la inaltime (Y=10) si putin in spate (Z=50)
Camera myCamera(glm::vec3(0.0f, 10.0f, 50.0f));

float width = 1200, height = 675;
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
// Lumina e sus pe Y (200.0f)
glm::vec3 lightPos = glm::vec3(100.0f, 200.0f, 100.0f);
glm::vec3 fogColor = glm::vec3(0.25f, 0.6f, 0.9f);

glm::mat4 matrUmbra;

// Matrice
glm::mat4 view, projection;

//instantare obiectele noastre
Tree tree;
Skybox sky;
Cloud cloud;
Sun sun;
Crosshair crosshair;

// --- DEFINITIE NORI ---
struct CloudData {
    glm::vec3 pos;
    glm::vec3 scale;
};

// Lista care va tine toti norii nostri
std::vector<CloudData> cloudsVector;

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

    // --- DEADZONE FIX ---
    // Ignoram miscarile foarte mici (tremuratul mainii sau erori de driver)
    float deadzone = 2.0f; // Poti experimenta cu 1.0f sau 3.0f

    if (abs(xoffset) < deadzone) xoffset = 0.0f;
    if (abs(yoffset) < deadzone) yoffset = 0.0f;

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

    // aplicam boundaries
    float borderLimit = 100.0f; // Zona jucabila (+/- 100)

    // Verificam X
    if (myCamera.Position.x > borderLimit)
        myCamera.Position.x = borderLimit;
    if (myCamera.Position.x < -borderLimit)
        myCamera.Position.x = -borderLimit;

    // Verificam Z
    if (myCamera.Position.z > borderLimit)
        myCamera.Position.z = borderLimit;
    if (myCamera.Position.z < -borderLimit)
        myCamera.Position.z = -borderLimit;
}

float RandomFloat(float min, float max) {
    return min + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (max - min)));
}

void GenerateClouds() {
    cloudsVector.clear();

    //numarul de nori care vor fi generati
    int numClouds = 40;

    for (int i = 0; i < numClouds; i++) {
        CloudData c;

        // POZITIE (X si Z pe o arie mare, Y sus in cer)
        // Imprastiem norii pe o suprafata de 800x800 (-400 la +400)
        float x = RandomFloat(-400.0f, 400.0f);
        float z = RandomFloat(-400.0f, 400.0f);

        // Inaltimea variaza putin (intre 100 si 180) ca sa nu fie toti la linie
        float y = RandomFloat(100.0f, 180.0f);

        // Evitam zona din mijloc (unde e jucatorul), ca sa nu ai un nor in cap cand pornesti
        if (abs(x) < 50.0f && abs(z) < 50.0f) continue;

        c.pos = glm::vec3(x, y, z);

        // 2. SCALARE -Lati si Plati
        // Latime mare (X), Inaltime mica (Y), Adancime medie (Z)
        float scaleX = RandomFloat(20.0f, 45.0f);
        float scaleY = RandomFloat(4.0f, 8.0f);  // Plati
        float scaleZ = RandomFloat(15.0f, 30.0f);

        c.scale = glm::vec3(scaleX, scaleY, scaleZ);

        cloudsVector.push_back(c);
    }
}

// --- SETUP GEOMETRIE sol ---
void CreateVBO(void)
{
    float size = 1000.0f; // Marimea solului
    float uvRepeat = 50.0f; // De cate ori se repeta textura

    // Format vertex nou:
    // X, Y, Z (Pos) | NX, NY, NZ (Normal) | U, V (Tex) | TX, TY, TZ (Tangent)

    GLfloat Vertices[] = {
        // Pozitie (Y=0)        // Normala (Sus)    // UV (Colturi)      // Tangenta (Dreapta)
        // Stanga-Jos (-X, -Z)
        -size, 0.0f,  size,     0.0f, 1.0f, 0.0f,   0.0f, 0.0f,          1.0f, 0.0f, 0.0f,
        // Dreapta-Jos (+X, -Z)
         size, 0.0f,  size,     0.0f, 1.0f, 0.0f,   uvRepeat, 0.0f,      1.0f, 0.0f, 0.0f,
         // Dreapta-Sus (+X, +Z)
          size, 0.0f, -size,     0.0f, 1.0f, 0.0f,   uvRepeat, uvRepeat,  1.0f, 0.0f, 0.0f,
          // Stanga-Sus (-X, +Z)
          -size, 0.0f, -size,     0.0f, 1.0f, 0.0f,   0.0f, uvRepeat,      1.0f, 0.0f, 0.0f,
    };

    GLuint Indices[] = {
        0, 1, 2,
        0, 2, 3
    };

    glGenVertexArrays(1, &VaoId); glBindVertexArray(VaoId);
    glGenBuffers(1, &VboId); glBindBuffer(GL_ARRAY_BUFFER, VboId);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices), Vertices, GL_STATIC_DRAW);
    glGenBuffers(1, &EboId); glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EboId);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Indices), Indices, GL_STATIC_DRAW);

    // TOTAL STRIDE = 3+3+2+3 = 11 floats
    int stride = 11 * sizeof(GLfloat);

    // Atribut 0: Pozitie (3 floats)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);

    // Atribut 1: Culoare (DEZACTIVAT/SCOS) 
    // OBS: Am scos culoarea vertexului pt ca folosim textura.
    // Dar ca sa nu stricam ordinea in shader (unde 1 era Color), putem sa sarim peste 1 sau sa punem altceva.
    // Hai sa punem Normalele pe locatia 1 momentan, dar va trebui sa schimbam in Shader layout-ul!
    // PLAN:
    // Loc 0 = Pos
    // Loc 1 = Normal
    // Loc 2 = TexCoord
    // Loc 3 = Tangent

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(GLfloat))); // Normal
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(GLfloat))); // UV
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, (void*)(8 * sizeof(GLfloat))); // Tangent
    glEnableVertexAttribArray(3);
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

GLuint LoadTexture(const char* path)
{
    GLuint textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    // stbi_load incarca imaginea si ne da pixelii
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);

    if (data)
    {
        GLenum format;
        if (nrComponents == 1) format = GL_RED;
        else if (nrComponents == 3) format = GL_RGB; // jpg are de obicei 3
        else format = GL_RGBA; // png are de obicei 4

        glBindTexture(GL_TEXTURE_2D, textureID);

        // Trimitem pixelii la placa video
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D); // Genereaza versiuni mai mici pt distanta

        // Setari de Tiling (Repetare)
        // GL_REPEAT e critic pt iarba, ca sa nu se intinda o singura poza pe 1000m
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        // Setari de Filtrare (Sa nu se vada pixelat cand e aproape/departe)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data); // Eliberam memoria RAM, acum e in VRAM
    }
    else
    {
        printf("Texture failed to load at path: %s\n", path);
        stbi_image_free(data);
    }

    return textureID;
}

void Initialize(void)
{
    // Fundalul are culoarea cetii (albastru deschis de zi)
    glClearColor(fogColor.r, fogColor.g, fogColor.b, 1.0f);

    CreateShaders();
    CreateVBO();

    stbi_set_flip_vertically_on_load(true);

    diffuseMap = LoadTexture("C:\\Users\\Azlo\\source\\repos\\Proiect_2_Grafica\\Proiect_2_Grafica\\grass_diff.jpg");
    normalMap = LoadTexture("C:\\Users\\Azlo\\source\\repos\\Proiect_2_Grafica\\Proiect_2_Grafica\\grass_norm.jpg");

    //initializam o singura data obiectele noastre si trimitem datele la GPU
    tree.Init();
    sky.Init();
    cloud.Init();
    sun.Init();
    crosshair.Init();

    GenerateClouds();

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
    fogStartLoc = glGetUniformLocation(ProgramId, "fogStart");
    fogEndLoc = glGetUniformLocation(ProgramId, "fogEnd");

    overrideColorLoc = glGetUniformLocation(ProgramId, "overrideColor");

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

    projection = glm::perspective(glm::radians(myCamera.Zoom), width / height, 0.1f, 2000.0f);
    glUniformMatrix4fv(projLocation, 1, GL_FALSE, &projection[0][0]);

    // 4. Setare Iluminare si Ceata
    glUniform3f(lightColorLoc, 1.0f, 1.0f, 1.0f); // Lumina Alba
    glUniform3f(lightPosLoc, lightPos.x, lightPos.y, lightPos.z);
    glUniform3f(viewPosLoc, myCamera.Position.x, myCamera.Position.y, myCamera.Position.z);

    //ceata
    glUniform3f(fogColorLoc, fogColor.r, fogColor.g, fogColor.b);

    float startDist = 150.0f;
    float endDist = 250.0f;

    glUniform1f(fogStartLoc, startDist);
    glUniform1f(fogEndLoc, endDist);

    // =========================================================
    //                DESENARE CER & NORI
    // =========================================================

    // Setam modul 2: Fara Lumina, Fara Umbra (Culori pure)
    glUniform1i(codColLocation, 2);

    // SKYBOX
    // Oprim testul de adancime ca cerul sa fie desenat "in spatele" tuturor
    glDisable(GL_DEPTH_TEST);
    sky.Render(modelLocation, myCamera.Position);
    glEnable(GL_DEPTH_TEST);  // Il pornim inapoi pentru restul lumii

    // =========================================================
    //                DESENARE SOARE 
    // =========================================================

    // Setam codul 4 (Galben/Soare)
    glUniform1i(codColLocation, 4);

    // Oprim Depth Test (Sa straluceasca peste cer)
    glDisable(GL_DEPTH_TEST);

    // Desenam. 
    // Parametrul nou 'view' este critic pentru a roti soarele spre noi.
    // Scale 15.0f este suficient pentru un billboard.
    sun.Render(modelLocation, lightPos, glm::vec3(15.0f), view);

    glEnable(GL_DEPTH_TEST);


    //NORI
    // 
    glUniform1i(codColLocation, 2);

    // Bucla care deseneaza toti norii generati
    for (auto& c : cloudsVector)
    {
        // ANIMATIE SIMPLA (Vant)
        // Facem norii sa se miste incet pe axa X
        // currentFrame e timpul total. Impartim la o valoare mare sa fie lenti.
        float windOffset = glutGet(GLUT_ELAPSED_TIME) * 0.005f;

        // Calculam o pozitie temporara doar pentru desenare
        glm::vec3 movingPos = c.pos;
        movingPos.x += windOffset;

        // TRUC: Daca norul iese din harta (ex: > 400), il mutam inapoi la inceput (-400)
        // Asta creeaza o bucla infinita de nori
        if (movingPos.x > 400.0f) {
            // Scadem 800 ca sa il ducem in partea opusa
            while (movingPos.x > 400.0f) movingPos.x -= 800.0f;
        }

        // Desenam norul
        cloud.Render(modelLocation, movingPos, c.scale);
    }

    // --- CALCUL MATRICE UMBRA---
    float ly = lightPos.y;
    float lx = lightPos.x;
    float lz = lightPos.z;

    // Resetam matricea
    matrUmbra = glm::mat4(0.0f);

    // Definirea matricei de proiectie pe planul Y=0
    // Aceasta matrice proiecteaza punctele astfel incat Y devine 0,
    // pastrand perspectiva corecta fata de sursa de lumina.

    // Coloana 0
    matrUmbra[0][0] = ly;
    matrUmbra[0][1] = 0;
    matrUmbra[0][2] = 0;
    matrUmbra[0][3] = 0;

    // Coloana 1 (Aici se intampla proiectia pe baza lui Y)
    matrUmbra[1][0] = -lx;
    matrUmbra[1][1] = 0;
    matrUmbra[1][2] = -lz;
    matrUmbra[1][3] = -1; // -1 este critic pentru perspectiva (W division)

    // Coloana 2
    matrUmbra[2][0] = 0;
    matrUmbra[2][1] = 0;
    matrUmbra[2][2] = ly;
    matrUmbra[2][3] = 0;

    // Coloana 3
    matrUmbra[3][0] = 0;
    matrUmbra[3][1] = 0;
    matrUmbra[3][2] = 0;
    matrUmbra[3][3] = ly;

    glUniformMatrix4fv(matrUmbraLocation, 1, GL_FALSE, &matrUmbra[0][0]);

    // =========================================================
    //                DESENARE SOL (TEXTURED)
    // =========================================================

    // 1. Setam codul 5 (Iarba cu Texturi)
    glUniform1i(codColLocation, 5);

    // 2. Activam Textura 0 (Diffuse)
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, diffuseMap);
    glUniform1i(glGetUniformLocation(ProgramId, "diffuseMap"), 0);

    // 3. Activam Textura 1 (Normal)
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normalMap);
    glUniform1i(glGetUniformLocation(ProgramId, "normalMap"), 1);

    // 4. Desenam
    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
    glBindVertexArray(VaoId);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    // --- DESENARE COPACI + UMBRE ---

    // Definim pozitiile copacilor (poti sa le muti si global daca vrei)
    struct TreePos { glm::vec3 pos; glm::vec3 scale; };
    std::vector<TreePos> padure = {
        { glm::vec3(50.0f, 0.0f, -50.0f),  glm::vec3(1.0f) },
        { glm::vec3(-80.0f, 0.0f, -30.0f), glm::vec3(1.5f) }, // Unul mai mare
        { glm::vec3(20.0f, 0.0f, 100.0f),  glm::vec3(0.8f) },
        { glm::vec3(-40.0f, 0.0f, 150.0f), glm::vec3(1.2f) }
    };

    // Desenam toti copacii NORMALI (Solizi)
    glUniform1i(codColLocation, 0); // Spunem shaderului: "Fa-i colorati"

    for (auto& treePos : padure) {
        tree.Render(modelLocation, treePos.pos, treePos.scale);
    }

    // Desenam UMBRELE tuturor copacilor
    glUniform1i(codColLocation, 1); // Setam Shaderul pe modul UMBRA

    // STENCIL BUFFER
    // Ideea: Creem o masca. Unde am desenat deja o umbra, marcam cu 1.
    // Nu mai desenam a doua oara acolo.
    glEnable(GL_STENCIL_TEST);

    // Curatam stencilul (totul devine 0)
    glClear(GL_STENCIL_BUFFER_BIT);

    // Regula: Toti pixelii trec testul initial, dar marcam pixelul cu '1' in stencil buffer
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    // BLENDING (Rezolva Transparenta)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // 3. POLYGON OFFSET (Rezolva "Jitter-ul/Palpairea")
    // Trage geometria spre camera suficient cat sa nu se bata cu solul
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1.0f, -1.0f); // Valori negative = trage spre noi

    for (auto& treePos : padure) {
        // Trimitem ACELEASI pozitii,shaderul va aplica matrUmbra peste ele.
        tree.Render(modelLocation, treePos.pos, treePos.scale);
    }

    glDisable(GL_POLYGON_OFFSET_FILL); // Oprim trucul
    glDisable(GL_BLEND); // oprimi si transaprentea

    // =========================================================
    //                DESENARE CROSSHAIR (UI FIX)
    // =========================================================

    // 1. Oprim Depth Test (Sa fie desenat PESTE orice altceva)
    glDisable(GL_DEPTH_TEST);


    glUniform1i(codColLocation, 2); // Unlit (ia culoarea din buffer, care e alba)


    // 3. RESETAM MATRICILE (CRITIC!)
    // Anulam perspectiva si rotatia camerei. 
    glm::mat4 identity = glm::mat4(1.0f);

    // Trimitem Identity la Proiectie si la View
    glUniformMatrix4fv(projLocation, 1, GL_FALSE, &identity[0][0]);
    glUniformMatrix4fv(viewLocation, 1, GL_FALSE, &identity[0][0]);
    // Model e trimis deja in Crosshair::Render ca Identity

    // 4. Desenam
    crosshair.Render(modelLocation);

    // 5. Curatenie (Reactivam Depth pentru urmatorul cadru)
    glEnable(GL_DEPTH_TEST);

    glutSwapBuffers();
    glFlush();
}

int main(int argc, char* argv[])
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
    glutInitWindowPosition(100, 100);
    glutInitWindowSize((int)width, (int)height);
    glutCreateWindow("Proiect - campie cu copaci");

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