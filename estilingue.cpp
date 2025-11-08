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
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

const int WIDTH = 1280;
const int HEIGHT = 720;

// Mundo de fÃ­sica Bullet
btDiscreteDynamicsWorld* dynamicsWorld;
btAlignedObjectArray<btCollisionShape*> collisionShapes;
btCollisionShape* projectileShape = nullptr;
btRigidBody* projectileBody = nullptr;
btCollisionShape* boxShape = nullptr;

btBroadphaseInterface* broadphase = nullptr;
btDefaultCollisionConfiguration* collisionConfiguration = nullptr;
btCollisionDispatcher* dispatcher = nullptr;
btSequentialImpulseConstraintSolver* solver = nullptr;

// NOVO: Estrutura para Material MTL
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

// NOVO: Estrutura para carregar e renderizar modelos OBJ com MTL
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

OBJModel projectileModel;  // NOVO: Modelo do projÃ©til (red.obj)
bool projectileModelLoaded = false;

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

struct Slingshot {
    float baseX, baseY, baseZ;
    float leftArmX, leftArmY, leftArmZ;
    float rightArmX, rightArmY, rightArmZ;
    float leftTopX, leftTopY, leftTopZ;
    float rightTopX, rightTopY, rightTopZ;
    float pouchX, pouchY, pouchZ;
    bool isPulling;
} slingshot;

int mouseX = 0, mouseY = 0;
bool mousePressed = false;

int startMouseX = 0;
int startMouseY = 0;
float startPouchX = 0.0f;
float startPouchY = 0.0f;
float startPouchZ = 0.0f;

float cameraAngleH = 45.0f;
float cameraAngleV = 20.0f;
float cameraDistance = 18.0f;
float cameraTargetY = 3.0f;

int score = 0;
int shotsRemaining = 8;
bool gameOver = false;

float pullDepth = 0.0f;

std::vector<btRigidBody*> targetBodies;

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
    trees.clear();
    trees.push_back(Tree(-8.0f, 0.0f, -10.0f, 10.0f));  // Reduzido de 5.0 para 0.5
    trees.push_back(Tree(-9.0f, 0.0f, -10.0f, 12.5f));
    trees.push_back(Tree(8.0f, 0.0f, -25.0f, 12.5f));
    trees.push_back(Tree(29.0f, 0.0f, -12.0f, 12.5f));
    trees.push_back(Tree(-14.0f, 0.0f, -18.0f, 13.5f));
    trees.push_back(Tree(13.5f, 0.0f, -20.0f, 14.8f));
    trees.push_back(Tree(-20.0f, 0.0f, -15.0f, 22.0f));
    trees.push_back(Tree(10.0f, 0.0f, -18.0f, 21.1f));
}

void clearProjectile() {
    if (projectileBody) {
        dynamicsWorld->removeRigidBody(projectileBody);
        delete projectileBody->getMotionState();
        delete projectileBody;
        projectileBody = nullptr;
    }
}

void createProjectileInPouch() {
    clearProjectile();
    
    float restX = (slingshot.leftTopX + slingshot.rightTopX) / 2.0f;
    float restY = (slingshot.leftTopY + slingshot.rightTopY) / 2.0f;
    float restZ = (slingshot.leftTopZ + slingshot.rightTopZ) / 2.0f;
    
    slingshot.pouchX = restX;
    slingshot.pouchY = restY;
    slingshot.pouchZ = restZ;
    
    btDefaultMotionState* motionState = new btDefaultMotionState(
        btTransform(btQuaternion(0, 0, 0, 1), btVector3(restX, restY, restZ)));
    
    btVector3 localInertia(0, 0, 0);
    float mass = 1.0f;
    projectileShape->calculateLocalInertia(mass, localInertia);
    
    btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, motionState, projectileShape, localInertia);
    projectileBody = new btRigidBody(rbInfo);
    
    projectileBody->setRestitution(0.6f);
    projectileBody->setFriction(0.5f);
    projectileBody->setRollingFriction(0.1f);
    
    projectileBody->setCollisionFlags(projectileBody->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
    projectileBody->setActivationState(DISABLE_DEACTIVATION);
    
    dynamicsWorld->addRigidBody(projectileBody);
}

