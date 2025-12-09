import random
import math

def generate_wood_texture(width, height, filename):
    # Cores base da madeira (RGB 0-255)
    dark_wood = (60, 30, 10)
    light_wood = (140, 90, 40)
    
    # Cabeçalho PPM (P3 = ASCII RGB)
    header = f"P3\n{width} {height}\n255\n"
    
    pixels = []
    
    center_x = width / 2
    center_y = height / 2
    
    for y in range(height):
        for x in range(width):
            # Distância do centro (para anéis)
            dx = x - center_x
            dy = y - center_y
            dist = math.sqrt(dx*dx + dy*dy)
            
            # Ruído simples para distorcer os anéis
            noise = random.random() * 10.0
            
            # Função seno para criar anéis
            # Ajuste a frequência (0.1) para anéis mais finos ou grossos
            sine_val = math.sin((dist + noise) * 0.1)
            
            # Normaliza de -1..1 para 0..1
            factor = (sine_val + 1.0) / 2.0
            
            # Interpola entre madeira escura e clara
            r = int(dark_wood[0] + (light_wood[0] - dark_wood[0]) * factor)
            g = int(dark_wood[1] + (light_wood[1] - dark_wood[1]) * factor)
            b = int(dark_wood[2] + (light_wood[2] - dark_wood[2]) * factor)
            
            # Adiciona um pouco de ruído granulado
            grain = random.randint(-10, 10)
            r = max(0, min(255, r + grain))
            g = max(0, min(255, g + grain))
            b = max(0, min(255, b + grain))
            
            pixels.append(f"{r} {g} {b}")
            
    with open(filename, "w") as f:
        f.write(header)
        # Escreve pixels em linhas para não ficar uma linha gigante
        for i in range(0, len(pixels), width):
            line = " ".join(pixels[i:i+width])
            f.write(line + "\n")

if __name__ == "__main__":
    generate_wood_texture(256, 256, "Objetos/wood_texture.ppm")
    print("Textura gerada em Objetos/wood_texture.ppm")
