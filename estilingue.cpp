#include <GL/glut.h>
#include <GL/glu.h>
#include <btBulletDynamicsCommon.h>
#include <cmath>
#include <cstdio>
#include <vector>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include "SlingshotManager.h"
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// --- Includes de Áudio (exemplo) ---
// Adicione os includes de áudio aqui se estiver usando (SDL_mixer.h, etc.)
// #include <SDL2/SDL.h>
// #include <SDL2/SDL_mixer.h>
// Mix_Chunk *som_esticar = nullptr;
// Mix_Chunk *som_soltar = nullptr;
// --- Fim Áudio ---


const int WIDTH = 1280;
const int HEIGHT = 720;

// Mundo de física Bullet
btDiscreteDynamicsWorld* dynamicsWorld;
btAlignedObjectArray<btCollisionShape*> collisionShapes;
btCollisionShape* projectileShape = nullptr;
btCollisionShape* boxShape = nullptr;

btBroadphaseInterface* broadphase = nullptr;
btDefaultCollisionConfiguration* collisionConfiguration = nullptr;
btCollisionDispatcher* dispatcher = nullptr;
btSequentialImpulseConstraintSolver* solver = nullptr;

// ... (Structs Material e OBJModel permanecem as mesmas) ...
// (O código das structs Material e OBJModel não foi colado aqui para economizar espaço,
// mas ele deve permanecer exatamente como estava no seu arquivo original)
struct Material {
    std::string name;
    float ambient[3] = {0.2f, 0.2f, 0.2f};
    float diffuse[3] = {0.8f, 0.8f, 0.8f};
    float specular[3] = {0.0f, 0.0f, 0.0f};
    float shininess = 0.0f;
    
    void apply() {
        GLfloat Ka[] = {ambient[0], ambient[1], ambient[2], 1.0f};
        GLfloat Kd[] = {diffuse[0], diffuse[1], diffuse[2], 1.0f};
        GLfloat Ks[] = {specular[0], specular[1], specular[2], 1.0f};
        
        glMaterialfv(GL_FRONT, GL_AMBIENT, Ka);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, Kd);
        glMaterialfv(GL_FRONT, GL_SPECULAR, Ks);
        glMaterialf(GL_FRONT, GL_SHININESS, shininess);
    }
};

struct OBJModel {
    std::vector<float> vertices;
    std::vector<float> normals;
    std::vector<unsigned int> indices;
    std::vector<Material> materials;
    Material currentMaterial;
    bool hasMaterial = false;
    
    bool loadMTL(const char* filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            printf("Aviso: Arquivo MTL %s nao encontrado\n", filename);
            return false;
        }
        
        Material* currentMat = nullptr;
        std::string line;
        
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string prefix;
            iss >> prefix;
            
