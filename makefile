# --- Configuração de Caminhos (MSYS2) ---
# Se nao estiver definido, assume o padrao
MSYS_PATH = /c/msys64/mingw64

# --- Variáveis do Compilador ---
CXX = g++

# IMPORTANTE: Colocamos os -I aqui dentro. Assim o compilador NUNCA esquece onde estao os headers.
INC_FLAGS = -I$(MSYS_PATH)/include/bullet -I$(MSYS_PATH)/include -I$(MSYS_PATH)/include/SDL2
CXXFLAGS = -std=c++11 -Wall -O2 $(INC_FLAGS)

TARGET = estilingue.exe

# --- Arquivos do Projeto ---
# VERIFIQUE SE OS ARQUIVOS ESTÃO NESSAS PASTAS EXATAS
# Se 'stb_impl.cpp' estiver na pasta 'stb', mude para 'stb/stb_impl.cpp'
# Se 'Porco.cpp' não existir mais, apague-o desta lista.
SOURCES = estilingue.cpp estilingue/SlingshotManager.cpp stb/stb_impl.cpp controle_audio/sdl_audio_manager.cpp menu/gameMenu.cpp porcos/porco.cpp canhao/canhao.cpp passaros/blue.cpp

OBJECTS = $(SOURCES:.cpp=.o)

# --- Bibliotecas (Linkagem) ---
LIB_PATH = -L$(MSYS_PATH)/lib

# Ordem de Linkagem Otimizada:
# 1. Audio (SDL_mixer usa SDL2)
# 2. SDL2 (Core)
# 3. Física (BulletDynamics usa Collision, que usa LinearMath)
# 4. Gráficos (FreeGLUT, OpenGL)
# 5. Sistema (winmm para sons legacy se precisar)
LIBS = -lmingw32 -lSDL2main -lSDL2_mixer -lSDL2 \
       -lBulletDynamics -lBulletCollision -lLinearMath \
       -lfreeglut -lglu32 -lopengl32 -lwinmm

# --- Regras do Make ---

# Regra padrão (executada ao digitar apenas 'make')
all: $(TARGET)

# Linkagem
$(TARGET): $(OBJECTS)
	@echo "==> Linkando o executável: $(TARGET)"
	$(CXX) $(OBJECTS) -o $(TARGET) $(LIB_PATH) $(LIBS)
	@echo "==> Build concluido com sucesso!"

# Compilação
# Removemos $(HEADERS) da dependência para evitar que o Make ignore esta regra se um .h faltar
%.o: %.cpp
	@echo "==> Compilando $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@echo "==> Limpando arquivos compilados..."
	rm -f $(TARGET)
	rm -f *.o estilingue/*.o stb/*.o controle_audio/*.o menu/*.o porcos/*.o canhao/*.o

run: all
	@echo "==> Executando..."
	./$(TARGET)

.PHONY: all clean run