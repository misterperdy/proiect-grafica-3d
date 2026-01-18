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
#include "Camera.h"
#include "SceneObjects.h"

using namespace std;

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
normalMap,
fireFrameTex[128];

static GLuint campfireTexWoods = 0;
static GLuint campfireTexRocks = 0;
static GLuint campfireTexBase  = 0;
static GLuint seatLogTex       = 0;

// Fire flipbook
static int fireFrameCount = 0;
static const float fireFps = 24.0f;

// Reglaje fine pentru FOC si AURA LUMINOSA
static const float FIRE_SCALE_XZ = 0.95f;
static const float FIRE_SCALE_Y = 1.55f;
// Intensitate crescuta (vizibilitate mai buna) pentru lumina focului
static const float FIRE_LIGHT_INTENSITY_SCALE = 0.45f;
static const glm::vec3 FIRE_LIGHT_COLOR_SCALE = glm::vec3(2.5f, 1.0f, 0.4f);

// Aliniere foc + reglaje aura
static const glm::vec3 CAMPFIRE_SCENE_POS(15.0f, 0.0f, 25.0f); // XZ fix; Y este calculat din inaltimea terenului
static const float FIRE_PIVOT_Y_OFFSET = -3.0f;              // coboara flacara mai mult in busteni
static const float FIRE_AURA_Y_OFFSET = 2.15f;               // pozitia aurei fata de foc
static const float FIRE_AURA_SCALE = 40.0f;                  // dimensiunea aurei (scalare)

// Offset-uri fine pentru aliniere (in world space, relativ la centrul focului)
static const float FIRE_PIVOT_X_OFFSET = -1.0f;              // muta flacara spre stanga
static const float FIRE_PIVOT_Z_OFFSET = -0.8f;              // muta flacara in fata/in spate

// Derivat din bounds ale `campfireModel`; calculat dupa incarcare
static glm::vec3 gCampfireModelCenter = glm::vec3(0.0f);
static bool gHasCampfireModelCenter = false;

// --- CAMERA SETTINGS ---
// Camera FPS customizata
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

// --- SISTEM DE INTERACTIUNE (Sit Logic) ---
bool g_isSitting = false;        // Stare: asezat sau nu
bool g_canInteract = false;      // Stare: este zona valida de interactiune?
glm::vec3 g_potentialSitPos;     // Pozitia tinta unde se va aseza camera
glm::vec3 g_restorePos;          // Pozitia salvata de unde s-a ridicat

// --- TRANZITIE CAMERA  ---
struct CameraTransition {
    bool active = false;
    float timeElapsed = 0.0f;
    float duration = 1.2f; // Secunde
    glm::vec3 startPos;
    glm::vec3 endPos;
    glm::vec3 fixedFront = glm::vec3(0.0f, 0.0f, -1.0f);
};
CameraTransition g_transition;

void RenderText(const char* text, float x, float y, float r, float g, float b)
{
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glUseProgram(0); 

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, width, 0, height);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glColor3f(r, g, b);
    glRasterPos2f(x, y);
    for (const char* c = text; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    
    // Revenim la setarile normale pentru restul scenei
    glEnable(GL_DEPTH_TEST);
    glUseProgram(ProgramId);
}

enum TimeOfDay {
    MIDDAY = 0, // Zi
    SUNSET = 1, // Apus (Lumina portocalie, umbre lungi)
    NIGHT = 2   // Noapte (Lumina albastra, licurici, stele)
};

TimeOfDay currentTimeOfDay = MIDDAY;

// Setarile de mediu (calculate in SetTimeOfDay)
// Lumina e sus pe Y (200.0f)
glm::vec3 lightPos = glm::vec3(100.0f, 200.0f, 100.0f);
glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
glm::vec3 fogColor = glm::vec3(0.25f, 0.6f, 0.9f);
glm::vec3 skyColorTop = glm::vec3(0.2f, 0.5f, 0.9f);
glm::vec3 skyColorBottom = glm::vec3(0.7f, 0.85f, 1.0f);

glm::mat4 matrUmbra;


glm::mat4 view, projection;


Tree tree;
Skybox sky;
Cloud cloud;
Sun sun;
Moon moon;
Stars stars;
Crosshair crosshair;
Model3D treeModel;  
Campfire campfire;
Model3D campfireModel;
Model3D firePlaneModel;
Model3D seatLogModel;
Fireflies fireflies;

static GLuint ShadowFbo = 0;      // Framebuffer Object pentru umbra
static GLuint ShadowDepthTex = 0; // Textura in care salvam adancimea
static GLuint ShadowProgramId = 0; // Shader separat, simplu, doar pentru adancime
static GLuint shadowLightSpaceLoc = 0;
static GLuint shadowModelLoc = 0;
static GLuint lightSpaceMatrixLocMain = 0;

static GLuint fireflyPosLoc = 0;         
static GLuint numFirefliesActiveLoc = 0; 

static const unsigned int SHADOW_MAP_SIZE = 2048;
static glm::mat4 lightSpaceMatrix;

// --- TEREN PROCEDURAL (Hills) ---
static GLuint groundVao = 0, groundVbo = 0, groundEbo = 0;
static int groundIndexCount = 0;

// Functie matematica pentru relieful solului
static float GroundHeight(float x, float z)
{
    return 2.0f * sinf(x * 0.03f) + 1.5f * cosf(z * 0.025f) + 0.8f * sinf((x + z) * 0.02f);
}