            if (prefix == "newmtl") {
                materials.push_back(Material());
                currentMat = &materials.back();
                iss >> currentMat->name;
                printf("  Material: %s\n", currentMat->name.c_str());
            }
            else if (currentMat) {
                if (prefix == "Ka") {
                    iss >> currentMat->ambient[0] >> currentMat->ambient[1] >> currentMat->ambient[2];
                }
                else if (prefix == "Kd") {
                    iss >> currentMat->diffuse[0] >> currentMat->diffuse[1] >> currentMat->diffuse[2];
                }
                else if (prefix == "Ks") {
                    iss >> currentMat->specular[0] >> currentMat->specular[1] >> currentMat->specular[2];
                }
                else if (prefix == "Ns") {
                    iss >> currentMat->shininess;
                }
            }
        }
        
        file.close();
        
        if (!materials.empty()) {
            currentMaterial = materials[0];
            hasMaterial = true;
            printf("  Carregados %zu materiais do MTL\n", materials.size());
        }
        
        return true;
    }
    
    bool loadFromFile(const char* filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            printf("Erro: Nao foi possivel abrir arquivo %s\n", filename);
            return false;
        }
        
        std::vector<float> temp_vertices;
        std::vector<float> temp_normals;
        std::vector<unsigned int> vertex_indices, normal_indices;
        
        std::string mtlFilename;
        
        float minX = FLT_MAX, minY = FLT_MAX, minZ = FLT_MAX;
        float maxX = -FLT_MAX, maxY = -FLT_MAX, maxZ = -FLT_MAX;
        
        std::string line;
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string prefix;
            iss >> prefix;
            
            if (prefix == "mtllib") {
                iss >> mtlFilename;
                printf("Arquivo MTL referenciado: %s\n", mtlFilename.c_str());
                
                const char* mtlPaths[] = {
                    (std::string("Objetos/") + mtlFilename).c_str(),
                    (std::string("./Objetos/") + mtlFilename).c_str(),
                    (std::string("../Objetos/") + mtlFilename).c_str(),
                    mtlFilename.c_str()
                };
                
                bool mtlLoaded = false;
                for (int i = 0; i < 4; i++) {
                    if (loadMTL(mtlPaths[i])) {
                        mtlLoaded = true;
                        break;
                    }
                }
                
                if (!mtlLoaded) {
                    loadMTL(mtlFilename.c_str());
                }
            }
            else if (prefix == "usemtl") {
                std::string matName;
                iss >> matName;
                for (auto& mat : materials) {
                    if (mat.name == matName) {
                        currentMaterial = mat;
                        printf("Usando material: %s\n", matName.c_str());
                        break;
                    }
                }
            }
            else if (prefix == "v") {
                float x, y, z;
                iss >> x >> y >> z;
                temp_vertices.push_back(x);
                temp_vertices.push_back(y);
                temp_vertices.push_back(z);
                
                if (x < minX) minX = x;
                if (x > maxX) maxX = x;
                if (y < minY) minY = y;
                if (y > maxY) maxY = y;
                if (z < minZ) minZ = z;
                if (z > maxZ) maxZ = z;
            }
            else if (prefix == "vn") {
                float nx, ny, nz;
                iss >> nx >> ny >> nz;
                temp_normals.push_back(nx);
                temp_normals.push_back(ny);
                temp_normals.push_back(nz);
            }
            else if (prefix == "f") {
                std::string vertex1, vertex2, vertex3;
                iss >> vertex1 >> vertex2 >> vertex3;
                
                auto parseFaceVertex = [](const std::string& str, unsigned int& v_idx, unsigned int& n_idx) {
                    size_t slash1 = str.find('/');
                    if (slash1 == std::string::npos) {
                        v_idx = std::stoi(str);
                        n_idx = 0;
                    } else {
                        v_idx = std::stoi(str.substr(0, slash1));
                        size_t slash2 = str.find('/', slash1 + 1);
                        if (slash2 != std::string::npos && slash2 > slash1 + 1) {
                            n_idx = std::stoi(str.substr(slash2 + 1));
                        } else if (slash2 != std::string::npos) {
                            n_idx = std::stoi(str.substr(slash2 + 1));
                        } else {
                            n_idx = 0;
                        }
                    }
                };
                
                unsigned int v1, v2, v3, n1, n2, n3;
                parseFaceVertex(vertex1, v1, n1);
                parseFaceVertex(vertex2, v2, n2);
                parseFaceVertex(vertex3, v3, n3);
                
                vertex_indices.push_back(v1 - 1);
                vertex_indices.push_back(v2 - 1);
                vertex_indices.push_back(v3 - 1);
                
                if (n1 > 0) normal_indices.push_back(n1 - 1);
                if (n2 > 0) normal_indices.push_back(n2 - 1);
                if (n3 > 0) normal_indices.push_back(n3 - 1);
            }
        }
        
        file.close();
        
        float sizeX = maxX - minX;
        float sizeY = maxY - minY;
        float sizeZ = maxZ - minZ;
        float maxSize = std::max(std::max(sizeX, sizeY), sizeZ);
        
        float normalizeScale = 0.6f / maxSize;
        float centerX = (minX + maxX) / 2.0f;
        float centerY = (minY + maxY) / 2.0f;
        float centerZ = (minZ + maxZ) / 2.0f;
        
        printf("Modelo original: tamanho %.2f x %.2f x %.2f\n", sizeX, sizeY, sizeZ);
        printf("Normalizando com escala: %.4f\n", normalizeScale);
        
        for (size_t i = 0; i < temp_vertices.size(); i += 3) {
            temp_vertices[i] = (temp_vertices[i] - centerX) * normalizeScale;
            temp_vertices[i + 1] = (temp_vertices[i + 1] - centerY) * normalizeScale;
            temp_vertices[i + 2] = (temp_vertices[i + 2] - centerZ) * normalizeScale;
        }
        
        for (size_t i = 0; i < vertex_indices.size(); i++) {
            unsigned int v_idx = vertex_indices[i];
            vertices.push_back(temp_vertices[v_idx * 3]);
            vertices.push_back(temp_vertices[v_idx * 3 + 1]);
            vertices.push_back(temp_vertices[v_idx * 3 + 2]);
            
            if (i < normal_indices.size()) {
                unsigned int n_idx = normal_indices[i];
                normals.push_back(temp_normals[n_idx * 3]);
                normals.push_back(temp_normals[n_idx * 3 + 1]);
                normals.push_back(temp_normals[n_idx * 3 + 2]);
            }
            
            indices.push_back(i);
        }
        
        printf("Modelo OBJ carregado: %zu vertices, %zu triangulos\n", 
               vertices.size() / 3, indices.size() / 3);
        return true;
    }

    // ... (Funções de áudio de OBJModel, se existirem, permanecem) ...
    // NOTE: As funções de áudio estavam *dentro* da sua struct OBJModel,
    // o que é estranho. Eu as removi de dentro da struct.
    // Se elas deveriam estar lá, coloque-as de volta.
    
    void draw() {
        if (vertices.empty()) return;
        
        if (hasMaterial) {
            currentMaterial.apply();
        }
        
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(3, GL_FLOAT, 0, vertices.data());
        
        if (!normals.empty()) {
            glEnableClientState(GL_NORMAL_ARRAY);
            glNormalPointer(GL_FLOAT, 0, normals.data());
        }
        
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, indices.data());
        
        glDisableClientState(GL_VERTEX_ARRAY);
        if (!normals.empty()) {
            glDisableClientState(GL_NORMAL_ARRAY);
        }
    }
};


