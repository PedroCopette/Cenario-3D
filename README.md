# Ilha Tropical

Projeto em C++ que apresenta uma cena 3D interativa de uma ilha tropical. A aplicação usa OpenGL para renderizar a ilha, a cabana, os coqueiros, a cadeira de praia e o barco, com água animada, espuma, partículas, iluminação e câmera em primeira pessoa.

## Requisitos

- Windows com MinGW-w64 e `g++` disponíveis no terminal.
- OpenGL e GLU.
- Bibliotecas e arquivos de desenvolvimento do GLEW, GLFW e Assimp, configurados para o compilador.

## Compilar e executar

No VS Code, abra a pasta do projeto e use **Terminal → Executar Tarefa de Compilação** (ou `Ctrl+Shift+B`). A tarefa `Build` compila o arquivo ativo; selecione `main.cpp` para gerar `main.exe`.

Também é possível compilar no PowerShell, a partir da pasta do projeto:

```powershell
g++ main.cpp -o main.exe -lopengl32 -lglew32 -lglfw3 -lglu32 -lassimp -lz -lgdi32
```

> As bibliotecas do GLEW, GLFW e Assimp precisam estar instaladas e nos caminhos de include e linkagem do MinGW.

Execute a partir da pasta do projeto para que os caminhos relativos dos modelos e da textura sejam encontrados:

```powershell
.\main.exe
```

## Controles

| Tecla ou ação | Função |
| --- | --- |
| Mouse | Olhar ao redor |
| `W`, `A`, `S`, `D` | Mover a câmera |
| `T` + setas | Transladar a cena nos eixos X e Y |
| `T` + `Page Up` / `Page Down` | Transladar a cena no eixo Z |
| `R` + setas | Rotacionar a cena nos eixos X e Y |
| `R` + `Page Up` / `Page Down` | Rotacionar a cena no eixo Z |
| `E` + seta direita / esquerda | Aumentar ou diminuir a escala da cena |
| `Espaço` | Restaurar a câmera e as transformações da cena |
| `Esc` | Fechar a aplicação |

## Estrutura

```text
.
├── main.cpp             # Aplicação, renderização, animações e controles
├── glb_model.h          # Carregamento e desenho de modelos via Assimp
├── font.h               # Fonte vetorial
├── stb_image.h          # Leitura de imagens
├── models/              # Modelos 3D em formato GLB
├── textures/            # Textura da água e imagens dos recursos
└── .vscode/             # Tarefas de compilação e configuração de depuração
```

Os modelos e a textura da água são carregados por caminhos relativos (`models/...` e `textures/agua.jpg`), por isso execute o programa com a pasta do projeto como diretório atual.
