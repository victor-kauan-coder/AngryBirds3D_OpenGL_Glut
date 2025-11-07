#include <GL/glut.h>
#include <btBulletDynamicsCommon.h>
#include <btBulletCollisionCommon.h> 
#include <cmath>
#include <vector>

// Definir M_PI se não estiver definido
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Configurações
const int WIDTH = 1280;
const int HEIGHT = 720;

// Mundo de física Bullet
btDiscreteDynamicsWorld* dynamicsWorld;
btAlignedObjectArray<btCollisionShape*> collisionShapes;
btCollisionShape* projectileShape = nullptr; 
btRigidBody* projectileBody = nullptr; 

// Ponteiros globais para os componentes do Bullet
btBroadphaseInterface* broadphase = nullptr;
btDefaultCollisionConfiguration* collisionConfiguration = nullptr;
btCollisionDispatcher* dispatcher = nullptr;
btSequentialImpulseConstraintSolver* solver = nullptr;

// Estrutura do estilingue
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

// NOVO: Posição inicial do mouse e do elástico ao clicar
int startMouseX = 0;
int startMouseY = 0;
float startPouchX = 0.0f;
float startPouchY = 0.0f;
float startPouchZ = 0.0f;

// Câmera
float cameraAngleX = 0.0f;
float cameraAngleY = 20.0f;
float cameraDistance = 15.0f;

void initBullet() {
    // Configuração do Bullet Physics
    broadphase = new btDbvtBroadphase();
    collisionConfiguration = new btDefaultCollisionConfiguration();
    dispatcher = new btCollisionDispatcher(collisionConfiguration);
    solver = new btSequentialImpulseConstraintSolver();

    dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);
    dynamicsWorld->setGravity(btVector3(0, -9.81f, 0));

    // Chão
    btCollisionShape* groundShape = new btStaticPlaneShape(btVector3(0, 1, 0), 0);
    collisionShapes.push_back(groundShape);

    btDefaultMotionState* groundMotionState = new btDefaultMotionState(
        btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, 0, 0))); 

    btRigidBody::btRigidBodyConstructionInfo groundRigidBodyCI(
        0, groundMotionState, groundShape, btVector3(0, 0, 0));
    btRigidBody* groundRigidBody = new btRigidBody(groundRigidBodyCI);
    dynamicsWorld->addRigidBody(groundRigidBody);

    // Criar a forma (shape) do projétil uma vez
    projectileShape = new btSphereShape(0.3f); // Raio de 0.3
    collisionShapes.push_back(projectileShape);
}

// Limpa o projétil atual
void clearProjectile() {
    if (projectileBody) {
        dynamicsWorld->removeRigidBody(projectileBody);
        delete projectileBody->getMotionState();
        delete projectileBody;
        projectileBody = nullptr;
    }
}

// Cria o projétil na posição de repouso
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

    projectileBody->setCollisionFlags(projectileBody->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
    projectileBody->setActivationState(DISABLE_DEACTIVATION);

    dynamicsWorld->addRigidBody(projectileBody);
}


/**
 * @brief Desenha um cilindro entre dois pontos (p1 e p2)
 */
void drawCylinder(float x1, float y1, float z1, float x2, float y2, float z2, float radius) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    float dz = z2 - z1;
    float length = sqrt(dx*dx + dy*dy + dz*dz);
    
    if (length == 0.0) return; // Não pode desenhar cilindro de comprimento zero

    float z_axis[3] = {0.0, 0.0, 1.0};
    float vector[3] = {dx, dy, dz};
    
    vector[0] /= length;
    vector[1] /= length;
    vector[2] /= length;

    float axis[3];
    axis[0] = z_axis[1] * vector[2] - z_axis[2] * vector[1]; // -dy
    axis[1] = z_axis[2] * vector[0] - z_axis[0] * vector[2]; //  dx
    axis[2] = z_axis[0] * vector[1] - z_axis[1] * vector[0]; //  0

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

/**
 * @brief Função de desenho do estilingue (REESCRITA)
 */
