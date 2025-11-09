// Arquivo: SlingshotManager.h (Corrigido)

#ifndef SLINGSHOTMANAGER_H
#define SLINGSHOTMANAGER_H

// 2. Includes necessários para a DECLARAÇÃO
// CORRIGIDO: Adicionado o prefixo "bullet/"
#include <btBulletDynamicsCommon.h> 

/**
 * @class SlingshotManager
 * @brief Gerencia toda a lógica, estado e renderização do Estilingue.
 */
class SlingshotManager {
private:
    // --- Geometria do Estilingue (Posições) ---
    float handleBaseX, handleBaseY, handleBaseZ;
    float leftArmBaseX, leftArmBaseY, leftArmBaseZ;
    float rightArmBaseX, rightArmBaseY, rightArmBaseZ;
    float leftForkTipX, leftForkTipY, leftForkTipZ;
    float rightForkTipX, rightForkTipY, rightForkTipZ;
    float pouchPositionX, pouchPositionY, pouchPositionZ;

    // --- Estado de Interação ---
    bool isPouchGrabbed;
    bool isBeingPulled; 
    int currentMouseX, currentMouseY;
    int grabMouseStartX, grabMouseStartY;
    float grabPouchStartX, grabPouchStartY, grabPouchStartZ;
    float pouchPullDepthZ;

    // --- Referências Externas (Ponteiros) ---
    btDiscreteDynamicsWorld* worldRef;
    btCollisionShape* projectileShapeRef;
    btRigidBody* projectileInPouch; 
    int* shotsRemainingRef;
    bool* isGameOverRef;

    // --- Funções Internas (Privadas) ---
    // CORRIGIDO: Removidos os corpos das funções, deixando apenas a declaração
    void clearProjectile();
    void createProjectileInPouch();
    void launchProjectile();
    void drawCylinder(float x1, float y1, float z1, float x2, float y2, float z2, float radius);
    void drawWoodenBase();
    void drawElasticBands();
    void drawAimLine();
    void updateElasticReturnPhysics();
    void updatePouchPosition();
    void screenToWorld(int screenX, int screenY, float depth, float& worldX, float& worldY, float& worldZ);

public:
    /**
     * @brief Construtor do SlingshotManager.
     */
    // CORRIGIDO: Removido o corpo da função
    SlingshotManager(btDiscreteDynamicsWorld* world, btCollisionShape* shape, int* shots, bool* game_over);
    
    // --- Funções Públicas de Interface ---

    /**
     * @brief Calcula e define a geometria 3D do estilingue.
     */
    // CORRIGIDO: Removidos os corpos das funções
    void initGeometry();

    /**
     * @brief Desenha o estilingue (madeira, elásticos e mira).
     */
    void draw();

    /**
     * @brief Atualiza a física interna do estilingue.
     */
    void update();

    /**
     * @brief Gerencia eventos de clique do mouse.
     */
    void handleMouseClick(int button, int state, int x, int y);

    /**
     * @brief Gerencia o arrastar do mouse (com botão pressionado).
     */
    void handleMouseDrag(int x, int y);

    /**
     * @brief Gerencia o movimento do mouse (sem botão pressionado).
     */
    void handlePassiveMouseMotion(int x, int y);

    /**
     * @brief Gerencia eventos de teclado (Q/E).
     */
    void handleKeyboard(unsigned char key);

    /**
     * @brief Reseta o estilingue para o estado inicial.
     */
    void reset();

    /**
     * @brief Retorna um ponteiro para o projétil na malha.
     */
    btRigidBody* getProjectileInPouch() const;

    /**
     * @brief Verifica se um corpo rígido (alvo) está na linha de mira.
     */
    bool isTargetInAimLine(btRigidBody* target);
};

#endif // SLINGSHOTMANAGER_H