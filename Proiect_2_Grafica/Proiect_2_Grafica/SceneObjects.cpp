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