void drawWoodenBase() {
    // COR MELHORADA: Uma cor de madeira mais rica
    glColor3f(0.5f, 0.35f, 0.05f);

    // Haste principal (Tronco do Y)
    // Desenha de (0,0,0) -> (0,3,0)
    drawCylinder(slingshot.baseX, slingshot.baseY, slingshot.baseZ, 
                 slingshot.leftArmX, slingshot.leftArmY, slingshot.leftArmZ, 
                 0.3); // Raio de 0.3

    // Forquilha esquerda
    // Desenha de (0,3,0) -> (ponta esquerda)
    drawCylinder(slingshot.leftArmX, slingshot.leftArmY, slingshot.leftArmZ, 
                 slingshot.leftTopX, slingshot.leftTopY, slingshot.leftTopZ, 
                 0.2); // Raio de 0.2

    // Forquilha direita
    // Desenha de (0,3,0) -> (ponta direita)
    drawCylinder(slingshot.rightArmX, slingshot.rightArmY, slingshot.rightArmZ, 
                 slingshot.rightTopX, slingshot.rightTopY, slingshot.rightTopZ, 
                 0.2); // Raio de 0.2
}


void drawElastic() {
    // COR MELHORADA: Elástico mais escuro
    glColor3f(0.05f, 0.05f, 0.05f);
    glLineWidth(12.0f);

    glBegin(GL_LINES);
    // Lado esquerdo
    glVertex3f(slingshot.leftTopX, slingshot.leftTopY, slingshot.leftTopZ);
    glVertex3f(slingshot.pouchX, slingshot.pouchY, slingshot.pouchZ);

    // Lado direito
    glVertex3f(slingshot.rightTopX, slingshot.rightTopY, slingshot.rightTopZ);
    glVertex3f(slingshot.pouchX, slingshot.pouchY, slingshot.pouchZ);
    glEnd();
}

void updateElasticPhysics() {
    // Controla o "balanço" visual do elástico quando não está sendo puxado
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
    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) {
            float wx, wy, wz;
            bool found = false;

            // 1. Calcular as coordenadas 3D atuais do elástico (ponto de repouso)
            float currentPouchX = (slingshot.leftTopX + slingshot.rightTopX) / 2.0f;
            float currentPouchY = (slingshot.leftTopY + slingshot.rightTopY) / 2.0f;
            float currentPouchZ = (slingshot.leftTopZ + slingshot.rightTopZ) / 2.0f;

            // 2. Usar GLdouble (double) em vez de GLfloat (float)
            GLdouble modelview[16];
            GLdouble projection[16];
            GLint viewport[4];
            GLdouble screenX, screenY, screenZ; 

            // Usar glGetDoublev e glGetIntegerv
            glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
            glGetDoublev(GL_PROJECTION_MATRIX, projection);
            glGetIntegerv(GL_VIEWPORT, viewport);

            gluProject(currentPouchX, currentPouchY, currentPouchZ,
                       modelview, projection, viewport,
                       &screenX, &screenY, &screenZ);
            
            // 3. Converter o clique do mouse de volta para 3D
            screenToWorld(x, y, (float)screenZ, wx, wy, wz);

            // 4. Verificar o hitbox
            float dist_x = fabs(wx - currentPouchX);
            float dist_y = fabs(wy - currentPouchY);

            if (dist_x < 1.5f && dist_y < 1.0f) {
                found = true;
            }

            if (found) {
                printf("Elástico capturado! Arraste para puxar.\n");
                mousePressed = true;
                slingshot.isPulling = true;
                createProjectileInPouch();
                
                // NOVO: Armazenar posições iniciais
                startMouseX = x;
                startMouseY = y;
                startPouchX = slingshot.pouchX; // Salva a posição 3D inicial
                startPouchY = slingshot.pouchY;
                startPouchZ = slingshot.pouchZ;

            } else {
                 printf("Clique não detectou o elástico.\n");
                 printf("   Clique (Mundo): X=%.2f, Y=%.2f, Z=%.2f\n", wx, wy, wz);
                 printf("   Elástico (Mundo): X=%.2f, Y=%.2f, Z=%.2f\n", currentPouchX, currentPouchY, currentPouchZ);
            }
            
        } else { // state == GLUT_UP
            if (mousePressed && slingshot.isPulling && projectileBody) {
                printf("Elástico solto! Lançando projétil!\n");
                
                float restX = (slingshot.leftTopX + slingshot.rightTopX) / 2.0f;
                float restY = (slingshot.leftTopY + slingshot.rightTopY) / 2.0f;
                float restZ = (slingshot.leftTopZ + slingshot.rightTopZ) / 2.0f;

                float impulseX = restX - slingshot.pouchX;
                float impulseY = restY - slingshot.pouchY;
                float impulseZ = restZ - slingshot.pouchZ;
                
                projectileBody->setCollisionFlags(projectileBody->getCollisionFlags() & ~btCollisionObject::CF_KINEMATIC_OBJECT);
                projectileBody->forceActivationState(ACTIVE_TAG);
                projectileBody->setLinearVelocity(btVector3(0,0,0));
                projectileBody->setAngularVelocity(btVector3(0,0,0));

                float forceMagnitude = 40.0f;
                btVector3 impulse(impulseX * forceMagnitude, impulseY * forceMagnitude, impulseZ * forceMagnitude);
                projectileBody->applyCentralImpulse(impulse);

                projectileBody = nullptr; 
            }
            mousePressed = false;
            slingshot.isPulling = false;
        }
    }
}