// Modelos OBJ globais
OBJModel treeModel;
bool treeModelLoaded = false;

OBJModel projectileModel;
bool projectileModelLoaded = false;

// ... (Classe Tree permanece a mesma) ...
class Tree {
public:
    float x, y, z;
    float scale;
    float rotation;
    
    Tree(float posX, float posY, float posZ, float s = 1.0f) 
        : x(posX), y(posY), z(posZ), scale(s) {
        rotation = (rand() % 360);
    }
    
    void draw() {
        glPushMatrix();
        glTranslatef(x, y, z);
        glRotatef(rotation, 0, 1, 0);
        glScalef(scale, scale, scale);
        
        if (treeModelLoaded) {
            glColor3f(0.3f, 0.5f, 0.2f);
            treeModel.draw();
        } else {
            drawProceduralTree();
        }
        
        glPopMatrix();
    }
    
private:
    void drawProceduralTree() {
        float height = 4.0f;
        float trunkRadius = 0.2f;
        float foliageRadius = 1.0f;
        
        glColor3f(0.4f, 0.25f, 0.1f);
        glRotatef(-90, 1, 0, 0);
        GLUquadric* trunk = gluNewQuadric();
        gluCylinder(trunk, trunkRadius, trunkRadius * 0.8f, height * 0.6f, 8, 8);
        gluDeleteQuadric(trunk);
        glRotatef(90, 1, 0, 0);
        
        glColor3f(0.2f, 0.5f, 0.2f);
        glTranslatef(0, height * 0.5f, 0);
        glutSolidSphere(foliageRadius, 12, 12);
        glTranslatef(0, height * 0.1f, 0);
        glColor3f(0.25f, 0.55f, 0.25f);
        glutSolidSphere(foliageRadius * 0.9f, 12, 12);
        glTranslatef(0, height * 0.1f, 0);
        glColor3f(0.3f, 0.6f, 0.3f);
        glutSolidSphere(foliageRadius * 0.7f, 10, 10);
    }
};
std::vector<Tree> trees;


// --- MODIFICADO: Variáveis Globais ---

// A 'struct Slingshot' e variáveis de mouse/estado foram removidas.
// Elas agora estão dentro da classe SlingshotManager.

// NOVO: Ponteiro global para o nosso gerenciador
SlingshotManager* g_slingshotManager = nullptr;


// Variáveis globais de câmera e jogo
float cameraAngleH = 45.0f;
float cameraAngleV = 20.0f;
float cameraDistance = 18.0f;
float cameraTargetY = 3.0f;

int score = 0;
int shotsRemaining = 8;
bool gameOver = false;

std::vector<btRigidBody*> targetBodies;


// --- Funções Globais (Muitas foram movidas para a classe) ---

btRigidBody* createTargetBox(float mass, const btVector3& position) {
    btTransform startTransform;
    startTransform.setIdentity();
    startTransform.setOrigin(position);
    
    btVector3 localInertia(0, 0, 0);
    if (mass != 0.f)
        boxShape->calculateLocalInertia(mass, localInertia);
    
    btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
    btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, boxShape, localInertia);
    btRigidBody* body = new btRigidBody(rbInfo);
    
    body->setFriction(1.0f);
    body->setRestitution(0.1f);
    
    dynamicsWorld->addRigidBody(body);
    targetBodies.push_back(body);
    return body;
}