void drawCylinder(float x1, float y1, float z1, float x2, float y2, float z2, float radius) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    float dz = z2 - z1;
    float length = sqrt(dx*dx + dy*dy + dz*dz);
    
    if (length == 0.0) return;
    
    float z_axis[3] = {0.0, 0.0, 1.0};
    float vector[3] = {dx, dy, dz};
    vector[0] /= length;
    vector[1] /= length;
    vector[2] /= length;
    
    float axis[3];
    axis[0] = z_axis[1] * vector[2] - z_axis[2] * vector[1];
    axis[1] = z_axis[2] * vector[0] - z_axis[0] * vector[2];
    axis[2] = z_axis[0] * vector[1] - z_axis[1] * vector[0];
    
    float dot_product = z_axis[0]*vector[0] + z_axis[1]*vector[1] + z_axis[2]*vector[2];
    float angle = acos(dot_product) * 180.0 / M_PI;
    
    if (dot_product < -0.99999) {
        axis[0] = 1.0;
        axis[1] = 0.0;
        axis[2] = 0.0;
        angle = 180.0;
    }
    
    glPushMatrix();
    glTranslatef(x1, y1, z1);
    glRotatef(angle, axis[0], axis[1], axis[2]);
    GLUquadric* quad = gluNewQuadric();
    gluCylinder(quad, radius, radius, length, 16, 16);
    gluDeleteQuadric(quad);
    glPopMatrix();
}

void drawWoodenBase() {
    glColor3f(0.5f, 0.35f, 0.05f);
    
    drawCylinder(slingshot.baseX, slingshot.baseY, slingshot.baseZ,
                 slingshot.leftArmX, slingshot.leftArmY, slingshot.leftArmZ,
                 0.3);
    
    drawCylinder(slingshot.leftArmX, slingshot.leftArmY, slingshot.leftArmZ,
                 slingshot.leftTopX, slingshot.leftTopY, slingshot.leftTopZ,
                 0.2);
    
    drawCylinder(slingshot.rightArmX, slingshot.rightArmY, slingshot.rightArmZ,
                 slingshot.rightTopX, slingshot.rightTopY, slingshot.rightTopZ,
                 0.2);
}

void drawElastic() {
    glColor3f(0.05f, 0.05f, 0.05f);
    glLineWidth(12.0f);
    glBegin(GL_LINES);
    glVertex3f(slingshot.leftTopX, slingshot.leftTopY, slingshot.leftTopZ);
    glVertex3f(slingshot.pouchX, slingshot.pouchY, slingshot.pouchZ);
    glVertex3f(slingshot.rightTopX, slingshot.rightTopY, slingshot.rightTopZ);
    glVertex3f(slingshot.pouchX, slingshot.pouchY, slingshot.pouchZ);
    glEnd();
}

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

bool isTargetInAimLine(btRigidBody* target) {
    if (!slingshot.isPulling || !projectileBody) return false;
    
    float restX = (slingshot.leftTopX + slingshot.rightTopX) / 2.0f;
    float restY = (slingshot.leftTopY + slingshot.rightTopY) / 2.0f;
    float restZ = (slingshot.leftTopZ + slingshot.rightTopZ) / 2.0f;
    
    float dirX = restX - slingshot.pouchX;
    float dirY = restY - slingshot.pouchY;
    float dirZ = restZ - slingshot.pouchZ;
    
    float length = sqrt(dirX*dirX + dirY*dirY + dirZ*dirZ);
    if (length < 0.01f) return false;
    
    dirX /= length;
    dirY /= length;
    dirZ /= length;
    
    btTransform trans;
    target->getMotionState()->getWorldTransform(trans);
    btVector3 targetPos = trans.getOrigin();
    
    float toTargetX = targetPos.x() - slingshot.pouchX;
    float toTargetY = targetPos.y() - slingshot.pouchY;
    float toTargetZ = targetPos.z() - slingshot.pouchZ;
    
    float targetDist = sqrt(toTargetX*toTargetX + toTargetY*toTargetY + toTargetZ*toTargetZ);
    if (targetDist < 0.01f) return false;
    
    toTargetX /= targetDist;
    toTargetY /= targetDist;
    toTargetZ /= targetDist;
    
    float dot = dirX * toTargetX + dirY * toTargetY + dirZ * toTargetZ;
    
    return dot > 0.95f;
}