void mouseMotion(int x, int y) {
    if (mousePressed && slingshot.isPulling) {
        
        // 1. Calcular o delta (mudança) do mouse em pixels desde o clique
        int deltaX_pixels = x - startMouseX;
        int deltaY_pixels = y - startMouseY; // y positivo é para baixo

        // 2. Definir a sensibilidade
        // Aumente estes valores se quiser AINDA mais sensibilidade
        float sensitivityXY = 0.05f; 
        float sensitivityZ = -0.03f; // Z Negativo: arrastar para baixo (deltaY > 0) puxa para trás (Z < 0)

        // 3. Calcular os deslocamentos 3D (COM A LÓGICA CORRETA)
        
        // Eixo X: Mouse Direita (deltaX > 0) -> Elástico Direita (dx > 0)
        float dx = (float)deltaX_pixels * sensitivityXY;

        // Eixo Y: Mouse Cima (deltaY < 0) -> Elástico Cima (dy > 0)
        // Por isso, invertemos o deltaY_pixels com um sinal negativo (-)
        float dy = (float)-deltaY_pixels * sensitivityXY;

        // Eixo Z (Gradual): Mouse Baixo (deltaY > 0) -> Elástico Trás (dz < 0)
        float dz = (float)deltaY_pixels * sensitivityZ;
        

        // 4. Limitar o puxão (Max Pull)
        float maxPull = 6.0f;
        float currentDist = sqrt(dx*dx + dy*dy + dz*dz);

        if (currentDist > maxPull) {
            dx = (dx / currentDist) * maxPull;
            dy = (dy / currentDist) * maxPull;
            dz = (dz / currentDist) * maxPull;
        }
        
        // Garantir que o elástico só puxe para trás (Z negativo)
        // Se o dz for positivo (usuário arrastou para cima), ele fica em 0.
        if (dz > 0.0f) {
            dz = 0.0f;
        }

        // 5. Atualizar a posição da bolsa (pouch)
        // Adicionamos os deslocamentos calculados à posição INICIAL do clique
        slingshot.pouchX = startPouchX + dx;
        slingshot.pouchY = startPouchY + dy;
        slingshot.pouchZ = startPouchZ + dz;
        
        // 6. Atualizar o corpo físico
        if (projectileBody) {
            btTransform trans;
            projectileBody->getMotionState()->getWorldTransform(trans);
            trans.setOrigin(btVector3(slingshot.pouchX, slingshot.pouchY, slingshot.pouchZ));
            projectileBody->getMotionState()->setWorldTransform(trans);
            projectileBody->setWorldTransform(trans); 
        }
    }
}

void passiveMouseMotion(int x, int y) {
    mouseX = x;
    mouseY = y;
}