void initBullet() {
    broadphase = new btDbvtBroadphase();
    collisionConfiguration = new btDefaultCollisionConfiguration();
    dispatcher = new btCollisionDispatcher(collisionConfiguration);
    solver = new btSequentialImpulseConstraintSolver();
    
    dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);
    dynamicsWorld->setGravity(btVector3(0, -9.81f, 0));
    
    btCollisionShape* groundShape = new btStaticPlaneShape(btVector3(0, 1, 0), 0);
    collisionShapes.push_back(groundShape);
    
    btDefaultMotionState* groundMotionState = new btDefaultMotionState(
        btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, 0, 0)));
    
    btRigidBody::btRigidBodyConstructionInfo groundRigidBodyCI(
        0, groundMotionState, groundShape, btVector3(0, 0, 0));
    
    btRigidBody* groundRigidBody = new btRigidBody(groundRigidBodyCI);
    
    groundRigidBody->setFriction(0.8f);
    groundRigidBody->setRestitution(0.3f);
    
    dynamicsWorld->addRigidBody(groundRigidBody);
    
    projectileShape = new btSphereShape(0.3f);
    collisionShapes.push_back(projectileShape);
    
    boxShape = new btBoxShape(btVector3(0.5f, 0.5f, 0.5f));
    collisionShapes.push_back(boxShape);
    
    targetBodies.clear();
    
    float boxMass = 1.0f;
    
    createTargetBox(boxMass, btVector3(-2.0f, 0.5f, -8.0f));
    createTargetBox(boxMass, btVector3(-1.0f, 0.5f, -8.0f));
    createTargetBox(boxMass, btVector3(0.0f, 0.5f, -8.0f));
    createTargetBox(boxMass, btVector3(1.0f, 0.5f, -8.0f));
    createTargetBox(boxMass, btVector3(-0.5f, 1.5f, -8.0f));
    createTargetBox(boxMass, btVector3(0.5f, 1.5f, -8.0f));
    
    createTargetBox(boxMass, btVector3(-3.0f, 0.5f, -12.0f));
    createTargetBox(boxMass, btVector3(-3.0f, 1.5f, -12.0f));
    createTargetBox(boxMass, btVector3(-3.0f, 2.5f, -12.0f));
    createTargetBox(boxMass, btVector3(2.0f, 0.5f, -12.0f));
    createTargetBox(boxMass, btVector3(2.0f, 1.5f, -12.0f));
    
    createTargetBox(boxMass, btVector3(0.0f, 0.5f, -16.0f));
    createTargetBox(boxMass, btVector3(-1.0f, 0.5f, -16.0f));
    createTargetBox(boxMass, btVector3(1.0f, 0.5f, -16.0f));
    createTargetBox(boxMass, btVector3(0.0f, 1.5f, -16.0f));
    
    createTargetBox(boxMass, btVector3(-5.0f, 0.5f, -10.0f));
    createTargetBox(boxMass, btVector3(-5.0f, 1.5f, -10.0f));
    createTargetBox(boxMass, btVector3(5.0f, 0.5f, -14.0f));
    createTargetBox(boxMass, btVector3(5.0f, 1.5f, -14.0f));
    
    trees.clear();
    trees.push_back(Tree(-8.0f, 0.0f, -10.0f, 10.0f));
    trees.push_back(Tree(-9.0f, 0.0f, -10.0f, 12.5f));
    trees.push_back(Tree(8.0f, 0.0f, -25.0f, 12.5f));
    trees.push_back(Tree(29.0f, 0.0f, -12.0f, 12.5f));
    trees.push_back(Tree(-14.0f, 0.0f, -18.0f, 13.5f));
    trees.push_back(Tree(13.5f, 0.0f, -20.0f, 14.8f));
    trees.push_back(Tree(-20.0f, 0.0f, -15.0f, 22.0f));
    trees.push_back(Tree(10.0f, 0.0f, -18.0f, 21.1f));
}

// MODIFICADO: As funções 'clearProjectile', 'createProjectileInPouch',
// 'drawCylinder', 'drawWoodenBase', 'drawElastic', 'drawAimLine',
// 'updateElasticPhysics', 'updatePouchPosition' e 'screenToWorld'
// foram MOVIDAS para dentro da classe SlingshotManager.

