#include "SceneObjects.h"
#include <vector>
#include <GL/freeglut.h>

// ----------------------------------------------------------
// CLASA TREE (Copac Procedural)
// Genereaza manual geometria pentru un copac Low-Poly:
// - Trunchi: Cub alungit
// - Coroana: Piramida
// ----------------------------------------------------------
Tree::Tree() {
    VaoId = 0; VboId = 0; EboId = 0;
}

Tree::~Tree() {
    Cleanup();
}

void Tree::Init() {
    // Definim varfurile pentru TRUNCHI (Maro) si COROANA (Verde) intr-un singur array
    // Format vertex: {Pozitie(4f), Culoare(4f), Normala(3f)}

    float trunkWidth = 2.0f;
    float trunkHeight = 10.0f;
    float leavesWidth = 8.0f;
    float leavesHeight = 15.0f;

    // Culori
    float rT = 0.4f, gT = 0.2f, bT = 0.0f; // Maro
    float rL = 0.1f, gL = 0.6f, bL = 0.1f; // Verde

    // --- GEOMETRIA ---
    std::vector<GLfloat> vertices = {
        // --- TRUNCHI (Cub) ---
        // Baza Trunchi (Y=0)
        -trunkWidth, 0.0f, -trunkWidth, 1.0f,   rT, gT, bT, 1.0f,  0.0f, 0.0f, 1.0f, // 0: Stanga-Spate-Jos
         trunkWidth, 0.0f, -trunkWidth, 1.0f,   rT, gT, bT, 1.0f,  0.0f, 0.0f, 1.0f, // 1: Dreapta-Spate-Jos
         trunkWidth, 0.0f,  trunkWidth, 1.0f,   rT, gT, bT, 1.0f,  0.0f, 0.0f, 1.0f, // 2: Dreapta-Fata-Jos
        -trunkWidth, 0.0f,  trunkWidth, 1.0f,   rT, gT, bT, 1.0f,  0.0f, 0.0f, 1.0f, // 3: Stanga-Fata-Jos

        // Top Trunchi (Y=trunkHeight)
        -trunkWidth, trunkHeight, -trunkWidth, 1.0f,   rT, gT, bT, 1.0f,  0.0f, 0.0f, 1.0f, // 4
         trunkWidth, trunkHeight, -trunkWidth, 1.0f,   rT, gT, bT, 1.0f,  0.0f, 0.0f, 1.0f, // 5
         trunkWidth, trunkHeight,  trunkWidth, 1.0f,   rT, gT, bT, 1.0f,  0.0f, 0.0f, 1.0f, // 6
        -trunkWidth, trunkHeight,  trunkWidth, 1.0f,   rT, gT, bT, 1.0f,  0.0f, 0.0f, 1.0f, // 7

        // --- COROANA (Piramida) ---
        // Baza Piramidei (porneste de unde se termina trunchiul)
        -leavesWidth, trunkHeight, -leavesWidth, 1.0f,  rL, gL, bL, 1.0f, 0.0f, 1.0f, 0.0f, // 8
         leavesWidth, trunkHeight, -leavesWidth, 1.0f,  rL, gL, bL, 1.0f, 0.0f, 1.0f, 0.0f, // 9
         leavesWidth, trunkHeight,  leavesWidth, 1.0f,  rL, gL, bL, 1.0f, 0.0f, 1.0f, 0.0f, // 10
        -leavesWidth, trunkHeight,  leavesWidth, 1.0f,  rL, gL, bL, 1.0f, 0.0f, 1.0f, 0.0f, // 11

        // Varful Piramidei
        0.0f, trunkHeight + leavesHeight, 0.0f, 1.0f,   rL, gL, bL, 1.0f, 0.0f, 1.0f, 0.0f  // 12
    };

    std::vector<GLuint> indices = {
        // Trunchi (Laturile) - format din triunghiuri
        0, 1, 5, 0, 5, 4, // Spate
        1, 2, 6, 1, 6, 5, // Dreapta
        2, 3, 7, 2, 7, 6, // Fata
        3, 0, 4, 3, 4, 7, // Stanga

        // Coroana (Triunghiuri laterale)
        8, 9, 12,  // Spate
        9, 10, 12, // Dreapta
        10, 11, 12, // Fata
        11, 8, 12, // Stanga

        // Baza coroanei
        8, 9, 10,
        8, 10, 11
    };

    indexCount = indices.size();

    // Generare OpenGL buffers
    glGenVertexArrays(1, &VaoId);
    glGenBuffers(1, &VboId);
    glGenBuffers(1, &EboId);

    glBindVertexArray(VaoId);

    glBindBuffer(GL_ARRAY_BUFFER, VboId);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EboId);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

    // Atribute (Pozitie, Culoare, Normala)
    // Stride = 11 floats (4 pos + 4 col + 3 norm)
    int stride = 11 * sizeof(GLfloat);

    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (void*)(4 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(8 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0); // Unbind
}

void Tree::Render(GLuint modelLocation, glm::vec3 position, glm::vec3 scale) {
    glBindVertexArray(VaoId);

    // Construim matricea de modelare locala
    // 1. Translatie la pozitia din lume
    // 2. Scalare la dimensiunea dorita
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::scale(model, scale);

    // Trimitem matricea la shader (uniform 'model')
    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, &model[0][0]);

    // Desenam folosind indicii (EBO)
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);
}

void Tree::Cleanup() {
    glDeleteBuffers(1, &VboId);
    glDeleteBuffers(1, &EboId);
    glDeleteVertexArrays(1, &VaoId);
}
// ==========================================================
//                      SKYBOX CLASS
// ==========================================================

Skybox::Skybox() {
    VaoId = 0; VboId = 0; EboId = 0;
}

Skybox::~Skybox() {
    Cleanup();
}