void keyboard(unsigned char key, int x, int y) {
    switch(key) {
        case 27: // ESC
            exit(0);
            break;
        case 'r':
        case 'R':
            clearProjectile(); 
            slingshot.pouchX = (slingshot.leftTopX + slingshot.rightTopX) / 2.0f;
            slingshot.pouchY = (slingshot.leftTopY + slingshot.rightTopY) / 2.0f;
            slingshot.pouchZ = (slingshot.leftTopZ + slingshot.rightTopZ) / 2.0f;
            slingshot.isPulling = false;
            mousePressed = false;
            printf("Estilingue resetado!\n");
            break;
        case ' ':
            if (mousePressed && slingshot.isPulling && projectileBody) {
                printf("Elástico solto! Lançando projétil!\n");
                
                float restX = (slingshot.leftTopX + slingshot.rightTopX) / 2.0f;
                float restY = (slingshot.leftTopY + slingshot.rightTopY) / 2.0f;
                float restZ = (slingshot.leftTopZ + slingshot.rightTopZ) / 2.0f;

                float impulseX = restX - slingshot.pouchX;
                float impulseY = restY - slingshot.pouchY;
                float impulseZ = restZ - slingshot.pouchZ;
                
                projectileBody->setCollisionFlags(projectileBody->getCollisionFlags() & ~btCollisionObject::CF_KINEMATIC_OBJECT);
                projectileBody->forceActivationState(ACTIVE_TAG);
                projectileBody->setLinearVelocity(btVector3(0,0,0));
                projectileBody->setAngularVelocity(btVector3(0,0,0));

                float forceMagnitude = 40.0f; 
                btVector3 impulse(impulseX * forceMagnitude, impulseY * forceMagnitude, impulseZ * forceMagnitude);
                projectileBody->applyCentralImpulse(impulse);

                projectileBody = nullptr; 
            }
            mousePressed = false;
            slingshot.isPulling = false;
            break;
    }
}

void specialKeys(int key, int x, int y) {
    switch(key) {
        case GLUT_KEY_LEFT:
            cameraAngleX -= 5.0f;
            break;
        case GLUT_KEY_RIGHT:
            cameraAngleX += 5.0f;
            break;
        case GLUT_KEY_UP:
            cameraAngleY += 5.0f;
            if (cameraAngleY > 80.0f) cameraAngleY = 80.0f;
            break;
        case GLUT_KEY_DOWN:
            cameraAngleY -= 5.0f;
            if (cameraAngleY < -80.0f) cameraAngleY = -80.0f;
            break;
    }
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    float camX = cameraDistance * sin(cameraAngleX * M_PI / 180.0f) * cos(cameraAngleY * M_PI / 180.0f);
    float camY = cameraDistance * sin(cameraAngleY * M_PI / 180.0f);
    float camZ = cameraDistance * cos(cameraAngleX * M_PI / 180.0f) * cos(cameraAngleY * M_PI / 180.0f);

    gluLookAt(camX, camY + 3.0f, camZ,
              0.0f, 3.0f, 0.0f, 
              0.0f, 1.0f, 0.0f);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    GLfloat lightPos[] = {5.0f, 10.0f, 15.0f, 1.0f}; 
    GLfloat lightAmb[] = {0.3f, 0.3f, 0.3f, 1.0f};
    GLfloat lightDif[] = {0.8f, 0.8f, 0.8f, 1.0f};
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDif);
    
    // Chão (Verde)
    glDisable(GL_LIGHTING);
    glColor3f(0.3f, 0.5f, 0.3f);
    glBegin(GL_QUADS);
    glVertex3f(-20.0f, 0.0f, -20.0f);
    glVertex3f(-20.0f, 0.0f, 20.0f);
    glVertex3f(20.0f, 0.0f, 20.0f);
    glVertex3f(20.0f, 0.0f, -20.0f);
    glEnd();
    glEnable(GL_LIGHTING);

    // Desenhar estilingue (com a nova lógica)
    drawWoodenBase();

    // Desenhar elástico
    glDisable(GL_LIGHTING);
    drawElastic();
    glEnable(GL_LIGHTING);

    // Desenhar todos os corpos rígidos do mundo Bullet
    for (int i = dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--) {
        btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[i];
        btRigidBody* body = btRigidBody::upcast(obj);

        if (body && body->getMotionState() && body->getInvMass() != 0) {
            btTransform trans;
            body->getMotionState()->getWorldTransform(trans);
            btVector3 pos = trans.getOrigin();

            glPushMatrix();
            glTranslatef(pos.getX(), pos.getY(), pos.getZ());

            if (body == projectileBody) {
                // COR MELHORADA: Bolsa de borracha (sem brilho)
                glMaterialf(GL_FRONT, GL_SHININESS, 0.0f); 
                glColor3f(0.15f, 0.1f, 0.1f); 
                glutSolidSphere(0.4, 20, 20); 
            } else {
                // COR MELHORADA: Projétil (vermelho brilhante)
                glMaterialf(GL_FRONT, GL_SHININESS, 50.0f);
                glColor3f(0.8f, 0.2f, 0.2f);  
                glutSolidSphere(0.3, 16, 16); 
            }
            glPopMatrix();
        }
    }
    // Restaura o brilho padrão para o estilingue no próximo frame
    glMaterialf(GL_FRONT, GL_SHININESS, 10.0f); 

    updateElasticPhysics();

    glutSwapBuffers();
}