// ... (Funções drawSky, drawGround permanecem as mesmas) ...
void drawSky() {
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, 1, 0, 1, -1, 1);
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    glBegin(GL_QUADS);
    glColor3f(0.4f, 0.6f, 0.9f);
    glVertex2f(0, 1);
    glVertex2f(1, 1);
    glColor3f(0.7f, 0.85f, 0.95f);
    glVertex2f(1, 0.3f);
    glVertex2f(0, 0.3f);
    glEnd();
    
    glColor3f(1.0f, 0.95f, 0.7f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(0.8f, 0.8f);
    for (int i = 0; i <= 20; i++) {
        float angle = (float)i / 20.0f * 2.0f * M_PI;
        glVertex2f(0.8f + cos(angle) * 0.08f, 0.8f + sin(angle) * 0.08f);
    }
    glEnd();
    
    glColor4f(1.0f, 1.0f, 1.0f, 0.7f);
    
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(0.2f, 0.85f);
    for (int i = 0; i <= 20; i++) {
        float angle = (float)i / 20.0f * 2.0f * M_PI;
        glVertex2f(0.2f + cos(angle) * 0.06f, 0.85f + sin(angle) * 0.03f);
    }
    glEnd();
    
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(0.25f, 0.87f);
    for (int i = 0; i <= 20; i++) {
        float angle = (float)i / 20.0f * 2.0f * M_PI;
        glVertex2f(0.25f + cos(angle) * 0.05f, 0.87f + sin(angle) * 0.025f);
    }
    glEnd();
    
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(0.55f, 0.9f);
    for (int i = 0; i <= 20; i++) {
        float angle = (float)i / 20.0f * 2.0f * M_PI;
        glVertex2f(0.55f + cos(angle) * 0.07f, 0.9f + sin(angle) * 0.035f);
    }
    glEnd();
    
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(0.6f, 0.88f);
    for (int i = 0; i <= 20; i++) {
        float angle = (float)i / 20.0f * 2.0f * M_PI;
        glVertex2f(0.6f + cos(angle) * 0.05f, 0.88f + sin(angle) * 0.025f);
    }
    glEnd();
    
    glColor3f(0.5f, 0.6f, 0.7f);
    glBegin(GL_TRIANGLES);
    glVertex2f(0.0f, 0.3f);
    glVertex2f(0.2f, 0.55f);
    glVertex2f(0.4f, 0.3f);
    
    glVertex2f(0.3f, 0.3f);
    glVertex2f(0.5f, 0.6f);
    glVertex2f(0.7f, 0.3f);
    
    glVertex2f(0.6f, 0.3f);
    glVertex2f(0.85f, 0.5f);
    glVertex2f(1.0f, 0.3f);
    glEnd();
    
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

void drawGround() {
    glDisable(GL_LIGHTING);
    
    glColor3f(0.3f, 0.6f, 0.3f);
    glBegin(GL_QUADS);
    glVertex3f(-40.0f, 0.0f, -40.0f);
    glVertex3f(-40.0f, 0.0f, 40.0f);
    glVertex3f(40.0f, 0.0f, 40.0f);
    glVertex3f(40.0f, 0.0f, -40.0f);
    glEnd();
    
    glColor3f(0.25f, 0.5f, 0.25f);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    for (float x = -40.0f; x <= 40.0f; x += 2.0f) {
        glVertex3f(x, 0.01f, -40.0f);
        glVertex3f(x, 0.01f, 40.0f);
    }
    for (float z = -40.0f; z <= 40.0f; z += 2.0f) {
        glVertex3f(-40.0f, 0.01f, z);
        glVertex3f(40.0f, 0.01f, z);
    }
    glEnd();
    
    glEnable(GL_LIGHTING);
}

// MODIFICADO: 'isTargetInAimLine' foi movida para a classe SlingshotManager

void drawHUD() {
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, WIDTH, 0, HEIGHT);
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
    glBegin(GL_QUADS);
    glVertex2f(10, HEIGHT - 140);
    glVertex2f(350, HEIGHT - 140);
    glVertex2f(350, HEIGHT - 10);
    glVertex2f(10, HEIGHT - 10);
    glEnd();
    
    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2f(20, HEIGHT - 30);
    char scoreText[50];
    sprintf(scoreText, "Pontos: %d", score);
    for (char* c = scoreText; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }
    
    glRasterPos2f(20, HEIGHT - 60);
    char shotsText[50];
    sprintf(shotsText, "Tiros: %d", shotsRemaining);
    for (char* c = shotsText; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }
    
    glColor3f(0.8f, 0.8f, 0.8f);
    glRasterPos2f(20, HEIGHT - 90);
    char* controlText1 = (char*)"Q/E: Aumentar/Diminuir Profundidade";
    for (char* c = controlText1; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }
    
    glRasterPos2f(20, HEIGHT - 110);
    char depthText[50];
    // MODIFICADO: Não podemos mais ler 'pullDepth' globalmente.
    // Esta é uma limitação da refatoração, a menos que
    // adicionemos um 'getPullDepth()' ao manager.
    // Por enquanto, vamos remover esta linha ou comentá-la.
    // sprintf(depthText, "Profundidade: %.1f", pullDepth); // ANTIGO
    sprintf(depthText, "Profundidade: (pressione Q/E)"); // NOVO
    for (char* c = depthText; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }
    
    if (gameOver) {
        glColor4f(0.0f, 0.0f, 0.0f, 0.7f);
        glBegin(GL_QUADS);
        glVertex2f(WIDTH/2 - 200, HEIGHT/2 - 100);
        glVertex2f(WIDTH/2 + 200, HEIGHT/2 - 100);
        glVertex2f(WIDTH/2 + 200, HEIGHT/2 + 100);
        glVertex2f(WIDTH/2 - 200, HEIGHT/2 + 100);
        glEnd();
        
        glColor3f(1.0f, 1.0f, 0.0f);
        glRasterPos2f(WIDTH/2 - 80, HEIGHT/2 + 30);
        char* gameOverText = (char*)"FIM DE JOGO!";
        for (char* c = gameOverText; *c != '\0'; c++) {
            glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *c);
        }
        
        glColor3f(1.0f, 1.0f, 1.0f);
        glRasterPos2f(WIDTH/2 - 100, HEIGHT/2 - 10);
        char finalScoreText[50];
        sprintf(finalScoreText, "Pontuacao Final: %d", score);
        for (char* c = finalScoreText; *c != '\0'; c++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
        }
        
        glRasterPos2f(WIDTH/2 - 120, HEIGHT/2 - 50);
        char* restartText = (char*)"Pressione R para reiniciar";
        for (char* c = restartText; *c != '\0'; c++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
        }
    }
    
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}


