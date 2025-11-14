#pragma once

#include <vector>
#include <btBulletDynamicsCommon.h>
#include <GL/glut.h>

// Para rand() e RAND_MAX
#include <cstdlib> 
#include <cmath>

/**
 * @struct Particle
 * @brief Armazena o estado de uma única partícula.
 */
struct Particle {
    btVector3 pos;      // Posição
    btVector3 vel;      // Velocidade
    btVector3 color;    // Cor
    float life;         // Tempo de vida restante (em segundos)
    bool active;        // Está ativa?

    Particle() : life(0.0f), active(false) {}
};

/**
 * @class ParticleManager
 * @brief Gerencia um pool de partículas para explosões.
 */
class ParticleManager {
private:
    std::vector<Particle> particles; // Nosso pool de partículas
    int maxParticles;
    float gravity;

    // Retorna um float aleatório entre -1.0 e 1.0
    float randFloat() {
        return (float(rand()) / (float(RAND_MAX) / 2.0f)) - 1.0f;
    }

public:
    ParticleManager(int maxP = 1000) : maxParticles(maxP), gravity(9.81f) {
        // Pré-aloca o pool de partículas
        particles.resize(maxParticles);
    }

    /**
     * @brief Encontra uma partícula inativa e a "acorda".
     */
    void spawn(const btVector3& pos, const btVector3& vel, const btVector3& color, float life) {
        for (int i = 0; i < maxParticles; i++) {
            if (!particles[i].active) {
                particles[i].active = true;
                particles[i].pos = pos;
                particles[i].vel = vel;
                particles[i].color = color;
                particles[i].life = life;
                return; // Sai após encontrar um slot
            }
        }
    }

    /**
     * @brief Cria uma explosão de N partículas em uma posição.
     */
    void createExplosion(const btVector3& pos, const btVector3& color, int count = 30) {
        for (int i = 0; i < count; i++) {
            // Velocidade aleatória para cima e para fora
            btVector3 vel(
                randFloat() * 5.0f,     // Velocidade X aleatória
                (randFloat() + 1.0f) * 4.0f, // Velocidade Y (sempre para cima)
                randFloat() * 5.0f      // Velocidade Z aleatória
            );
            
            // Tempo de vida aleatório (entre 0.5 e 1.5 segundos)
            float life = 0.5f + (float(rand()) / float(RAND_MAX));
            
            spawn(pos, vel, color, life);
        }
    }

    /**
     * @brief Atualiza a física de todas as partículas ativas.
     */
    void update(float deltaTime) {
        for (int i = 0; i < maxParticles; i++) {
            if (particles[i].active) {
                // Aplica gravidade
                particles[i].vel.setY(particles[i].vel.y() - gravity * deltaTime);
                // Move a partícula
                particles[i].pos += particles[i].vel * deltaTime;
                // Reduz a vida
                particles[i].life -= deltaTime;

                // Desativa se a vida acabar ou se cair pelo chão
                if (particles[i].life <= 0.0f || particles[i].pos.y() < -1.0f) {
                    particles[i].active = false;
                }
            }
        }
    }

    /**
     * @brief Desenha todas as partículas ativas.
     */
    void draw() {
        // Desliga a iluminação para as partículas (para que a cor apareça pura)
        glDisable(GL_LIGHTING);
        // Define o tamanho dos pontos (partículas)
        glPointSize(3.0f);

        glBegin(GL_POINTS);
        for (int i = 0; i < maxParticles; i++) {
            if (particles[i].active) {
                glColor3f(particles[i].color.x(), particles[i].color.y(), particles[i].color.z());
                glVertex3f(particles[i].pos.x(), particles[i].pos.y(), particles[i].pos.z());
            }
        }
        glEnd();

        glEnable(GL_LIGHTING);
    }
};