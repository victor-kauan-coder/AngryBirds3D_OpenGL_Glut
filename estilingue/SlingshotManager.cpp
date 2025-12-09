
#include "SlingshotManager.h"


#include <GL/glut.h>
#include <GL/glu.h> 
#include <cmath>    
#include <cstdio>  
#include <string>
//objeto a ser lançado pelo esitlingue
#include "../passaros/passaro.h"
#include "../controle_audio/audio_manager.h"
// (Constante M_PI, se não estiver definida)
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


extern AudioManager g_audioManager;

SlingshotManager::SlingshotManager(btDiscreteDynamicsWorld* world, Passaro* projectile, int* shots, bool* game_over)

    : worldRef(world),               
      projectileRef(projectile),
      shotsRemainingRef(shots),      
      isGameOverRef(game_over),    
      
      projectileInPouch(nullptr),  
      isBeingPulled(false),      
      isPouchGrabbed(false),     
      currentMouseX(0),          
      currentMouseY(0),
      grabMouseStartX(0),        
      grabMouseStartY(0),
      grabPouchStartX(0),        
      grabPouchStartY(0),
      grabPouchStartZ(0),
      pouchPullDepthZ(0.0f),
      damageCount(0),
      slingshotBody(nullptr),
      damageCooldown(0.0f),
      slingshotModel(new OBJModel())
{
    slingshotModel->loadFromFile("Objetos/slingshot.obj");
    slingshotModel->loadMTL("Objetos/slingshot.mtl");
    extern GLuint loadGlobalTexture(const char* filename);
    crackTextureID = loadGlobalTexture("Objetos/texturas/crack.png"); 

    if (crackTextureID == 0) {
        printf("ERRO: Textura de rachadura nao encontrada!\n"); //verificar se consegui encontrar a textura da rachadura
    }
    initGeometry();
}

SlingshotManager::~SlingshotManager() {
    if (slingshotModel) {
        delete slingshotModel;
        slingshotModel = nullptr;
    }
    if (slingshotBody) {

        //remover o corpo do mubdo antes de deletar o ponteiro
        if (worldRef) {
            worldRef->removeRigidBody(slingshotBody);
        }

        if (slingshotBody->getMotionState()) delete slingshotBody->getMotionState();
        if (slingshotBody->getCollisionShape()) delete slingshotBody->getCollisionShape();
        
        delete slingshotBody;
        slingshotBody = nullptr;
    }
}


/**
 * @brief Calcula e define a geometria 3D estática do estilingue.
 * Esta função é chamada apenas uma vez, pelo construtor.
 */
void SlingshotManager::initGeometry() {
    float slingshotPosZ = 16.0f; //posição do estilingue eixo Z
    
    handleBaseX = 0.0f;
    handleBaseY = 2.5f; 
    handleBaseZ = slingshotPosZ; 
    
    leftArmBaseX = 0.0f;
    leftArmBaseY = 3.5f; 
    leftArmBaseZ = slingshotPosZ; 
    
    rightArmBaseX = 0.0f;
    rightArmBaseY = 3.5f; 
    rightArmBaseZ = slingshotPosZ; 
    
    float armHeight = 1.5f;
    float angle = 35.0f * M_PI / 180.0f; //angulo de abertura do estilingue Y
    
    
    leftForkTipX = leftArmBaseX + sin(angle) * armHeight;
    leftForkTipY = leftArmBaseY + cos(angle) * armHeight;
    leftForkTipZ = leftArmBaseZ;
    
    rightForkTipX = rightArmBaseX - sin(angle) * armHeight;
    rightForkTipY = rightArmBaseY + cos(angle) * armHeight;
    rightForkTipZ = rightArmBaseZ;
    
    pouchPositionX = (leftForkTipX + rightForkTipX) / 2.0f;
    pouchPositionY = (leftForkTipY + rightForkTipY) / 2.0f;
    pouchPositionZ = (leftForkTipZ + rightForkTipZ) / 2.0f;

    btCollisionShape* shape = new btBoxShape(btVector3(1.0f, 2.5f, 0.5f));
    
    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(btVector3(0.0f, 2.5f, slingshotPosZ));
    
    btDefaultMotionState* motionState = new btDefaultMotionState(transform);
    btRigidBody::btRigidBodyConstructionInfo rbInfo(0.0f, motionState, shape, btVector3(0,0,0)); //obj estatico(massa==0)
    
    slingshotBody = new btRigidBody(rbInfo);
    slingshotBody->setRestitution(0.5f);
    slingshotBody->setFriction(0.8f);

    //adiciona no mundo
    if (worldRef) {
        worldRef->addRigidBody(slingshotBody);
        printf("DEBUG: SlingshotBody (%p) ADICIONADO ao mundo %p.\n", slingshotBody, worldRef);
    } else {
        printf("ERRO CRITICO: SlingshotManager criado sem worldRef!\n");
    }
}