// --- MODIFICADO: Callbacks de Eventos ---

void mouse(int button, int state, int x, int y) {
    // Delega o evento para o gerenciador
    if (g_slingshotManager) {
        g_slingshotManager->handleMouseClick(button, state, x, y);
    }
}

void mouseMotion(int x, int y) {
    // Delega o evento para o gerenciador
    if (g_slingshotManager) {
        g_slingshotManager->handleMouseDrag(x, y);
    }
}

void passiveMouseMotion(int x, int y) {
    // Delega o evento para o gerenciador
    if (g_slingshotManager) {
        g_slingshotManager->handlePassiveMouseMotion(x, y);
    }
}

void keyboard(unsigned char key, int x, int y) {
    // Primeiro, delega as teclas do estilingue
    if (g_slingshotManager) {
        g_slingshotManager->handleKeyboard(key);
    }

    // Teclas globais do jogo
    switch(key) {
        case 27: // ESC
            exit(0);
            break;
            
        // Q e E foram movidos para o handleKeyboard do manager
            
        case 'r':
        case 'R':
            // Lógica de reset global
            score = 0;
            shotsRemaining = 8;
            gameOver = false;
            
            for (int i = dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--) {
                btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[i];
                btRigidBody* body = btRigidBody::upcast(obj);
                
                if (body && body->getInvMass() != 0) {
                    dynamicsWorld->removeRigidBody(body);
                    delete body->getMotionState();
                    delete body;
                }
            }
            
            initBullet(); // Recria os alvos
            
            // NOVO: Reseta o estado do estilingue
            if (g_slingshotManager) {
                g_slingshotManager->reset();
            }
            
            printf("Jogo reiniciado!\n");
            break;
    }
}

void specialKeys(int key, int x, int y) {
    // Controles da câmera permanecem globais
    switch(key) {
        case GLUT_KEY_LEFT:
            cameraAngleH -= 3.0f;
            break;
        case GLUT_KEY_RIGHT:
            cameraAngleH += 3.0f;
            break;
        case GLUT_KEY_UP:
            cameraAngleV += 2.0f;
            if (cameraAngleV > 60.0f) cameraAngleV = 60.0f;
            break;
        case GLUT_KEY_DOWN:
            cameraAngleV -= 2.0f;
            if (cameraAngleV < 5.0f) cameraAngleV = 5.0f;
            break;
        case GLUT_KEY_PAGE_UP:
            cameraDistance -= 1.0f;
            if (cameraDistance < 8.0f) cameraDistance = 8.0f;
            break;
        case GLUT_KEY_PAGE_DOWN:
            cameraDistance += 1.0f;
            if (cameraDistance > 30.0f) cameraDistance = 30.0f;
            break;
    }
}