void Skybox::Init() {
    // Un cub mare. Dimensiunea fizica nu conteaza daca oprim Depth Test,
    // dar il facem mare (500) pentru orice eventualitate.
    float s = 500.0f;

    // Culori Gradient (Stil "Windows XP Bliss")
    // Top: Albastru de cer
    float rT = 0.0f, gT = 0.4f, bT = 0.8f;
    // Bottom: Alb-Bleu (pentru a se imbina vizual cu ceata la orizont)
    float rB = 0.5f, gB = 0.8f, bB = 1.0f;

    // Definim varfurile manual
    std::vector<GLfloat> vertices = {
        // Pozitie (X,Y,Z,W)            // Culoare (R,G,B,A)      // Normala (0,0,0 - nu folosim lumina pe cer)

        // Spate (Z = -s)
        -s, -s, -s, 1.0f,   rB, gB, bB, 1.0f,  0,0,0, // Jos-Stanga
         s, -s, -s, 1.0f,   rB, gB, bB, 1.0f,  0,0,0, // Jos-Dreapta
         s,  s, -s, 1.0f,   rT, gT, bT, 1.0f,  0,0,0, // Sus-Dreapta
        -s,  s, -s, 1.0f,   rT, gT, bT, 1.0f,  0,0,0, // Sus-Stanga

        // Fata (Z = s)
        -s, -s,  s, 1.0f,   rB, gB, bB, 1.0f,  0,0,0,
         s, -s,  s, 1.0f,   rB, gB, bB, 1.0f,  0,0,0,
         s,  s,  s, 1.0f,   rT, gT, bT, 1.0f,  0,0,0,
        -s,  s,  s, 1.0f,   rT, gT, bT, 1.0f,  0,0,0,
    };

    // Indici - INVERSATE fata de un cub normal, pentru ca noi stam INAUNTRU
    // (Desenam fetele interioare)
    std::vector<GLuint> indices = {
        0, 3, 1, 1, 3, 2, // Spate
        4, 5, 7, 5, 6, 7, // Fata
        0, 1, 4, 1, 5, 4, // Jos
        2, 3, 6, 3, 7, 6, // Sus
        0, 4, 3, 3, 4, 7, // Stanga
        1, 2, 5, 2, 6, 5  // Dreapta
    };

    // Generare Buffere
    glGenVertexArrays(1, &VaoId);
    glBindVertexArray(VaoId);

    glGenBuffers(1, &VboId);
    glBindBuffer(GL_ARRAY_BUFFER, VboId);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_DYNAMIC_DRAW);

    glGenBuffers(1, &EboId);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EboId);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

    // Layout atribute (trebuie sa fie identic cu cel din Tree/Ground: pos, col, norm)
    int stride = 11 * sizeof(GLfloat);
    // 0: Pos
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    // 1: Color
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (void*)(4 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    // 2: Normal
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(8 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void Skybox::UpdateColors(glm::vec3 topColor, glm::vec3 bottomColor) {
    // Functie apelata cand se schimba ciclul zi/noapte.
    // Actualizeaza dinamic culorile varfurilor in VBO.
    float s = 500.0f;
    float rT = topColor.r, gT = topColor.g, bT = topColor.b;
    float rB = bottomColor.r, gB = bottomColor.g, bB = bottomColor.b;

    std::vector<GLfloat> vertices = {
        -s, -s, -s, 1.0f,   rB, gB, bB, 1.0f,  0,0,0,
         s, -s, -s, 1.0f,   rB, gB, bB, 1.0f,  0,0,0,
         s,  s, -s, 1.0f,   rT, gT, bT, 1.0f,  0,0,0,
        -s,  s, -s, 1.0f,   rT, gT, bT, 1.0f,  0,0,0,
        -s, -s,  s, 1.0f,   rB, gB, bB, 1.0f,  0,0,0,
         s, -s,  s, 1.0f,   rB, gB, bB, 1.0f,  0,0,0,
         s,  s,  s, 1.0f,   rT, gT, bT, 1.0f,  0,0,0,
        -s,  s,  s, 1.0f,   rT, gT, bT, 1.0f,  0,0,0,
    };

    glBindBuffer(GL_ARRAY_BUFFER, VboId);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(GLfloat), vertices.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Skybox::Render(GLuint modelLocation, glm::vec3 playerPos) {
    glBindVertexArray(VaoId);

    // Skybox-ul se misca odata cu jucatorul!
    // Astfel, nu poti ajunge niciodata la "marginea" cerului.
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, playerPos);

    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, &model[0][0]);

    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0); // 36 indici = 12 triunghiuri (6 fete)

    glBindVertexArray(0);
}

void Skybox::Cleanup() {
    // Codul complet de stergere
    if (VboId) glDeleteBuffers(1, &VboId);
    if (EboId) glDeleteBuffers(1, &EboId);
    if (VaoId) glDeleteVertexArrays(1, &VaoId);
}


// ==========================================================
//                       CLOUD CLASS
// ==========================================================

Cloud::Cloud() {
    VaoId = 0; VboId = 0; EboId = 0;
    indexCount = 0;
}

Cloud::~Cloud() {
    Cleanup();
}