void drawAimLine() {
    if (slingshot.isPulling && projectileBody) {
        glDisable(GL_LIGHTING);
        
        float restX = (slingshot.leftTopX + slingshot.rightTopX) / 2.0f;
        float restY = (slingshot.leftTopY + slingshot.rightTopY) / 2.0f;
        float restZ = (slingshot.leftTopZ + slingshot.rightTopZ) / 2.0f;
        
        float dirX = restX - slingshot.pouchX;
        float dirY = restY - slingshot.pouchY;
        float dirZ = restZ - slingshot.pouchZ;
        
        float length = sqrt(dirX*dirX + dirY*dirY + dirZ*dirZ);
        if (length > 0.01f) {
            dirX /= length;
            dirY /= length;
            dirZ /= length;
            
            glColor4f(1.0f, 1.0f, 0.0f, 0.8f);
            glLineWidth(3.0f);
            glEnable(GL_LINE_STIPPLE);
            glLineStipple(3, 0xAAAA);
            
            glBegin(GL_LINES);
            glVertex3f(slingshot.pouchX, slingshot.pouchY, slingshot.pouchZ);
            glVertex3f(slingshot.pouchX + dirX * 15.0f, 
                      slingshot.pouchY + dirY * 15.0f, 
                      slingshot.pouchZ + dirZ * 15.0f);
            glEnd();
            
            glDisable(GL_LINE_STIPPLE);
        }
        
        glEnable(GL_LIGHTING);
    }
}

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
    sprintf(depthText, "Profundidade: %.1f", pullDepth);
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

void updateElasticPhysics() {
    if (!slingshot.isPulling && !projectileBody) {
        float restX = (slingshot.leftTopX + slingshot.rightTopX) / 2.0f;
        float restY = (slingshot.leftTopY + slingshot.rightTopY) / 2.0f;
        float restZ = (slingshot.leftTopZ + slingshot.rightTopZ) / 2.0f;
        
        float dx = slingshot.pouchX - restX;
        float dy = slingshot.pouchY - restY;
        float dz = slingshot.pouchZ - restZ;
        
        float k = 25.0f;
        static float vx = 0.0f, vy = 0.0f, vz = 0.0f;
        float damping = 0.88f;
        
        float fx = -k * dx;
        float fy = -k * dy;
        float fz = -k * dz;
        
        float mass = 0.08f;
        float ax = fx / mass;
        float ay = fy / mass;
        float az = fz / mass;
        
        float dt = 0.016f;
        vx += ax * dt;
        vy += ay * dt;
        vz += az * dt;
        
        vx *= damping;
        vy *= damping;
        vz *= damping;
        
        slingshot.pouchX += vx * dt;
        slingshot.pouchY += vy * dt;
        slingshot.pouchZ += vz * dt;
        
        float distance = sqrt(dx*dx + dy*dy + dz*dz);
        if (distance < 0.05f && sqrt(vx*vx + vy*vy + vz*vz) < 0.3f) {
            slingshot.pouchX = restX;
            slingshot.pouchY = restY;
            slingshot.pouchZ = restZ;
            vx = vy = vz = 0.0f;
        }
    }
}

