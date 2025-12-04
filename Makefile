# --- Variáveis do Compilador e Executável ---
CXX = g++
CXXFLAGS = -std=c++11 -Wall -O2 
TARGET = estilingue.exe

# --- Arquivos do Projeto ---
SOURCES = estilingue.cpp estilingue/SlingshotManager.cpp stb/stb_impl.cpp controle_audio/sdl_audio_manager.cpp porcos/porco.cpp canhao/canhao.cpp
OBJECTS = $(SOURCES:.cpp=.o)

# --- Caminhos e Bibliotecas (Configuração do MinGW) ---
INC_PATHS = -I/c/msys64/mingw64/include/bullet -I/c/msys64/mingw64/include
LIB_PATH = -L/c/msys64/mingw64/lib

# Nomes das Bibliotecas (Linkagem)
# ADICIONADO: -lwinmm para a função PlaySoundA
LIBS = -lfreeglut -lglu32 -lopengl32 -lBulletDynamics -lBulletCollision -lLinearMath -lSDL2 -lSDL2_mixer

# --- Headers Locais para Dependências ---
HEADERS = blocos/BlocoDestrutivel.h controle_audio/audio_manager.h util/enums.h util/loads.h estilingue/SlingshotManager.h 

# --- Regras do Make ---

all: $(TARGET)

# Regra para "linkar" o executável final
$(TARGET): $(OBJECTS)
	@echo "==> Linkando o executável: $(TARGET)"
	$(CXX) $(OBJECTS) $(LIB_PATH) $(LIBS) -o $(TARGET)
	@echo "==> Build concluído!"

# Regra padrão para compilar um .cpp em um .o
%.o: %.cpp $(HEADERS)
	@echo "==> Compilando $<..."
	$(CXX) $(CXXFLAGS) $(INC_PATHS) -c $< -o $@

# Regra "clean" - para limpar os arquivos compilados
clean:
	@echo "==> Limpando arquivos compilados..."
	rm -f $(TARGET) $(OBJECTS)

# Regra "run" - para compilar E executar
run: all
	@echo "==> Executando $(TARGET)..."
	./$(TARGET)