void Cloud::Init() {
    //Construim un cub standard (1x1x1) centrat in origine.
    //Il vom escala in Render() ca sa arate plat si lung (ca un nor Unturned).
    //Culoarea va fi ALB pur (1,1,1).

    float s = 0.5f; // Jumatate de latura (ca sa aiba latura 1.0 total)

    std::vector<GLfloat> vertices = {
        // X, Y, Z, W      R, G, B, A      NX, NY, NZ

        // Fata
        -s, -s,  s, 1.0f,  1,1,1,1,  0,0,1,
         s, -s,  s, 1.0f,  1,1,1,1,  0,0,1,
         s,  s,  s, 1.0f,  1,1,1,1,  0,0,1,
        -s,  s,  s, 1.0f,  1,1,1,1,  0,0,1,

        // Spate
        -s, -s, -s, 1.0f,  1,1,1,1,  0,0,-1,
         s, -s, -s, 1.0f,  1,1,1,1,  0,0,-1,
         s,  s, -s, 1.0f,  1,1,1,1,  0,0,-1,
        -s,  s, -s, 1.0f,  1,1,1,1,  0,0,-1,

        // Stanga
        -s, -s, -s, 1.0f,  1,1,1,1,  -1,0,0,
        -s, -s,  s, 1.0f,  1,1,1,1,  -1,0,0,
        -s,  s,  s, 1.0f,  1,1,1,1,  -1,0,0,
        -s,  s, -s, 1.0f,  1,1,1,1,  -1,0,0,

        // Dreapta
         s, -s, -s, 1.0f,  1,1,1,1,  1,0,0,
         s, -s,  s, 1.0f,  1,1,1,1,  1,0,0,
         s,  s,  s, 1.0f,  1,1,1,1,  1,0,0,
         s,  s, -s, 1.0f,  1,1,1,1,  1,0,0,

         // Sus
         -s,  s, -s, 1.0f,  1,1,1,1,  0,1,0,
          s,  s, -s, 1.0f,  1,1,1,1,  0,1,0,
          s,  s,  s, 1.0f,  1,1,1,1,  0,1,0,
         -s,  s,  s, 1.0f,  1,1,1,1,  0,1,0,

         // Jos
         -s, -s, -s, 1.0f,  1,1,1,1,  0,-1,0,
          s, -s, -s, 1.0f,  1,1,1,1,  0,-1,0,
          s, -s,  s, 1.0f,  1,1,1,1,  0,-1,0,
         -s, -s,  s, 1.0f,  1,1,1,1,  0,-1,0,
    };

    std::vector<GLuint> indices = {
        0, 1, 2, 2, 3, 0,       // Fata
        4, 5, 6, 6, 7, 4,       // Spate
        8, 9, 10, 10, 11, 8,    // Stanga
        12, 13, 14, 14, 15, 12, // Dreapta
        16, 17, 18, 18, 19, 16, // Sus
        20, 21, 22, 22, 23, 20  // Jos
    };

    indexCount = indices.size();

    // Buffer Setup
    glGenVertexArrays(1, &VaoId);
    glBindVertexArray(VaoId);

    glGenBuffers(1, &VboId);
    glBindBuffer(GL_ARRAY_BUFFER, VboId);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &EboId);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EboId);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

    int stride = 11 * sizeof(GLfloat);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, stride, (void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (void*)(4 * sizeof(GLfloat))); glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(8 * sizeof(GLfloat))); glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void Cloud::Render(GLuint modelLocation, glm::vec3 position, glm::vec3 scale) {
    glBindVertexArray(VaoId);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::scale(model, scale);

    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, &model[0][0]);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);
}

void Cloud::Cleanup() {
    if (VboId) glDeleteBuffers(1, &VboId);
    if (EboId) glDeleteBuffers(1, &EboId);
    if (VaoId) glDeleteVertexArrays(1, &VaoId);
}

// ==========================================================
// CLASA SUN (Soare / Aura)
// Geometrie: Triangle Fan (evantai de triunghiuri) pentru aforma un cerc.
// Efect: Billboarding (se roteste mereu spre camera).
// ==========================================================

// --- SUN CLASS ---
Sun::Sun() { VaoId = 0; VboId = 0; }
Sun::~Sun() { Cleanup(); }

void Sun::Init() {
    // Generam un cerc prin aproximare cu segmente
    int segments = 40; 
    float radius = 0.5f;
    
    std::vector<GLfloat> vertices;
    
    // Vertex Central - Culoare intensa (Miezul soarelui)
    vertices.insert(vertices.end(), {0.0f, 0.0f, 0.0f, 1.0f,  1.0f, 1.0f, 0.95f, 1.0f,  0,0,1});
    
    // Vertecsi pe circumferinta - Gradient spre transparenta/portocaliu
    for (int i = 0; i <= segments; i++) {
        float angle = (float)i / (float)segments * 2.0f * 3.14159f;
        float x = cos(angle) * radius;
        float y = sin(angle) * radius;
        
        // Gradient: centru alb, marginile portocalii cu transparenta pentru efect de stralucire
        vertices.insert(vertices.end(), {x, y, 0.0f, 1.0f,  1.0f, 0.85f, 0.3f, 0.95f,  0,0,1});
    }

    glGenVertexArrays(1, &VaoId); glBindVertexArray(VaoId);
    glGenBuffers(1, &VboId); glBindBuffer(GL_ARRAY_BUFFER, VboId);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);

    int stride = 11 * sizeof(GLfloat);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, stride, (void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (void*)(4 * sizeof(GLfloat))); glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(8 * sizeof(GLfloat))); glEnableVertexAttribArray(2);
    glBindVertexArray(0);
}

// ATENTIE: Am modificat Render sa primeasca View Matrix!
// Avem nevoie de matricea camerei ca sa stim cum sa ne orientam spre ea.
void Sun::Render(GLuint modelLocation, glm::vec3 position, glm::vec3 scale, glm::mat4 viewMatrix) {
    glBindVertexArray(VaoId);

    // --- BILLBOARDING MATH ---
    // 1. Punem soarele la pozitia luminii
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);

    // 2. TRUCUL MAGIC: BILLBOARDING
    // Anulam rotatia camerei setand rotatia obiectului ca fiind inversa rotatiei camerei (transpusa matricei View).
    // Astfel, obiectul va fi mereu paralel cu ecranul (2D look in 3D space).
    model[0][0] = viewMatrix[0][0]; model[0][1] = viewMatrix[1][0]; model[0][2] = viewMatrix[2][0];
    model[1][0] = viewMatrix[0][1]; model[1][1] = viewMatrix[1][1]; model[1][2] = viewMatrix[2][1];
    model[2][0] = viewMatrix[0][2]; model[2][1] = viewMatrix[1][2]; model[2][2] = viewMatrix[2][2];

    // 3. Scalam
    model = glm::scale(model, scale);

    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, &model[0][0]);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 42); // 1 center + 41 circle vertices
    glBindVertexArray(0);
}