void updatePouchPosition() {
    if (mousePressed && slingshot.isPulling) {
        int deltaX_pixels = mouseX - startMouseX;
        int deltaY_pixels = mouseY - startMouseY;
        
        float sensitivityX = 0.04f;
        float sensitivityY = 0.04f;
        
        float dx = (float)deltaX_pixels * sensitivityX;
        float dy = (float)-deltaY_pixels * sensitivityY;
        float dz = pullDepth;
        
        float maxPull = 7.0f;
        float currentDist = sqrt(dx*dx + dy*dy + dz*dz);
        
        if (currentDist > maxPull) {
            dx = (dx / currentDist) * maxPull;
            dy = (dy / currentDist) * maxPull;
            dz = (dz / currentDist) * maxPull;
        }
        
        slingshot.pouchX = startPouchX + dx;
        slingshot.pouchY = startPouchY + dy;
        slingshot.pouchZ = startPouchZ + dz;
        
        if (projectileBody) {
            btTransform trans;
            projectileBody->getMotionState()->getWorldTransform(trans);
            trans.setOrigin(btVector3(slingshot.pouchX, slingshot.pouchY, slingshot.pouchZ));
            projectileBody->getMotionState()->setWorldTransform(trans);
            projectileBody->setWorldTransform(trans);
        }
    }
}

void screenToWorld(int screenX, int screenY, float depth, float& worldX, float& worldY, float& worldZ) {
    GLint viewport[4];
    GLdouble modelview[16];
    GLdouble projection[16];
    GLdouble posX, posY, posZ;
    
    glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
    glGetDoublev(GL_PROJECTION_MATRIX, projection);
    glGetIntegerv(GL_VIEWPORT, viewport);
    
    GLfloat winX = (float)screenX;
    GLfloat winY = (float)viewport[3] - (float)screenY;
    
    gluUnProject(winX, winY, depth, modelview, projection, viewport, &posX, &posY, &posZ);
    
    worldX = posX;
    worldY = posY;
    worldZ = posZ;
}

void mouse(int button, int state, int x, int y) {
    if (gameOver) return;
    
    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) {
            float wx, wy, wz;
            bool found = false;
            
            float currentPouchX = (slingshot.leftTopX + slingshot.rightTopX) / 2.0f;
            float currentPouchY = (slingshot.leftTopY + slingshot.rightTopY) / 2.0f;
            float currentPouchZ = (slingshot.leftTopZ + slingshot.rightTopZ) / 2.0f;
            
            GLdouble modelview[16];
            GLdouble projection[16];
            GLint viewport[4];
            GLdouble screenX, screenY, screenZ;
            
            glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
            glGetDoublev(GL_PROJECTION_MATRIX, projection);
            glGetIntegerv(GL_VIEWPORT, viewport);
            
            gluProject(currentPouchX, currentPouchY, currentPouchZ,
                      modelview, projection, viewport,
                      &screenX, &screenY, &screenZ);
            
            screenToWorld(x, y, (float)screenZ, wx, wy, wz);
            
            float dist_x = fabs(wx - currentPouchX);
            float dist_y = fabs(wy - currentPouchY);
            
            if (dist_x < 1.5f && dist_y < 1.0f) {
                found = true;
            }
            
            if (found && shotsRemaining > 0) {
                printf("Estilingue capturado! Arraste e use Q/E para profundidade.\n");
                mousePressed = true;
                slingshot.isPulling = true;
                createProjectileInPouch();
                startMouseX = x;
                startMouseY = y;
                startPouchX = slingshot.pouchX;
                startPouchY = slingshot.pouchY;
                startPouchZ = slingshot.pouchZ;
                pullDepth = 0.0f;
            }
            
        } else {
            if (mousePressed && slingshot.isPulling && projectileBody) {
                printf("Lancando projetil!\n");
                
                float restX = (slingshot.leftTopX + slingshot.rightTopX) / 2.0f;
                float restY = (slingshot.leftTopY + slingshot.rightTopY) / 2.0f;
                float restZ = (slingshot.leftTopZ + slingshot.rightTopZ) / 2.0f;
                
                float impulseX = restX - slingshot.pouchX;
                float impulseY = restY - slingshot.pouchY;
                float impulseZ = restZ - slingshot.pouchZ;
                
                projectileBody->setCollisionFlags(projectileBody->getCollisionFlags() & ~btCollisionObject::CF_KINEMATIC_OBJECT);
                projectileBody->forceActivationState(ACTIVE_TAG);
                
                projectileBody->setRestitution(0.6f);
                projectileBody->setFriction(0.5f);
                projectileBody->setRollingFriction(0.1f);
                projectileBody->setDamping(0.05f, 0.1f);
                
                projectileBody->setLinearVelocity(btVector3(0,0,0));
                projectileBody->setAngularVelocity(btVector3(0,0,0));
                
                float forceMagnitude = 20.0f;
                btVector3 impulse(impulseX * forceMagnitude, impulseY * forceMagnitude, impulseZ * forceMagnitude);
                projectileBody->applyCentralImpulse(impulse);
                
                projectileBody = nullptr;
                shotsRemaining--;
                
                if (shotsRemaining <= 0) {
                    gameOver = true;
                }
            }
            
            mousePressed = false;
            slingshot.isPulling = false;
            pullDepth = 0.0f;
        }
    }
}

