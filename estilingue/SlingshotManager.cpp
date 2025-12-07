// 1. Inclui o header correspondente
// Este include "ensina" ao .cpp qual é a estrutura da classe
// (quais funções e variáveis ela tem) que foi definida no .h
#include "SlingshotManager.h"

// 2. Inclui todas as bibliotecas necessárias para a IMPLEMENTAÇÃO
#include <GL/glut.h> // Necessário para funções OpenGL/GLUT (gl..., glu...)
#include <GL/glu.h>  // Necessário para funções GLU (gluProject, gluCylinder, etc.)
#include <cmath>     // Necessário para funções matemáticas (sin, cos, sqrt, fabs)
#include <cstdio>  // Necessário para a função de console 'printf'
#include <string>
//objeto a ser lançado pelo esitlingue
#include "../passaros/passaro.h"
#include "../controle_audio/audio_manager.h"
// (Constante M_PI, se não estiver definida)
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


extern AudioManager g_audioManager;
// 3. Implementação do Construtor
// A sintaxe "SlingshotManager::" significa que esta função "SlingshotManager"
// pertence à classe "SlingshotManager". Este é o construtor.
SlingshotManager::SlingshotManager(btDiscreteDynamicsWorld* world, Passaro* projectile, int* shots, bool* game_over)
    // --- Lista de Inicialização de Membros (Modificada) ---
    : worldRef(world),               
      projectileRef(projectile),     // <-- Modificado
      shotsRemainingRef(shots),      
      isGameOverRef(game_over),    
      
      // --- Inicialização dos Estados Internos ---
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
      damageCooldown(0.0f)
{
    initGeometry();
}

SlingshotManager::~SlingshotManager() {
    if (slingshotBody) {
        // --- CORREÇÃO CRÍTICA PARA EVITAR CRASH ---
        // É OBRIGATÓRIO remover o corpo do mundo antes de deletar o ponteiro.
        // Se não fizermos isso, o 'cleanupBullet' vai tentar acessar memória morta depois.
        if (worldRef) {
            worldRef->removeRigidBody(slingshotBody);
        }
        // ------------------------------------------

        if (slingshotBody->getMotionState()) delete slingshotBody->getMotionState();
        if (slingshotBody->getCollisionShape()) delete slingshotBody->getCollisionShape();
        
        delete slingshotBody;
        slingshotBody = nullptr;
    }
}

// 4. Implementação das Funções Públicas

/**
 * @brief Calcula e define a geometria 3D estática do estilingue.
 * Esta função é chamada apenas uma vez, pelo construtor.
 */
void SlingshotManager::initGeometry() {
    // Define a posição da base (cabo) e o início dos braços da forquilha
    float slingshotPosZ = 16.0f; // <-- Defina a profundidade aqui

    // Define a posição da base (cabo) e o início dos braços da forquilha
    handleBaseX = 0.0f;
    handleBaseY = 0.0f;
    handleBaseZ = slingshotPosZ; // <-- MUDADO
    
    leftArmBaseX = 0.0f;
    leftArmBaseY = 3.0f;
    leftArmBaseZ = slingshotPosZ; // <-- MUDADO
    
    rightArmBaseX = 0.0f;
    rightArmBaseY = 3.0f;
    rightArmBaseZ = slingshotPosZ; // <-- MUDADO
    
    // Define os parâmetros da forquilha (forma de "Y")
    float armHeight = 2.5f; // Comprimento de cada braço da forquilha
    float angle = 30.0f * M_PI / 180.0f; // Ângulo de 30 graus (convertido para radianos)
    
    // Calcula a posição da PONTA ESQUERDA da forquilha usando trigonometria
    leftForkTipX = leftArmBaseX + sin(angle) * armHeight;
    leftForkTipY = leftArmBaseY + cos(angle) * armHeight;
    leftForkTipZ = leftArmBaseZ;
    
    // Calcula a posição da PONTA DIREITA da forquilha
    rightForkTipX = rightArmBaseX - sin(angle) * armHeight;
    rightForkTipY = rightArmBaseY + cos(angle) * armHeight;
    rightForkTipZ = rightArmBaseZ;
    
    // A posição de repouso da malha (pouch) é o ponto médio entre as duas pontas
    pouchPositionX = (leftForkTipX + rightForkTipX) / 2.0f;
    pouchPositionY = (leftForkTipY + rightForkTipY) / 2.0f;
    pouchPositionZ = (leftForkTipZ + rightForkTipZ) / 2.0f;

    // --- Criação do Corpo Físico do Estilingue (Para Colisões) ---
    // Cria uma caixa estática que envolve a base e os braços do estilingue
    // Posição: (0, 2.5, 12) - Centro aproximado
    // Tamanho: (1.5, 2.5, 0.5) - Half extents (largura total 3, altura 5, prof 1)
    btCollisionShape* shape = new btBoxShape(btVector3(1.0f, 2.5f, 0.5f));
    
    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(btVector3(0.0f, 2.5f, slingshotPosZ));
    
    btDefaultMotionState* motionState = new btDefaultMotionState(transform);
    btRigidBody::btRigidBodyConstructionInfo rbInfo(0.0f, motionState, shape, btVector3(0,0,0)); // Massa 0 = Estático
    
    slingshotBody = new btRigidBody(rbInfo);
    slingshotBody->setRestitution(0.5f);
    slingshotBody->setFriction(0.8f);
    
    // REMOVIDO: CF_KINEMATIC_OBJECT. O estilingue é estático.
    // slingshotBody->setCollisionFlags(slingshotBody->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
    // slingshotBody->setActivationState(DISABLE_DEACTIVATION);

    // Adiciona ao mundo
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
    // Desenha as partes de madeira (que recebem luz)
    drawWoodenBase();
    
    // Desliga a iluminação OpenGL padrão.
    // Isso é feito para que os elásticos e a linha de mira tenham
    // uma cor "pura" (chapada), sem sombras, facilitando a visualização.
    glDisable(GL_LIGHTING);
    
    // Desenha os elásticos e a linha de mira
    drawElasticBands();
    drawAimLine();
    
    // Liga a iluminação novamente para o resto dos objetos da cena.
    glEnable(GL_LIGHTING);
}

