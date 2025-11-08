// 1. Inclui o header correspondente
// Este include "ensina" ao .cpp qual é a estrutura da classe
// (quais funções e variáveis ela tem) que foi definida no .h
#include "SlingshotManager.h"

// 2. Inclui todas as bibliotecas necessárias para a IMPLEMENTAÇÃO
#include <GL/glut.h> // Necessário para funções OpenGL/GLUT (gl..., glu...)
#include <GL/glu.h>  // Necessário para funções GLU (gluProject, gluCylinder, etc.)
#include <cmath>     // Necessário para funções matemáticas (sin, cos, sqrt, fabs)
#include <cstdio>  // Necessário para a função de console 'printf'

// (Constante M_PI, se não estiver definida)
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// 3. Implementação do Construtor
// A sintaxe "SlingshotManager::" significa que esta função "SlingshotManager"
// pertence à classe "SlingshotManager". Este é o construtor.
SlingshotManager::SlingshotManager(btDiscreteDynamicsWorld* world, btCollisionShape* shape, int* shots, bool* game_over)
    // --- Lista de Inicialização de Membros ---
    // Esta é a forma correta e eficiente em C++ de atribuir os valores
    // recebidos pelo construtor às variáveis de membro da classe.
    : worldRef(world),               // Atribui o ponteiro 'world' à nossa variável interna 'worldRef'
      projectileShapeRef(shape),     // Atribui o 'shape' ao 'projectileShapeRef'
      shotsRemainingRef(shots),      // Atribui o ponteiro 'shots' ao 'shotsRemainingRef'
      isGameOverRef(game_over),    // Atribui o ponteiro 'game_over' ao 'isGameOverRef'
      
      // --- Inicialização dos Estados Internos ---
      // Define todos os valores padrão para o estado inicial do estilingue.
      projectileInPouch(nullptr),  // Começa sem nenhum projétil na malha
      isBeingPulled(false),      // Não está sendo puxado
      isPouchGrabbed(false),     // A malha não está agarrada
      currentMouseX(0),          // Posição inicial do mouse
      currentMouseY(0),
      grabMouseStartX(0),        // Posição inicial do clique (será definida ao clicar)
      grabMouseStartY(0),
      grabPouchStartX(0),        // Posição inicial da malha (será definida ao clicar)
      grabPouchStartY(0),
      grabPouchStartZ(0),
      pouchPullDepthZ(0.0f)      // Profundidade inicial (controlada por Q/E)
{
    // O corpo do construtor.
    // Agora que todas as variáveis estão inicializadas,
    // chamamos nossa função interna para calcular a geometria do estilingue.
    initGeometry();
}

// 4. Implementação das Funções Públicas

/**
 * @brief Calcula e define a geometria 3D estática do estilingue.
 * Esta função é chamada apenas uma vez, pelo construtor.
 */
void SlingshotManager::initGeometry() {
    // Define a posição da base (cabo) e o início dos braços da forquilha
    handleBaseX = 0.0f;
    handleBaseY = 0.0f;
    handleBaseZ = 0.0f;
    
    leftArmBaseX = 0.0f;
    leftArmBaseY = 3.0f;
    leftArmBaseZ = 0.0f;
    
    rightArmBaseX = 0.0f;
    rightArmBaseY = 3.0f;
    rightArmBaseZ = 0.0f;
    
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
    updateElasticReturnPhysics();
}

/**
 * @brief Gerencia eventos de clique (pressionar/soltar) do mouse.
 * Chamado pela 'mouse()' global.
 */