void Sun::RenderGlow(GLuint modelLocation, glm::vec3 position, glm::vec3 scale, glm::mat4 viewMatrix, float alpha) {
    // Create a temporary glow circle with transparency
    int segments = 40;
    float radius = 0.5f;
    std::vector<GLfloat> glowVertices;
    
    // Center - transparent yellow
    glowVertices.insert(glowVertices.end(), {0.0f, 0.0f, 0.0f, 1.0f,  1.0f, 0.9f, 0.5f, alpha,  0,0,1});
    
    // Edges - even more transparent
    for (int i = 0; i <= segments; i++) {
        float angle = (float)i / (float)segments * 2.0f * 3.14159f;
        float x = cos(angle) * radius;
        float y = sin(angle) * radius;
        glowVertices.insert(glowVertices.end(), {x, y, 0.0f, 1.0f,  1.0f, 0.85f, 0.4f, alpha * 0.3f,  0,0,1});
    }
    
    // Upload temporary glow data
    GLuint tempVbo, tempVao;
    glGenVertexArrays(1, &tempVao);
    glBindVertexArray(tempVao);
    
    glGenBuffers(1, &tempVbo);
    glBindBuffer(GL_ARRAY_BUFFER, tempVbo);
    glBufferData(GL_ARRAY_BUFFER, glowVertices.size() * sizeof(GLfloat), glowVertices.data(), GL_DYNAMIC_DRAW);
    
    int stride = 11 * sizeof(GLfloat);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, stride, (void*)0); 
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (void*)(4 * sizeof(GLfloat))); 
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(8 * sizeof(GLfloat))); 
    glEnableVertexAttribArray(2);

    // Same billboarding as Render
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    
    model[0][0] = viewMatrix[0][0]; model[0][1] = viewMatrix[1][0]; model[0][2] = viewMatrix[2][0];
    model[1][0] = viewMatrix[0][1]; model[1][1] = viewMatrix[1][1]; model[1][2] = viewMatrix[2][1];
    model[2][0] = viewMatrix[0][2]; model[2][1] = viewMatrix[1][2]; model[2][2] = viewMatrix[2][2];
    
    model = glm::scale(model, scale);

    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, &model[0][0]);
    
    glDrawArrays(GL_TRIANGLE_FAN, 0, 42);
    
    // Cleanup temp buffers
    glBindVertexArray(0);
    glDeleteBuffers(1, &tempVbo);
    glDeleteVertexArrays(1, &tempVao);
}

void Sun::Cleanup() {
    if (VboId) glDeleteBuffers(1, &VboId);
    if (VaoId) glDeleteVertexArrays(1, &VaoId);
}

// ==========================================================
//                     CROSSHAIR CLASS
// ==========================================================

Crosshair::Crosshair() {
    VaoId = 0; VboId = 0; EboId = 0;
}

Crosshair::~Crosshair() {
    Cleanup();
}

void Crosshair::Init() {
    // 1. DIMENSIUNE (Foarte mica pentru precizie)
    float s = 0.0025f;

    // 2. ASPECT RATIO FIX
    // Corectam faptul ca ecranele sunt dreptunghiulare (1200x675), altfel patratul ar arata turtit.
    float h = s * 1.33f; // 1200 / 675 = 1.77 (aprox) - ajustat ochiometric

    // 3. POZITIE
    // Daca ti se pare ca e prea jos, mareste valoarea asta (ex: 0.01f sau 0.02f)
    // Daca e perfect, las-o 0.0f.
    float yCorrection = 0.0f;

    std::vector<GLfloat> vertices = {
        // X,      Y,              Z,    W       R,G,B,A (Alb)      Normala
        -s, -h + yCorrection, 0.0f, 1.0f,   1,1,1,1,   0,0,1,
         s, -h + yCorrection, 0.0f, 1.0f,   1,1,1,1,   0,0,1,
         s,  h + yCorrection, 0.0f, 1.0f,   1,1,1,1,   0,0,1,
        -s,  h + yCorrection, 0.0f, 1.0f,   1,1,1,1,   0,0,1,
    };

    std::vector<GLuint> indices = {
        0, 1, 2,
        2, 3, 0
    };

    // --- BOILERPLATE STANDARD (Nu schimba nimic mai jos) ---
    glGenVertexArrays(1, &VaoId); glBindVertexArray(VaoId);
    glGenBuffers(1, &VboId); glBindBuffer(GL_ARRAY_BUFFER, VboId);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);
    glGenBuffers(1, &EboId); glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EboId);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

    int stride = 11 * sizeof(GLfloat);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, stride, (void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (void*)(4 * sizeof(GLfloat))); glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(8 * sizeof(GLfloat))); glEnableVertexAttribArray(2);
    glBindVertexArray(0);
}

