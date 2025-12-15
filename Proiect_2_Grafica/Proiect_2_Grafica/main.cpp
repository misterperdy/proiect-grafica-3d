//
// ================================================
// | Grafica pe calculator                        |
// ================================================
// | Laboratorul XI - 11_02_amestecare3D.cpp |
// ===========================================
// 
//	Program ce deseneaza o piramida ce se intrepatrunde cu un cub, folosindu-se tehnicile MODERN OpenGL;
//	Elemente de NOUTATE:
//	 - folosirea celei de-a 4-a componente din codul RGBA;
//   - utilizarea functiilor specifice pentru amestecare: 
//		* glEnable(GL_BLEND); 
//		* glBlendFunc(GL_SRC_ALPHA, GL_SRC_ALPHA);
//	 - combinarea: 
//		* ordinii de desenare a obiectelor;
//		* testului de adancime;
//		* efectelor de amestecare;
// 
// 
// 
//	Biblioteci
#include <windows.h>        //	Utilizarea functiilor de sistem Windows (crearea de ferestre, manipularea fisierelor si directoarelor);
#include <stdlib.h>         //  Biblioteci necesare pentru citirea shaderelor;
#include <stdio.h>
#include <math.h>			//	Biblioteca pentru calcule matematice;
#include <GL/glew.h>        //  Definește prototipurile functiilor OpenGL si constantele necesare pentru programarea OpenGL moderna; 
#include <GL/freeglut.h>    //	Include functii pentru: 
							//	- gestionarea ferestrelor si evenimentelor de tastatura si mouse, 
							//  - desenarea de primitive grafice precum dreptunghiuri, cercuri sau linii, 
							//  - crearea de meniuri si submeniuri;
#include "loadShaders.h"	//	Fisierul care face legatura intre program si shadere;
#include "glm/glm.hpp"		//	Bibloteci utilizate pentru transformari grafice;
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/quaternion.hpp" 
#include "glm/gtx/quaternion.hpp"

#include "Camera.h"

using namespace std;
float PI = 3.141592;

GLuint
	VaoId,
	VboId1, VboId2,
	EboId1, EboId2,
	ProgramId,
	myMatrixLocation,
	matrUmbraLocation,
	viewLocation,
	projLocation,
	matrRotlLocation,
	codColLocation,
	lightColorLoc,
	lightPosLoc,
	viewPosLoc,
	codCol,
	fogColorLoc,
	fogDensityLoc,
	modelLocation;

//declarare camera
Camera myCamera(glm::vec3(0.0f, 20.0f, 100.0f));

// Dimensiunile ferestrei (trebuie sa coincida cu glutInitWindowSize)
float width = 1200;
float height = 900;

// Elemente pentru proiectie
float znear = 0.1f; // Cat de aproape vedem
float fov = 45.0f;  // Field of View (cat de larg vedem)

// Timp pentru miscare lina
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Mouse
float lastX = 400, lastY = 300; // Centrul ecranului
bool firstMouse = true;

// Vector de taste apasate (pentru miscare continua WASD)
bool keys[1024];

bool keyF_down = false;
bool keyG_down = false;
float currentAngle = 0.0f; // Unghiul acumulat
float rotationSpeed = 0.003f; // Viteza de rotatie

// sursa de lumina
float xL = 500.f, yL = 0.f, zL = 400.f;

// matrice
glm::mat4 view, projection, matrUmbra;

void processNormalKeys(unsigned char key, int x, int y)
{
	// Actualizam vectorul de taste
	if (key >= 0 && key < 1024)
	{
		keys[key] = true;
	}

	// Tastele pt rotatia obiectului pt demonstratie
	switch (key) {
	case 'f':
	case 'F':
		keyF_down = true;
		break;
	case 'g':
	case 'G':
		keyG_down = true;
		break;
	}
	if (key == 27)
		exit(0);
}

void processNormalKeysUp(unsigned char key, int x, int y)
{
	// Actualizam vectorul de taste
	if (key >= 0 && key < 1024)
	{
		keys[key] = false;
	}

	switch (key) {
	case 'f':
	case 'F':
		keyF_down = false;
		break;
	case 'g':
	case 'G':
		keyG_down = false;
		break;
	}
}

//callback si pt mouse
void processMouseMovement(int x, int y)
{
	if (firstMouse)
	{
		lastX = x;
		lastY = y;
		firstMouse = false;
	}

	float xoffset = x - lastX;
	float yoffset = lastY - y; // Inversat deoarece coordonatele Y merg de sus in jos

	lastX = x;
	lastY = y;

	myCamera.ProcessMouseMovement(xoffset, yoffset);
}