// --- MODIFICADO: Funções Principais (Display, Timer, Init) ---

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    drawSky();
    
    glLoadIdentity();
    
    float angleH_rad = cameraAngleH * M_PI / 180.0f;
    float angleV_rad = cameraAngleV * M_PI / 180.0f;
    
    float camX = cameraDistance * sin(angleH_rad) * cos(angleV_rad);
    float camY = cameraDistance * sin(angleV_rad);
    float camZ = cameraDistance * cos(angleH_rad) * cos(angleV_rad);
    
    gluLookAt(camX, camY + cameraTargetY, camZ,
              0.0f, cameraTargetY, -8.0f,
              0.0f, 1.0f, 0.0f);
    
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    
    GLfloat lightPos[] = {10.0f, 15.0f, 10.0f, 1.0f};
    GLfloat lightAmb[] = {0.4f, 0.4f, 0.4f, 1.0f};
    GLfloat lightDif[] = {1.0f, 1.0f, 0.9f, 1.0f};
    GLfloat lightSpec[] = {0.8f, 0.8f, 0.8f, 1.0f};
    
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDif);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpec);
    
    drawGround();
    
    for (auto& tree : trees) {
        tree.draw();
    }
    
    // MODIFICADO: Chama o 'draw' do gerenciador
    if (g_slingshotManager) {
        g_slingshotManager->draw();
    }
    
    // Desenha todos os corpos rígidos no mundo
    for (int i = dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--) {
        btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[i];
        btRigidBody* body = btRigidBody::upcast(obj);
        
        if (body && body->getMotionState() && body->getInvMass() != 0) {
            btTransform trans;
            body->getMotionState()->getWorldTransform(trans);
            btScalar m[16];
            trans.getOpenGLMatrix(m);
            
            glPushMatrix();
            glMultMatrixf(m);
            
            btCollisionShape* shape = body->getCollisionShape();
            
            // MODIFICADO: Verifica se o corpo é o projétil ATUAL
            btRigidBody* currentProjectile = g_slingshotManager ? g_slingshotManager->getProjectileInPouch() : nullptr;

            if (body == currentProjectile) {
                // É o projétil na malha (cinemático)
                glMaterialf(GL_FRONT, GL_SHININESS, 0.0f);
                
                if (projectileModelLoaded) {
                    glColor3f(0.8f, 0.2f, 0.2f);
                    glPushMatrix();
                    glRotatef(180, 0, 1, 0);
                    projectileModel.draw();
                    glPopMatrix();
                } else {
                    glColor3f(0.8f, 0.2f, 0.2f);
                    glutSolidSphere(0.3, 20, 20);
                }
            }
            else if (shape == projectileShape) {
                // É um projétil já lançado (dinâmico)
                glMaterialf(GL_FRONT, GL_SHININESS, 50.0f);
                
                if (projectileModelLoaded) {
                    glColor3f(0.8f, 0.2f, 0.2f);
                    projectileModel.draw();
                } else {
                    glColor3f(0.8f, 0.2f, 0.2f);
                    glutSolidSphere(0.3, 16, 16);
                }
            }
            else if (shape == boxShape) {
                // É um alvo (caixa)
                glMaterialf(GL_FRONT, GL_SHININESS, 5.0f);
                
                // MODIFICADO: Chama a função de mira do gerenciador
                if (g_slingshotManager && g_slingshotManager->isTargetInAimLine(body)) {
                    glColor3f(0.2f, 1.0f, 0.2f);
                } else {
                    glColor3f(0.6f, 0.4f, 0.2f);
                }
                
                glutSolidCube(1.0);
                
                glDisable(GL_LIGHTING);
                glColor3f(0.3f, 0.2f, 0.1f);
                glLineWidth(2.0f);
                
                glBegin(GL_LINES);
                glVertex3f(-0.5f, 0.0f, -0.5f);
                glVertex3f(0.5f, 0.0f, -0.5f);
                glVertex3f(-0.5f, 0.0f, 0.5f);
                glVertex3f(0.5f, 0.0f, 0.5f);
                glEnd();
                
                glEnable(GL_LIGHTING);
            }
            
            glPopMatrix();
        }
    }
    
    glMaterialf(GL_FRONT, GL_SHININESS, 10.0f);
    
    // MODIFICADO: 'updateElasticPhysics' é chamado pelo 'update' do manager no 'timer'
    drawHUD();
    
    glutSwapBuffers();
}

void timer(int value) {
    // Simula a física
    dynamicsWorld->stepSimulation(1.0f / 60.0f, 10);
    
    // NOVO: Atualiza a física do estilingue (ex: malha voltando)
    if (g_slingshotManager) {
        g_slingshotManager->update();
    }
    
    // Lógica de pontuação (permanece global)
    for (int i = dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--) {
        btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[i];
        btRigidBody* body = btRigidBody::upcast(obj);
        
        if (body && body->getCollisionShape() == boxShape) {
            btTransform trans;
            body->getMotionState()->getWorldTransform(trans);
            float y = trans.getOrigin().getY();
            
            if (y < -2.0f) {
                score += 100;
                
                auto it = std::find(targetBodies.begin(), targetBodies.end(), body);
                if (it != targetBodies.end()) {
                    targetBodies.erase(it);
                }
                
                dynamicsWorld->removeRigidBody(body);
                delete body->getMotionState();
                delete body;
            }
        }
    }
    
    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)w / (double)h, 0.1, 100.0);
    glMatrixMode(GL_MODELVIEW);
}

