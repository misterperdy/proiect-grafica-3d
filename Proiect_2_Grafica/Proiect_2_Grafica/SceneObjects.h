#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// --COPAC--
class Tree
{
public:
    // ID-urile bufferelor OpenGL
    GLuint VaoId, VboId, EboId;
    int indexCount;

    // Constructor / Destructor
    Tree();
    ~Tree();

    // Functia care creeaza geometria (Trunchi + Coroana)
    void Init();

    // Functia care deseneaza copacul la o pozitie specifica
    // Avem nevoie de ID-ul shader-ului pentru matricea Model
    void Render(GLuint modelLocation, glm::vec3 position, glm::vec3 scale = glm::vec3(1.0f));

private:
    void Cleanup();
};

// --- SKYBOX (Gradientul Cerului) ---
class Skybox
{
public:
    GLuint VaoId, VboId, EboId;
    Skybox();
    ~Skybox();
    void Init();
    void Render(GLuint modelLocation, glm::vec3 playerPos); // Se misca cu jucatorul!
private:
    void Cleanup();
};

// --- NOR (Cluster de cuburi albe) ---
class Cloud
{
public:
    GLuint VaoId, VboId, EboId;
    int indexCount;

    Cloud();
    ~Cloud();
    void Init(); // Genereaza o forma random de nor
    void Render(GLuint modelLocation, glm::vec3 position, glm::vec3 scale);
private:
    void Cleanup();
};