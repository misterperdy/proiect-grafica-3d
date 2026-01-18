#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

// ============================================================================
//  SceneObjects.h
//  Declarațiile claselor pentru obiectele din scenă.
//  Fiecare obiect are de obicei:
//   - resurse GPU (VAO/VBO/EBO)
//   - o funcție `Init()` care creează geometria și o urcă pe GPU
//   - o funcție `Render()` care setează matricea Model și desenează
//   - `Cleanup()` pentru eliberarea resurselor OpenGL
// ============================================================================

// ============================================================================
//  TREE (Copac procedural simplu)
//  Geometrie low-poly generată în cod:
//   - Trunchi: cub alungit
//   - Coroană: piramidă
// ============================================================================
class Tree
{
public:
    // ID-urile bufferelor OpenGL
    GLuint VaoId, VboId, EboId;
    int indexCount;

    Tree();
    ~Tree();

    // Creează geometria și o încarcă pe GPU
    void Init();

    // Desenează copacul la o poziție (cu scalare opțională)
    // `modelLocation` = locația uniformului `model` din shader.
    void Render(GLuint modelLocation, glm::vec3 position, glm::vec3 scale = glm::vec3(1.0f));

private:
    void Cleanup();
};

// ============================================================================
//  SKYBOX (Cer cu gradient)
//  Implementare: cub mare randat "din interior".
//  Truc: se desenează fără Depth Test, ca să fie mereu în fundal.
// ============================================================================
class Skybox
{
public:
    GLuint VaoId, VboId, EboId;
    Skybox();
    ~Skybox();

    void Init();

    // Randare: skybox-ul se translatează cu poziția jucătorului
    // pentru a părea la infinit.
    void Render(GLuint modelLocation, glm::vec3 playerPos);

    // Actualizează culorile pentru tranziții Zi/Apus/Noapte.
    void UpdateColors(glm::vec3 topColor, glm::vec3 bottomColor);

private:
    void Cleanup();
};

// ============================================================================
//  CLOUD (Nor simplu)
//  Implementare: cub alb randat cu o scalare "plată" (arătând ca un nor low-poly).
/// ============================================================================
class Cloud
{
public:
    GLuint VaoId, VboId, EboId;
    int indexCount;

    Cloud();
    ~Cloud();

    void Init();
    void Render(GLuint modelLocation, glm::vec3 position, glm::vec3 scale);

private:
    void Cleanup();
};

// ============================================================================
//  SUN (Soare + Glow)
//  Implementare: cerc (triangle fan) + billboarding (orientare spre cameră).
/// ============================================================================
class Sun {
public:
    GLuint VaoId, VboId;

    Sun();
    ~Sun();

    void Init();

    // `viewMatrix` e necesară pentru billboarding.
    void Render(GLuint modelLocation, glm::vec3 position, glm::vec3 scale, glm::mat4 viewMatrix);
    void RenderGlow(GLuint modelLocation, glm::vec3 position, glm::vec3 scale, glm::mat4 viewMatrix, float alpha);

private:
    void Cleanup();
};

// ============================================================================
//  CROSSHAIR (Țintă 2D)
//  Se desenează în centru, cu matricele view/projection resetate (identitate).
// ============================================================================
class Crosshair
{
public:
    GLuint VaoId, VboId, EboId;

    Crosshair();
    ~Crosshair();

    void Init();

    // Nu avem nevoie de poziție/scale: e mereu în centru.
    void Render(GLuint modelLocation);

private:
    void Cleanup();
};

// ============================================================================
//  STARS (Stele)
//  Implementare: GL_POINTS + variație periodică a dimensiunii (twinkle).
// ============================================================================
class Stars
{
public:
    GLuint VaoId, VboId;
    int starCount;

    Stars();
    ~Stars();

    void Init();
    void Render(GLuint modelLocation, glm::vec3 playerPos);

private:
    void Cleanup();
};

