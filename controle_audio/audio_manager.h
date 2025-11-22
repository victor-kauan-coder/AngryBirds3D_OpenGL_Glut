#pragma once

#include <string>
#include <map>
#include <cstdio>
#include "../util/enums.h" // Inclui a definição CANÔNICA de MaterialTipo

// --- Configurações de Compilação/Inclusões SDL ---
// Nota: Em um projeto real, você usaria includes do SDL aqui,
// mas para manter a compilação cruzada mais limpa, usaremos apenas a declaração
// adianate, assumindo que a implementação no .cpp cuida dos includes do SDL.

// Forward Declaration para o tipo SDL_mixer
struct Mix_Chunk; 

// A extensão de áudio mais recomendada para portabilidade é OGG/WAV.
#define AUDIO_EXTENSION ".wav" 
#define MIX_MAX_VOLUME 128 // Volume máximo padrão do SDL_mixer

/**
 * @enum SomTipo
 * @brief Identificadores de som no jogo.
 */
enum class SomTipo {
    BLOCO_MADEIRA_DESTRUIDO,
    BLOCO_PEDRA_DESTRUIDO,
    BLOCO_GELO_DESTRUIDO,
    LANCAMENTO_PASSARO,
    ESTILINGUE_PUXANDO,
    ESTILINGUE_SOLTANDO,
    COLISAO_GELO,
    COLISAO_MADEIRA,
    COLISAO_PEDRA,
    COLISAO_PASSARO,
    TIPO_MAX
};

/**
 * @class AudioManager
 * @brief Gerencia o sistema de áudio SDL_mixer, permitindo controle de volume.
 * Esta classe depende da implementação em 'sdl_audio_manager.cpp'.
 */
class AudioManager {
private:
    // Mapa para armazenar os recursos de som do SDL_mixer
    std::map<SomTipo, Mix_Chunk*> soundChunks;

    // Função interna para carregar um som (definida no .cpp)
    bool loadSound(SomTipo type, const std::string& path);
    
public:
    // Construtor vazio (a inicialização ocorre em initAudio)
    AudioManager() {}

    // --- Gerenciamento do Ciclo de Vida ---
    /**
     * @brief Inicializa o SDL_mixer e carrega todos os sons.
     * Deve ser chamada uma única vez no início do jogo.
     */
    bool initAudio(); 
    
    /**
     * @brief Libera todos os recursos de áudio.
     * Deve ser chamada na saída do jogo.
     */
    void cleanup();   

    // --- Funções de Reprodução ---
    
    /**
     * @brief Toca um som com base no seu identificador e volume.
     * @param type O identificador do som.
     * @param volume Nível de volume (0 a 100).
     */
    void play(SomTipo type, int volume = 100);
    
    /**
     * @brief Toca o som de destruição correto para um material, com controle de volume.
     * @param material O tipo de material destruído (usa o enum de enums.h).
     * @param volume Nível de volume (0 a 100).
     */
    void playDestruction(MaterialTipo material, int volume = 60) {
        switch (material) {
            case MaterialTipo::MADEIRA:
                play(SomTipo::BLOCO_MADEIRA_DESTRUIDO, volume);
                break;
            case MaterialTipo::PEDRA:
                play(SomTipo::BLOCO_PEDRA_DESTRUIDO, volume);
                break;
            case MaterialTipo::GELO:
                play(SomTipo::BLOCO_GELO_DESTRUIDO, volume);
                break;
            default:
                break;
        }
    }
    void playSlingshot(bool puxando,int volume=60) {
        SomTipo som = puxando ? SomTipo::ESTILINGUE_PUXANDO : SomTipo::ESTILINGUE_SOLTANDO;
        play(som,volume);
    }

    void playPassaro(SomTipo tipoSom,int volume=50){
        play(tipoSom, volume);
    }
    void playColisao(MaterialTipo material, int volume = 60){
                switch (material) {
            case MaterialTipo::MADEIRA:
                play(SomTipo::COLISAO_MADEIRA, volume);
                break;
            case MaterialTipo::PEDRA:
                play(SomTipo::COLISAO_PEDRA, volume);
                break;
            case MaterialTipo::GELO:
                play(SomTipo::COLISAO_GELO, volume);
                break;
            default:
                break;
        }
    }
};

extern AudioManager g_audioManager;