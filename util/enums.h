#pragma once

// --- Tipos de Materiais de Blocos ---

/**
 * @enum MaterialTipo
 * @brief Define os tipos de materiais para blocos e para o sistema de áudio/jogo.
 */
enum class MaterialTipo {
    MADEIRA,
    PEDRA,
    GELO
};


enum class MusicaTipo {
    MENU,
    JOGO,
    VITORIA,
    DERROTA
};

/**
 * @enum SomTipo
 * @brief Identificadores de som no jogo.
 */
enum class SomTipo {
    BLOCO_MADEIRA_DESTRUIDO,
    BLOCO_PEDRA_DESTRUIDO,
    BLOCO_GELO_DESTRUIDO,
    LANCAMENTO_PASSARO_BOMB,
    LANCAMENTO_PASSARO_CHUCK,
    LANCAMENTO_PASSARO_BLUE,
    LANCAMENTO_PASSARO,
    ESTILINGUE_PUXANDO,
    ESTILINGUE_SOLTANDO,
    COLISAO_GELO,
    COLISAO_MADEIRA,
    COLISAO_PEDRA,
    COLISAO_PASSARO,
    TIPO_MAX,
    CHUCK_SPEED_BOOST,
    MORTE_PASSARO,
    MORTE_PORCO, 
    SOM_CANHAO,
    EXPLOSAO,
    COLISAO_CHUCK,
    COLISAO_BOMB,
    COLISAO_BLUE,
    COLISAO_PORCO,
    DANO_PORCO,
    PORCO,
    ENTRANDO_MENU,
    SAINDO_MENU,
    PORCO_PULANDO,
    CHUCK_SELECIONADO,
    RED_SELECIONADO,
    BLUE_SELECIONADO,
    BOMB_SELECIONADO
};