void DoMovement()
{
	// Camera controls
	if (keys['w'] || keys['W'])
		myCamera.ProcessKeyboard(FORWARD, deltaTime);
	if (keys['s'] || keys['S'])
		myCamera.ProcessKeyboard(BACKWARD, deltaTime);
	if (keys['a'] || keys['A'])
		myCamera.ProcessKeyboard(LEFT, deltaTime);
	if (keys['d'] || keys['D'])
		myCamera.ProcessKeyboard(RIGHT, deltaTime);
}

void CreateVBO(void) // sunt folosite doua buffere
{
	// varfurile pentru cub si "ground" 
	GLfloat Vertices1[] =
	{
		// coordonate                   // culori			    // normale
		-50.0f,  -50.0f, 50.0f, 1.0f,  0.0f, 0.5f, 0.9f, 0.5f,  -1.0f, -1.0f, -1.0f,
		50.0f,  -50.0f,  50.0f, 1.0f,  0.0f, 0.5f, 0.9f, 0.5f,  1.0f, -1.0f, -1.0f,
		50.0f,  50.0f,  50.0f, 1.0f,   0.0f, 0.5f, 0.9f, 0.5f, 1.0f, 1.0f, -1.0f,
		-50.0f,  50.0f, 50.0f, 1.0f,   0.0f, 0.5f, 0.9f, 0.5f, -1.0f, 1.0f, -1.0f,
		-50.0f,  -50.0f, 150.0f, 1.0f,  0.0f, 0.5f, 0.9f, 0.5f, -1.0f, -1.0f, 1.0f,
		50.0f,  -50.0f,  150.0f, 1.0f,  0.0f, 0.5f, 0.9f, 0.5f, 1.0f, -1.0f, 1.0f,
		50.0f,  50.0f,  150.0f, 1.0f,   0.0f, 0.5f, 0.9f, 0.5f, 1.0f, 1.0f, 1.0f,
		-50.0f,  50.0f, 150.0f, 1.0f,   0.0f, 0.5f, 0.9f, 0.5f, -1.0f, 1.0f, 1.0f,
		// 
	   -1000.0f,  -1000.0f, 0.0f, 1.0f,  1.0f, 1.0f, 0.5f, 1.0f,  0.0f, 0.0f, 1.0f,
		1000.0f,  -1000.0f, 0.0f, 1.0f,  1.0f, 1.0f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f,
		1000.0f,  1000.0f,  0.0f, 1.0f,  1.0f, 1.0f, 0.5f, 1.0f,0.0f, 0.0f, 1.0f,
	   -1000.0f,  1000.0f,  0.0f, 1.0f,  1.0f, 1.0f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f,
	};
	// indicii pentru varfuri
	GLubyte Indices1[] =
	{
	  1, 2, 0,   2, 0, 3,
	  2, 3, 6,   6, 3, 7,
	  7, 3, 4,   4, 3, 0,
	  4, 0, 5,   5, 0, 1,
	  1, 2, 5,   5, 2, 6,
	  5, 6, 4,   4, 6, 7,
	  9, 10, 8, 10, 8, 11
	};

	// varfurile pentru piramida
	GLfloat Vertices2[] =
	{
		// coordonate                   // culori			     // normale
		-40.0f, -69.28f, 70.0f, 1.0f,    0.1f, 1.0f, 0.2f, 1.0f,  -40.0f, -69.28f, 80.0f,
		40.0f, -69.28f, 70.0f, 1.0f,     0.1f, 1.0f, 0.2f, 1.0f,  40.0f, -69.28f, 80.0f,
		80.0f, 0.0f, 70.0f, 1.0f,        0.1f, 1.0f, 0.2f, 1.0f,  80.0f, 0.0f, 80.0f,
		40.0f, 69.28f, 70.0f, 1.0f,      0.1f, 1.0f, 0.2f, 1.0f,  40.0f, 69.28f, 80.0f,
		-40.0f, 69.28f, 70.0f, 1.0f,     0.1f, 1.0f, 0.2f, 1.0f, -40.0f, 69.28f, 80.0f,
		-80.0f, 0.0f,  70.0f, 1.0f,      0.1f, 1.0f, 0.2f, 1.0f, -80.0f, 0.0f, 80.0f,
		 0.0f, 0.0f, 170.0f, 1.0f,      0.3f, 1.0f, 0.2f, 1.0f,  0.0f, 0.0f, 1.0f,
	};
	// indicii pentru piramida
	GLubyte Indices2[] =
	{
	   0, 1, 6,
	   1, 2, 6,
	   2, 3, 6,
	   3, 4, 6,
	   4, 5, 6,
	   5, 0, 6
	};

	// se creeaza un VAO (Vertex Array Object) - util cand se utilizeaza mai multe VBO
	glGenVertexArrays(1, &VaoId);
	glBindVertexArray(VaoId);
	glGenBuffers(1, &VboId1);
	glGenBuffers(1, &EboId1);
	glGenBuffers(1, &VboId2);
	glGenBuffers(1, &EboId2);

	glBindBuffer(GL_ARRAY_BUFFER, VboId1);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices1), Vertices1, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EboId1);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Indices1), Indices1, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, VboId2);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices2), Vertices2, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EboId2);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Indices2), Indices2, GL_STATIC_DRAW);

	// se activeaza atributele
	glEnableVertexAttribArray(0); // atributul 0 = pozitie
	glEnableVertexAttribArray(1); // atributul 1 = culoare
	glEnableVertexAttribArray(2); // atributul 2 = normale
}
void AssociateAttributePointers()
{
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(4 * sizeof(GLfloat)));
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(8 * sizeof(GLfloat)));
}
void DestroyVBO(void)
{
	glDisableVertexAttribArray(2);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDeleteBuffers(1, &VboId1);
	glDeleteBuffers(1, &EboId1);
	glDeleteBuffers(1, &VboId2);
	glDeleteBuffers(1, &EboId2);
	glBindVertexArray(0);
	glDeleteVertexArrays(1, &VaoId);
}
void CreateShaders(void)
{
	ProgramId = LoadShaders("11_02_Shader.vert", "11_02_Shader.frag");
	glUseProgram(ProgramId);
}
void DestroyShaders(void)
{
	glDeleteProgram(ProgramId);
}
void Initialize(void)
{
	glClearColor(0.7f, 0.7f, 0.7f, 1.0f); // fundal de culoarea cetii

	CreateShaders();
	CreateVBO();
	viewLocation = glGetUniformLocation(ProgramId, "view");
	projLocation = glGetUniformLocation(ProgramId, "projection");
	matrUmbraLocation = glGetUniformLocation(ProgramId, "matrUmbra");
	lightColorLoc = glGetUniformLocation(ProgramId, "lightColor");
	lightPosLoc = glGetUniformLocation(ProgramId, "lightPos");
	viewPosLoc = glGetUniformLocation(ProgramId, "viewPos");
	codColLocation = glGetUniformLocation(ProgramId, "codCol");
	fogColorLoc = glGetUniformLocation(ProgramId, "fogColor");
	fogDensityLoc = glGetUniformLocation(ProgramId, "fogDensity");
	modelLocation = glGetUniformLocation(ProgramId, "model");
}
void RenderFunction(void)
{
	// Calcul Timp
	float currentFrame = glutGet(GLUT_ELAPSED_TIME) * 0.001f;
	deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;

	// Proceseaza miscarea (WASD)
	DoMovement();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	// --- CAMERA NOUA ---
	// In loc sa calculam Obsx, Obsy manual, cerem matricea de la obiectul Camera
	view = myCamera.GetViewMatrix();
	glUniformMatrix4fv(viewLocation, 1, GL_FALSE, &view[0][0]);

	// projection = glm::infinitePerspective(fov, GLfloat(width) / GLfloat(height), znear); // cu zoom-ul declarat de noi aici (fov)
	projection = glm::infinitePerspective(myCamera.Zoom, GLfloat(width) / GLfloat(height), znear); //folosim zoom-ul camerei
	glUniformMatrix4fv(projLocation, 1, GL_FALSE, &projection[0][0]);

	// Variabile uniforme pentru iluminare
	glUniform3f(lightColorLoc, 1.0f, 1.0f, 1.0f);
	glUniform3f(lightPosLoc, xL, yL, zL);
	glUniform3f(viewPosLoc, myCamera.Position.x, myCamera.Position.y, myCamera.Position.z);

	glUniform3f(fogColorLoc, 0.7f, 0.7f, 0.7f);
	glUniform1f(fogDensityLoc, 0.0015f); // culoarea si densitatea cetii

	// matricea pentru umbra
	float D = -5.f;
	matrUmbra[0][0] = zL + D; matrUmbra[0][1] = 0; matrUmbra[0][2] = 0; matrUmbra[0][3] = 0;
	matrUmbra[1][0] = 0; matrUmbra[1][1] = zL + D; matrUmbra[1][2] = 0; matrUmbra[1][3] = 0;
	matrUmbra[2][0] = -xL; matrUmbra[2][1] = -yL; matrUmbra[2][2] = D; matrUmbra[2][3] = -1;
	matrUmbra[3][0] = -D * xL; matrUmbra[3][1] = -D * yL; matrUmbra[3][2] = -D * zL; matrUmbra[3][3] = zL;
	glUniformMatrix4fv(matrUmbraLocation, 1, GL_FALSE, &matrUmbra[0][0]);

	// desenare obiecte opace
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
    
    codCol = 0; glUniform1i(codColLocation, codCol);
    
    // Desenam Piramida
    glBindBuffer(GL_ARRAY_BUFFER, VboId2); glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EboId2);
    AssociateAttributePointers();
    glDrawElements(GL_TRIANGLES, 18, GL_UNSIGNED_BYTE, 0);

    //desenare sol
    glBindBuffer(GL_ARRAY_BUFFER, VboId1); glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EboId1);
    AssociateAttributePointers();
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, (void*)(36));// --- 2. CALCUL ROTAȚIE CUB (CUATERNIONI) ---
    // Calculam matricea cubului AICI, ca sa o folosim si la umbra
	if (keyF_down) currentAngle += rotationSpeed;
	if (keyG_down) currentAngle -= rotationSpeed;

	glm::vec3 axis = glm::normalize(glm::vec3(1.0f, 1.0f, 0.0f));
	glm::quat rotationQuat = glm::angleAxis(currentAngle, axis);
	glm::mat4 rotationMatrix = glm::toMat4(rotationQuat);

	// Matricea completa a cubului (Centrare + Rotatie + Pozitionare)
	glm::vec3 centerOffset = glm::vec3(0.0f, 0.0f, 100.0f);
	glm::mat4 modelCube = glm::mat4(1.0f);
	modelCube = glm::translate(modelCube, glm::vec3(0.0f, 0.0f, 100.0f));
	modelCube = modelCube * rotationMatrix;
	modelCube = glm::translate(modelCube, -centerOffset);


	// --- 3. DESENARE UMBRA CUBULUI ---
	// Acum avem matricea "modelCube", o trimitem la shader
	// Shaderul va face: matrUmbra * modelCube * vertex

	codCol = 1; glUniform1i(codColLocation, codCol);
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, &modelCube[0][0]); // <--- AICI E SECRETUL

	// Desenam Umbra (folosind geometria cubului din VboId1)
	// Atentie: Umbra se deseneaza folosind aceiasi indici ca si cubul, dar proiectata pe sol
	// Cubul e in VboId1, indicii 0-36. 
	// In codul tau original umbra era desenata cu "glDrawElements(..., 36, ..., 0)" din VboId1?
	// Verificam VBO-ul: Cubul e la inceputul VboId1.
	glBindBuffer(GL_ARRAY_BUFFER, VboId1); glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EboId1);
	AssociateAttributePointers();

	// In VBO-ul tau, cubul are 36 indici (12 triunghiuri x 3), dar ai desenat doar "36" in codul vechi.
	// Daca cubul e definit prin indicii 0..36 in EboId1:
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_BYTE, 0);


	// --- 4. DESENARE CUB TRANSPARENT (Obiectul propriu-zis) ---
	glEnable(GL_BLEND);
	glDepthMask(GL_FALSE);
	glBlendFunc(GL_SRC_ALPHA, GL_SRC_ALPHA);

	codCol = 0; glUniform1i(codColLocation, codCol);

	// Trimitem din nou matricea cubului (deja calculata)
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, &modelCube[0][0]);

	// Desenam Cubul
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_BYTE, 0);

	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);

	glutSwapBuffers();
	glFlush();
}
void Cleanup(void)
{
	DestroyShaders();
	DestroyVBO();
}
int main(int argc, char* argv[])
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(1200, 900);
	glutCreateWindow("Proiect 3D OpenGL");
	glewInit();
	Initialize();
	glutSetCursor(GLUT_CURSOR_NONE); // ascundem cursorul si il blocam in fereastra
	glutIdleFunc(RenderFunction);
	glutDisplayFunc(RenderFunction);
	glutKeyboardFunc(processNormalKeys);
	glutKeyboardUpFunc(processNormalKeysUp);
	glutPassiveMotionFunc(processMouseMovement);
	glutCloseFunc(Cleanup);
	glutMainLoop();
}