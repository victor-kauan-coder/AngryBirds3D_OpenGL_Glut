#include "audio_manager.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

// Definição da variável global declarada em audio_manager.h
AudioManager g_audioManager;

// --- Implementação da Classe AudioManager ---

// Função de inicialização chamada uma vez
bool AudioManager::initAudio() {
    // Inicializa SDL para áudio
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        printf("ERRO SDL: Falha ao inicializar SDL: %s\n", SDL_GetError());
        return false;
    }

    // Inicializa SDL_mixer
    // (Frequência: 44100Hz, Formato: padrão, Canais: 2 (estéreo), Buffer: 2048)
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        printf("ERRO SDL_mixer: Falha ao abrir o mixer: %s\n", Mix_GetError());
        return false;
    }

    // Define o volume máximo inicial para todos os canais de áudio.
    Mix_Volume(-1, MIX_MAX_VOLUME); 

    // Carregamento de todos os sons
    printf("Carregando recursos de audio...\n");

    // NOTA: Os caminhos agora procuram arquivos OGG (mais portáteis)
    // Se você só tiver WAV, mude AUDIO_EXTENSION para ".wav" e remova a flag MIX_INIT_OGG.

    if (loadSound(SomTipo::BLOCO_MADEIRA_DESTRUIDO, "song/madeira_quebrando" AUDIO_EXTENSION) &&
        loadSound(SomTipo::BLOCO_PEDRA_DESTRUIDO, "song/pedra_quebrando" AUDIO_EXTENSION) &&
        loadSound(SomTipo::BLOCO_GELO_DESTRUIDO, "song/gelo_quebrando" AUDIO_EXTENSION) &&
        loadSound(SomTipo::LANCAMENTO_PASSARO, "song/lancamento_passaro" AUDIO_EXTENSION)) 
    {
        printf("Audio carregado com sucesso.\n");
        return true;
    }

    printf("ERRO: Falha ao carregar pelo menos um arquivo de audio.\n");
    return false;
}

// Função auxiliar para carregar um som e mapeá-lo
bool AudioManager::loadSound(SomTipo type, const std::string& path) {
    Mix_Chunk* chunk = Mix_LoadWAV(path.c_str());
    if (chunk == nullptr) {
        printf("AVISO DE AUDIO: Nao foi possivel carregar o som: %s. Erro: %s\n", 
               path.c_str(), Mix_GetError());
        return false;
    }
    soundChunks[type] = chunk;
    return true;
}


void AudioManager::play(SomTipo type, int volume) {
    auto it = soundChunks.find(type);
    if (it != soundChunks.end()) {
        Mix_Chunk* chunk = it->second;

        // 1. Converte volume (0-100) para o padrão SDL_mixer (0-128)
        int sdlVolume = (volume * MIX_MAX_VOLUME) / 100;
        
        // 2. Define o volume apenas para esta chamada (chunk)
        Mix_VolumeChunk(chunk, sdlVolume);
        
        // 3. Toca o som (canal -1 = próximo canal livre, loops = 0)
        Mix_PlayChannel(-1, chunk, 0);
    } else {
        printf("AVISO DE AUDIO: Som nao carregado ou mapeado para este tipo.\n");
    }
}

void AudioManager::cleanup() {
    for (auto const& [type, chunk] : soundChunks) {
        if (chunk) {
            Mix_FreeChunk(chunk);
        }
    }
    soundChunks.clear();
    Mix_CloseAudio();
    SDL_Quit();
    printf("Sistema de audio limpo.\n");
}