/**
 * @brief Desenha todas as partes visuais do estilingue.
 * Chamado pela 'display()' global a cada quadro.
 */
void SlingshotManager::draw() {
    //desenhar a base do estilingue
    drawWoodenBase();
    
    //para a linha e o eslastico nao tenham iluminação
    glDisable(GL_LIGHTING);
    
    //elastico-
    drawElasticBands();
    //linha de mirar
    drawAimLine();

    glEnable(GL_LIGHTING);
}

/**
 * @brief Atualiza a física interna do estilingue (não-Bullet).
 * Chamado pelo 'timer()' global a cada quadro.
 */
void SlingshotManager::update() {
    //simula o efeito do elastico
    if (damageCooldown > 0.0f) {
        damageCooldown -= 0.016f; //aproximadamente 1 frame
    }
    updateElasticReturnPhysics();
}

void SlingshotManager::handleMouseScroll(int button) {
    if (isBeingPulled) {
        //botão referente ao scroll para tras é o 4
        if (button == 4) { 
            pouchPullDepthZ += 0.3f;
            if (pouchPullDepthZ > 6.0f) pouchPullDepthZ = 6.0f;
            g_audioManager.playSlingshot(true, 30);
            printf("Scroll UP. Profundidade: %.2f\n", pouchPullDepthZ);
            
        //botão referente ao scroll para frente é o 3
        } else if (button == 3) { 
            pouchPullDepthZ -= 0.3f;
            if (pouchPullDepthZ < 0.0f) pouchPullDepthZ = 0.0f;
            printf("Scroll DOWN. Profundidade: %.2f\n", pouchPullDepthZ);
        }
        
        //recalcula a posição
        updatePouchPosition(); 
    }
}
/**
 * @brief Gerencia eventos de clique (pressionar/soltar) do mouse.
 * Chamado pela 'mouse()' global.
 */