void SlingshotManager::handleMouseClick(int button, int state, int x, int y) {
    // Se o jogo acabou, ignora qualquer clique
    if (*isGameOverRef) return;

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
        case 'q':
        case 'Q':
            // Só funciona se estivermos puxando
            if (isBeingPulled) {
                pouchPullDepthZ += 0.3f; // Aumenta a profundidade (puxa "para frente")
                if (pouchPullDepthZ > 6.0f) pouchPullDepthZ = 6.0f; // Limite
                updatePouchPosition(); // Atualiza a posição 3D
            }
            break;
            
        case 'e':
        case 'E':
            if (isBeingPulled) {
                pouchPullDepthZ -= 0.3f; // Diminui a profundidade (puxa "para trás")
                if (pouchPullDepthZ < 0.0f) pouchPullDepthZ = 0.0f; // Limite
                updatePouchPosition(); // Atualiza a posição 3D
            }
            break;
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


// 5. Implementação das Funções Privadas
// (Funções auxiliares que só a própria classe pode chamar)

/**
 * @brief Remove e deleta com segurança o projétil da malha.
 */
void SlingshotManager::clearProjectile() {
    // Só faz algo se houver um projétil
    if (projectileInPouch) {
        // Remove o corpo do mundo de física Bullet
        worldRef->removeRigidBody(projectileInPouch);
        
        // Deleta os componentes do corpo da memória
        delete projectileInPouch->getMotionState();
        delete projectileInPouch;
        
        // Define o ponteiro como nulo para evitar "ponteiros selvagens"
        projectileInPouch = nullptr;
    }
}

/**
 * @brief Cria um novo projétil e o coloca na malha como um objeto "cinemático".
 */
void SlingshotManager::createProjectileInPouch() {
    // Garante que a malha esteja vazia antes de criar um novo
    clearProjectile();
    
    // Posição de repouso (onde o projétil é criado)
    float restX = (leftForkTipX + rightForkTipX) / 2.0f;
    float restY = (leftForkTipY + rightForkTipY) / 2.0f;
    float restZ = (leftForkTipZ + rightForkTipZ) / 2.0f;
    
    // Define a posição da malha para o ponto de repouso
    pouchPositionX = restX;
    pouchPositionY = restY;
    pouchPositionZ = restZ;
    
    // --- Criação Padrão do Bullet ---
    btDefaultMotionState* motionState = new btDefaultMotionState(
        btTransform(btQuaternion(0, 0, 0, 1), btVector3(restX, restY, restZ)));
    
    btVector3 localInertia(0, 0, 0);
    float mass = 1.0f;
    projectileShapeRef->calculateLocalInertia(mass, localInertia);
    
    btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, motionState, projectileShapeRef, localInertia);
    
    // Armazena o novo corpo rígido na nossa variável
    projectileInPouch = new btRigidBody(rbInfo);
    
    // Define propriedades físicas
    projectileInPouch->setRestitution(0.6f);
    projectileInPouch->setFriction(0.5f);
    projectileInPouch->setRollingFriction(0.1f);
    
    // --- [ A PARTE MAIS IMPORTANTE ] ---
    // Define o flag "CINEMÁTICO" (CF_KINEMATIC_OBJECT).
    // Isso diz ao Bullet: "Não aplique física (gravidade, etc.) neste objeto.
    // Eu (nosso código) vou controlar sua posição manualmente a cada quadro."
    projectileInPouch->setCollisionFlags(projectileInPouch->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
    projectileInPouch->setActivationState(DISABLE_DEACTIVATION); // Impede de "dormir"
    
    // Adiciona o projétil (agora cinemático) ao mundo
    worldRef->addRigidBody(projectileInPouch);
}

/**
 * @brief Lança o projétil, devolvendo seu controle para a física Bullet.
 */
void SlingshotManager::launchProjectile() {
    // Segurança: não faz nada se não houver projétil
    if (!projectileInPouch) return;

    printf("Lancando projetil!\n");
    
    // Posição de repouso
    float restX = (leftForkTipX + rightForkTipX) / 2.0f;
    float restY = (leftForkTipY + rightForkTipY) / 2.0f;
    float restZ = (leftForkTipZ + rightForkTipZ) / 2.0f;
    
    // Calcula o vetor de impulso (da malha puxada para a malha em repouso)
    float impulseX = restX - pouchPositionX;
    float impulseY = restY - pouchPositionY;
    float impulseZ = restZ - pouchPositionZ;
    
    // --- [ A PARTE MAIS IMPORTANTE ] ---
    // Remove o flag "CINEMÁTICO" usando o operador E (bitwise AND)
    // e o operador NÃO (bitwise NOT, ~).
    // Isso diz ao Bullet: "Ok, eu não controlo mais este objeto.
    // Ele agora é um objeto DINÂMICO. Assuma o controle (gravidade, colisões, etc.)."
    projectileInPouch->setCollisionFlags(projectileInPouch->getCollisionFlags() & ~btCollisionObject::CF_KINEMATIC_OBJECT);
    projectileInPouch->forceActivationState(ACTIVE_TAG); // "Acorda" o objeto
    
    // Redefine propriedades físicas (agora que é dinâmico)
    projectileInPouch->setRestitution(0.6f);
    projectileInPouch->setFriction(0.5f);
    projectileInPouch->setRollingFriction(0.1f);
    projectileInPouch->setDamping(0.05f, 0.1f); // Simula resistência do ar
    
    // Zera qualquer velocidade anterior (cinemática)
    projectileInPouch->setLinearVelocity(btVector3(0,0,0));
    projectileInPouch->setAngularVelocity(btVector3(0,0,0));
    
    // Define a força do lançamento
    float forceMagnitude = 20.0f;
    btVector3 impulse(impulseX * forceMagnitude, impulseY * forceMagnitude, impulseZ * forceMagnitude);
    
    // Aplica o "peteleco" (impulso) no centro do objeto
    projectileInPouch->applyCentralImpulse(impulse);
    
    // O projétil foi lançado. Ele não está mais "na nossa malha".
    // Limpamos o ponteiro. O objeto *continua existindo* no mundo Bullet,
    // mas não é mais o projétil que estamos controlando.
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
    glColor3f(0.5f, 0.35f, 0.05f); // Cor de madeira
    
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
void SlingshotManager::drawAimLine() {
    // Só desenha se estivermos ativamente puxando um projétil
    if (isBeingPulled && projectileInPouch) {
        
        // Posição de repouso (para onde a mira aponta)
        float restX = (leftForkTipX + rightForkTipX) / 2.0f;
        float restY = (leftForkTipY + rightForkTipY) / 2.0f;
        float restZ = (leftForkTipZ + rightForkTipZ) / 2.0f;
        
        // Vetor de direção (da malha puxada para a posição de repouso)
        float dirX = restX - pouchPositionX;
        float dirY = restY - pouchPositionY;
        float dirZ = restZ - pouchPositionZ;
        float length = sqrt(dirX*dirX + dirY*dirY + dirZ*dirZ);

        if (length > 0.01f) { // Só desenha se houver uma direção
            // Normaliza o vetor de direção
            dirX /= length;
            dirY /= length;
            dirZ /= length;
            
            glColor4f(1.0f, 1.0f, 0.0f, 0.8f); // Amarelo translúcido
            glLineWidth(3.0f);
            
            // Ativa o "pontilhado" da linha
            glEnable(GL_LINE_STIPPLE);
            glLineStipple(3, 0xAAAA); // Define o padrão (3 pixels ligados, 3 desligados, etc.)
            
            glBegin(GL_LINES);
                // Ponto inicial (na malha)
                glVertex3f(pouchPositionX, pouchPositionY, pouchPositionZ);
                // Ponto final (15 unidades à frente na direção da mira)
                glVertex3f(pouchPositionX + dirX * 15.0f, 
                           pouchPositionY + dirY * 15.0f, 
                           pouchPositionZ + dirZ * 15.0f);
            glEnd();
            
            // Desativa o "pontilhado"
            glDisable(GL_LINE_STIPPLE);
        }
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