// Asigura-te ca Render-ul clasei Crosshair trimite identitatea la modelLocation!
void Crosshair::Render(GLuint modelLocation) {
    glBindVertexArray(VaoId);
    // Trimitem Matricea Identitate (1.0) -> Nu mutam patratul, il lasam la 0,0 (Centru)
    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Crosshair::Cleanup() {
    if (VboId) glDeleteBuffers(1, &VboId);
    if (EboId) glDeleteBuffers(1, &EboId);
    if (VaoId) glDeleteVertexArrays(1, &VaoId);
}

// ==========================================================
// CLASA STARS (Stele)
// Randate ca GL_POINTS. Simularea sclipirii se face in Render().
// ==========================================================

Stars::Stars() {
    VaoId = 0; VboId = 0;
    starCount = 0;
}

Stars::~Stars() {
    Cleanup();
}

void Stars::Init() {
    // Generate random stars across the sky dome
    starCount = 400; // DOUBLED for more spectacular night sky
    std::vector<GLfloat> vertices;
    
    srand(12345); // Fixed seed for consistent star positions
    
    for (int i = 0; i < starCount; i++) {
        // Random position on a large sphere (only upper hemisphere for sky)
        float theta = ((float)rand() / RAND_MAX) * 2.0f * 3.14159f; // 0 to 2?
        float phi = ((float)rand() / RAND_MAX) * 1.57f; // 0 to ?/2 (upper half only)
        
        float radius = 450.0f; // Slightly farther for depth
        
        float x = radius * sin(phi) * cos(theta);
        float y = radius * cos(phi) + 80.0f; // Higher offset
        float z = radius * sin(phi) * sin(theta);
        
        // Random brightness (more variation)
        float brightness = 0.5f + ((float)rand() / RAND_MAX) * 0.5f;
        
        // Position (X,Y,Z,W)  Color (White with varying brightness)  Normal
        vertices.insert(vertices.end(), {
            x, y, z, 1.0f,
            brightness, brightness, brightness + 0.1f, 1.0f, // Slight blue tint
            0, 0, 1
        });
    }
    
    glGenVertexArrays(1, &VaoId);
    glBindVertexArray(VaoId);
    
    glGenBuffers(1, &VboId);
    glBindBuffer(GL_ARRAY_BUFFER, VboId);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);
    
    int stride = 11 * sizeof(GLfloat);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (void*)(4 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(8 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);
    
    glBindVertexArray(0);
}

void Stars::Render(GLuint modelLocation, glm::vec3 playerPos) {
    glBindVertexArray(VaoId);
    
    // Stelele se misca odata cu jucatorul (sunt la infinit)
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, playerPos);
    
    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, &model[0][0]);
    
    // EFECT SCLIPIRE (Twinkle)
    // Modificam dimensiunea punctelor in functie de timp (sinusoida)
    float time = glutGet(GLUT_ELAPSED_TIME) * 0.001f;
    float twinkle = 2.5f + sin(time * 2.0f) * 0.5f;
    
    glPointSize(twinkle);
    glDrawArrays(GL_POINTS, 0, starCount);
    glPointSize(1.0f); // Reset
    
    glBindVertexArray(0);
}

void Stars::Cleanup() {
    if (VboId) glDeleteBuffers(1, &VboId);
    if (VaoId) glDeleteVertexArrays(1, &VaoId);
}

// ==========================================================
// CLASA MOON (Luna)
// Geometrie similara cu soarele, dar cu textura gri-albicioasa.
// ==========================================================

Moon::Moon() {
    VaoId = 0; VboId = 0;
}

Moon::~Moon() {
    Cleanup();
}

void Moon::Init() {
    // Create a circle (like the sun but will be rendered white/gray)
    int segments = 32;
    float radius = 0.5f;
    
    std::vector<GLfloat> vertices;
    
    // Center vertex - pale white/gray color for moon
    vertices.insert(vertices.end(), {0.0f, 0.0f, 0.0f, 1.0f,  0.9f, 0.9f, 1.0f, 1.0f,  0,0,1});

    // Circle vertices
    for (int i = 0; i <= segments; i++) {
        float angle = (float)i / (float)segments * 2.0f * 3.14159f;
        float x = cos(angle) * radius;
        float y = sin(angle) * radius;
        
        vertices.insert(vertices.end(), {x, y, 0.0f, 1.0f,  0.9f, 0.9f, 1.0f, 1.0f,  0,0,1});
    }
    
    glGenVertexArrays(1, &VaoId);
    glBindVertexArray(VaoId);
    glGenBuffers(1, &VboId);
    glBindBuffer(GL_ARRAY_BUFFER, VboId);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);
    
    int stride = 11 * sizeof(GLfloat);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (void*)(4 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(8 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
}

void Moon::Render(GLuint modelLocation, glm::vec3 position, glm::vec3 scale, glm::mat4 viewMatrix) {
    glBindVertexArray(VaoId);
    
    // Billboarding (same as sun)
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    
    model[0][0] = viewMatrix[0][0]; model[0][1] = viewMatrix[1][0]; model[0][2] = viewMatrix[2][0];
    model[1][0] = viewMatrix[0][1]; model[1][1] = viewMatrix[1][1]; model[1][2] = viewMatrix[2][1];
    model[2][0] = viewMatrix[0][2]; model[2][1] = viewMatrix[1][2]; model[2][2] = viewMatrix[2][2];
    
    model = glm::scale(model, scale);
    
    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, &model[0][0]);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 34);
    glBindVertexArray(0);
}

void Moon::Cleanup() {
    if (VboId) glDeleteBuffers(1, &VboId);
    if (VaoId) glDeleteVertexArrays(1, &VaoId);
}

// ==========================================================
//                    MODEL3D CLASS (OBJ Loader)
// ==========================================================

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#include <unordered_map>

Model3D::Model3D() {
    VaoId = 0; VboId = 0; EboId = 0;
    indexCount = 0;
}

Model3D::~Model3D() {
    Cleanup();
}