void SlingshotManager::handleMouseClick(int button, int state, int x, int y) {
    //verifica se acabou o jogo para ignorar o clique
    if (*isGameOverRef) return;

    if (projectileRef && projectileRef->isEmVoo()) {
        return;
    }

    if (state == GLUT_UP && (button == 3 || button == 4)) { 
        handleMouseScroll(button);
        return;
    }

    if (button == GLUT_LEFT_BUTTON) {
        
        //se o botão esquerdo foi pressionado
        if (state == GLUT_DOWN) {
            float worldX, worldY, worldZ; //coordenadas do clique
            bool pouchWasFound = false;
            
            // Calcula a posição de repouso da malha
            float pouchRestX = (leftForkTipX + rightForkTipX) / 2.0f;
            float pouchRestY = (leftForkTipY + rightForkTipY) / 2.0f;
            float pouchRestZ = (leftForkTipZ + rightForkTipZ) / 2.0f;
            
            // Variáveis necessárias para a projeção OpenGL
            GLdouble modelview[16], projection[16];
            GLint viewport[4];
            GLdouble screenX, screenY, screenZ; //coordenadas da tela
            
            //pega as matrizes atuais de Modelo/Visão e Projeção e a Viewport
            glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
            glGetDoublev(GL_PROJECTION_MATRIX, projection);
            glGetIntegerv(GL_VIEWPORT, viewport);
            
            //calculando onde fica o pixel central do elastico no mundo 3D
            //converte a posição 3D da malha (mundo) para coordenadas 2D (tela)
            gluProject(pouchRestX, pouchRestY, pouchRestZ,
                       modelview, projection, viewport,
                       &screenX, &screenY, &screenZ);
            
            //converte a posição 2D do MOUSE (tela) para coordenadas 3D (mundo)
            //    Usamos 'screenZ' da malha para que o clique seja calculado na profundidade correta.
            screenToWorld(x, y, (float)screenZ, worldX, worldY, worldZ);
            
            //compara a distancia do clique com a distancia do centro do elastico 
            float dist_x = fabs(worldX - pouchRestX); // Distância absoluta em X
            float dist_y = fabs(worldY - pouchRestY); // Distância absoluta em Y
            
            //areaa do clique 
            if (dist_x < 1.5f && dist_y < 1.0f) {
                pouchWasFound = true;
            }
            

            // Se acertamos a malha E ainda temos tiros
            if (pouchWasFound && *shotsRemainingRef > 0) {
                // printf("Arraste e use scroll para profundidade.\n");
                
                isPouchGrabbed = true; 
                isBeingPulled = true; 
                
                //cria o passaro no elastico
                createProjectileInPouch();
                
                //ssalva as posições INICIAIS. Isso é crucial para calcular o "delta" (diferença) do arrasto.
                grabMouseStartX = x;            // Posição inicial do mouse
                grabMouseStartY = y;
                grabPouchStartX = pouchPositionX; // Posição inicial da malha
                grabPouchStartY = pouchPositionY;
                grabPouchStartZ = pouchPositionZ;
                pouchPullDepthZ = 0.0f;         // Reseta a profundidade
            }
            

        } else { // state == GLUT_UP soltamos o botão esquerdo 
            
            if (isPouchGrabbed && isBeingPulled && projectileInPouch) {
                //laçando o passaro
                launchProjectile();
                // printf("Som do estilingue funcionando \n");
                g_audioManager.playSlingshot(false,100);
                std::string tipo_passaro = projectileRef->getTipo();

                if (tipo_passaro == "Bomb") {
                    g_audioManager.playPassaro(SomTipo::LANCAMENTO_PASSARO_BOMB, 75);
                } 
                else if (tipo_passaro == "Blue") {
                    g_audioManager.playPassaro(SomTipo::LANCAMENTO_PASSARO_BLUE, 75);
                } 
                else if (tipo_passaro == "Chuck") {
                    g_audioManager.playPassaro(SomTipo::LANCAMENTO_PASSARO_CHUCK, 75);
                } 
                else {
                    //som do red ou de qualquer novo passaro 
                    g_audioManager.playPassaro(SomTipo::LANCAMENTO_PASSARO, 75);
                }
                

            }
            
            // Reseta todos os estados de interação
            isPouchGrabbed = false;
            isBeingPulled = false;
            pouchPullDepthZ = 0.0f;
        }
    }
}

/**
 * @brief Gerencia o movimento do mouse enquanto o botão está pressionado (arrastar).
 * Chamado pela 'mouseMotion()' global.
 */
void SlingshotManager::handleMouseDrag(int x, int y) {
    //se o pássaro atual já foi lançado e está em voo, não permite arrastar
    if (projectileRef && projectileRef->isEmVoo()) {
        return;
    }

    //atualiza a posição atual do mouse
    currentMouseX = x;
    currentMouseY = y;
    
    //recalcula a posição 3D da malha e do passaro com base no arrasto
    updatePouchPosition();
}

/**
 * @brief Gerencia o movimento do mouse sem botão pressionado (passivo).
 * Chamado pela 'passiveMouseMotion()' global.
 */
void SlingshotManager::handlePassiveMouseMotion(int x, int y) {
    currentMouseX = x;
    currentMouseY = y;
}

/**
 * @brief Gerencia os eventos de teclado (Q/E) para profundidade.
 * Chamado pela 'keyboard()' global.
 */
void SlingshotManager::handleKeyboard(unsigned char key) {
    switch(key) {

     }
}

/**
 * @brief Reseta o estilingue ao estado inicial.
 */
void SlingshotManager::reset() {

    clearProjectile();
    
    //retorna elastico na posição inicial
    pouchPositionX = (leftForkTipX + rightForkTipX) / 2.0f;
    pouchPositionY = (leftForkTipY + rightForkTipY) / 2.0f;
    pouchPositionZ = (leftForkTipZ + rightForkTipZ) / 2.0f;

    isBeingPulled = false;
    isPouchGrabbed = false;
    pouchPullDepthZ = 0.0f;
    
    //dano = 0
    damageCount = 0;
}

/**
 * @brief Função "Getter" que retorna um ponteiro para o projétil na malha.
 * Usado pela 'display()' global.
 * 'const' no final significa que esta função "promete" não modificar a classe.
 */