void timer(int value) {
    dynamicsWorld->stepSimulation(1.0f / 60.0f, 10);
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
    glClearColor(0.53f, 0.81f, 0.92f, 1.0f); 

    // NOVO: Habilita materiais para cores melhores
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    GLfloat spec[] = {0.2f, 0.2f, 0.2f, 1.0f}; // Um leve brilho especular
    glMaterialfv(GL_FRONT, GL_SPECULAR, spec);
    glMaterialf(GL_FRONT, GL_SHININESS, 10.0f); // Brilho baixo para a madeira
    // Fim da adição

    // --- Posições Corretas do Estilingue "Y" ---
    slingshot.baseX = 0.0f;
    slingshot.baseY = 0.0f; // Começa no chão
    slingshot.baseZ = 0.0f;

    // A forquilha começa no topo da haste (Y=3.0)
    slingshot.leftArmX = 0.0f;
    slingshot.leftArmY = 3.0f;
    slingshot.leftArmZ = 0.0f;

    slingshot.rightArmX = 0.0f;
    slingshot.rightArmY = 3.0f;
    slingshot.rightArmZ = 0.0f;

    // Calcular posições do topo das forquilhas
    float armHeight = 2.5f; 
    float angle = 30.0f * M_PI / 180.0f;

    slingshot.leftTopX = slingshot.leftArmX + sin(angle) * armHeight;
    slingshot.leftTopY = slingshot.leftArmY + cos(angle) * armHeight;
    slingshot.leftTopZ = slingshot.leftArmZ;

    slingshot.rightTopX = slingshot.rightArmX - sin(angle) * armHeight;
    slingshot.rightTopY = slingshot.rightArmY + cos(angle) * armHeight;
    slingshot.rightTopZ = slingshot.rightArmZ;
    // --- Fim das Posições Corretas ---

    slingshot.pouchX = (slingshot.leftTopX + slingshot.rightTopX) / 2.0f;
    slingshot.pouchY = (slingshot.leftTopY + slingshot.rightTopY) / 2.0f;
    slingshot.pouchZ = (slingshot.leftTopZ + slingshot.rightTopZ) / 2.0f;

    slingshot.isPulling = false;

    initBullet();

    printf("=== CONTROLES DO ESTILINGUE ===\n");
    printf("Mouse Esquerdo: Clique e arraste o elástico para atirar\n");
    printf("Setas: Mover câmera\n");
    printf("R: Resetar\n");
    printf("ESPAÇO: Soltar elástico\n");
    printf("ESC: Sair\n");
    printf("==============================\n\n");
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(WIDTH, HEIGHT);
    glutCreateWindow("Estilingue Realista com Física - C++");

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

    // Cleanup completo do Bullet
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