/**
 * @brief Atualiza a física interna do estilingue (não-Bullet).
 * Chamado pelo 'timer()' global a cada quadro.
 */
void SlingshotManager::update() {
    // Esta função executa a pequena simulação de mola que faz
    // a malha balançar de volta ao centro quando solta.
    if (damageCooldown > 0.0f) {
        damageCooldown -= 0.016f; // Aproximadamente 1 frame (60 FPS)
    }
    updateElasticReturnPhysics();
}

void SlingshotManager::handleMouseScroll(int button) {
    // Só funciona se estivermos puxando
    if (isBeingPulled) {
        // Scroll UP (Geralmente 3): Puxa 'para trás' (aumenta Z)
        if (button == 4) { 
            pouchPullDepthZ += 0.3f;
            if (pouchPullDepthZ > 6.0f) pouchPullDepthZ = 6.0f;
            g_audioManager.playSlingshot(true, 30);
            printf("Scroll UP. Profundidade: %.2f\n", pouchPullDepthZ);
            
        // Scroll DOWN (Geralmente 4): Puxa 'para frente' (diminui Z)
        } else if (button == 3) { 
            pouchPullDepthZ -= 0.3f;
            if (pouchPullDepthZ < 0.0f) pouchPullDepthZ = 0.0f;
            printf("Scroll DOWN. Profundidade: %.2f\n", pouchPullDepthZ);
        }
        
        // Recalcula a posição 3D
        updatePouchPosition(); 
    }
}
/**
 * @brief Gerencia eventos de clique (pressionar/soltar) do mouse.
 * Chamado pela 'mouse()' global.
 */