btRigidBody* SlingshotManager::getProjectileInPouch() const {
    return projectileInPouch;
}

/**
 * @brief Define o novo pássaro a ser lançado.
 */
void SlingshotManager::setProjectile(Passaro* novoPassaro) {
    //limpa o passaro atual
    clearProjectile();
    
    projectileRef = novoPassaro;
    
    //reseta os estados
    isBeingPulled = false;
    isPouchGrabbed = false;
    pouchPullDepthZ = 0.0f;
    
    //malha na posição de repouso
    pouchPositionX = (leftForkTipX + rightForkTipX) / 2.0f;
    pouchPositionY = (leftForkTipY + rightForkTipY) / 2.0f;
    pouchPositionZ = (leftForkTipZ + rightForkTipZ) / 2.0f;
}

/**
 * @brief Obtém a posição atual da malha do estilingue.
 */
void SlingshotManager::getPouchPosition(float& x, float& y, float& z) const {
    x = pouchPositionX;
    y = pouchPositionY;
    z = pouchPositionZ;
}


/**
 * @brief Remove e deleta com segurança o projétil da malha.
 */
void SlingshotManager::clearProjectile() {
    //chama a função de limpar fisica do passaro
    if (projectileRef && projectileRef->getRigidBody()) {
        projectileRef->limparFisica();
    }
    projectileInPouch = nullptr;
}


/**
 * @brief Cria um novo projétil e o coloca no elastico
 */
