# --- Variáveis de Configuração ---
CXX = g++
OUTPUT_DIR = output
TARGET = $(OUTPUT_DIR)/main.exe
SRC = main.cpp

# Flags de Include: -I. para arquivos .h locais, se houverem (ex: "stb_image.h")
BULLET_INCLUDES = -IC:/msys64/mingw64/include/bullet

# Flags de Include: -I. para arquivos .h locais, e o caminho do Bullet
CXXFLAGS = -g -I. $(BULLET_INCLUDES)


# Bibliotecas: Apenas listamos os nomes das bibliotecas.
# O MSYS2/MinGW já sabe onde encontrá-las.
# Adicione a flag -static se quiser que o executável não dependa das DLLs.
LIBS = -lBulletDynamics -lBulletCollision -lLinearMath -lfreeglut -lopengl32 -lglu32

# --- Regras do Make ---
.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
    # Comando de compilação limpo: Compilador + Flags de Compilação + Fonte + Output + Bibliotecas
	$(CXX) $(CXXFLAGS) $< -o $@ $(LIBS)
	@echo "Construção concluída: $(TARGET)"

clean:
	rm -f $(TARGET)
	@echo "Executável $(TARGET) limpo."

.PHONY: clean-all
clean-all:
	rm -rf $(OUTPUT_DIR)
	@echo "Pasta $(OUTPUT_DIR) removida."