void mouseMotion(int x, int y) {
    mouseX = x;
    mouseY = y;
    updatePouchPosition();
}

void passiveMouseMotion(int x, int y) {
    mouseX = x;
    mouseY = y;
}

void keyboard(unsigned char key, int x, int y) {
    switch(key) {
        case 27:
            exit(0);
            break;
            
        case 'q':
        case 'Q':
            if (slingshot.isPulling) {
                pullDepth += 0.3f;
                if (pullDepth > 6.0f) pullDepth = 6.0f;
                updatePouchPosition();
            }
            break;
            
        case 'e':
        case 'E':
            if (slingshot.isPulling) {
                pullDepth -= 0.3f;
                if (pullDepth < 0.0f) pullDepth = 0.0f;
                updatePouchPosition();
            }
            break;
            
        case 'r':
        case 'R':
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
            
            initBullet();
            
            clearProjectile();
            slingshot.pouchX = (slingshot.leftTopX + slingshot.rightTopX) / 2.0f;
            slingshot.pouchY = (slingshot.leftTopY + slingshot.rightTopY) / 2.0f;
            slingshot.pouchZ = (slingshot.leftTopZ + slingshot.rightTopZ) / 2.0f;
            slingshot.isPulling = false;
            mousePressed = false;
            pullDepth = 0.0f;
            
            printf("Jogo reiniciado!\n");
            break;
    }
}