void SlingshotManager::handleMouseClick(int button, int state, int x, int y) {
    // Se o jogo acabou, ignora qualquer clique
    if (*isGameOverRef) return;

    // Se o pássaro atual já foi lançado e está em voo, não permite interagir com o estilingue
    if (projectileRef && projectileRef->isEmVoo()) {
        return;
    }

    if (state == GLUT_UP && (button == 3 || button == 4)) { 
        handleMouseScroll(button);
        
        return; // Termina a função após tratar o scroll
    }
    // Só nos importamos com o botão esquerdo do mouse
    if (button == GLUT_LEFT_BUTTON) {
        
        // --- SE O BOTÃO FOI PRESSIONADO ---
        if (state == GLUT_DOWN) {
            float worldX, worldY, worldZ; // Variáveis para receber as coordenadas 3D do clique
            bool pouchWasFound = false;   // Flag para saber se acertamos a malha
            
            // Calcula a posição de repouso da malha (o "hotspot" do clique)
            float pouchRestX = (leftForkTipX + rightForkTipX) / 2.0f;
            float pouchRestY = (leftForkTipY + rightForkTipY) / 2.0f;
            float pouchRestZ = (leftForkTipZ + rightForkTipZ) / 2.0f;
            
            // Variáveis necessárias para a projeção OpenGL
            GLdouble modelview[16], projection[16];
            GLint viewport[4];
            GLdouble screenX, screenY, screenZ; // Coordenadas 2D da malha na tela
            
            // Pega as matrizes atuais de Modelo/Visão e Projeção e a Viewport
            glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
            glGetDoublev(GL_PROJECTION_MATRIX, projection);
            glGetIntegerv(GL_VIEWPORT, viewport);
            
            // 1. PROJEÇÃO: Converte a posição 3D da malha (mundo) para coordenadas 2D (tela)
            gluProject(pouchRestX, pouchRestY, pouchRestZ,
                       modelview, projection, viewport,
                       &screenX, &screenY, &screenZ);
            
            // 2. DESPROJEÇÃO: Converte a posição 2D do MOUSE (tela) para coordenadas 3D (mundo)
            //    Usamos 'screenZ' da malha para que o clique seja calculado na profundidade correta.
            screenToWorld(x, y, (float)screenZ, worldX, worldY, worldZ);
            
            // 3. VERIFICAÇÃO (Hit Test):
            //    Compara a coordenada 3D do clique com a coordenada 3D da malha
            float dist_x = fabs(worldX - pouchRestX); // Distância absoluta em X
            float dist_y = fabs(worldY - pouchRestY); // Distância absoluta em Y
            
            // Define uma "hitbox" (área de clique)
            if (dist_x < 1.5f && dist_y < 1.0f) {
                pouchWasFound = true;
            }
            
            // 4. SUCESSO!
            // Se acertamos a malha E ainda temos tiros...
            if (pouchWasFound && *shotsRemainingRef > 0) {
                printf("Estilingue capturado! Arraste e use Q/E para profundidade.\n");
                
                // Define os estados de interação
                isPouchGrabbed = true; // Agarrado!
                isBeingPulled = true;  // Puxando!
                
                // Cria o projétil "cinemático" na posição da malha
                createProjectileInPouch();
                
                // Salva as posições INICIAIS. Isso é crucial para
                // calcular o "delta" (diferença) do arrasto.
                grabMouseStartX = x;            // Posição inicial do mouse
                grabMouseStartY = y;
                grabPouchStartX = pouchPositionX; // Posição inicial da malha
                grabPouchStartY = pouchPositionY;
                grabPouchStartZ = pouchPositionZ;
                pouchPullDepthZ = 0.0f;         // Reseta a profundidade Q/E
            }
            
        // --- SE O BOTÃO FOI SOLTO ---
        } else { // state == GLUT_UP
            
            // Se estávamos segurando, puxando E tínhamos um projétil...
            if (isPouchGrabbed && isBeingPulled && projectileInPouch) {
                // ...LANCE!
                launchProjectile();
                printf("DEBUG: Tentando tocar som de soltar estilingue!\n");
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
                    // Default: Som do Red ou genérico
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
    // Se o pássaro atual já foi lançado e está em voo, não permite arrastar
    if (projectileRef && projectileRef->isEmVoo()) {
        return;
    }

    // Atualiza a posição ATUAL do mouse
    currentMouseX = x;
    currentMouseY = y;
    
    // Recalcula a posição 3D da malha (e do projétil) com base no arrasto
    updatePouchPosition();
}

/**
 * @brief Gerencia o movimento do mouse sem botão pressionado (passivo).
 * Chamado pela 'passiveMouseMotion()' global.
 */
void SlingshotManager::handlePassiveMouseMotion(int x, int y) {
    // Apenas atualiza a posição ATUAL do mouse (pode ser usado para mira, etc.)
    currentMouseX = x;
    currentMouseY = y;
}

/**
 * @brief Gerencia os eventos de teclado (Q/E) para profundidade.
 * Chamado pela 'keyboard()' global.
 */
void SlingshotManager::handleKeyboard(unsigned char key) {
    switch(key) {
    //     case 'q':
    //     case 'Q':
    //         // Só funciona se estivermos puxando
    //         if (isBeingPulled) {
    //             pouchPullDepthZ += 0.3f; // Aumenta a profundidade (puxa "para frente")
    //             if (pouchPullDepthZ > 6.0f) pouchPullDepthZ = 6.0f; // Limite
    //             updatePouchPosition(); // Atualiza a posição 3D
    //         }
    //         break;
            
    //     case 'e':
    //     case 'E':
    //         if (isBeingPulled) {
    //             pouchPullDepthZ -= 0.3f; // Diminui a profundidade (puxa "para trás")
    //             if (pouchPullDepthZ < 0.0f) pouchPullDepthZ = 0.0f; // Limite
    //             updatePouchPosition(); // Atualiza a posição 3D
    //         }
    //         break;
     }
}

/**
 * @brief Reseta o estilingue ao estado inicial.
 * Chamado pela 'keyboard()' global (tecla 'R').
 */
void SlingshotManager::reset() {
    // Limpa qualquer projétil que possa estar na malha
    clearProjectile();
    
    // Retorna a malha à sua posição de repouso
    pouchPositionX = (leftForkTipX + rightForkTipX) / 2.0f;
    pouchPositionY = (leftForkTipY + rightForkTipY) / 2.0f;
    pouchPositionZ = (leftForkTipZ + rightForkTipZ) / 2.0f;
    
    // Reseta todos os estados de interação
    isBeingPulled = false;
    isPouchGrabbed = false;
    pouchPullDepthZ = 0.0f;
    
    // Reseta dano
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
 * @brief Função "Getter" que verifica se um alvo está na mira.
 * Usado pela 'display()' global.
 */
bool SlingshotManager::isTargetInAimLine(btRigidBody* target) {
    // Se não estivermos puxando, nada está na mira
    if (!isBeingPulled || !projectileInPouch) return false;
    
    // Posição de repouso da malha
    float restX = (leftForkTipX + rightForkTipX) / 2.0f;
    float restY = (leftForkTipY + rightForkTipY) / 2.0f;
    float restZ = (leftForkTipZ + rightForkTipZ) / 2.0f;
    
    // 1. Calcula o VETOR DE MIRA (Direção do Lançamento)
    //    (Da malha PUXADA para a malha em REPOUSO)
    float dirX = restX - pouchPositionX;
    float dirY = restY - pouchPositionY;
    float dirZ = restZ - pouchPositionZ;
    
    // Normaliza o vetor de mira (transforma em um vetor de comprimento 1)
    float length = sqrt(dirX*dirX + dirY*dirY + dirZ*dirZ);
    if (length < 0.01f) return false; // Evita divisão por zero
    dirX /= length;
    dirY /= length;
    dirZ /= length;
    
    // Pega a posição 3D do alvo
    btTransform trans;
    target->getMotionState()->getWorldTransform(trans);
    btVector3 targetPos = trans.getOrigin();
    
    // 2. Calcula o VETOR PARA O ALVO
    //    (Da malha PUXADA para o ALVO)
    float toTargetX = targetPos.x() - pouchPositionX;
    float toTargetY = targetPos.y() - pouchPositionY;
    float toTargetZ = targetPos.z() - pouchPositionZ;
    
    // Normaliza o vetor para o alvo
    float targetDist = sqrt(toTargetX*toTargetX + toTargetY*toTargetY + toTargetZ*toTargetZ);
    if (targetDist < 0.01f) return false;
    toTargetX /= targetDist;
    toTargetY /= targetDist;
    toTargetZ /= targetDist;
    
    // 3. Calcula o PRODUTO ESCALAR (Dot Product)
    //    Mede o "quão alinhados" os dois vetores estão.
    float dot = dirX * toTargetX + dirY * toTargetY + dirZ * toTargetZ;
    
    // Se o resultado for 1.0, estão perfeitamente alinhados.
    // Se for > 0.95 (muito próximo de 1), consideramos que está na mira.
    return dot > 0.95f;
}

/**
 * @brief Define o novo pássaro a ser lançado.
 */
void SlingshotManager::setProjectile(Passaro* novoPassaro) {
    // Limpa qualquer projétil atual
    clearProjectile();
    
    // Atualiza a referência
    projectileRef = novoPassaro;

    
    
    // Reseta estados de interação
    isBeingPulled = false;
    isPouchGrabbed = false;
    pouchPullDepthZ = 0.0f;
    
    // Retorna a malha para a posição de repouso
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


// 5. Implementação das Funções Privadas
// (Funções auxiliares que só a própria classe pode chamar)

/**
 * @brief Remove e deleta com segurança o projétil da malha.
 */
void SlingshotManager::clearProjectile() {
    // Pede ao pássaro para limpar sua própria física
    if (projectileRef && projectileRef->getRigidBody()) {
        projectileRef->limparFisica();
    }
    // Limpa nosso ponteiro de rastreamento
    projectileInPouch = nullptr;
}


/**
 * @brief Cria um novo projétil e o coloca na malha como um objeto "cinemático". (MODIFICADO)
 */
void SlingshotManager::createProjectileInPouch() {
    // Garante que a malha esteja vazia antes de criar um novo
    clearProjectile();
    
    // Checagem de segurança
    if (!projectileRef || !worldRef) return; 

    // Posição de repouso (onde o projétil é criado)
    float restX = (leftForkTipX + rightForkTipX) / 2.0f;
    float restY = (leftForkTipY + rightForkTipY) / 2.0f;
    float restZ = (leftForkTipZ + rightForkTipZ) / 2.0f;
    
    // Define a posição da malha para o ponto de repouso
    pouchPositionX = restX;
    pouchPositionY = restY;
    pouchPositionZ = restZ;
    
    // --- [ MUDANÇA CRÍTICA ] ---
    
    // 1. Reseta o pássaro para a posição inicial
    projectileRef->resetar(restX, restY, restZ); 
    projectileRef->setEmVoo(false); // Garante que o pássaro NÃO está em voo enquanto está no estilingue
    // 2. Pede ao pássaro para criar seu corpo físico no mundo
    projectileRef->inicializarFisica(worldRef, restX, restY, restZ);
    
    // 3. Armazena o ponteiro para o corpo rígido que o pássaro acabou de criar
    projectileInPouch = projectileRef->getRigidBody();

    if (!projectileInPouch) {
        printf("ERRO (SlingshotManager): Falha ao inicializar a fisica do passaro!\n");
        return;
    }
    
    // --- NOVA LÓGICA: Ignorar colisão entre Pássaro e Estilingue ---
    if (slingshotBody) {
        projectileInPouch->setIgnoreCollisionCheck(slingshotBody, true);
        slingshotBody->setIgnoreCollisionCheck(projectileInPouch, true);
    }

    // 4. Define o corpo como CINEMÁTICO (controlado por nós, não pelo Bullet)
    projectileInPouch->setCollisionFlags(projectileInPouch->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
    projectileInPouch->setActivationState(DISABLE_DEACTIVATION); // Impede de "dormir"
    
    // (Não adicionamos ao worldRef, pois 'inicializarFisica' já deve fazer isso)
}
/**
 * @brief Lança o projétil, devolvendo seu controle para a física Bullet. (MODIFICADO)
 */
void SlingshotManager::launchProjectile() {
    // Segurança: não faz nada se não houver projétil ou pássaro
    if (!projectileInPouch || !projectileRef) return;

    printf("Lancando projetil (Passaro)!\n");
    
    // Posição de repouso
    float restX = (leftForkTipX + rightForkTipX) / 2.0f;
    float restY = (leftForkTipY + rightForkTipY) / 2.0f;
    float restZ = (leftForkTipZ + rightForkTipZ) / 2.0f;
    
    // Calcula o vetor de impulso
    float impulseX = restX - pouchPositionX;
    float impulseY = restY - pouchPositionY;
    float impulseZ = restZ - pouchPositionZ;
    
    // --- [ MUDANÇA CRÍTICA ] ---
    
    // 1. Remove o flag "CINEMÁTICO", devolvendo o controle ao Bullet
    projectileInPouch->setCollisionFlags(projectileInPouch->getCollisionFlags() & ~btCollisionObject::CF_KINEMATIC_OBJECT);
    projectileInPouch->forceActivationState(ACTIVE_TAG); // "Acorda" o objeto
    
    // 2. Define propriedades de física para o voo
    projectileInPouch->setRestitution(0.6f);
    projectileInPouch->setFriction(0.5f);
    projectileInPouch->setRollingFriction(0.1f);
    projectileInPouch->setDamping(0.05f, 0.1f); // Simula resistência do ar
    
    // Zera qualquer velocidade anterior (cinemática)
    projectileInPouch->setLinearVelocity(btVector3(0,0,0));
    projectileInPouch->setAngularVelocity(btVector3(0,0,0));
    
    // 3. Define a força do lançamento
    float forceMagnitude = 20.0f; // Ajuste este valor
    btVector3 impulse(impulseX * forceMagnitude, impulseY * forceMagnitude, impulseZ * forceMagnitude);
    
    // 4. Aplica o "peteleco" (impulso)
    projectileInPouch->applyCentralImpulse(impulse);

    // 5. Avisa a classe Passaro que ela está em voo (baseado no README)
    projectileRef->setEmVoo(true);
    std::string tipo_passaro = projectileRef->getTipo();
    // Toca som de lançamento
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
        // Default: Som do Red ou genérico
        g_audioManager.playPassaro(SomTipo::LANCAMENTO_PASSARO);
    }

    // O projétil foi lançado. Ele não está mais "na nossa malha".
    projectileInPouch = nullptr; 
    
    // Usa o ponteiro para a variável global para decrementar os tiros
    (*shotsRemainingRef)--;
    
    // Verifica se os tiros acabaram
    if (*shotsRemainingRef <= 0) {
        *isGameOverRef = true; // Define o estado global de fim de jogo
    }
}
/**
 * @brief Função auxiliar para desenhar um cilindro 3D entre dois pontos.
 */
void SlingshotManager::drawCylinder(float x1, float y1, float z1, float x2, float y2, float z2, float radius) {
    // Calcula o vetor (direção) e o comprimento do cilindro
    float dx = x2 - x1;
    float dy = y2 - y1;
    float dz = z2 - z1;
    float length = sqrt(dx*dx + dy*dy + dz*dz);
    
    if (length == 0.0) return; // Evita divisão por zero
    
    // Salva a matriz de transformação atual
    glPushMatrix();
    
    // 1. Translação: Move o sistema de coordenadas para o ponto inicial (x1, y1, z1)
    glTranslatef(x1, y1, z1);
    
    // 2. Rotação: Calcula o eixo e o ângulo para alinhar o cilindro (que
    //    por padrão é desenhado ao longo do eixo Z) com o nosso vetor (dx, dy, dz).
    //    Isso usa matemática vetorial (Produto Vetorial e Produto Escalar).
    float z_axis[3] = {0.0, 0.0, 1.0}; // O eixo padrão do cilindro do GLU
    float vector[3] = {dx/length, dy/length, dz/length}; // Nosso vetor normalizado
    float axis[3]; // O eixo de rotação (resultado do Produto Vetorial)
    axis[0] = z_axis[1] * vector[2] - z_axis[2] * vector[1];
    axis[1] = z_axis[2] * vector[0] - z_axis[0] * vector[2];
    axis[2] = z_axis[0] * vector[1] - z_axis[1] * vector[0];
    float dot_product = z_axis[0]*vector[0] + z_axis[1]*vector[1] + z_axis[2]*vector[2];
    float angle = acos(dot_product) * 180.0 / M_PI; // O ângulo de rotação (do Produto Escalar)
    
    // Correção para o caso de o vetor ser oposto (ângulo de 180 graus)
    if (dot_product < -0.99999) {
        axis[0] = 1.0; axis[1] = 0.0; axis[2] = 0.0;
        angle = 180.0;
    }
    
    // Aplica a rotação
    glRotatef(angle, axis[0], axis[1], axis[2]);
    
    // 3. Desenho: Desenha o cilindro
    GLUquadric* quad = gluNewQuadric();
    gluCylinder(quad, radius, radius, length, 16, 16); // (raio_base, raio_topo, altura, fatias, pilhas)
    gluDeleteQuadric(quad);
    
    // Restaura a matriz de transformação original
    glPopMatrix();
}

/**
 * @brief Desenha a base de madeira usando a função drawCylinder.
 */
void SlingshotManager::drawWoodenBase() {
    glColor3f(0.30f, 0.18f, 0.06f); // Cor de madeira
    
    // Desenha o cabo (da base até o centro da forquilha)
    drawCylinder(handleBaseX, handleBaseY, handleBaseZ,
                 leftArmBaseX, leftArmBaseY, leftArmBaseZ,
                 0.3); // Raio de 0.3
    
    // Desenha o braço esquerdo da forquilha
    drawCylinder(leftArmBaseX, leftArmBaseY, leftArmBaseZ,
                 leftForkTipX, leftForkTipY, leftForkTipZ,
                 0.2); // Raio de 0.2
    
    // Desenha o braço direito da forquilha
    drawCylinder(rightArmBaseX, rightArmBaseY, rightArmBaseZ,
                 rightForkTipX, rightForkTipY, rightForkTipZ,
                 0.2); // Raio de 0.2

    // Desenha rachaduras se houver dano
    if (damageCount > 0) {
        drawCracks();
    }
}

/**
 * @brief Desenha as duas bandas elásticas como linhas 3D.
 */
void SlingshotManager::drawElasticBands() {
    glColor3f(0.05f, 0.05f, 0.05f); // Cor de borracha escura
    glLineWidth(12.0f); // Define a espessura da linha
    glBegin(GL_LINES); // Começa a desenhar linhas
        // Linha 1: da ponta esquerda até a malha
        glVertex3f(leftForkTipX, leftForkTipY, leftForkTipZ);
        glVertex3f(pouchPositionX, pouchPositionY, pouchPositionZ);
        // Linha 2: da ponta direita até a malha
        glVertex3f(rightForkTipX, rightForkTipY, rightForkTipZ);
        glVertex3f(pouchPositionX, pouchPositionY, pouchPositionZ);
    glEnd(); // Termina de desenhar linhas
}

/**
 * @brief Desenha a linha de mira pontilhada.
 */
/**
 * @brief Desenha a linha de mira (MODIFICADO PARA PARÁBOLA)
 */
void SlingshotManager::drawAimLine() {
    // Só desenha se estivermos ativamente puxando um projétil
    if (isBeingPulled && projectileRef) {
        
        // --- 1. Obter Posição Inicial (p0) ---
        // A posição inicial é a malha puxada
        btVector3 p0(pouchPositionX, pouchPositionY, pouchPositionZ);

        // --- 2. Calcular Velocidade Inicial (v0) ---
        // Posição de repouso (para onde a mira aponta)
        float restX = (leftForkTipX + rightForkTipX) / 2.0f;
        float restY = (leftForkTipY + rightForkTipY) / 2.0f;
        float restZ = (leftForkTipZ + rightForkTipZ) / 2.0f;

        // Vetor de impulso (da malha puxada para a posição de repouso)
        float impulseX = restX - pouchPositionX;
        float impulseY = restY - pouchPositionY;
        float impulseZ = restZ - pouchPositionZ;
        
        // Constantes da física (DEVEM ser iguais às de launchProjectile e initBullet)
        const float forceMagnitude = 20.0f; // Mesmo valor de launchProjectile
        const float mass = projectileRef->getMassa(); // Massa do pássaro
        const float gravityY = -9.81f; // Gravidade do seu mundo

        // Calcular a velocidade inicial (v0 = (Impulso * Força) / Massa)
        btVector3 v0(
            (impulseX * forceMagnitude) / mass,
            (impulseY * forceMagnitude) / mass,
            (impulseZ * forceMagnitude) / mass
        );

        // --- 3. Desenhar a Parábola ---
        glColor4f(1.0f, 1.0f, 1.0f, 0.8f); 
        glLineWidth(3.0f);
        
        // Ativa o "pontilhado" da linha
        glEnable(GL_LINE_STIPPLE);
        glLineStipple(3, 0xAAAA); // Mesmo pontilhado de antes
        
        glBegin(GL_LINES); // Usa GL_LINES para desenhar segmentos pontilhados

        int numSegments = 20; // Quantos segmentos de linha desenhar
        float timeStep = 0.08f; // O "espaço" de tempo de cada segmento (ajuste se necessário)
        
        for (int i = 0; i < numSegments; i++) {
            // Fórmula da trajetória: p(t) = p0 + v0*t + 0.5*a*t^2
            
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
        
        // Desativa o "pontilhado"
        glDisable(GL_LINE_STIPPLE);
    }
}

/**
 * @brief Simula a malha voltando ao lugar (física de mola simples).
 * Esta é uma física "fake", separada do Bullet, apenas para efeito visual.
 */
void SlingshotManager::updateElasticReturnPhysics() {
    // Só executa se NÃO estivermos puxando E NÃO houver projétil
    if (!isBeingPulled && !projectileInPouch) {
        
        // Posição de repouso (o "alvo" da mola)
        float restX = (leftForkTipX + rightForkTipX) / 2.0f;
        float restY = (leftForkTipY + rightForkTipY) / 2.0f;
        float restZ = (leftForkTipZ + rightForkTipZ) / 2.0f;
        
        // Vetor de deslocamento (o "X" da Lei de Hooke)
        float dx = pouchPositionX - restX;
        float dy = pouchPositionY - restY;
        float dz = pouchPositionZ - restZ;
        
        // Constantes da física da mola
        float k = 25.0f; // Constante da mola (quão "dura" ela é)
        static float vx = 0.0f, vy = 0.0f, vz = 0.0f; // Velocidade da malha (estática)
        float damping = 0.88f; // Amortecimento (para ela parar de balançar)
        
        // Lei de Hooke (F = -k * x): Calcula a força da mola
        float fx = -k * dx;
        float fy = -k * dy;
        float fz = -k * dz;
        
        float mass = 0.08f; // Massa da malha
        // F = m * a  =>  a = F / m
        float ax = fx / mass; // Aceleração
        float ay = fy / mass;
        float az = fz / mass;
        
        float dt = 0.016f; // Delta time (passo de tempo, ~60fps)
        
        // v = v0 + a * t
        vx += ax * dt; // Atualiza velocidade
        vy += ay * dt;
        vz += az * dt;
        
        // Aplica amortecimento (freio)
        vx *= damping;
        vy *= damping;
        vz *= damping;
        
        // p = p0 + v * t
        pouchPositionX += vx * dt; // Atualiza posição
        pouchPositionY += vy * dt;
        pouchPositionZ += vz * dt;
        
        // Condição de parada: se estiver perto E devagar, trava no lugar
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
    // Só atualiza se o mouse estiver pressionado E segurando a malha
    if (isPouchGrabbed && isBeingPulled) {
        
        // 1. Calcula o Delta (diferença) do mouse em pixels
        int deltaX_pixels = currentMouseX - grabMouseStartX;
        int deltaY_pixels = currentMouseY - grabMouseStartY;
        
        float sensitivityX = 0.04f; // Sensibilidade do mouse
        float sensitivityY = 0.04f;
        
        // 2. Converte o Delta de pixels para Delta 3D
        float dx = (float)deltaX_pixels * sensitivityX;
        float dy = (float)-deltaY_pixels * sensitivityY; // Y do mouse é invertido
        float dz = pouchPullDepthZ; // Profundidade do Q/E
        
        // 3. Limita a distância máxima que se pode puxar
        float maxPull = 7.0f;
        float currentDist = sqrt(dx*dx + dy*dy + dz*dz);
        if (currentDist > maxPull) {
            // Se passou do limite, "normaliza" o vetor e o escala para o máximo
            dx = (dx / currentDist) * maxPull;
            dy = (dy / currentDist) * maxPull;
            dz = (dz / currentDist) * maxPull;
        }
        
        // 4. Calcula a nova Posição 3D da Malha
        //    Posição = Posição_Inicial_Salva + Delta_3D
        pouchPositionX = grabPouchStartX + dx;
        pouchPositionY = grabPouchStartY + dy;
        pouchPositionZ = grabPouchStartZ + dz;
        
        // 5. Atualiza o Projétil Cinemático
        //    Se houver um projétil na malha...
        if (projectileInPouch) {
            btTransform trans;
            // Pega a transformação atual dele
            projectileInPouch->getMotionState()->getWorldTransform(trans);
            // Define a origem (posição) dele para ser EXATAMENTE
            // a nova posição da malha que acabamos de calcular.
            trans.setOrigin(btVector3(pouchPositionX, pouchPositionY, pouchPositionZ));
            // Aplica a transformação de volta ao projétil
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
    // Pega as matrizes e viewport atuais
    GLint viewport[4];
    GLdouble modelview[16];
    GLdouble projection[16];
    GLdouble posX, posY, posZ; // Variáveis de saída
    
    glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
    glGetDoublev(GL_PROJECTION_MATRIX, projection);
    glGetIntegerv(GL_VIEWPORT, viewport);
    
    // Converte as coordenadas do mouse (origem no topo-esquerdo)
    // para coordenadas de janela OpenGL (origem no baixo-esquerdo)
    GLfloat winX = (float)screenX;
    GLfloat winY = (float)viewport[3] - (float)screenY; // Inverte o Y
    
    // Função mágica do GLU que faz a conversão 2D -> 3D
    gluUnProject(winX, winY, depth, 
                 modelview, projection, viewport, 
                 &posX, &posY, &posZ);
    
    // Retorna os valores 3D usando as referências
    worldX = posX;
    worldY = posY;
    worldZ = posZ;
}

void SlingshotManager::takeDamage() {
    if (damageCooldown > 0.0f) return;

    damageCount++;
    damageCooldown = 2.0f; // <--- Fica invencível por 2 segundos após tomar tiro
    
    printf("Estilingue atingido! Dano: %d/3\n", damageCount);
    g_audioManager.play(SomTipo::COLISAO_MADEIRA);
    
    if (damageCount >= 3) {
        triggerGameOver();
    }
    g_audioManager.play(SomTipo::COLISAO_MADEIRA);
}

void SlingshotManager::triggerGameOver() {
    printf("GAME OVER: Estilingue destruido!\n");
    if (isGameOverRef) {
        *isGameOverRef = true;
    }
    // Opcional: Tocar som de destruição total
    g_audioManager.play(SomTipo::BLOCO_MADEIRA_DESTRUIDO);
}

btRigidBody* SlingshotManager::getRigidBody() const {
    return slingshotBody;
}

void SlingshotManager::drawCracks() {
    glDisable(GL_LIGHTING);
    glColor3f(0.0f, 0.0f, 0.0f); // Preto para as rachaduras
    glLineWidth(2.0f);

    // Rachaduras simples (linhas aleatórias pré-definidas)
    glBegin(GL_LINES);
    
    if (damageCount >= 1) {
        // Rachadura no cabo
        glVertex3f(handleBaseX - 0.2f, handleBaseY + 1.0f, handleBaseZ + 0.4f);
        glVertex3f(handleBaseX + 0.2f, handleBaseY + 1.5f, handleBaseZ + 0.4f);
        
        glVertex3f(handleBaseX + 0.2f, handleBaseY + 1.5f, handleBaseZ + 0.4f);
        glVertex3f(handleBaseX - 0.1f, handleBaseY + 2.0f, handleBaseZ + 0.4f);
    }
    
    if (damageCount >= 2) {
        // Rachadura no braço esquerdo
        glVertex3f(leftArmBaseX, leftArmBaseY + 0.5f, leftArmBaseZ + 0.3f);
        glVertex3f(leftArmBaseX - 0.3f, leftArmBaseY + 1.5f, leftArmBaseZ + 0.3f);
    }
    
    if (damageCount >= 3) {
        // Rachadura no braço direito (crítica)
        glVertex3f(rightArmBaseX, rightArmBaseY + 0.5f, rightArmBaseZ + 0.3f);
        glVertex3f(rightArmBaseX + 0.3f, rightArmBaseY + 1.5f, rightArmBaseZ + 0.3f);
        
        glVertex3f(handleBaseX, handleBaseY + 2.5f, handleBaseZ + 0.4f);
        glVertex3f(handleBaseX, handleBaseY + 0.5f, handleBaseZ + 0.4f);
    }

    glEnd();
    glEnable(GL_LIGHTING);
}