bool Model3D::LoadOBJ(const char* filepath, glm::vec3 forceColor) {
    // Structuri folosite de tinyobjloader
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    // Detectam folderul pentru a gasi fisierele .mtl asociate
    std::string mtl_basedir = filepath;
    size_t last_slash = mtl_basedir.find_last_of("/\\");
    if (last_slash != std::string::npos) {
        mtl_basedir = mtl_basedir.substr(0, last_slash + 1);
    } else {
        mtl_basedir = "";
    }

    // Load OBJ file
    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath, mtl_basedir.c_str())) {
        printf("ERROR loading OBJ: %s\n", (warn + err).c_str());
        return false;
    }

    if (!warn.empty()) {
        printf("WARNING: %s\n", warn.c_str());
    }

    std::vector<GLfloat> vertices;
    std::vector<GLuint> indices;

    // Deduplicate vertices using a hash map
    std::unordered_map<std::string, GLuint> uniqueVertices;

    // Check if we should force a color
    bool useForceColor = (forceColor.r >= 0.0f);

    // Reset bounds
    hasBounds = false;
    boundsMin = glm::vec3(0.0f);
    boundsMax = glm::vec3(0.0f);
    boundsCenter = glm::vec3(0.0f);

    // Bucla prin forme
    for (const auto& shape : shapes) {
        // Bucla prin fete (triunghiuri)
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            int fv = shape.mesh.num_face_vertices[f];

            // ID material asociat fetei curente
            int material_id = shape.mesh.material_ids[f];

            // Culoare implicită (gri deschis) dacă nu există materiale
            glm::vec3 diffuseColor(0.8f, 0.8f, 0.8f);
            
            // Dacă avem culoare forțată o folosim, altfel citim din material (MTL)
            if (useForceColor) {
                diffuseColor = forceColor;
            } else if (material_id >= 0 && material_id < (int)materials.size()) {
                diffuseColor = glm::vec3(
                    materials[material_id].diffuse[0],
                    materials[material_id].diffuse[1],
                    materials[material_id].diffuse[2]
                );
            }

            // Bucla prin vertecșii feței (de regulă 3 pentru triunghi)
            for (size_t v = 0; v < (size_t)fv; v++) {
                tinyobj::index_t idx = shape.mesh.indices[f * (size_t)fv + v];

                // 1. Poziție vertex
                glm::vec3 pos(
                    attrib.vertices[3 * idx.vertex_index + 0],
                    attrib.vertices[3 * idx.vertex_index + 1],
                    attrib.vertices[3 * idx.vertex_index + 2]
                );

                // Calculăm Bounding Box (util pentru aliniere / centrare model)
                if (!hasBounds) {
                    boundsMin = boundsMax = pos;
                    hasBounds = true;
                } else {
                    boundsMin = glm::min(boundsMin, pos);
                    boundsMax = glm::max(boundsMax, pos);
                }

                // 2. Normală (dacă există în fișier)
                glm::vec3 normal(0.0f, 1.0f, 0.0f);
                if (idx.normal_index >= 0) {
                    normal = glm::vec3(
                        attrib.normals[3 * idx.normal_index + 0],
                        attrib.normals[3 * idx.normal_index + 1],
                        attrib.normals[3 * idx.normal_index + 2]
                    );
                }

           
              
                char key[256];
                sprintf_s(key, sizeof(key), "%.6f_%.6f_%.6f_%.6f_%.6f_%.6f_%.3f_%.3f_%.3f", 
                    pos.x, pos.y, pos.z, normal.x, normal.y, normal.z,
                    diffuseColor.r, diffuseColor.g, diffuseColor.b);
                std::string keyStr(key);

                // Dacă vertex-ul nu există încă în map, îl adăugăm
                if (uniqueVertices.find(keyStr) == uniqueVertices.end()) {
                    uniqueVertices[keyStr] = (GLuint)(vertices.size() / 11);

                    // Format vertex: X,Y,Z,W | R,G,B,A | NX,NY,NZ
                    vertices.insert(vertices.end(), {
                        pos.x, pos.y, pos.z, 1.0f,
                        diffuseColor.r, diffuseColor.g, diffuseColor.b, 1.0f,
                        normal.x, normal.y, normal.z
                    });
                }

                // Adăugăm index-ul către vertex-ul deja existent
                indices.push_back(uniqueVertices[keyStr]);
            }
        }
    }

    if (vertices.empty()) {
        printf("ERROR: No geometry found in OBJ file\n");
        return false;
    }

    if (hasBounds) {
        boundsCenter = 0.5f * (boundsMin + boundsMax);
    }

    indexCount = (int)indices.size();

    glGenVertexArrays(1, &VaoId);
    glBindVertexArray(VaoId);

    glGenBuffers(1, &VboId);
    glBindBuffer(GL_ARRAY_BUFFER, VboId);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &EboId);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EboId);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

    int stride = 11 * sizeof(GLfloat);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (void*)(4 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(8 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    printf("? Model loaded with TinyObjLoader: %d vertices, %d triangles\n", 
        (int)(vertices.size()/11), indexCount / 3);
    return true;
}

void Model3D::Render(GLuint modelLocation, glm::vec3 position, glm::vec3 rotation, glm::vec3 scale) {
    glBindVertexArray(VaoId);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, rotation.x, glm::vec3(1, 0, 0));
    model = glm::rotate(model, rotation.y, glm::vec3(0, 1, 0));
    model = glm::rotate(model, rotation.z, glm::vec3(0, 0, 1));
    model = glm::scale(model, scale);

    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, &model[0][0]);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);
}

void Model3D::Cleanup() {
    if (VboId) glDeleteBuffers(1, &VboId);
    if (EboId) glDeleteBuffers(1, &EboId);
    if (VaoId) glDeleteVertexArrays(1, &VaoId);
}

// ==========================================================
// Foc de tabara + Busteni


Campfire::Campfire() {}
Campfire::~Campfire() { Cleanup(); }