void init() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glClearColor(0.7f, 0.85f, 0.95f, 1.0f);
    
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    
    GLfloat spec[] = {0.3f, 0.3f, 0.3f, 1.0f};
    glMaterialfv(GL_FRONT, GL_SPECULAR, spec);
    glMaterialf(GL_FRONT, GL_SHININESS, 20.0f);
    
    // MODIFICADO: A inicialização das posições do estilingue foi
    // movida para o construtor do SlingshotManager.
    
    initBullet();
    
    // NOVO: Cria a instância do gerenciador do estilingue
    // Passamos ponteiros para os sistemas que ele precisa controlar
    g_slingshotManager = new SlingshotManager(dynamicsWorld, projectileShape, &shotsRemaining, &gameOver);

    
    // ... (Carregamento dos modelos OBJ permanece o mesmo) ...
    printf("Tentando carregar modelo red.obj do projetil...\n");
    const char* projectilePaths[] = {
        "Objetos/red.obj", "./Objetos/red.obj", "../Objetos/red.obj", "red.obj"
    };
    projectileModelLoaded = false;
    for (const char* path : projectilePaths) {
        printf("  Tentando: %s\n", path);
        if (projectileModel.loadFromFile(path)) {
            projectileModelLoaded = true;
            printf("  ✓ Modelo red.obj carregado com sucesso de: %s\n", path);
            break;
        }
    }
    if (!projectileModelLoaded) {
        printf("  Modelo red.obj nao encontrado. Usando esfera vermelha padrao.\n");
    }
    
    printf("\nTentando carregar modelo OBJ da arvore...\n");
    const char* possiblePaths[] = {
        "Objetos/arvore.obj", "./Objetos/arvore.obj", "../Objetos/arvore.obj", "arvore.obj", "tree.obj"
    };
    treeModelLoaded = false;
    for (const char* path : possiblePaths) {
        printf("  Tentando: %s\n", path);
        if (treeModel.loadFromFile(path)) {
            treeModelLoaded = true;
            printf("  ✓ Modelo carregado com sucesso de: %s\n", path);
            break;
        }
    }
    if (!treeModelLoaded) {
        printf("  Modelo OBJ nao encontrado em nenhum caminho. Usando geometria procedural.\n");
    }
    
    // ... (Impressão dos controles permanece a mesma) ...
    printf("\n=== ESTILINGUE 3D MELHORADO ===\n");
    printf("CONTROLES:\n");
    printf("  Mouse Esquerdo: Clique e arraste para mirar\n");
    printf("  Q: Aumentar profundidade (puxar para frente)\n");
    printf("  E: Diminuir profundidade (voltar)\n");
    printf("  Setas Esquerda/Direita: Rotacionar camera\n");
    printf("  Setas Cima/Baixo: Angulo vertical da camera\n");
    printf("  Page Up/Down: Zoom da camera\n");
    printf("  R: Reiniciar jogo\n");
    printf("  ESC: Sair\n");
    printf("\nRECURSOS:\n");
    printf("  - Arvores decorativas (modelo OBJ ou procedural)\n");
    printf("  - Projetil RED.OBJ (modelo 3D personalizado)\n");
    printf("  - Fundo com sol, nuvens e montanhas\n");
    printf("  - Alvos ficam VERDES quando na mira\n");
    printf("===================================\n\n");
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(WIDTH, HEIGHT);
    glutCreateWindow("Estilingue 3D - Refatorado com Classe");
    
    // if (!init_audio()) return -1; // Exemplo de áudio
    // if (!load_sounds()) return -1; // Exemplo de áudio
    
    init();
    
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMotionFunc(mouseMotion);
    glutPassiveMotionFunc(passiveMouseMotion);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutTimerFunc(0, timer, 0);
    
    glutMainLoop();
    
    // --- Limpeza ---
    
    // MODIFICADO: Não precisamos mais do 'clearProjectile()' global
    
    for (int i = dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--) {
        btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[i];
        btRigidBody* body = btRigidBody::upcast(obj);
        
        if (body && body->getMotionState()) {
            delete body->getMotionState();
        }
        dynamicsWorld->removeCollisionObject(obj);
        delete obj;
    }
    
    for (int i = 0; i < collisionShapes.size(); i++) {
        delete collisionShapes[i];
    }
    
    // NOVO: Limpa o gerenciador do estilingue
    delete g_slingshotManager;
    
    delete dynamicsWorld;
    delete solver;
    delete dispatcher;
    delete collisionConfiguration;
    delete broadphase;

    // close_audio(); // Exemplo de áudio
    
    return 0;
}