static glm::vec3 GroundNormal(float x, float z)
{
    // Diferente finite pentru a aproxima normala
    const float eps = 0.5f;
    float hL = GroundHeight(x - eps, z);
    float hR = GroundHeight(x + eps, z);
    float hD = GroundHeight(x, z - eps);
    float hU = GroundHeight(x, z + eps);

    glm::vec3 n = glm::normalize(glm::vec3(hL - hR, 2.0f * eps, hD - hU));
    return n;
}

// Cream un grid mare de varfuri si indici pentru teren
// Procedural (la start), fara modele 3D externe
static void CreateGroundMesh()
{
    if (groundVao != 0) return;

    const int gridSize = 200;          
    const float halfSize = 500.0f;     
    const float step = (halfSize * 2.0f) / gridSize;

    std::vector<GLfloat> verts;
    std::vector<GLuint> inds;
    verts.reserve((gridSize + 1) * (gridSize + 1) * 11);

    for (int z = 0; z <= gridSize; z++)
    {
        for (int x = 0; x <= gridSize; x++)
        {
            float wx = -halfSize + x * step;
            float wz = -halfSize + z * step;
            float wy = GroundHeight(wx, wz);

            glm::vec3 n = GroundNormal(wx, wz);

            float t = (wy + 6.0f) / 12.0f;
            t = glm::clamp(t, 0.0f, 1.0f);
            glm::vec3 col = glm::mix(glm::vec3(0.10f, 0.45f, 0.10f), glm::vec3(0.18f, 0.65f, 0.18f), t);

            verts.insert(verts.end(), {
                wx, wy, wz, 1.0f,
                col.r, col.g, col.b, 1.0f,
                n.x, n.y, n.z
            });
        }
    }

    auto idx = [gridSize](int x, int z) { return (GLuint)(z * (gridSize + 1) + x); };

    for (int z = 0; z < gridSize; z++)
    {
        for (int x = 0; x < gridSize; x++)
        {
            GLuint i0 = idx(x, z);
            GLuint i1 = idx(x + 1, z);
            GLuint i2 = idx(x + 1, z + 1);
            GLuint i3 = idx(x, z + 1);

            inds.insert(inds.end(), { i0, i1, i2, i0, i2, i3 });
        }
    }

    groundIndexCount = (int)inds.size();

    glGenVertexArrays(1, &groundVao);
    glBindVertexArray(groundVao);

    glGenBuffers(1, &groundVbo);
    glBindBuffer(GL_ARRAY_BUFFER, groundVbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(GLfloat), verts.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &groundEbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, groundEbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, inds.size() * sizeof(GLuint), inds.data(), GL_STATIC_DRAW);

    int stride = 11 * sizeof(GLfloat);

    // location 0: vec4 position
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);

    // location 1: vec4 color
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (void*)(4 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    // location 2: vec3 normal
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(8 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

// --- DEFINITIE NORI ---
// Structura pentru a descrie un nor (pozitie, scalare)
struct CloudData {
    glm::vec3 pos;
    glm::vec3 scale;
};

// Lista care va tine toti norii nostri
std::vector<CloudData> cloudsVector;

// --- FORWARD DECLARATION ---
// Functii ajutatoare mai jos in cod (CreateShadowResources, RenderShadowPass)
void SetTimeOfDay(TimeOfDay tod);
static void CreateShadowResources();
static void RenderShadowPass(const std::vector<glm::mat4>& modelMatrices);
void CreateShaders(void);
GLuint LoadTexture(const char* path);

// --- FUNCTII CALLBACK INPUT ---
//
// Apasare tasta
void processNormalKeys(unsigned char key, int x, int y)
{
    if (key == 27) exit(0); // ESC iesire

    // INTERACTION KEYS
    if ((key == 'e' || key == 'E') && g_canInteract && !g_isSitting && !g_transition.active)
    {
        // Ne asezam - Start Transition
        g_isSitting = true;
        g_restorePos = myCamera.Position;

        g_transition.active = true;
        g_transition.timeElapsed = 0.0f;
        g_transition.startPos = myCamera.Position;
        // Pozitie tinta: pe banca, ajustata pntru ochi
        g_transition.endPos = g_potentialSitPos + glm::vec3(0.0f, 2.5f, 0.0f);

        // Varianta simpla: nu rotim camera in tranzitie (pastram directia curenta)
        g_transition.fixedFront = myCamera.Front;
    }

    if (key == ' ' && g_isSitting && !g_transition.active)
    {
        // Ne ridicam - Start Transition
        g_isSitting = false;

        g_transition.active = true;
        g_transition.timeElapsed = 0.0f;
        g_transition.startPos = myCamera.Position;
        g_transition.endPos = g_restorePos + glm::vec3(0.0f, 1.0f, 0.0f);

        // Pastram directia curenta
        g_transition.fixedFront = myCamera.Front;
    }

    if (key == '1') SetTimeOfDay(MIDDAY);
    if (key == '2') SetTimeOfDay(SUNSET);
    if (key == '3') SetTimeOfDay(NIGHT);

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
// Ignoram miscarile foarte mici (tremuratulmainii sau erori de driver)
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
    // Daca stam pe banca (si nu suntem in tranzitie, sau suntem), NU ne miscam WASD
    if (g_isSitting || g_transition.active) return;

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

// Functie pentru a genera un float random intre min si max
float RandomFloat(float min, float max) {
    return min + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (max - min)));
}

// Generam nori random pe o suprafata mare
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

        // Evitam zona din mijloc ( unde e jucatorul), ca sa nu ai un nor in cap cand portiesti
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
// Nu mai folosim vechiul VBO texturat.
// Acum folosim doar CreateGroundMesh()
void CreateVBO(void)
{
    // Nu mai folosim vechiul sol texturat.
    // Folosim mesh procedural generat in `CreateGroundMesh()`.
    return;
}

// Curatare: eliberam memorie (GPU)
void Cleanup(void)
{
    glDeleteBuffers(1, &VboId);
    glDeleteBuffers(1, &EboId);
    glDeleteVertexArrays(1, &VaoId);

    if (groundVbo) glDeleteBuffers(1, &groundVbo);
    if (groundEbo) glDeleteBuffers(1, &groundEbo);
    if (groundVao) glDeleteVertexArrays(1, &groundVao);

    glDeleteProgram(ProgramId);

    if (ShadowDepthTex) glDeleteTextures(1, &ShadowDepthTex);
    if (ShadowFbo) glDeleteFramebuffers(1, &ShadowFbo);
    if (ShadowProgramId) glDeleteProgram(ShadowProgramId);

    for (int i = 0; i < 128; ++i)
    {
        if (fireFrameTex[i]) glDeleteTextures(1, &fireFrameTex[i]);
        fireFrameTex[i] = 0;
    }

    if (campfireTexWoods) glDeleteTextures(1, &campfireTexWoods);
    if (campfireTexRocks) glDeleteTextures(1, &campfireTexRocks);
    if (campfireTexBase)  glDeleteTextures(1, &campfireTexBase);
    campfireTexWoods = campfireTexRocks = campfireTexBase = 0;

    if (seatLogTex) glDeleteTextures(1, &seatLogTex);
    seatLogTex = 0;
}

// --- SHADOW MAPPING FUNCTIONS ---
// 1. Cream o textura de adancime (shadow map) in care salvam informatia de umbra.
static void CreateShadowResources()
{
    if (ShadowFbo != 0) return;

    ShadowProgramId = LoadShaders("shadow_depth.vert", "shadow_depth.frag");
    shadowLightSpaceLoc = glGetUniformLocation(ShadowProgramId, "lightSpaceMatrix");
    shadowModelLoc = glGetUniformLocation(ShadowProgramId, "model");

    glGenFramebuffers(1, &ShadowFbo);

    glGenTextures(1, &ShadowDepthTex);
    glBindTexture(GL_TEXTURE_2D, ShadowDepthTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, ShadowFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, ShadowDepthTex, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// Calculam matricea de vizualizare a LUMINI (Light View-Projection Matrix)
// Practic, mutam o "camera" in pozitia soarelui care se uita spre scena.
static glm::mat4 ComputeLightSpaceMatrix()
{
    // Lumina directionala: consideram directia dinspre lumina spre origine
    glm::vec3 lightDir = glm::normalize(-lightPos);

    // Ne concentram pe zona din jurul camerei (ca umbrele sa fie detaliate acolo)
    glm::vec3 center = myCamera.Position;
    glm::vec3 lightTarget = center;
    glm::vec3 lightCamPos = lightTarget + (-lightDir) * 250.0f;

    glm::mat4 lightView = glm::lookAt(lightCamPos, lightTarget, glm::vec3(0, 1, 0));

    // Volum ortografic in jurul camerei (cat cuprinde harta de umbre)
    float orthoSize = 220.0f;
    glm::mat4 lightProj = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, 1.0f, 600.0f);

    return lightProj * lightView;
}

// Randam scena din punctul de vedere al luminii PENTRU A GENERA UMBRELE.
// Nu generam culori, doar adancime (Z-buffer).
static void RenderShadowPass(const std::vector<glm::mat4>& treeMatrices, const std::vector<glm::mat4>& benchMatrices)
{
    lightSpaceMatrix = ComputeLightSpaceMatrix();

    // 1. Setam viewport la dimensiunea texturii de umbra (ex: 2048x2048)
    glViewport(0, 0, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
    
    // 2. Activam Framebuffer-ul custom (ShadowFbo)
    glBindFramebuffer(GL_FRAMEBUFFER, ShadowFbo);
    glClear(GL_DEPTH_BUFFER_BIT);

    // 3. Folosim shaderul simplu de adancime
    glUseProgram(ShadowProgramId);
    glUniformMatrix4fv(shadowLightSpaceLoc, 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

    // Randam COPACII
    if (!treeMatrices.empty())
    {
        glBindVertexArray(treeModel.VaoId);
        for (const auto& m : treeMatrices)
        {
            glUniformMatrix4fv(shadowModelLoc, 1, GL_FALSE, glm::value_ptr(m));
            glDrawElements(GL_TRIANGLES, treeModel.indexCount, GL_UNSIGNED_INT, 0);
        }
    }

    // Randam BANCILE
    if (!benchMatrices.empty() && seatLogModel.VaoId != 0)
    {
        glBindVertexArray(seatLogModel.VaoId);
        for (const auto& m : benchMatrices)
        {
            glUniformMatrix4fv(shadowModelLoc, 1, GL_FALSE, glm::value_ptr(m));
            glDrawElements(GL_TRIANGLES, seatLogModel.indexCount, GL_UNSIGNED_INT, 0);
        }
    }

    glBindVertexArray(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // restore viewport
    glViewport(0, 0, (int)width, (int)height);
}

void CreateShaders(void)
{
    ProgramId = LoadShaders("11_02_Shader.vert", "11_02_Shader.frag");
    glUseProgram(ProgramId);
}

// Functia de incarcare a texturilor
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

        // Trimitem pixelii la placavideo
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

    // Cream shaderele, geometria de baza (terenul), si resursele pentru umbra
    CreateShaders();
    CreateVBO();
    CreateGroundMesh();
    CreateShadowResources();

    stbi_set_flip_vertically_on_load(true);

    // Dezactivam solul texturat (folosim teren procedural)
    // diffuseMap = LoadTexture("...");
    // normalMap = LoadTexture("...");

    // Initializam o singura data obiectele noastre si trimitem datele la GPU
    tree.Init();
    sky.Init();
    cloud.Init();
    sun.Init();
    moon.Init();
    stars.Init();
    crosshair.Init();
    campfire.Init();
    fireflies.Init(30, CAMPFIRE_SCENE_POS, 25.0f); // Raspandire mai mare (raza 12 -> 25)

    // Incarcam modelul 3D de copac (foloseste culorile din MTL)
    if (!treeModel.LoadOBJ("D:\\Proiect_2_Grafica\\models\\Lowpoly_tree_sample.obj")) {
        printf("ATENTIE: Nu s-a putut incarca modelul de copac!\n");
    }

    // Incarcam asset-uri pentru foc
    if (!campfireModel.LoadOBJ("D:\\Proiect_2_Grafica\\models\\Campfire.obj")) {
        printf("ATENTIE: Nu s-a putut incarca Campfire.obj\n");
    }
    if (!firePlaneModel.LoadOBJ("D:\\Proiect_2_Grafica\\models\\Planar_Fire.obj")) {
        printf("ATENTIE: Nu s-a putut incarca Planar_Fire.obj\n");
    }

    // Incarcam modelul bancii
    if (!seatLogModel.LoadOBJ("D:\\Proiect_2_Grafica\\models\\bench.obj")) {
        printf("ATENTIE: Nu s-a putut incarca bench.obj\n");
    }

    // Salvam centrul bounds-ului pentru a alinia perfect focul + aura
    gHasCampfireModelCenter = campfireModel.hasBounds;
    if (gHasCampfireModelCenter) {
        gCampfireModelCenter = campfireModel.boundsCenter;
        printf("Centru bounds campfire (spatiu model): (%.3f, %.3f, %.3f)\n", gCampfireModelCenter.x, gCampfireModelCenter.y, gCampfireModelCenter.z);
    } else {
        gCampfireModelCenter = glm::vec3(0.0f);
    }

    // Incarcam cadrele pentru animatia focului (flipbook)
    {
        fireFrameCount = 0;
        for (int i = 0; i < 128; i++) fireFrameTex[i] = 0;

        // Your folder currently has 00.png..59.png (and possibly more). We'll load sequentially until first missing.
        for (int i = 0; i < 128; ++i)
        {
            char path[512];
            sprintf_s(path, sizeof(path), "D:\\Proiect_2_Grafica\\models\\fire frames\\%02d.png", i);

            FILE* f = nullptr;
            fopen_s(&f, path, "rb");
            if (!f) break;
            fclose(f);

            fireFrameTex[fireFrameCount] = LoadTexture(path);
            fireFrameCount++;
        }

        printf("Cadre animate foc incarcate: %d\n", fireFrameCount);
    }

    GenerateClouds();

    // Campfire basecolor textures (best-effort)
    campfireTexWoods = LoadTexture("D:\\Proiect_2_Grafica\\models\\Campfire_Woods_Mat_BaseColor.jpg");
    campfireTexRocks = LoadTexture("D:\\Proiect_2_Grafica\\models\\Campfire_Rocks_Mat_BaseColor.jpg");
    campfireTexBase  = LoadTexture("D:\\Proiect_2_Grafica\\models\\Campfire_Base_Mat_BaseColor.jpg");
    
    // Textura banca
    seatLogTex = LoadTexture("D:\\Proiect_2_Grafica\\models\\wood_bech_dif.png");

    // Locatii Uniforme
    viewLocation = glGetUniformLocation(ProgramId, "view");
    projLocation = glGetUniformLocation(ProgramId, "projection");
    modelLocation = glGetUniformLocation(ProgramId, "model");
    matrUmbraLocation = glGetUniformLocation(ProgramId, "matrUmbra");

    // shadow uniforms in main program
    lightSpaceMatrixLocMain = glGetUniformLocation(ProgramId, "lightSpaceMatrix");

    lightColorLoc = glGetUniformLocation(ProgramId, "lightColor");
    lightPosLoc = glGetUniformLocation(ProgramId, "lightPos");
    viewPosLoc = glGetUniformLocation(ProgramId, "viewPos");

    codColLocation = glGetUniformLocation(ProgramId, "codCol");
    fogColorLoc = glGetUniformLocation(ProgramId, "fogColor");
    fogStartLoc = glGetUniformLocation(ProgramId, "fogStart");
    fogEndLoc = glGetUniformLocation(ProgramId, "fogEnd");

    overrideColorLoc = glGetUniformLocation(ProgramId, "overrideColor");

    fireflyPosLoc = glGetUniformLocation(ProgramId, "fireflyPos");
    numFirefliesActiveLoc = glGetUniformLocation(ProgramId, "numFirefliesActive");

    // Set default time of day
    SetTimeOfDay(MIDDAY);

    // Ascundem cursorul si il punem in centru
    glutSetCursor(GLUT_CURSOR_NONE);
    glutWarpPointer((int)width / 2, (int)height / 2);
    
    printf("=== CONTROALE TIME OF DAY ===\n");
    printf("Apasa '1' pentru ZI\n");
    printf("Apasa '2' pentru APUS\n");
    printf("Apasa '3' pentru NOAPTE\n");
}

void RenderFunction(void)
{
    // 1. Calcul Delta Time (timpul dintre frame-uri pentru miscare independenta de FPS)
    float currentFrame = glutGet(GLUT_ELAPSED_TIME) * 0.001f;
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    // --- UPDATE TRANZITIE CAMERA (Interpolare) ---
    // Varianta simpla: interpolam doar pozitia; rotatia ramane constanta.
    if (g_transition.active)
    {
        g_transition.timeElapsed += deltaTime;
        float t = g_transition.timeElapsed / g_transition.duration;

        // Easing (Smooth Step) - miscare mai lina la capete
        float smoothT = t * t * (3.0f - 2.0f * t);

        if (t >= 1.0f)
        {
            t = 1.0f;
            smoothT = 1.0f;
            g_transition.active = false;
        }

        // Interpolare Pozitie (LERP)
        myCamera.Position = glm::mix(g_transition.startPos, g_transition.endPos, smoothT);

        // Pastram orientarea constanta in tranzitie
        myCamera.Front = glm::normalize(g_transition.fixedFront);
        myCamera.Right = glm::normalize(glm::cross(myCamera.Front, myCamera.WorldUp));
        myCamera.Up    = glm::normalize(glm::cross(myCamera.Right, myCamera.Front));
    }

    // 2. Procesare Input (Miscare WASD + Mouse)
    DoMovement();

    // Build tree model matrices once for both shadow and main pass
    struct TreePos { glm::vec3 pos; glm::vec3 scale; };
    std::vector<TreePos> padure = {
        { glm::vec3(50.0f, 0.0f, -50.0f),  glm::vec3(1.0f) },
        { glm::vec3(-80.0f, 0.0f, -30.0f), glm::vec3(1.5f) },
        { glm::vec3(20.0f, 0.0f, 100.0f),  glm::vec3(0.8f) },
        { glm::vec3(-40.0f, 0.0f, 150.0f), glm::vec3(1.2f) },
        // Expanded forest
        { glm::vec3(-120.0f, 0.0f, 50.0f), glm::vec3(1.1f) },
        { glm::vec3(140.0f, 0.0f, 80.0f),  glm::vec3(0.9f) },
        { glm::vec3(90.0f, 0.0f, -100.0f), glm::vec3(1.3f) },
        { glm::vec3(-60.0f, 0.0f, -120.0f), glm::vec3(1.0f) },
        { glm::vec3(120.0f, 0.0f, -60.0f), glm::vec3(1.4f) },
        { glm::vec3(-150.0f, 0.0f, 10.0f), glm::vec3(0.85f)},
        { glm::vec3(60.0f, 0.0f, 180.0f),  glm::vec3(1.15f)},
        { glm::vec3(-90.0f, 0.0f, 130.0f), glm::vec3(0.95f)},
        // Clusters near center (but keeping clearing open)
        { glm::vec3(35.0f, 0.0f, -25.0f),  glm::vec3(0.7f) },
        { glm::vec3(-35.0f, 0.0f, 40.0f),  glm::vec3(0.75f)},
    };

    std::vector<glm::mat4> treeModels;
    treeModels.reserve(padure.size());
    for (auto& tp : padure)
    {
        glm::mat4 m(1.0f);
        m = glm::translate(m, tp.pos);
        m = glm::scale(m, tp.scale);
        treeModels.push_back(m);
    }

    // --- CALCUL MATRICI BANCI ----
    const int seatCount = 4;
    const float seatRadius = 18.0f; 
    const glm::vec3 seatCenterOffset(5.0f, 0.0f, -5.0f); 
    const glm::vec3 seatScale(5.0f);
    float campY_forCalc = GroundHeight(CAMPFIRE_SCENE_POS.x, CAMPFIRE_SCENE_POS.z);
    glm::vec3 CampfirePos_forCalc(CAMPFIRE_SCENE_POS.x, campY_forCalc, CAMPFIRE_SCENE_POS.z);
    glm::vec3 SeatCircleCenter = CampfirePos_forCalc + seatCenterOffset;
    
   
    float manualRotations[4] = { 90.0f, 90.0f, 90.0f, 90.0f };

 
    g_canInteract = false;

    std::vector<glm::mat4> benchModels;
    if (seatLogModel.VaoId != 0) {
        benchModels.reserve(seatCount);
        for (int i = 0; i < seatCount; ++i)
        {
            float a = (float)i * (2.0f * 3.14159265f / (float)seatCount) + 0.785f;
            glm::vec3 p = SeatCircleCenter + glm::vec3(cosf(a) * seatRadius, 0.0f, sinf(a) * seatRadius);
            p.y = GroundHeight(p.x, p.z) + 3.0f;

           
            if (!g_isSitting) 
            {
                float dist = glm::distance(myCamera.Position, p);
                // Daca suntem aproape (ex: 8 unitati) de aceasta banca
                if (dist < 10.0f) 
                {
                    g_canInteract = true;
                    // Directia dinspre centru spre banca
                    glm::vec3 dirFromCenter = glm::normalize(glm::vec3(cosf(a), 0.0f, sinf(a)));
                    
            
                    g_potentialSitPos = p; 
                }
            }

            float yawToCenter = atan2f(SeatCircleCenter.x - p.x, SeatCircleCenter.z - p.z);
            float finalYaw = glm::degrees(yawToCenter) + manualRotations[i];

            glm::mat4 m(1.0f);
            m = glm::translate(m, p);
            m = glm::rotate(m, glm::radians(finalYaw), glm::vec3(0, 1, 0));
            m = glm::scale(m, seatScale);
            benchModels.push_back(m);
        }
    }

    // Shadow pass (TREES + BENCHES)
    // Randam intai umbrele in textura off-screen
    RenderShadowPass(treeModels, benchModels);

    // Revenim la buffer-ul ecranului (default)
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    // 3. Setare Matrici (View & Projection) - CAMERA NORMALA
    view = myCamera.GetViewMatrix();
    glUniformMatrix4fv(viewLocation, 1, GL_FALSE, &view[0][0]);

    projection = glm::perspective(glm::radians(myCamera.Zoom), width / height, 0.1f, 2000.0f);
    glUniformMatrix4fv(projLocation, 1, GL_FALSE, &projection[0][0]);

    // Bind shadow map and lightSpaceMatrix
    glUseProgram(ProgramId);
    glUniformMatrix4fv(lightSpaceMatrixLocMain, 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, ShadowDepthTex);
    glUniform1i(glGetUniformLocation(ProgramId, "shadowMap"), 2);

    // 4. Setare Iluminare si Ceata
    // Trimitem pozitia luminii si culoarea (care depind de timpul zilei)
    glm::vec3 finalLightPos = lightPos;
    glm::vec3 finalLightColor = lightColor;

 
    float campY = GroundHeight(CAMPFIRE_SCENE_POS.x, CAMPFIRE_SCENE_POS.z);
    const glm::vec3 CampfirePos(CAMPFIRE_SCENE_POS.x, campY, CAMPFIRE_SCENE_POS.z);

    
    const glm::vec3 campfireScale(1.4f);
    glm::vec3 CampfireCenterWorld = CampfirePos;
    if (gHasCampfireModelCenter) {
        CampfireCenterWorld = CampfirePos + gCampfireModelCenter * campfireScale;
        float modelBottomY = campfireModel.boundsMin.y * campfireScale.y;
        float modelTopY = campfireModel.boundsMax.y * campfireScale.y;
        float approxLogsY = modelBottomY + 0.42f * (modelTopY - modelBottomY);
        CampfireCenterWorld.y = CampfirePos.y + approxLogsY;
    }

    glm::vec3 firePos, fireColor;
    float fireIntensity;
    campfire.GetLight(CampfireCenterWorld, currentFrame, firePos, fireColor, fireIntensity);

    // Tune campfire effect: smaller radius (lower intensity), but hotter color
    fireIntensity *= FIRE_LIGHT_INTENSITY_SCALE;
    fireColor *= FIRE_LIGHT_COLOR_SCALE;



    glUseProgram(ProgramId);
    
   
    glUniform3f(glGetUniformLocation(ProgramId, "firePosFrag"), CampfirePos.x, CampfirePos.y + 3.0f, CampfirePos.z);
    glm::vec3 fireFinalColor = fireColor * fireIntensity * 8.0f; 
    glUniform3f(glGetUniformLocation(ProgramId, "fireColorFrag"), fireFinalColor.r, fireFinalColor.g, fireFinalColor.b);

    // --- FIREFLY LIGHTS ---
    // Calculam luminile licuricilor doar noaptea
    if (currentTimeOfDay == NIGHT)
    {
        std::vector<glm::vec3> ffLights;
        fireflies.GetLightPositions(currentFrame, 6, ffLights);

        int activeFF = (int)ffLights.size();
        glUniform1i(numFirefliesActiveLoc, activeFF);
        if (activeFF > 0)
            glUniform3fv(fireflyPosLoc, activeFF, glm::value_ptr(ffLights[0]));
    }
    else
    {
        glUniform1i(numFirefliesActiveLoc, 0);
    }

    glUniform3f(lightColorLoc, finalLightColor.r, finalLightColor.g, finalLightColor.b);
    glUniform3f(lightPosLoc, finalLightPos.x, finalLightPos.y, finalLightPos.z);
    glUniform3f(viewPosLoc, myCamera.Position.x, myCamera.Position.y, myCamera.Position.z);

    //ceata
    glUniform3f(fogColorLoc, fogColor.r, fogColor.g, fogColor.b);

    float startDist, endDist;
    if (currentTimeOfDay == NIGHT) {
        startDist = 60.0f;   // Closer fog at night
        endDist = 120.0f;
    } else if (currentTimeOfDay == SUNSET) {
        startDist = 95.0f;   // slightly clearer than before
        endDist = 220.0f;
    } else {
        startDist = 120.0f;  // Clear day
        endDist = 300.0f;
    }

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
    //                DESENARE SOARE / LUNA
    // =========================================================

    // Setam codul 4 (Galben/Soare sau Alb/Luna)
    glUniform1i(codColLocation, 4);

    // Oprim Depth Test (Sa straluceasca peste cer)
    glDisable(GL_DEPTH_TEST);
    
    // Enable blending for glow effect
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Desenam soarele sau luna in functie de vremii
    if (currentTimeOfDay == NIGHT) {
        // Draw MOON at night
        glm::vec3 moonPos = glm::vec3(-lightPos.x, lightPos.y + 50.0f, -lightPos.z);
        moon.Render(modelLocation, moonPos, glm::vec3(18.0f), view);
    } else {
        // Draw SUN during day/sunset with GLOW HALO
        float sunScale = (currentTimeOfDay == SUNSET) ? 35.0f : 22.0f;
        
        // *** SUN GLOW LAYERS ***

        // 1. OUTER GLOW (Halo) - Large, transparent
        sun.RenderGlow(modelLocation, lightPos, glm::vec3(sunScale * 2.2f), view, 0.15f);
        
        // 2. MIDDLE GLOW - Medium, semi-transparent
        sun.RenderGlow(modelLocation, lightPos, glm::vec3(sunScale * 1.5f), view, 0.35f);
        
        // 3. CORE - Solid, bright sun
        sun.Render(modelLocation, lightPos, glm::vec3(sunScale), view);
    }
    
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);

    // =========================================================
    //                DESENARE STELE 
    // =========================================================

    if (currentTimeOfDay == NIGHT) {
        glUniform1i(codColLocation, 2); // Unlit mode
        glDisable(GL_DEPTH_TEST);
        stars.Render(modelLocation, myCamera.Position);
        glEnable(GL_DEPTH_TEST);
    }

    //NORI
    // 
    glUniform1i(codColLocation, 2);

    // Bucla care deseneaza toti norii generati
    for (auto& c : cloudsVector)
    {
       
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
    // Practic, proiectam toate obiectele (copaci, banci) pe plana Y=0
    // pentru a crea efectul de umbra pe suprafata terenului.
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
    //                DESENARE SOL 
    // =========================================================

    glUniform1i(codColLocation, 0); // Cod 0 = Iluminare Phong + Umbre

    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
    glBindVertexArray(groundVao);
    glDrawElements(GL_TRIANGLES, groundIndexCount, GL_UNSIGNED_INT, 0);

    // --- DESENARE COPACI ---
    // Moved here (BEFORE transparent objects like Fire/Aura) to fix depth issues
    glUniform1i(codColLocation, 0);
    for (size_t i = 0; i < padure.size(); i++) {
        // use same matrices as shadow pass
        glm::mat4 m = treeModels[i];
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(m));
        glBindVertexArray(treeModel.VaoId);
        glDrawElements(GL_TRIANGLES, treeModel.indexCount, GL_UNSIGNED_INT, 0);
    }
    glBindVertexArray(0);

    // --- DESENARE CAMPFIRE (models folder assets) ---

    glUniform1i(codColLocation, 7);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, campfireTexWoods ? campfireTexWoods : campfireTexBase);
    glUniform1i(glGetUniformLocation(ProgramId, "diffuseMap"), 0);

    campfireModel.Render(modelLocation, CampfirePos, glm::vec3(0.0f), campfireScale);

    // --- DESENARE BANCI ---
    if (seatLogModel.VaoId != 0)
    {
        glUniform1i(codColLocation, 7);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, seatLogTex ? seatLogTex : campfireTexWoods);
        glUniform1i(glGetUniformLocation(ProgramId, "diffuseMap"), 0);

        // Folosim matricile precalculate
    
        glBindVertexArray(seatLogModel.VaoId);
        
        for (auto m : benchModels)
        {
            glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(m));
  
            
            glDrawElements(GL_TRIANGLES, seatLogModel.indexCount, GL_UNSIGNED_INT, 0);
        }
        glBindVertexArray(0);
    }

    // =========================================================
    //                FIRE AURA
    // =========================================================

    glUniform1i(codColLocation, 8);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);

    {
        glm::mat4 auraModel(1.0f);
        auraModel = glm::translate(auraModel, CampfireCenterWorld + glm::vec3(0.0f, FIRE_AURA_Y_OFFSET, 0.0f));

        auraModel[0][0] = view[0][0]; auraModel[0][1] = view[1][0]; auraModel[0][2] = view[2][0];
        auraModel[1][0] = view[0][1]; auraModel[1][1] = view[1][1]; auraModel[1][2] = view[2][1];
        auraModel[2][0] = view[0][2]; auraModel[2][1] = view[1][2]; auraModel[2][2] = view[2][2];

        float pulse = 0.90f + 0.10f * sinf(currentFrame * 7.5f);
        float s = FIRE_AURA_SCALE * pulse;
        auraModel = glm::scale(auraModel, glm::vec3(s, s, s));

        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(auraModel));
        glBindVertexArray(sun.VaoId);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 42);
        glBindVertexArray(0);
    }

    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glm::vec3 camDir = glm::normalize(glm::vec3(myCamera.Position.x - CampfireCenterWorld.x, 0.0f, myCamera.Position.z - CampfireCenterWorld.z));
    float yaw = atan2f(camDir.x, camDir.z);
    float flicker = 0.92f + 0.08f * sinf(currentFrame * 10.0f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUniform1i(codColLocation, 6);
    glDepthMask(GL_FALSE);

    int frame = 0;
    if (fireFrameCount > 0)
        frame = (int)floorf(currentFrame * fireFps) % fireFrameCount;

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, fireFrameTex[frame]);
    glUniform1i(glGetUniformLocation(ProgramId, "fireMap"), 3);

    auto drawFire = [&](float extraYaw)
    {
        glm::mat4 fireModel(1.0f);
        fireModel = glm::translate(fireModel, CampfireCenterWorld + glm::vec3(FIRE_PIVOT_X_OFFSET, FIRE_PIVOT_Y_OFFSET, FIRE_PIVOT_Z_OFFSET));
        fireModel = glm::rotate(fireModel, yaw + extraYaw, glm::vec3(0, 1, 0));
        fireModel = glm::scale(fireModel, glm::vec3(FIRE_SCALE_XZ * flicker, FIRE_SCALE_Y * flicker, FIRE_SCALE_XZ * flicker));

        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(fireModel));
        glBindVertexArray(firePlaneModel.VaoId);
        glDrawElements(GL_TRIANGLES, firePlaneModel.indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    };

    drawFire(0.0f);

    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);

    glEnable(GL_DEPTH_TEST);
    glUniform1i(codColLocation, 8);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);

    {
       
        glm::mat4 glowGround(1.0f);
     
        glowGround = glm::translate(glowGround, CampfirePos + glm::vec3(0.0f, 0.2f, 0.0f));
        glowGround = glm::rotate(glowGround, glm::radians(90.0f), glm::vec3(1, 0, 0));
        float groundGlowScale = 4.0f * (0.95f + 0.05f * sinf(currentFrame * 12.0f));

        float s = groundGlowScale * 1.5f;
        glowGround = glm::scale(glowGround, glm::vec3(s, s, s));

        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(glowGround));

        glBindVertexArray(sun.VaoId);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 42);
        glBindVertexArray(0);
    }

    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);


    // =========================================================
    //                DESENARE CROSSHAIR (UI FIX)
    // =========================================================

    // 1. Oprim Depth Test (Sa fie desenat PESTE orice altceva)
    glDisable(GL_DEPTH_TEST);


    glUniform1i(codColLocation, 2); // Unlit (ia culoarea din buffer, care e alba)


    // 3. RESETAM MATRICIle
    // Anulam perspectiva si rotatia camerei. 
    glm::mat4 identity = glm::mat4(1.0f);

    // Trimitem Identity la Proiectie si la View
    glUniformMatrix4fv(projLocation, 1, GL_FALSE, &identity[0][0]);
    glUniformMatrix4fv(viewLocation, 1, GL_FALSE, &identity[0][0]);
    // Model e trimis deja in Crosshair::Render ca Identity

    // 4. Desenam
    crosshair.Render(modelLocation);

    // --- UI: INTERACTION PROMPT ---
    if (g_canInteract && !g_isSitting)
    {
        // Desenam textul "Press E to Sit" in centrul ecranului, putin sub tinta
        float textX = (float)width / 2.0f - 50.0f; 
        float textY = (float)height / 2.0f - 60.0f;
        RenderText("Press 'E' to Sit", textX, textY, 1.0f, 1.0f, 0.0f); // Galben
    }
    else if (g_isSitting)
    {
        // Mesaj pentru a te ridica
        float textX = (float)width / 2.0f - 80.0f;
        float textY = 100.0f; // Jos pe ecran
        RenderText("Press 'Space' to Stand Up", textX, textY, 1.0f, 1.0f, 1.0f);
    }

    glUniformMatrix4fv(viewLocation, 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(projLocation, 1, GL_FALSE, &projection[0][0]);

    // Desenare licurici (fireflies) 
    // Se face la final, pentru a nu afecta alte obiecte
    if (currentTimeOfDay == NIGHT)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDepthMask(GL_FALSE);

        // Trimitem pozitiile curente la shader + desenare particule
        fireflies.Render(modelLocation, codColLocation, overrideColorLoc, currentFrame, view);

        glDepthMask(GL_TRUE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    // 6. Curatenie (Reactivam Depth pentru urmatorul cadru)
    glEnable(GL_DEPTH_TEST);

    glutSwapBuffers();
    glFlush();
}

// Functie pentru schimbarea moment zi (Zi / Apus / Noapte)
void SetTimeOfDay(TimeOfDay tod) {
    currentTimeOfDay = tod;
    
    switch(tod) {
        case MIDDAY:
            // Soarele sus, lumina alba calda, ceata albastruie
            lightPos = glm::vec3(0.0f, 300.0f, 100.0f);
            lightColor = glm::vec3(1.0f, 0.98f, 0.9f); 
            fogColor = glm::vec3(0.6f, 0.75f, 0.9f);
            skyColorTop = glm::vec3(0.1f, 0.4f, 0.8f); 
            skyColorBottom = glm::vec3(0.6f, 0.8f, 0.95f); 
            break;
            
        case SUNSET:
            // Soarele jos, portocaliu intens, ceata densa
            lightPos = glm::vec3(350.0f, 55.0f, 180.0f);
            lightColor = glm::vec3(1.0f, 0.62f, 0.32f);
            fogColor = glm::vec3(0.85f, 0.48f, 0.32f);
            skyColorTop = glm::vec3(0.18f, 0.10f, 0.28f);
            skyColorBottom = glm::vec3(0.95f, 0.42f, 0.18f);
            printf("Time: SUNSET\n");
            break;
            
        case NIGHT:
            // Moon position
            lightPos = glm::vec3(-100.0f, 200.0f, -100.0f);
            lightColor = glm::vec3(0.06f, 0.08f, 0.15f); 
            fogColor = glm::vec3(0.02f, 0.05f, 0.12f); 
            skyColorTop = glm::vec3(0.0f, 0.01f, 0.03f);
            skyColorBottom = glm::vec3(0.03f, 0.08f, 0.15f); 
            printf("Time: NIGHT\n");
            break;
    }
    
    glClearColor(fogColor.r, fogColor.g, fogColor.b, 1.0f);
    sky.UpdateColors(skyColorTop, skyColorBottom);
}

int main(int argc, char* argv[])
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_STENCIL | GLUT_DOUBLE);
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