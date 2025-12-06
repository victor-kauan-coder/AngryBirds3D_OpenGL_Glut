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

    Mix_AllocateChannels(32); 

    // 2. Reserva os primeiros 4 canais EXCLUSIVAMENTE para sons importantes (Pássaro, UI)
    // Os sons de colisão de blocos (que usam -1) não poderão usar os canais 0, 1, 2 e 3.
    Mix_ReserveChannels(4);
    // Define o volume máximo inicial para todos os canais de áudio.
    Mix_Volume(-1, MIX_MAX_VOLUME); 

    // Carregamento de todos os sons
    printf("Carregando recursos de audio...\n");

    musicaMenu = Mix_LoadMUS("song/musica_menu.mp3"); 
    if (!musicaMenu) printf("Erro carregar musica menu: %s\n", Mix_GetError());

    musicaJogo = Mix_LoadMUS("song/musica_jogo.mp3");
    if (!musicaJogo) printf("Erro carregar musica jogo: %s\n", Mix_GetError());

    // NOTA: Os caminhos agora procuram arquivos OGG (mais portáteis)
    // Se você só tiver WAV, mude AUDIO_EXTENSION para ".wav" e remova a flag MIX_INIT_OGG.

    if (loadSound(SomTipo::BLOCO_MADEIRA_DESTRUIDO, "song/madeira_quebrando" AUDIO_EXTENSION) &&
        loadSound(SomTipo::BLOCO_PEDRA_DESTRUIDO, "song/pedra_quebrando" AUDIO_EXTENSION) &&
        loadSound(SomTipo::BLOCO_GELO_DESTRUIDO, "song/gelo_quebrando" AUDIO_EXTENSION) &&
        loadSound(SomTipo::LANCAMENTO_PASSARO_BOMB, "song/lancamento_passaro_bomb" AUDIO_EXTENSION) &&
        loadSound(SomTipo::LANCAMENTO_PASSARO_CHUCK, "song/lancamento_passaro_chuck" AUDIO_EXTENSION) &&
        loadSound(SomTipo::LANCAMENTO_PASSARO_BLUE, "song/lancamento_passaro_blue" AUDIO_EXTENSION) &&
        loadSound(SomTipo::LANCAMENTO_PASSARO, "song/lancamento_passaro" AUDIO_EXTENSION) &&
        loadSound(SomTipo::ESTILINGUE_PUXANDO, "song/estilingue_puxando" AUDIO_EXTENSION)&&
        loadSound(SomTipo::ESTILINGUE_SOLTANDO, "song/estilingue_soltando" AUDIO_EXTENSION)&&
        loadSound(SomTipo::COLISAO_GELO, "song/colisao_gelo" AUDIO_EXTENSION)&&
        loadSound(SomTipo::COLISAO_MADEIRA, "song/colisao_madeira" AUDIO_EXTENSION)&&
        loadSound(SomTipo::COLISAO_PEDRA, "song/colisao_pedra" AUDIO_EXTENSION)&&
        loadSound(SomTipo::COLISAO_PASSARO, "song/colisao_red" AUDIO_EXTENSION)&&
        loadSound(SomTipo::MORTE_PASSARO, "song/passaro_morrendo" AUDIO_EXTENSION)&&
        loadSound(SomTipo::MORTE_PASSARO, "song/porco_morrendo" AUDIO_EXTENSION)&&
        loadSound(SomTipo::SOM_CANHAO, "song/som_canhao" AUDIO_EXTENSION)&&
        loadSound(SomTipo::EXPLOSAO, "song/explosao" AUDIO_EXTENSION)&&
        loadSound(SomTipo::COLISAO_BOMB, "song/colisao_bomb" AUDIO_EXTENSION)&&
        loadSound(SomTipo::COLISAO_CHUCK, "song/colisao_chuck" AUDIO_EXTENSION)&&
        loadSound(SomTipo::COLISAO_BLUE, "song/colisao_blue" AUDIO_EXTENSION)) 
    {
        printf("Audio carregado com sucesso.\n");
        return true;
    }

    printf("ERRO: Falha ao carregar pelo menos um arquivo de audio.\n");
    return false;
}