static void AppendBox(std::vector<GLfloat>& vertices, std::vector<GLuint>& indices,
    const glm::vec3& half, const glm::vec4& color, const glm::mat4& xform)
{

    glm::vec3 p[8] = {
        {-half.x, -half.y, -half.z},
        { half.x, -half.y, -half.z},
        { half.x,  half.y, -half.z},
        {-half.x,  half.y, -half.z},
        {-half.x, -half.y,  half.z},
        { half.x, -half.y,  half.z},
        { half.x,  half.y,  half.z},
        {-half.x,  half.y,  half.z},
    };


    glm::vec3 n[6] = {
        { 0, 0,-1}, { 0, 0, 1}, {-1, 0, 0}, { 1, 0, 0}, { 0, 1, 0}, { 0,-1, 0}
    };

   
    int f[6][4] = {
        {0,1,2,3}, // -Z
        {4,5,6,7}, // +Z
        {0,4,7,3}, // -X
        {1,5,6,2}, // +X
        {3,2,6,7}, // +Y
        {0,1,5,4}  // -Y
    };

    GLuint base = (GLuint)(vertices.size() / 11);

    for (int face = 0; face < 6; ++face)
    {
        for (int k = 0; k < 4; ++k)
        {
            glm::vec4 wp = xform * glm::vec4(p[f[face][k]], 1.0f);
            glm::vec3 wn = glm::normalize(glm::mat3(xform) * n[face]);
            vertices.insert(vertices.end(), {
                wp.x, wp.y, wp.z, 1.0f,
                color.r, color.g, color.b, color.a,
                wn.x, wn.y, wn.z
                });
        }

      
        indices.insert(indices.end(), {
            base + (GLuint)(face * 4 + 0),
            base + (GLuint)(face * 4 + 1),
            base + (GLuint)(face * 4 + 2),
            base + (GLuint)(face * 4 + 0),
            base + (GLuint)(face * 4 + 2),
            base + (GLuint)(face * 4 + 3)
        });
    }
}

void Campfire::Init()
{
    if (LogsVao != 0) return;

  
    {
        std::vector<GLfloat> v;
        std::vector<GLuint> i;

        glm::vec4 wood(0.28f, 0.16f, 0.07f, 1.0f);

      
        {
            glm::mat4 m(1.0f);
            m = glm::rotate(m, glm::radians(25.0f), glm::vec3(0, 1, 0));
            m = glm::rotate(m, glm::radians(8.0f), glm::vec3(1, 0, 0));
            AppendBox(v, i, glm::vec3(6.0f, 0.6f, 0.8f), wood, m);
        }

   
        {
            glm::mat4 m(1.0f);
            m = glm::rotate(m, glm::radians(-35.0f), glm::vec3(0, 1, 0));
            m = glm::rotate(m, glm::radians(-6.0f), glm::vec3(1, 0, 0));
            AppendBox(v, i, glm::vec3(6.0f, 0.6f, 0.8f), wood, m);
        }

        logsIndexCount = (int)i.size();

        glGenVertexArrays(1, &LogsVao);
        glBindVertexArray(LogsVao);

        glGenBuffers(1, &LogsVbo);
        glBindBuffer(GL_ARRAY_BUFFER, LogsVbo);
        glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(GLfloat), v.data(), GL_STATIC_DRAW);

        glGenBuffers(1, &LogsEbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, LogsEbo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, i.size() * sizeof(GLuint), i.data(), GL_STATIC_DRAW);

        int stride = 11 * sizeof(GLfloat);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, stride, (void*)0); glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (void*)(4 * sizeof(GLfloat))); glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(8 * sizeof(GLfloat))); glEnableVertexAttribArray(2);

        glBindVertexArray(0);
    }

  
    {
        std::vector<GLfloat> v;
        std::vector<GLuint> i;

        auto addQuad = [&](const glm::mat4& rot)
        {
            glm::vec4 c0(1.0f, 0.85f, 0.25f, 0.65f);
            glm::vec4 c1(1.0f, 0.45f, 0.12f, 0.10f);

            glm::vec3 p0(-0.6f, 0.0f, 0.0f);
            glm::vec3 p1( 0.6f, 0.0f, 0.0f);
            glm::vec3 p2( 0.6f, 1.6f, 0.0f);
            glm::vec3 p3(-0.6f, 1.6f, 0.0f);

            glm::vec3 n(0,0,1);

            GLuint base = (GLuint)(v.size() / 11);
            glm::vec4 wp;

            wp = rot * glm::vec4(p0, 1.0f);
            v.insert(v.end(), { wp.x, wp.y, wp.z, 1.0f, c1.r, c1.g, c1.b, c1.a, n.x, n.y, n.z });
            wp = rot * glm::vec4(p1, 1.0f);
            v.insert(v.end(), { wp.x, wp.y, wp.z, 1.0f, c1.r, c1.g, c1.b, c1.a, n.x, n.y, n.z });
            wp = rot * glm::vec4(p2, 1.0f);
            v.insert(v.end(), { wp.x, wp.y, wp.z, 1.0f, c0.r, c0.g, c0.b, c0.a, n.x, n.y, n.z });
            wp = rot * glm::vec4(p3, 1.0f);
            v.insert(v.end(), { wp.x, wp.y, wp.z, 1.0f, c0.r, c0.g, c0.b, c0.a, n.x, n.y, n.z });

            i.insert(i.end(), { base + 0, base + 1, base + 2, base + 0, base + 2, base + 3 });
        };

        addQuad(glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(0, 1, 0)));
        addQuad(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0, 1, 0)));

        flameIndexCount = (int)i.size();

        glGenVertexArrays(1, &FlameVao);
        glBindVertexArray(FlameVao);

        glGenBuffers(1, &FlameVbo);
        glBindBuffer(GL_ARRAY_BUFFER, FlameVbo);
        glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(GLfloat), v.data(), GL_STATIC_DRAW);

        glGenBuffers(1, &FlameEbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, FlameEbo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, i.size() * sizeof(GLuint), i.data(), GL_STATIC_DRAW);

        int stride = 11 * sizeof(GLfloat);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, stride, (void*)0); glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (void*)(4 * sizeof(GLfloat))); glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(8 * sizeof(GLfloat))); glEnableVertexAttribArray(2);

        glBindVertexArray(0);
    }
}