// ============================================================================
//  MOON (Lună)
//  Similar cu soarele: cerc (triangle fan) + billboarding.
// ============================================================================
class Moon
{
public:
    GLuint VaoId, VboId;

    Moon();
    ~Moon();

    void Init();
    void Render(GLuint modelLocation, glm::vec3 position, glm::vec3 scale, glm::mat4 viewMatrix);

private:
    void Cleanup();
};

// ============================================================================
//  MODEL3D (Loader OBJ)
//  Încărcare fișiere `.obj` cu `tiny_obj_loader`.
//  Salvează bounds (min/max/center) în spațiul modelului pentru aliniere în scenă.
// ============================================================================
class Model3D
{
public:
    GLuint VaoId, VboId, EboId;
    int indexCount;

    // Limite în spațiul modelului (calculat din pozițiile încărcate)
    glm::vec3 boundsMin = glm::vec3(0.0f);
    glm::vec3 boundsMax = glm::vec3(0.0f);
    glm::vec3 boundsCenter = glm::vec3(0.0f);
    bool hasBounds = false;

    Model3D();
    ~Model3D();

    // `forceColor`: dacă e setat (>= 0), ignoră materialele și forțează o culoare unică.
    bool LoadOBJ(const char* filepath, glm::vec3 forceColor = glm::vec3(-1.0f));

    // `rotation` este în radiani (pe axe X/Y/Z). `scale` este vectorial.
    void Render(GLuint modelLocation, glm::vec3 position, glm::vec3 rotation, glm::vec3 scale);

private:
    void Cleanup();
};

// ============================================================================
//  CAMPFIRE (Foc de tabără)
//   -  bucăți de lemn + flacără simplă)
//   - furnizează parametrii unei lumini pâlpâitoare (poziție/culoare/intensitate)
// ============================================================================
class Campfire
{
public:
    GLuint LogsVao = 0, LogsVbo = 0, LogsEbo = 0;
    int logsIndexCount = 0;

    GLuint FlameVao = 0, FlameVbo = 0, FlameEbo = 0;
    int flameIndexCount = 0;

    Campfire();
    ~Campfire();

    void Init();
    void Cleanup();

    // Obține informații despre lumina focului (pâlpâire) pentru shader.
    void GetLight(glm::vec3 position, float timeSeconds, glm::vec3& outPos, glm::vec3& outColor, float& outIntensity) const;

private:
    float baseIntensity = 0.8f;
};

// ============================================================================
//  FIREFLIES (Licurici)
//  Sistem simplu de particule:
//   - fiecare licurici este un quad (2 triunghiuri) randat ca billboard
//   - mișcare pseudo-aleatoare (sin/cos + parametri individuali)
//   - clipire (blink) prin funcție sinus + putere
//  Extra: pozițiile unui număr mic de licurici sunt trimise ca lumini punctiforme
//         către shader (max 6), pentru iluminare subtilă.
// ============================================================================
class Fireflies
{
public:
    void Init(int count, glm::vec3 centerPos, float radius);
    void Render(GLuint modelLoc, GLuint codColLoc, GLuint colorLoc, float time, glm::mat4 viewMatrix);

    // Întoarce pozițiile curente ale maxim `maxLights` licurici aprinși,
    // pentru a fi folosiți ca surse de lumină în shader.
    void GetLightPositions(float time, int maxLights, std::vector<glm::vec3>& outPositions);

private:
    struct Particle {
        glm::vec3 offset; // Offset inițial aleator (în jurul centrului)
        float speed;      // Viteză individuală
        float phase;      // Fază (pentru clipire și mișcare decalate)
        float yRange;     // Amplitudinea mișcării pe verticală
    };

    std::vector<Particle> particles;
    glm::vec3 center;

    // Geometria unui quad (VAO/VBO) reutilizată pentru toți licuricii
    GLuint vao = 0, vbo = 0;
};
