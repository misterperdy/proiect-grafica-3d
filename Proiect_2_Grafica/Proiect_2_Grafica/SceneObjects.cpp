#include "SceneObjects.h"
#include <vector>

Tree::Tree() {
    VaoId = 0; VboId = 0; EboId = 0;
}

Tree::~Tree() {
    Cleanup();
}

void Tree::Init() {
    // Definim varfurile pentru TRUNCHI (Maro) si COROANA (Verde) intr-un singur array
    // Format: X, Y, Z, W | R, G, B, A | NX, NY, NZ

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
        // Trunchi (Laturile)
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

    // Calculam matricea Model pentru ACEST copac
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::scale(model, scale);

    // O trimitem la shader
    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, &model[0][0]);

    // Desenam
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
    // Un cub mare. Dimensiunea nu conteaza foarte mult daca il desenam fara Depth Test,
    // dar e bine sa fie maricel.
    float s = 500.0f;

    // Culori pentru Gradient (Windows XP Bliss vibe)
    // Sus: Albastru intens de cer
    float rT = 0.0f, gT = 0.4f, bT = 0.8f;
    // Jos: Alb-Bleu deschis (se va uni cu ceata)
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

    // Indici - INVERSATI fata de un cub normal, pentru ca noi stam INAUNTRU
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
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);

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

void Skybox::Render(GLuint modelLocation, glm::vec3 playerPos) {
    glBindVertexArray(VaoId);

    // Skybox-ul se misca odata cu jucatorul
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, playerPos);

    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, &model[0][0]);

    // Desenam cubul (36 indici)
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

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
    // Construim un cub standard (1x1x1) centrat in origine.
    // Il vom scala in Render() ca sa arate plat si lung (ca un nor Unturned).
    // Culoarea va fi ALB pur (1,1,1).

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
//                       SUN CLASS
// ==========================================================

// --- SUN CLASS ---
Sun::Sun() { VaoId = 0; VboId = 0; EboId = 0; }
Sun::~Sun() { Cleanup(); }

void Sun::Init() {
    // Un patrat simplu vertical (fata spre Z)
    float s = 0.5f;

    // X, Y, Z, W         R,G,B,A (Galben)    Normala (0,0,1)
    std::vector<GLfloat> vertices = {
        -s, -s, 0.0f, 1.0f,   1, 1, 0, 1,   0,0,1, // Stanga-Jos
         s, -s, 0.0f, 1.0f,   1, 1, 0, 1,   0,0,1, // Dreapta-Jos
         s,  s, 0.0f, 1.0f,   1, 1, 0, 1,   0,0,1, // Dreapta-Sus
        -s,  s, 0.0f, 1.0f,   1, 1, 0, 1,   0,0,1, // Stanga-Sus
    };

    std::vector<GLuint> indices = { 0, 1, 2, 2, 3, 0 };

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

// ATENTIE: Am modificat Render sa primeasca View Matrix!
// Avem nevoie de matricea camerei ca sa stim cum sa ne orientam spre ea.
void Sun::Render(GLuint modelLocation, glm::vec3 position, glm::vec3 scale, glm::mat4 viewMatrix) {
    glBindVertexArray(VaoId);

    // --- BILLBOARDING MATH ---
    // 1. Punem soarele la pozitia luminii
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);

    // 2. TRUCUL MAGIC: Anulam rotatia camerei.
    // Setam partea de rotatie a matricei Model sa fie transpusa matricei View.
    // Asta face ca obiectul sa fie mereu paralel cu ecranul.
    model[0][0] = viewMatrix[0][0]; model[0][1] = viewMatrix[1][0]; model[0][2] = viewMatrix[2][0];
    model[1][0] = viewMatrix[0][1]; model[1][1] = viewMatrix[1][1]; model[1][2] = viewMatrix[2][1];
    model[2][0] = viewMatrix[0][2]; model[2][1] = viewMatrix[1][2]; model[2][2] = viewMatrix[2][2];

    // 3. Scalam
    model = glm::scale(model, scale);

    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, &model[0][0]);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Sun::Cleanup() {
    if (VboId) glDeleteBuffers(1, &VboId);
    if (EboId) glDeleteBuffers(1, &EboId);
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
    // 1. DIMENSIUNE (CAT DE MARE E PUNCTUL)
    // 0.02f era urias. Punem 0.0025f pentru un punct fin.
    float s = 0.0025f;

    // 2. ASPECT RATIO (SA FIE PATRAT, NU DREPTUNGHI)
    // Deoarece ecranul e mai lat decat inalt (1200 vs 900), 
    // trebuie sa facem inaltimea (h) putin mai mare in procente ca sa para patrat in pixeli.
    // 1200 / 900 = 1.33
    float h = s * 1.33f;

    // 3. POZITIE (CENTRARE MANUALA)
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