void AudioManager::playMusic(MusicaTipo tipo) {
    Mix_Music* targetMusic = nullptr;

    if (tipo == MusicaTipo::MENU) targetMusic = musicaMenu;
    else if (tipo == MusicaTipo::JOGO) targetMusic = musicaJogo;

    if (targetMusic) {
        // Toca a música em loop (-1)
        // O SDL_mixer troca automaticamente (para a anterior e inicia a nova)
        if (Mix_PlayMusic(targetMusic, -1) == -1) {
            printf("Erro ao tocar musica: %s\n", Mix_GetError());
        } else {
            musicaAtual = tipo;
            musicaEstaTocando = true;
        }
    }
}

void AudioManager::stopMusic() {
    Mix_HaltMusic();
    musicaEstaTocando = false;
}


void AudioManager::setVolume(float volumePercent) {
    if (volumePercent < 0) volumePercent = 0;
    if (volumePercent > 100) volumePercent = 100;
    int vol = (int)((volumePercent / 100.0f) * MIX_MAX_VOLUME);

    Mix_Volume(-1, vol);      // Volume dos efeitos
    Mix_VolumeMusic(vol / 1.5); // Volume da música (geralmente queremos música mais baixa que efeitos)
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

        // Converte volume (0-100) para SDL (0-128)
        int sdlVolume = (volume * MIX_MAX_VOLUME) / 100;
        Mix_VolumeChunk(chunk, sdlVolume);
        
        // --- LÓGICA DE PRIORIDADE ---
        int canal = -1; // -1 significa "qualquer canal livre"

        // Verifica se é um som de Pássaro ou Estilingue (Sons VIP)
        // Você pode adicionar mais tipos aqui se quiser
        bool ehSomImportante = (type == SomTipo::LANCAMENTO_PASSARO || 
                                type == SomTipo::LANCAMENTO_PASSARO_CHUCK ||
                                type == SomTipo::LANCAMENTO_PASSARO_BOMB ||
                                type == SomTipo::LANCAMENTO_PASSARO_BLUE ||
                                type == SomTipo::ESTILINGUE_SOLTANDO ||
                                type == SomTipo::COLISAO_PASSARO);

        if (ehSomImportante) {
            // Tenta tocar no canal 0 (Reservado VIP)
            // Se o canal 0 estiver ocupado, o SDL_mixer tentará o próximo reservado automaticamente? 
            // Não, precisamos especificar. Vamos forçar o canal 0 ou 1.
            
            // Mix_PlayChannel(canal, ...)
            // Se usarmos um número >= 0, forçamos aquele canal.
            // Vamos tentar usar um dos canais reservados (0 a 3).
            
            // Simples: Pássaros sempre tocam no canal 0 ou 1, atropelando o que estiver lá se necessário.
            // Isso garante que o grito saia.
            canal = 0; 
        }
        
        // Toca o som
        // Se canal for -1 (blocos), ele procura um livre entre 4 e 31.
        // Se canal for 0 (pássaro), ele usa o 0 (que está protegido dos blocos).
        Mix_PlayChannel(canal, chunk, 0);

    } else {
        // printf("AVISO DE AUDIO: Som nao encontrado.\n");
    }
}

void AudioManager::cleanup() {
    // Libera músicas
    if (musicaMenu) Mix_FreeMusic(musicaMenu);
    if (musicaJogo) Mix_FreeMusic(musicaJogo);

    // Libera efeitos (seu código antigo)
    for (auto const& [type, chunk] : soundChunks) {
        if (chunk) Mix_FreeChunk(chunk);
    }
    soundChunks.clear();
    Mix_CloseAudio();
    Mix_Quit(); // Importante para liberar MP3/OGG
    SDL_Quit();
}