void specialKeys(int key, int x, int y) {
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
    
    drawWoodenBase();
    
    glDisable(GL_LIGHTING);
    drawElastic();
    drawAimLine();
    glEnable(GL_LIGHTING);
    
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
            
            // MODIFICADO: Desenhar modelo OBJ red.obj ao invÃ©s da esfera
            if (body == projectileBody) {
                glMaterialf(GL_FRONT, GL_SHININESS, 0.0f);
                
                if (projectileModelLoaded) {
                    // Usar modelo red.obj
                    glColor3f(0.8f, 0.2f, 0.2f);
                    glPushMatrix();
 
                    glRotatef(180, 0, 1, 0); // 180 graus em torno do eixo Y

                    projectileModel.draw();
                    glPopMatrix();
                } else {
                    // Fallback: esfera vermelha
                    glColor3f(0.8f, 0.2f, 0.2f);
                    glutSolidSphere(0.3, 20, 20);
                }
            }
            else if (shape == projectileShape) {
                glMaterialf(GL_FRONT, GL_SHININESS, 50.0f);
                
                if (projectileModelLoaded) {
                    // Usar modelo red.obj
                    glColor3f(0.8f, 0.2f, 0.2f);
                    
                    projectileModel.draw();
                } else {
                    // Fallback: esfera vermelha
                    glColor3f(0.8f, 0.2f, 0.2f);
                    glutSolidSphere(0.3, 16, 16);
                }
            }
            else if (shape == boxShape) {
                glMaterialf(GL_FRONT, GL_SHININESS, 5.0f);
                
                if (isTargetInAimLine(body)) {
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
    
    updateElasticPhysics();
    drawHUD();
    
    glutSwapBuffers();
}

void timer(int value) {
    dynamicsWorld->stepSimulation(1.0f / 60.0f, 10);
    
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
    
    slingshot.baseX = 0.0f;
    slingshot.baseY = 0.0f;
    slingshot.baseZ = 0.0f;
    
    slingshot.leftArmX = 0.0f;
    slingshot.leftArmY = 3.0f;
    slingshot.leftArmZ = 0.0f;
    
    slingshot.rightArmX = 0.0f;
    slingshot.rightArmY = 3.0f;
    slingshot.rightArmZ = 0.0f;
    
    float armHeight = 2.5f;
    float angle = 30.0f * M_PI / 180.0f;
    
    slingshot.leftTopX = slingshot.leftArmX + sin(angle) * armHeight;
    slingshot.leftTopY = slingshot.leftArmY + cos(angle) * armHeight;
    slingshot.leftTopZ = slingshot.leftArmZ;
    
    slingshot.rightTopX = slingshot.rightArmX - sin(angle) * armHeight;
    slingshot.rightTopY = slingshot.rightArmY + cos(angle) * armHeight;
    slingshot.rightTopZ = slingshot.rightArmZ;
    
    slingshot.pouchX = (slingshot.leftTopX + slingshot.rightTopX) / 2.0f;
    slingshot.pouchY = (slingshot.leftTopY + slingshot.rightTopY) / 2.0f;
    slingshot.pouchZ = (slingshot.leftTopZ + slingshot.rightTopZ) / 2.0f;
    slingshot.isPulling = false;
    
    initBullet();
    
    // NOVO: Carregar modelo red.obj para o projÃ©til
    printf("Tentando carregar modelo red.obj do projetil...\n");
    
    const char* projectilePaths[] = {
        "Objetos/red.obj",
        "./Objetos/red.obj",
        "../Objetos/red.obj",
        "red.obj"
    };
    
    projectileModelLoaded = false;
    for (const char* path : projectilePaths) {
        printf("  Tentando: %s\n", path);
        if (projectileModel.loadFromFile(path)) {
            projectileModelLoaded = true;
            printf("  âœ“ Modelo red.obj carregado com sucesso de: %s\n", path);
            break;
        }
    }
    
    if (!projectileModelLoaded) {
        printf("  Modelo red.obj nao encontrado. Usando esfera vermelha padrao.\n");
    }
    
    // Carregar modelo da Ã¡rvore
    printf("\nTentando carregar modelo OBJ da arvore...\n");
    
    const char* possiblePaths[] = {
        "Objetos/arvore.obj",
        "./Objetos/arvore.obj",
        "../Objetos/arvore.obj",
        "arvore.obj",
        "tree.obj"
    };
    
    treeModelLoaded = false;
    for (const char* path : possiblePaths) {
        printf("  Tentando: %s\n", path);
        if (treeModel.loadFromFile(path)) {
            treeModelLoaded = true;
            printf("  âœ“ Modelo carregado com sucesso de: %s\n", path);
            break;
        }
    }
    
    if (!treeModelLoaded) {
        printf("  Modelo OBJ nao encontrado em nenhum caminho. Usando geometria procedural.\n");
    }
    
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
    glutCreateWindow("Estilingue 3D - Melhorado com Red.obj");
    
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
    
    clearProjectile();
    
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
    
    delete dynamicsWorld;
    delete solver;
    delete dispatcher;
    delete collisionConfiguration;
    delete broadphase;
    
    return 0;
}
