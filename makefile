# --- Variáveis do Compilador ---
CXX = g++
# C++17 recomendado, ativa warnings, otimização O2
# -D_WIN32_WINNT=0x0600 é as vezes necessário para versões recentes do MinGW
CXXFLAGS = -std=c++17 -Wall -O2 -D_WIN32_WINNT=0x0600

# Nome do Executável
TARGET = estilingue.exe

# --- Arquivos do Projeto ---
# Adicionei os arquivos de audio e a implementação do stb
SOURCES = estilingue.cpp \
          estilingue/SlingshotManager.cpp \
          controle_audio/sdl_audio_manager.cpp \
          stb/stb_impl.cpp

# Converte a lista de .cpp para .o (arquivos objeto)
OBJECTS = $(SOURCES:.cpp=.o)

# --- Caminhos (Configuração do MSYS2/MinGW) ---
# IMPORTANTE: Verifique se o caminho /c/msys64/... corresponde à sua instalação
MSYS_PATH = /c/msys64/mingw64

# Caminhos de Inclusão (.h)
# Adicionado include do SDL2
INC_PATHS = -I$(MSYS_PATH)/include \
            -I$(MSYS_PATH)/include/bullet \
            -I$(MSYS_PATH)/include/SDL2

# Caminho das Bibliotecas (.a / .dll)
LIB_PATH = -L$(MSYS_PATH)/lib

# --- Bibliotecas (Linkagem) ---
# 1. Gráficos (FreeGLUT, OpenGL)
# 2. Física (Bullet)
# 3. Áudio (SDL2, SDL2_mixer)
LIBS = -lfreeglut -lglu32 -lopengl32 \
       -lBulletDynamics -lBulletCollision -lLinearMath \
       -lmingw32 -lSDL2main -lSDL2 -lSDL2_mixer

# --- Regras do Make ---

# Regra padrão (executada ao digitar apenas 'make')
all: $(TARGET)

# Regra para linkar o executável
$(TARGET): $(OBJECTS)
	@echo "==> Linkando o executável: $(TARGET)"
	$(CXX) $(OBJECTS) -o $(TARGET) $(LIB_PATH) $(LIBS)
	@echo "==> Build concluido com sucesso!"

# Regra genérica para compilar .cpp em .o
# O $< pega o nome do .cpp e $@ pega o nome do .o
%.o: %.cpp
	@echo "==> Compilando $<..."
	$(CXX) $(CXXFLAGS) $(INC_PATHS) -c $< -o $@

# Regra para limpar arquivos temporários
clean:
	@echo "==> Limpando arquivos compilados..."
	rm -f $(TARGET) $(OBJECTS)

# Regra para compilar e rodar
run: all
	@echo "==> Executando $(TARGET)..."
	./$(TARGET)

.PHONY: all clean run