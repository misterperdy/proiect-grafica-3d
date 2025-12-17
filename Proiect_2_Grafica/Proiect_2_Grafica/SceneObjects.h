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

// --- SOARE (Un cub galben stralucitor) ---
class Sun {
public:
    GLuint VaoId, VboId, EboId;
    Sun();
    ~Sun();
    void Init();
    // Am adaugat parametrul "glm::mat4 viewMatrix" la final
    void Render(GLuint modelLocation, glm::vec3 position, glm::vec3 scale, glm::mat4 viewMatrix);
private:
    void Cleanup();
};

// --- CROSSHAIR (Tinta 2D) ---
class Crosshair
{
public:
    GLuint VaoId, VboId, EboId;
    Crosshair();
    ~Crosshair();
    void Init();
    // Nu avem nevoie de pozitie/scale in Render, pentru ca e mereu in centru
    void Render(GLuint modelLocation);
private:
    void Cleanup();
};  