void SlingshotManager::createProjectileInPouch() {
    //limpar para nao criar um passaro em cima de outro
    clearProjectile();

    if (!projectileRef || !worldRef) return; 

    //posição do passaro
    float restX = (leftForkTipX + rightForkTipX) / 2.0f;
    float restY = (leftForkTipY + rightForkTipY) / 2.0f;
    float restZ = (leftForkTipZ + rightForkTipZ) / 2.0f;
    
    pouchPositionX = restX;
    pouchPositionY = restY;
    pouchPositionZ = restZ;
    
    //reseta o pássaro para a posição inicial
    projectileRef->resetar(restX, restY, restZ); 
    projectileRef->setEmVoo(false); // Garante que o pássaro NÃO está em voo enquanto está no estilingue
    //criar seu corpo físico no mundo para o passaro
    projectileRef->inicializarFisica(worldRef, restX, restY, restZ);
    
    //ponteiro para o corpo rígido que o pássaro acabou de criar
    projectileInPouch = projectileRef->getRigidBody();

    if (!projectileInPouch) {
        printf("erro no SlingshotManager.cpp: Falha ao inicializar a fisica do passaro!\n");
        return;
    }
    

    if (slingshotBody) {
        projectileInPouch->setIgnoreCollisionCheck(slingshotBody, true);
        slingshotBody->setIgnoreCollisionCheck(projectileInPouch, true);
    }

    //define o corpo como cinematico nao permite a gravidade agir enquanto esta em lançamento
    projectileInPouch->setCollisionFlags(projectileInPouch->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
    projectileInPouch->setActivationState(DISABLE_DEACTIVATION); //impede de "dormir"
    
}
/**
 * @brief Lança o projétil, devolvendo seu controle para a física Bullet. (MODIFICADO)
 */
void SlingshotManager::launchProjectile() {
    //não faz nada se não houver projétil ou pássaro
    if (!projectileInPouch || !projectileRef) return;

    // printf("lancando projetil (Passaro)!\n");
    
    //posição de repouso
    float restX = (leftForkTipX + rightForkTipX) / 2.0f;
    float restY = (leftForkTipY + rightForkTipY) / 2.0f;
    float restZ = (leftForkTipZ + rightForkTipZ) / 2.0f;
    
    //calcula o vetor de impulso
    float impulseX = restX - pouchPositionX;
    float impulseY = restY - pouchPositionY;
    float impulseZ = restZ - pouchPositionZ;
    

    //o objeto deixa de ser cinematico e passa a ser influenciado pela gravidade
    projectileInPouch->setCollisionFlags(projectileInPouch->getCollisionFlags() & ~btCollisionObject::CF_KINEMATIC_OBJECT);
    projectileInPouch->forceActivationState(ACTIVE_TAG); //"Acorda" o objeto
    

    projectileInPouch->setRestitution(0.6f); // se ele quica 
    projectileInPouch->setFriction(0.5f); // atrito ao deslizar
    projectileInPouch->setRollingFriction(0.1f); // atrito ao rolar
    projectileInPouch->setDamping(0.05f, 0.1f); //simula resistencia do ar
    
    //zera a velocidade do objeto
    projectileInPouch->setLinearVelocity(btVector3(0,0,0));
    projectileInPouch->setAngularVelocity(btVector3(0,0,0));
    
    //força do lançamento
    float forceMagnitude = 20.0f; 
    btVector3 impulse(impulseX * forceMagnitude, impulseY * forceMagnitude, impulseZ * forceMagnitude);
    
    //aplica o impulso no passaro
    projectileInPouch->applyCentralImpulse(impulse);

    //seta o passaro como voando agora
    projectileRef->setEmVoo(true);
    std::string tipo_passaro = projectileRef->getTipo();
    //som do lançamento
    if (tipo_passaro == "Bomb") {
        g_audioManager.playPassaro(SomTipo::LANCAMENTO_PASSARO_BOMB);
    } 
    else if (tipo_passaro == "Blue") {
        g_audioManager.playPassaro(SomTipo::LANCAMENTO_PASSARO_BLUE);
    } 
    else if (tipo_passaro == "Chuck") {
        g_audioManager.playPassaro(SomTipo::LANCAMENTO_PASSARO_CHUCK);
    } 
    else {
        //som do red ou de qualquuer outro passaro que for adicionado
        g_audioManager.playPassaro(SomTipo::LANCAMENTO_PASSARO);
    }

    //passaro saiu do elastico ponteiro agora nao aponta para nada
    projectileInPouch = nullptr; 
    
    //decrementa os tiros restantes
    (*shotsRemainingRef)--;
    
    //verifica se os tiros acabaram
    if (*shotsRemainingRef <= 0) {
        *isGameOverRef = true;
    }
}


/**
 * @brief Desenha a base de madeira usando a função drawCylinder.
 */
void SlingshotManager::drawWoodenBase() {

    glClear(GL_STENCIL_BUFFER_BIT); //limpa o buffer do stencil
    glEnable(GL_STENCIL_TEST); //ativa o stencil

    glStencilFunc(GL_ALWAYS, 1, 0xFF); 
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    glPushMatrix();
    glTranslatef(handleBaseX, handleBaseY, handleBaseZ);
    glScalef(8.0f, 8.0f, 8.0f);

    glEnable(GL_TEXTURE_2D); 
    glColor3f(1.0f, 1.0f, 1.0f); 

    if (slingshotModel) {
        slingshotModel->draw();
    }
    
    glDisable(GL_TEXTURE_2D);
    glPopMatrix();

    glStencilFunc(GL_EQUAL, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP); 


    if (damageCount >= 1) {
        drawCracks();
    }

    glDisable(GL_STENCIL_TEST);
}

/**
 * @brief Desenha as duas bandas elásticas como linhas 3D.
 */
void SlingshotManager::drawElasticBands() {
    glColor3f(0.05f, 0.05f, 0.05f); //cor do elastico
    glLineWidth(12.0f); //tamanho da linha
    glBegin(GL_LINES); // Começa a desenhar linhas
        //ponta esquerda até a malha
        glVertex3f(leftForkTipX, leftForkTipY, leftForkTipZ);
        glVertex3f(pouchPositionX, pouchPositionY, pouchPositionZ);
        //ponta direita até a malha
        glVertex3f(rightForkTipX, rightForkTipY, rightForkTipZ);
        glVertex3f(pouchPositionX, pouchPositionY, pouchPositionZ);
    glEnd();
}


/**
 * @brief Desenha a linha de mira
 */
void SlingshotManager::drawAimLine() {
    if (isBeingPulled && projectileRef) {
        //posição inicial é a malha puxada vou chamar de p0
        btVector3 p0(pouchPositionX, pouchPositionY, pouchPositionZ);
        //posição de repouso (para onde a mira aponta)
        float restX = (leftForkTipX + rightForkTipX) / 2.0f;
        float restY = (leftForkTipY + rightForkTipY) / 2.0f;
        float restZ = (leftForkTipZ + rightForkTipZ) / 2.0f;

        //vetor de impulso 
        float impulseX = restX - pouchPositionX;
        float impulseY = restY - pouchPositionY;
        float impulseZ = restZ - pouchPositionZ;
        
        //constantes para calcular a trajetória
        const float forceMagnitude = 20.0f; 
        const float mass = projectileRef->getMassa(); 
        const float gravityY = -9.81f; 

        //velocidade inicial (v0 = (Impulso * Força) / Massa)
        btVector3 v0(
            (impulseX * forceMagnitude) / mass,
            (impulseY * forceMagnitude) / mass,
            (impulseZ * forceMagnitude) / mass
        );
        glColor4f(1.0f, 1.0f, 1.0f, 0.8f); //cor branca 
        glLineWidth(3.0f);
        
        //linha pontilhada
        glEnable(GL_LINE_STIPPLE);
        glLineStipple(3, 0xAAAA);
        
        glBegin(GL_LINES);

        int numSegments = 20; //quantos segmentos de linha desenhar
        float timeStep = 0.08f; //espaçamento entre as linhas
        
        for (int i = 0; i < numSegments; i++) {
            //formula: p(t) = p0 + v0*t + 0.5*a*t^2
            
            // Ponto inicial do segmento
            float t1 = (float)i * timeStep;
            float p1x = p0.x() + v0.x() * t1;
            float p1y = p0.y() + v0.y() * t1 + 0.5f * gravityY * t1 * t1;
            float p1z = p0.z() + v0.z() * t1;
            
            // Ponto final do segmento
            float t2 = (float)(i + 1) * timeStep;
            float p2x = p0.x() + v0.x() * t2;
            float p2y = p0.y() + v0.y() * t2 + 0.5f * gravityY * t2 * t2;
            float p2z = p0.z() + v0.z() * t2;

            glVertex3f(p1x, p1y, p1z);
            glVertex3f(p2x, p2y, p2z);
        }
        glEnd();
        //desativa o "pontilhado"
        glDisable(GL_LINE_STIPPLE);
    }
}

/**
 * @brief Simula a malha voltando ao lugar (física de mola simples).
 * Esta é uma física "fake", separada do Bullet, apenas para efeito visual.
 */
void SlingshotManager::updateElasticReturnPhysics() {

    if (!isBeingPulled && !projectileInPouch) {
        //posição de repouso (o "alvo" da mola)
        float restX = (leftForkTipX + rightForkTipX) / 2.0f;
        float restY = (leftForkTipY + rightForkTipY) / 2.0f;
        float restZ = (leftForkTipZ + rightForkTipZ) / 2.0f;
        
        //vetor da variação de deslocamento
        float dx = pouchPositionX - restX;
        float dy = pouchPositionY - restY;
        float dz = pouchPositionZ - restZ;
        
        //constantes da física do elastico
        float k = 25.0f; // coeficiente elastico
        static float vx = 0.0f, vy = 0.0f, vz = 0.0f; //velocidade da malha (estática)
        float damping = 0.88f; // Amortecimento do elastico
        
        //lei de Hooke (F = -k * x): Calcula a força do elastico
        float fx = -k * dx;
        float fy = -k * dy;
        float fz = -k * dz;
        
        float mass = 0.08f; //massa do elastico
        float ax = fx / mass; //aceleração
        float ay = fy / mass;
        float az = fz / mass;
        
        float dt = 0.016f; // Delta time (passo de tempo, ~60fps)
        
        // v = v0 + a * t
        vx += ax * dt; //atualiza velocidade
        vy += ay * dt;
        vz += az * dt;
        
        //aplica amortecimento
        vx *= damping;
        vy *= damping;
        vz *= damping;
        
        // p = p0 + v * t
        pouchPositionX += vx * dt; //atualiza posição
        pouchPositionY += vy * dt;
        pouchPositionZ += vz * dt;
        
        //se estiver perto e devagar, trava no lugar
        float distance = sqrt(dx*dx + dy*dy + dz*dz);
        if (distance < 0.05f && sqrt(vx*vx + vy*vy + vz*vz) < 0.3f) {
            pouchPositionX = restX;
            pouchPositionY = restY;
            pouchPositionZ = restZ;
            vx = vy = vz = 0.0f;
        }
    }
}

/**
 * @brief Atualiza a posição 3D da malha com base no arrastar do mouse.
 * Esta é a função que faz o projétil cinemático seguir o mouse.
 */
void SlingshotManager::updatePouchPosition() {

    if (isPouchGrabbed && isBeingPulled) {

        int deltaX_pixels = currentMouseX - grabMouseStartX;
        int deltaY_pixels = currentMouseY - grabMouseStartY;
        
        float sensitivityX = 0.04f; //sensibilidade do mouse
        float sensitivityY = 0.04f;
        
        //converte o Delta de pixels para Delta 3D
        float dx = (float)deltaX_pixels * sensitivityX;
        float dy = (float)-deltaY_pixels * sensitivityY; // Y do mouse é invertido
        float dz = pouchPullDepthZ; // profundidade do scroll do mouse
        
        //limite da profundidade maxima do mouse
        float maxPull = 7.0f;
        float currentDist = sqrt(dx*dx + dy*dy + dz*dz);
        if (currentDist > maxPull) {
            //se passou do limite, "normaliza" o vetor e o escala para o máximo
            dx = (dx / currentDist) * maxPull;
            dy = (dy / currentDist) * maxPull;
            dz = (dz / currentDist) * maxPull;
        }
        //Posição = Posição_Inicial_Salva + Delta_3D
        pouchPositionX = grabPouchStartX + dx;
        pouchPositionY = grabPouchStartY + dy;
        pouchPositionZ = grabPouchStartZ + dz;
        
        //se houver um projétil no elastico
        if (projectileInPouch) {
            btTransform trans;
            //da um get na transformação atual do passaro
            projectileInPouch->getMotionState()->getWorldTransform(trans);
            //faz o objeto acompanhar o elastico
            trans.setOrigin(btVector3(pouchPositionX, pouchPositionY, pouchPositionZ));
            //aplica a transformação no passaro novamente
            projectileInPouch->getMotionState()->setWorldTransform(trans);
            projectileInPouch->setWorldTransform(trans);
        }
    }
}

/**
 * @brief Converte coordenadas de tela 2D (mouse) para mundo 3D.
 * Função auxiliar do OpenGL.
 */
void SlingshotManager::screenToWorld(int screenX, int screenY, float depth, float& worldX, float& worldY, float& worldZ) {
    //pega as matrizes e viewport atuais
    GLint viewport[4];
    GLdouble modelview[16];
    GLdouble projection[16];
    GLdouble posX, posY, posZ; //variáveis de saída
    
    glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
    glGetDoublev(GL_PROJECTION_MATRIX, projection);
    glGetIntegerv(GL_VIEWPORT, viewport);

    GLfloat winX = (float)screenX;
    GLfloat winY = (float)viewport[3] - (float)screenY; // Inverte o Y
    
    //converte 2d em 3d
    gluUnProject(winX, winY, depth, 
                 modelview, projection, viewport, 
                 &posX, &posY, &posZ);
    
    //retorna os valores 3d usados
    worldX = posX;
    worldY = posY;
    worldZ = posZ;
}

void SlingshotManager::takeDamage() {
    if (damageCooldown > 0.0f) return;

    damageCount++;
    damageCooldown = 2.0f;
    
    // printf("Estilingue atingido! Dano: %d/3\n", damageCount);
    g_audioManager.play(SomTipo::COLISAO_MADEIRA);
    
    if (damageCount >= 3) {
        triggerGameOver();
    }

}

void SlingshotManager::triggerGameOver() {
    printf("GAME OVER: Estilingue destruido!\n");
    if (isGameOverRef) {
        *isGameOverRef = true;
    }
    g_audioManager.play(SomTipo::BLOCO_MADEIRA_DESTRUIDO);
}

btRigidBody* SlingshotManager::getRigidBody() const {
    return slingshotBody;
}

void SlingshotManager::drawCracks() {
    if (crackTextureID == 0) return; //verifica se existe a textura da rachadura

    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_TEXTURE_BIT);

    glDisable(GL_LIGHTING); //sem luz para destacar bem o preto da rachadura
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); //respeitar a transparencia da imagem
    
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, crackTextureID);
    
    glEnable(GL_POLYGON_OFFSET_FILL);//garante que as rachaduras fiquem sobre a madeira do estilingue
    glPolygonOffset(-3.0f, -3.0f); 

    glColor3f(1.0f, 1.0f, 1.0f); 
    
    float wCabo = 0.8f;  //largura do cabo
    float hCabo = 2.0f;  //altura do cabo 
    
    float wBraco = 1.0f; //largura do braço 
    float hBraco = 2.5f; //comprimento do braço

    float zFino = 0.1f;

    glBegin(GL_QUADS);

    if (damageCount >= 1) {
        float x = handleBaseX;
        float y = handleBaseY + 1.0f; //centralizado no cabo
        
        //FRENTE
        float zFront = handleBaseZ + 0.5f + zFino;
        glTexCoord2f(0, 0); glVertex3f(x - wCabo, y - hCabo, zFront);
        glTexCoord2f(1, 0); glVertex3f(x + wCabo, y - hCabo, zFront);
        glTexCoord2f(1, 1); glVertex3f(x + wCabo, y + hCabo, zFront);
        glTexCoord2f(0, 1); glVertex3f(x - wCabo, y + hCabo, zFront);

        //COSTAS
        float zBack = handleBaseZ - 0.5f - zFino;
        glTexCoord2f(0, 0); glVertex3f(x - wCabo, y - hCabo, zBack);
        glTexCoord2f(1, 0); glVertex3f(x + wCabo, y - hCabo, zBack);
        glTexCoord2f(1, 1); glVertex3f(x + wCabo, y + hCabo, zBack);
        glTexCoord2f(0, 1); glVertex3f(x - wCabo, y + hCabo, zBack);
    }
    
    if (damageCount >= 2) {
        // Ponto médio do braço esquerdo
        float x = leftArmBaseX + (leftForkTipX - leftArmBaseX) * 0.5f; 
        float y = leftArmBaseY + (leftForkTipY - leftArmBaseY) * 0.5f;
        
        float zFront = leftArmBaseZ + 0.3f + zFino;
        float zBack = leftArmBaseZ - 0.3f - zFino;

        //FRENTE
        glTexCoord2f(0, 0); glVertex3f(x - wBraco, y - hBraco, zFront);
        glTexCoord2f(1, 0); glVertex3f(x + wBraco, y - hBraco, zFront);
        glTexCoord2f(1, 1); glVertex3f(x + wBraco, y + hBraco, zFront);
        glTexCoord2f(0, 1); glVertex3f(x - wBraco, y + hBraco, zFront);

        //COSTAS
        glTexCoord2f(0, 0); glVertex3f(x - wBraco, y - hBraco, zBack);
        glTexCoord2f(1, 0); glVertex3f(x + wBraco, y - hBraco, zBack);
        glTexCoord2f(1, 1); glVertex3f(x + wBraco, y + hBraco, zBack);
        glTexCoord2f(0, 1); glVertex3f(x - wBraco, y + hBraco, zBack);
    }
    

    if (damageCount >= 3) {
        float x = rightArmBaseX + (rightForkTipX - rightArmBaseX) * 0.5f;
        float y = rightArmBaseY + (rightForkTipY - rightArmBaseY) * 0.5f;

        float zFront = rightArmBaseZ + 0.3f + zFino;
        float zBack = rightArmBaseZ - 0.3f - zFino;

        //FRENTE
        glTexCoord2f(0, 0); glVertex3f(x - wBraco, y - hBraco, zFront);
        glTexCoord2f(1, 0); glVertex3f(x + wBraco, y - hBraco, zFront);
        glTexCoord2f(1, 1); glVertex3f(x + wBraco, y + hBraco, zFront);
        glTexCoord2f(0, 1); glVertex3f(x - wBraco, y + hBraco, zFront);

        //COSTAS
        glTexCoord2f(0, 0); glVertex3f(x - wBraco, y - hBraco, zBack);
        glTexCoord2f(1, 0); glVertex3f(x + wBraco, y - hBraco, zBack);
        glTexCoord2f(1, 1); glVertex3f(x + wBraco, y + hBraco, zBack);
        glTexCoord2f(0, 1); glVertex3f(x - wBraco, y + hBraco, zBack);
    }

    glEnd();
    glPopAttrib();
}

int SlingshotManager::getHealth() const {
    //a vida máxima é 3. Subtraímos o dano atual.
    //se damageCount for 0, retorna 3 corações.
    //se damageCount for 1, retorna 2 corações.
    int vida = 3 - damageCount;
    if (vida < 0) vida = 0;
    return vida;
}