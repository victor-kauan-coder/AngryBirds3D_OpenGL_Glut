#pragma once

#include <string>
#include <map>
#include <cstdio>
#include "../util/enums.h"

struct Mix_Chunk; 
struct Mix_Music;

//extensao dos efeitos sonoros
#define AUDIO_EXTENSION ".wav" 
#define MIX_MAX_VOLUME 128 //volume máximo padrão do SDL_mixer

/**
 * @class AudioManager
 * @brief Gerencia o sistema de áudio SDL_mixer, permitindo controle de volume.
 * Esta classe depende da implementação em 'sdl_audio_manager.cpp'.
 */
class AudioManager {
private:
// Mapa para Efeitos Sonoros (Tiros, Colisões)
    std::map<SomTipo, Mix_Chunk*> soundChunks;

    float masterVolumePercent = 100.0f;
    
    //musicas presentes no jogo
    Mix_Music* musicaMenu = nullptr;
    Mix_Music* musicaJogo = nullptr;
    Mix_Music* somDerrota = nullptr;
    Mix_Music* somVitoria = nullptr;
    MusicaTipo musicaAtual;
    bool musicaEstaTocando = false;

    bool loadSound(SomTipo type, const std::string& path);

    
public:

    AudioManager() {}

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
    void playDestruction(MaterialTipo material, int volume = 30) {
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

    void playSlingshot(bool puxando,int volume=100) {
        SomTipo som = puxando ? SomTipo::ESTILINGUE_PUXANDO : SomTipo::ESTILINGUE_SOLTANDO;
        play(som,volume);
    }

    void playPassaro(SomTipo tipoSom,int volume=50){
        play(tipoSom, volume);
    }
    void playColisao(MaterialTipo material, int volume = 30){
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

    void setVolume(float volumePercent);

    void playMusic(MusicaTipo tipo, int volume=10); //toca a música (loop infinito)

    void stopMusic();

};

extern AudioManager g_audioManager;