void Campfire::Cleanup()
{
    if (LogsVbo) glDeleteBuffers(1, &LogsVbo);
    if (LogsEbo) glDeleteBuffers(1, &LogsEbo);
    if (LogsVao) glDeleteVertexArrays(1, &LogsVao);

    if (FlameVbo) glDeleteBuffers(1, &FlameVbo);
    if (FlameEbo) glDeleteBuffers(1, &FlameEbo);
    if (FlameVao) glDeleteVertexArrays(1, &FlameVao);

    LogsVao = LogsVbo = LogsEbo = 0;
    FlameVao = FlameVbo = FlameEbo = 0;
    logsIndexCount = flameIndexCount = 0;
}

void Campfire::GetLight(glm::vec3 worldPos, float time, glm::vec3& outPos, glm::vec3& outColor, float& outIntensity) const
{
    outPos = worldPos + glm::vec3(0.0f, 2.5f, 0.0f);

    float flicker = sinf(time * 10.0f) * 0.1f + sinf(time * 23.0f) * 0.05f + sinf(time * 37.0f) * 0.02f;
    
    outIntensity = baseIntensity + flicker;
    
    outColor = glm::vec3(1.0f, 0.6f, 0.2f);
}

// --- FIREFLIES IMPLEMENTATION ---

void Fireflies::Init(int count, glm::vec3 centerPos, float radius)
{
    center = centerPos;
    particles.resize(count);

    for (int i = 0; i < count; ++i)
    {
        // Initializare randomizata
        float angle = (float)rand() / RAND_MAX * 6.28f;
        float r = ((float)rand() / RAND_MAX) * radius;
        
        // Offset de baza (pozitia de pornire)
        particles[i].offset = glm::vec3(cosf(angle) * r, 
                                        ((float)rand() / RAND_MAX) * 8.0f + 0.5f, // Inaltime variabila (0.5m - 8.5m)
                                        sinf(angle) * r);
        
        // Parametri de miscare individuali
        particles[i].speed = 0.5f + ((float)rand() / RAND_MAX); // Viteza de rotatie/oscilatie
        particles[i].phase = ((float)rand() / RAND_MAX) * 6.28f; // Faza (ca sa nu clipeasca toti deodata)
        particles[i].yRange = 1.0f + ((float)rand() / RAND_MAX) * 2.5f; // Amplitudinea miscarii pe verticala
    }

    // Geometria unui singur licurici (un patrat mic)
    float vertices[] = {
        // Pos
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.5f,  0.5f, 0.0f,
        -0.5f, -0.5f, 0.0f,
         0.5f,  0.5f, 0.0f,
        -0.5f,  0.5f, 0.0f
    };

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    glBindVertexArray(0);
}

void Fireflies::Render(GLuint modelLoc, GLuint codColLoc, GLuint colorLoc, float time, glm::mat4 viewMatrix)
{
    glUniform1i(codColLoc, 3); 

    glBindVertexArray(vao);

    // Greenish-Yellow "Glow" color
    glm::vec3 fireflyBaseColor(0.8f, 1.0f, 0.2f); 

    for (auto p : particles)
    {
        float offsetX = sinf(time * p.speed + p.offset.x) * 3.5f;
        float offsetZ = cosf(time * p.speed * 0.8f + p.offset.z) * 3.5f;
        float offsetY = sinf(time * p.speed * 0.5f + p.phase) * p.yRange;

        glm::vec3 pos = center + p.offset + glm::vec3(offsetX, offsetY, offsetZ);

        // 2. Blink Effect
        // Functia sinus adusa in intervalul [0, 1] si ridicata la putere pentru "spike-uri" de lumina scurta.
        float blink = (sinf(time * 3.0f + p.phase) + 1.0f) * 0.5f; 
        blink = pow(blink, 4.0f);
        
        if (blink < 0.05f) 
            continue; 

       
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, pos);
        
   
        model[0][0] = viewMatrix[0][0]; model[0][1] = viewMatrix[1][0]; model[0][2] = viewMatrix[2][0];
        model[1][0] = viewMatrix[0][1]; model[1][1] = viewMatrix[1][1]; model[1][2] = viewMatrix[2][1];
        model[2][0] = viewMatrix[0][2]; model[2][1] = viewMatrix[1][2]; model[2][2] = viewMatrix[2][2];

   
        float scale = 0.15f * blink; 
        model = glm::scale(model, glm::vec3(scale));

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniform4f(colorLoc, fireflyBaseColor.r, fireflyBaseColor.g, fireflyBaseColor.b, blink);

        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    glBindVertexArray(0);
}

void Fireflies::GetLightPositions(float time, int maxLights, std::vector<glm::vec3>& outPositions)
{
    outPositions.clear();
    int count = 0;
    for (auto p : particles)
    {
        if (count >= maxLights) 
            break;

  
        float offsetX = sinf(time * p.speed + p.offset.x) * 3.5f;
        float offsetZ = cosf(time * p.speed * 0.8f + p.offset.z) * 3.5f;
        float offsetY = sinf(time * p.speed * 0.5f + p.phase) * p.yRange;

        glm::vec3 pos = center + p.offset + glm::vec3(offsetX, offsetY, offsetZ);
        
        float blink = (sinf(time * 3.0f + p.phase) + 1.0f) * 0.5f;
        blink = pow(blink, 4.0f);

        if (blink > 0.1f) {
            outPositions.push_back(pos);
            count++;
        }
    }
}