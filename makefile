# --- Variáveis de Configuração ---
CXX = g++
OUTPUT_DIR = output
TARGET = $(OUTPUT_DIR)/estilingue.exe
TARGET_PASSARO = $(OUTPUT_DIR)/passaro.exe
TARGET_PORCO = $(OUTPUT_DIR)/porco.exe
TARGET_BULLET = $(OUTPUT_DIR)/exemplo_bullet.exe
TARGET_RED = $(OUTPUT_DIR)/red.exe
SRC = estilingue.cpp
SRC_PASSARO = passaros/passaro.cpp
SRC_RED = passaros/red.cpp
SRC_PORCO = porco.cpp
SRC_BULLET = passaros/exemplo_bullet.cpp

# Flags de Include: -I. para arquivos .h locais, se houverem (ex: "stb_image.h")
BULLET_INCLUDES = -IC:/msys64/mingw64/include/bullet

# Flags de Include: -I. para arquivos .h locais, e o caminho do Bullet
CXXFLAGS = -g -I. $(BULLET_INCLUDES)


# Bibliotecas: Apenas listamos os nomes das bibliotecas.
# O MSYS2/MinGW já sabe onde encontrá-las.
# Adicione a flag -static se quiser que o executável não dependa das DLLs.
LIBS = -lBulletDynamics -lBulletCollision -lLinearMath -lfreeglut -lopengl32 -lglu32

# --- Regras do Make ---
.PHONY: all clean passaro porco bullet

all: $(TARGET)

passaro: $(TARGET_PASSARO)

porco: $(TARGET_PORCO)

bullet: $(TARGET_BULLET)

red: $(TARGET_RED)

$(TARGET): $(SRC)
    # Comando de compilação limpo: Compilador + Flags de Compilação + Fonte + Output + Bibliotecas
	$(CXX) $(CXXFLAGS) $< -o $@ $(LIBS)
	@echo "Construção concluída: $(TARGET)"

$(TARGET_PASSARO): $(SRC_PASSARO)
	$(CXX) $(CXXFLAGS) $< -o $@ $(LIBS)
	@echo "Construção concluída: $(TARGET_PASSARO)"

$(TARGET_PORCO): $(SRC_PORCO)
	$(CXX) $(CXXFLAGS) $< -o $@ $(LIBS)
	@echo "Construção concluída: $(TARGET_PORCO)"

$(TARGET_BULLET): $(SRC_BULLET)
	$(CXX) $(CXXFLAGS) $< -o $@ $(LIBS)
	@echo "Construção concluída: $(TARGET_BULLET)"
	@echo "Execute com: .\\output\\exemplo_bullet.exe"

$(TARGET_RED): $(SRC_RED)
	$(CXX) $(CXXFLAGS) $< -o $@ $(LIBS)
	@echo "Construção concluída: $(TARGET_RED)"

clean:
	rm -f $(TARGET) $(TARGET_PASSARO) $(TARGET_PORCO) $(TARGET_BULLET) $(TARGET_RED)
	@echo "Executáveis limpos."

.PHONY: clean-all
clean-all:
	rm -rf $(OUTPUT_DIR)
	@echo "Pasta $(OUTPUT_DIR) removida."