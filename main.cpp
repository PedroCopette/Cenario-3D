#define STB_IMAGE_IMPLEMENTATION

#include <GL/glew.h>
#include <GL/glu.h>
#include <GLFW/glfw3.h>

#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <sstream>

#include "font.h"
#include "glb_model.h"

// ==========================================================
// CAMERA
// ==========================================================

const GLfloat kCameraSpeed = 0.035f;
const GLfloat kMouseSensitivity = 0.1f;

// Altura dos olhos acima do topo da ilha (modelo normalizado para 24 unidades).
const GLfloat kDefaultCameraX = -2.2f;
const GLfloat kDefaultCameraY = 7.5f;
const GLfloat kDefaultCameraZ = -8.0f;

GLfloat camera_eye_x = kDefaultCameraX;
GLfloat camera_eye_y = kDefaultCameraY;
GLfloat camera_eye_z = kDefaultCameraZ;

bool camera_caminhando = false;
GLfloat camera_bob_intensidade = 0.0f;

GLfloat camera_direction_x = 0.0f;
GLfloat camera_direction_y = -0.15f;
GLfloat camera_direction_z = 1.0f;

const GLfloat kCameraUpX = 0.0f;
const GLfloat kCameraUpY = 1.0f;
const GLfloat kCameraUpZ = 0.0f;

const GLfloat kDefaultCameraYaw = 35.0f;
const GLfloat kDefaultCameraPitch = -5.5f;
const GLfloat kCameraPitchLimit = 89.0f;

GLfloat camera_yaw = kDefaultCameraYaw;
GLfloat camera_pitch = kDefaultCameraPitch;

// ==========================================================
// PROJECAO
// ==========================================================

const GLfloat kPerspectiveFieldOfViewAngle = 45.0f;
const GLfloat kPerspectiveNearZ = 0.1f;
const GLfloat kPerspectiveFarZ = 300.0f;

// ==========================================================
// TRANSFORMACOES GLOBAIS
// ==========================================================

bool translating = false;
bool rotating = false;
bool scaling = false;

const GLfloat kTranslateIncrement = 0.05f;
const GLfloat kTranslateLimit = 10.0f;
const GLfloat kRotateIncrement = 1.0f;

const GLfloat kScaleMin = 0.1f;
const GLfloat kScaleMax = 5.0f;
const GLfloat kScaleIncrement = 0.05f;

GLfloat translate_x = 0.0f;
GLfloat translate_y = 0.0f;
GLfloat translate_z = 0.0f;

GLfloat rotate_x = 0.0f;
GLfloat rotate_y = 0.0f;
GLfloat rotate_z = 0.0f;

GLfloat scale_x = 1.0f;
GLfloat scale_y = 1.0f;
GLfloat scale_z = 1.0f;

// ==========================================================
// MODELOS
// ==========================================================

GlbModel ilha;
GlbModel cabana;
GlbModel coqueiro;
GlbModel cadeira_praia;
GlbModel barco;

// ==========================================================
// CONFIGURACAO DO BARCO
// ==========================================================

GLfloat barco_x = 28.0f;
GLfloat barco_z = 0.0f;
const GLfloat barco_altura = 5.0f;
GLfloat barco_rotacao_y = 90.0f;
const GLfloat barco_tamanho = 11.0f;

GLfloat barco_balanço_x = 0.0f;
GLfloat barco_balanço_z = 0.0f;
GLfloat barco_flutuacao = 0.0f;

// ==========================================================
// HUD
// ==========================================================

std::ostringstream hud_text;

// ==========================================================
// TEXTURA E ANIMACAO DA AGUA
// ==========================================================

GLuint agua_texture = 0;

GLfloat agua_movimento_x = 0.0f;
GLfloat agua_movimento_z = 0.0f;

const GLfloat agua_velocidade_x = 0.015f;
const GLfloat agua_velocidade_z = 0.008f;

// ==========================================================
// PARTICULAS DE AGUA
// ==========================================================

struct ParticulaAgua
{
    GLfloat x, y, z;

    GLfloat velocidade_x;
    GLfloat velocidade_y;
    GLfloat velocidade_z;

    GLfloat vida;
    GLfloat tamanho;
};

const int MAX_PARTICULAS = 150;

ParticulaAgua particulas[MAX_PARTICULAS];

GLfloat tempo_ultima_atualizacao = 0.0f;
GLfloat tempo_emissao_particulas = 0.0f;

// ==========================================================
// CARREGAR TEXTURA DA AGUA
// ==========================================================

bool load_water_texture()
{
    const char* caminhos[] = {
        "textures/agua.jpg",
        "../textures/agua.jpg",
        "../../textures/agua.jpg"
    };

    int largura = 0;
    int altura = 0;
    int canais = 0;

    unsigned char* imagem = nullptr;

    for (const char* caminho : caminhos) {
        imagem = stbi_load(
            caminho,
            &largura,
            &altura,
            &canais,
            0
        );

        if (imagem != nullptr) {
            std::cout
                << "Textura encontrada em: "
                << caminho << std::endl;
            break;
        }
    }

    if (imagem == nullptr) {
        std::cerr
            << "ERRO: nao foi possivel carregar textures/agua.jpg."
            << std::endl;

        std::cerr
            << "Detalhe: "
            << stbi_failure_reason()
            << std::endl;

        return false;
    }

    GLenum formato;

    if (canais == 4) {
        formato = GL_RGBA;
    }
    else if (canais == 3) {
        formato = GL_RGB;
    }
    else {
        std::cerr
            << "A imagem precisa ter 3 ou 4 canais."
            << std::endl;

        stbi_image_free(imagem);
        return false;
    }

    glGenTextures(1, &agua_texture);
    glBindTexture(GL_TEXTURE_2D, agua_texture);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        formato,
        largura,
        altura,
        0,
        formato,
        GL_UNSIGNED_BYTE,
        imagem
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MIN_FILTER,
        GL_LINEAR_MIPMAP_LINEAR
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MAG_FILTER,
        GL_LINEAR
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_S,
        GL_REPEAT
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_T,
        GL_REPEAT
    );

    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(imagem);

    glBindTexture(GL_TEXTURE_2D, 0);

    std::cout
        << "Textura da agua carregada: "
        << largura << "x" << altura
        << std::endl;

    return true;
}

// ==========================================================
// ATUALIZAR MOVIMENTO DA AGUA
// ==========================================================

void update_water_animation()
{
    static double ultimo_tempo = 0.0;

    double tempo_atual = glfwGetTime();
    double delta_tempo = tempo_atual - ultimo_tempo;

    ultimo_tempo = tempo_atual;

    if (delta_tempo > 0.05)
        delta_tempo = 0.05;

    if (delta_tempo < 0.0)
        delta_tempo = 0.0;

    agua_movimento_x +=
        agua_velocidade_x * static_cast<GLfloat>(delta_tempo);

    agua_movimento_z +=
        agua_velocidade_z * static_cast<GLfloat>(delta_tempo);

    if (agua_movimento_x >= 1.0f)
        agua_movimento_x -= 1.0f;

    if (agua_movimento_z >= 1.0f)
        agua_movimento_z -= 1.0f;
}

// ==========================================================
// ILUMINACAO NOTURNA
// ==========================================================

void configure_boat_light()
{
    const GLfloat luz_ambiente[] = {
        0.65f, 0.48f, 0.25f, 1.0f
    };

    const GLfloat luz_difusa[] = {
        1.0f, 0.85f, 0.55f, 1.0f
    };

    const GLfloat luz_especular[] = {
        1.0f, 0.95f, 0.75f, 1.0f
    };

    glLightfv(GL_LIGHT0, GL_AMBIENT, luz_ambiente);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, luz_difusa);
    glLightfv(GL_LIGHT0, GL_SPECULAR, luz_especular);

    glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 0.5f);
    glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.001f);
    glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.00002f);

    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);

    const GLfloat ambiente_global[] = {
        0.28f, 0.25f, 0.20f, 1.0f
    };

    glLightModelfv(
        GL_LIGHT_MODEL_AMBIENT,
        ambiente_global
    );
}

// ==========================================================
// DESENHAR AGUA COM TEXTURA EM MOVIMENTO
// ==========================================================

void draw_water()
{
    glPushAttrib(
        GL_ENABLE_BIT |
        GL_CURRENT_BIT |
        GL_DEPTH_BUFFER_BIT |
        GL_POLYGON_BIT |
        GL_TEXTURE_BIT |
        GL_LIGHTING_BIT
    );


    glDisable(GL_CULL_FACE);

    const GLfloat altura_agua = 5.0f;
    const GLfloat tamanho_agua = 100.0f;
    const int segmentos = 100;
    const GLfloat passo = (2.0f * tamanho_agua) / segmentos;
    const GLfloat repeticoes = 18.0f;
    const GLfloat tempo = static_cast<GLfloat>(glfwGetTime());

    if (agua_texture != 0) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, agua_texture);

        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    }
    else {
        glDisable(GL_TEXTURE_2D);
        glColor3f(0.0f, 0.52f, 0.66f);
    }

    // A luz do barco cria reflexos especulares sobre as cristas.
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    const GLfloat agua_difusa[] = { 0.72f, 0.86f, 0.92f, 1.0f };
    const GLfloat agua_especular[] = { 0.92f, 0.97f, 1.0f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, agua_difusa);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, agua_especular);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 96.0f);

    // Ondas sobrepostas com normais calculadas a partir da inclinacao local.
    auto emitir_vertice_agua = [&](GLfloat x, GLfloat z) {
        const GLfloat fase1 = x * 0.10f + z * 0.06f + tempo * 1.15f;
        const GLfloat fase2 = -x * 0.07f + z * 0.12f + tempo * 0.82f;

        const GLfloat altura = altura_agua
            + 0.14f * std::sin(fase1)
            + 0.07f * std::sin(fase2);

        const GLfloat derivada_x =
            0.14f * 0.10f * std::cos(fase1)
            - 0.07f * 0.07f * std::cos(fase2);

        const GLfloat derivada_z =
            0.14f * 0.06f * std::cos(fase1)
            + 0.07f * 0.12f * std::cos(fase2);

        const GLfloat comprimento = std::sqrt(
            derivada_x * derivada_x + 1.0f + derivada_z * derivada_z
        );

        glNormal3f(
            -derivada_x / comprimento,
            1.0f / comprimento,
            -derivada_z / comprimento
        );

        glTexCoord2f(
            ((x + tamanho_agua) / (2.0f * tamanho_agua)) * repeticoes
                + agua_movimento_x,
            ((z + tamanho_agua) / (2.0f * tamanho_agua)) * repeticoes
                + agua_movimento_z
        );
        glVertex3f(x, altura, z);
    };

    for (int linha = 0; linha < segmentos; ++linha) {
                const GLfloat z0 = -tamanho_agua + linha * passo + 12.0f;
        const GLfloat z1 = z0 + passo;
        glBegin(GL_TRIANGLE_STRIP);

        for (int coluna = 0; coluna <= segmentos; ++coluna) {
            const GLfloat x = -tamanho_agua + coluna * passo;
            emitir_vertice_agua(x, z0);
            emitir_vertice_agua(x, z1);

        }

        glEnd();
    }

    glBindTexture(GL_TEXTURE_2D, 0);

    glPopAttrib();
}

// Faixa translúcida de espuma irregular ao redor da linha da costa.
void draw_beach_foam()
{
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1.0f, -1.0f);
    glDepthMask(GL_FALSE);

    const GLfloat tempo = static_cast<GLfloat>(glfwGetTime());
    const GLfloat pi = 3.14159265f;
    const int segmentos = 240;

    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= segmentos; ++i) {
        const GLfloat angulo = 2.0f * pi * i / segmentos;
        const GLfloat forma = 12.10f
            + 0.30f * std::sin(angulo * 3.0f + 0.4f)
            + 0.18f * std::sin(angulo * 7.0f - tempo * 0.22f);

        const GLfloat padrao = 0.5f
            + 0.5f * std::sin(angulo * 17.0f + tempo * 1.4f);
        const GLfloat espuma = 0.12f + 0.78f * padrao * padrao * padrao;

        for (int borda = 0; borda < 2; ++borda) {
            const GLfloat raio = forma + (borda == 0 ? -0.38f : 0.38f);
            const GLfloat x = raio * std::cos(angulo);
            const GLfloat z = raio * std::sin(angulo);
            const GLfloat fase1 = x * 0.10f + z * 0.06f + tempo * 1.15f;
            const GLfloat fase2 = -x * 0.07f + z * 0.12f + tempo * 0.82f;
            const GLfloat y = 5.0f
                + 0.14f * std::sin(fase1)
                + 0.07f * std::sin(fase2)
                + 0.02f;

            const GLfloat alpha = borda == 0 ? espuma * 0.82f : 0.0f;
            glColor4f(0.82f, 0.97f, 1.0f, alpha);
            glVertex3f(x, y, z);
        }
    }
    glEnd();

    glPopAttrib();
}

// ==========================================================
// MOVIMENTO AUTOMATICO DO BARCO
// ==========================================================

void update_boat_animation()
{
    GLfloat tempo = static_cast<GLfloat>(glfwGetTime());

    // Balanco lateral mais rapido
    barco_balanço_z = 2.0f * std::sin(tempo * 2.5f);

    // Balanco frontal e traseiro mais rapido
    barco_balanço_x = 1.2f * std::sin(tempo * 2.0f);

    // Movimento vertical mais rapido
    barco_flutuacao = 0.15f * std::sin(tempo * 3.0f);

    // Mantem o barco parado na mesma posicao
    barco_x = 28.0f;
    barco_z = 0.0f;
        barco_rotacao_y = 90.0f;
}

// ==========================================================
// CRIAR PARTICULA DE AGUA
// ==========================================================

void criar_particula_agua() 
{
    for (int i = 0; i < MAX_PARTICULAS; i++)
    {
        if (particulas[i].vida <= 0.0f)
        {
            // Emissao das particulas ao redor do barco.
            particulas[i].x = barco_x +
                ((rand() % 100 - 50) / 10.0f);

            // Altura ligeiramente acima da superficie da agua.
            particulas[i].y = 5.15f;

            particulas[i].z = barco_z +
                ((rand() % 100 - 50) / 10.0f);

            // Movimento horizontal suave.
            particulas[i].velocidade_x =
                (rand() % 100 - 50) / 50.0f;

            // Pequenos respingos verticais.
            particulas[i].velocidade_y =
                0.2f + (rand() % 100) / 200.0f;

            particulas[i].velocidade_z =
                (rand() % 100 - 50) / 50.0f;

            // Tempo de vida da particula.
            particulas[i].vida = 1.2f;

            // Tamanho da particula.
            particulas[i].tamanho = 3.0f + (rand() % 4);

            break;
        }
    }
}

// ==========================================================
// ATUALIZAR PARTICULAS DE AGUA
// ==========================================================

void update_water_particles()
{
    GLfloat tempo_atual =
        static_cast<GLfloat>(glfwGetTime());

    GLfloat delta_tempo =
        tempo_atual - tempo_ultima_atualizacao;

    tempo_ultima_atualizacao = tempo_atual;

    if (delta_tempo > 0.05f)
        delta_tempo = 0.05f;

    if (delta_tempo < 0.0f)
        delta_tempo = 0.0f;

    tempo_emissao_particulas += delta_tempo;

    // Emite novas gotas em intervalos curtos.
    if (tempo_emissao_particulas >= 0.05f)
    {
        criar_particula_agua();

        tempo_emissao_particulas = 0.0f;
    }

    for (int i = 0; i < MAX_PARTICULAS; i++)
    {
        if (particulas[i].vida > 0.0f)
        {
            particulas[i].x +=
                particulas[i].velocidade_x * delta_tempo;

            particulas[i].y +=
                particulas[i].velocidade_y * delta_tempo;

            particulas[i].z +=
                particulas[i].velocidade_z * delta_tempo;

            // Gravidade faz as gotas voltarem para a agua.
            particulas[i].velocidade_y -=
                3.5f * delta_tempo;

            // Diminuir a vida da particula.
            particulas[i].vida -= delta_tempo;
        }
    }
}

// ==========================================================
// DESENHAR PARTICULAS DE AGUA
// ==========================================================


void draw_water_particles()
{
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_POINT_SMOOTH);
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);

    for (int i = 0; i < MAX_PARTICULAS; i++)
    {
        if (particulas[i].vida > 0)
        {
            GLfloat alpha = particulas[i].vida / 1.2f;

            if (alpha > 1.0f)
                alpha = 1.0f;

            // Cor azul-ciano da agua
            glColor4f(0.55f, 0.90f, 1.0f, alpha * 0.65f);

            // Define tamanho antes de desenhar
            glPointSize(particulas[i].tamanho * 2.0f);

            glBegin(GL_POINTS);

            glVertex3f(
                particulas[i].x,
                particulas[i].y,
                particulas[i].z
            );

            glEnd();
        }
    }

    glDisable(GL_POINT_SMOOTH);
    glDisable(GL_BLEND);

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
}


// ==========================================================
// ATUALIZAR VETORES DA CAMERA
// ==========================================================

void update_camera_vectors()
{
    GLfloat yaw_rad = camera_yaw * 3.14159265f / 180.0f;
    GLfloat pitch_rad = camera_pitch * 3.14159265f / 180.0f;

    camera_direction_x = std::cos(yaw_rad) * std::cos(pitch_rad);
    camera_direction_y = std::sin(pitch_rad);
    camera_direction_z = std::sin(yaw_rad) * std::cos(pitch_rad);
}

// Colisao circular no plano XZ para a camera nao atravessar os objetos.
struct Obstaculo
{
    GLfloat x;
    GLfloat z;
    GLfloat raio;
};

const Obstaculo obstaculos[] = {
    {  0.0f,  0.0f, 2.0f }, // cabana
    { -3.0f,  0.2f, 1.6f }, // guarda-sol e cadeira de praia na ilha
    { -5.2f,  2.0f, 0.8f }, // coqueiros
    {  5.2f,  2.0f, 0.8f },
    { -4.8f, -2.0f, 0.8f },
    {  5.0f, -2.0f, 0.8f },
    { 28.0f,  0.0f, 4.0f }  // barco
};

const GLfloat raio_camera = 0.45f;
// Limite circular aproximado da area caminhavel da areia.
const GLfloat raio_areia = 9.2f;

bool camera_pode_ocupar(GLfloat x, GLfloat z)
{
    // Mantem o jogador dentro do contorno da ilha.
    if (x * x + z * z > raio_areia * raio_areia)
        return false;

    for (const Obstaculo& obstaculo : obstaculos) {
        const GLfloat dx = x - obstaculo.x;
        const GLfloat dz = z - obstaculo.z;
        const GLfloat distancia_minima = raio_camera + obstaculo.raio;

        if (dx * dx + dz * dz < distancia_minima * distancia_minima)
            return false;
    }

    return true;
}

void mover_camera_com_colisao(GLfloat dx, GLfloat dz)
{
    const GLfloat x_anterior = camera_eye_x;
    const GLfloat z_anterior = camera_eye_z;

    // Testar cada eixo separadamente permite deslizar ao longo dos objetos.
    if (camera_pode_ocupar(camera_eye_x + dx, camera_eye_z))
        camera_eye_x += dx;

    if (camera_pode_ocupar(camera_eye_x, camera_eye_z + dz))
        camera_eye_z += dz;

    if (camera_eye_x != x_anterior || camera_eye_z != z_anterior)
        camera_caminhando = true;
}

void update_camera_bob()
{
    static double ultimo_tempo = glfwGetTime();
    const double tempo_atual = glfwGetTime();
    double delta_tempo = tempo_atual - ultimo_tempo;
    ultimo_tempo = tempo_atual;

    if (delta_tempo > 0.05)
        delta_tempo = 0.05;
    if (delta_tempo < 0.0)
        delta_tempo = 0.0;

    const GLfloat alvo = camera_caminhando ? 1.0f : 0.0f;
    const GLfloat suavizacao = static_cast<GLfloat>(delta_tempo * 8.0);
    camera_bob_intensidade +=
        (alvo - camera_bob_intensidade) * suavizacao;
}

// ==========================================================
// CONTROLE DO MOUSE
// ==========================================================

void mouse_read(
    GLFWwindow* window,
    double mouse_x,
    double mouse_y
)
{
    static bool first_mouse = true;
    static double last_mouse_x = 0.0;
    static double last_mouse_y = 0.0;

    if (first_mouse) {
        last_mouse_x = mouse_x;
        last_mouse_y = mouse_y;
        first_mouse = false;
    }

    double offset_x = mouse_x - last_mouse_x;
    double offset_y = last_mouse_y - mouse_y;

    last_mouse_x = mouse_x;
    last_mouse_y = mouse_y;

    camera_yaw += static_cast<GLfloat>(
        offset_x * kMouseSensitivity
    );

    camera_pitch += static_cast<GLfloat>(
        offset_y * kMouseSensitivity
    );

    if (camera_pitch > kCameraPitchLimit)
        camera_pitch = kCameraPitchLimit;

    if (camera_pitch < -kCameraPitchLimit)
        camera_pitch = -kCameraPitchLimit;

    update_camera_vectors();
}

// ==========================================================
// CONTROLE DO TECLADO
// ==========================================================


void keyboard_read(GLFWwindow* window)
{
    hud_text.str("");
    hud_text.clear();
    camera_caminhando = false;

    // ESC: fechar janela.
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    // W / S: caminhar para frente e para tras sem sair do plano da areia.
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        mover_camera_com_colisao(
            camera_direction_x * kCameraSpeed,
            camera_direction_z * kCameraSpeed
        );
    }

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        mover_camera_com_colisao(
            -camera_direction_x * kCameraSpeed,
            -camera_direction_z * kCameraSpeed
        );
    }

    // A / D: movimentar camera lateralmente.
    GLfloat right_x = camera_direction_z;
    GLfloat right_z = -camera_direction_x;

    // A: movimentar para a esquerda.
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        mover_camera_com_colisao(
            right_x * kCameraSpeed,
            right_z * kCameraSpeed
        );
    }

    // D: movimentar para a direita.
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        mover_camera_com_colisao(
            -right_x * kCameraSpeed,
            -right_z * kCameraSpeed
        );
    }

    // SPACE: restaurar cena.
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        camera_eye_x = kDefaultCameraX;
        camera_eye_y = kDefaultCameraY;
        camera_eye_z = kDefaultCameraZ;

        camera_yaw = kDefaultCameraYaw;
        camera_pitch = kDefaultCameraPitch;

        translate_x = 0.0f;
        translate_y = 0.0f;
        translate_z = 0.0f;

        rotate_x = 0.0f;
        rotate_y = 0.0f;
        rotate_z = 0.0f;

        scale_x = 1.0f;
        scale_y = 1.0f;
        scale_z = 1.0f;

        update_camera_vectors();
    }

    // T + setas: translacao.
    translating = glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS;

    if (translating) {
        hud_text << "TRANSLACAO\n";

        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
            translate_x += kTranslateIncrement;

        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
            translate_x -= kTranslateIncrement;

        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
            translate_y += kTranslateIncrement;

        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
            translate_y -= kTranslateIncrement;

        if (glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_PRESS)
            translate_z += kTranslateIncrement;

        if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS)
            translate_z -= kTranslateIncrement;

        if (translate_x > kTranslateLimit)
            translate_x = kTranslateLimit;

        if (translate_x < -kTranslateLimit)
            translate_x = -kTranslateLimit;

        if (translate_y > kTranslateLimit)
            translate_y = kTranslateLimit;

        if (translate_y < -kTranslateLimit)
            translate_y = -kTranslateLimit;

        if (translate_z > kTranslateLimit)
            translate_z = kTranslateLimit;

        if (translate_z < -kTranslateLimit)
            translate_z = -kTranslateLimit;
    }

    // R + setas: rotacao.
    rotating = glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;

    if (rotating) {
        hud_text << "ROTACAO\n";

        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
            rotate_y += kRotateIncrement;

        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
            rotate_y -= kRotateIncrement;

        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
            rotate_x += kRotateIncrement;

        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
            rotate_x -= kRotateIncrement;

        if (glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_PRESS)
            rotate_z += kRotateIncrement;

        if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS)
            rotate_z -= kRotateIncrement;
    }

    // E + setas: escala.
    scaling = glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS;

    if (scaling) {
        hud_text << "ESCALA\n";

        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
            scale_x += kScaleIncrement;
            scale_y += kScaleIncrement;
            scale_z += kScaleIncrement;
        }

        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
            scale_x -= kScaleIncrement;
            scale_y -= kScaleIncrement;
            scale_z -= kScaleIncrement;
        }

        if (scale_x < kScaleMin) scale_x = kScaleMin;
        if (scale_y < kScaleMin) scale_y = kScaleMin;
        if (scale_z < kScaleMin) scale_z = kScaleMin;

        if (scale_x > kScaleMax) scale_x = kScaleMax;
        if (scale_y > kScaleMax) scale_y = kScaleMax;
        if (scale_z > kScaleMax) scale_z = kScaleMax;
    }
}


// ==========================================================
// REDIMENSIONAR JANELA
// ==========================================================

void resize_window(GLFWwindow* window)
{
    int width, height;

    glfwGetFramebufferSize(window, &width, &height);

    glViewport(0, 0, width, height);
}

// ==========================================================
// DESENHAR OBJETO
// ==========================================================

void draw_object(
    GlbModel& object,
    GLfloat x,
    GLfloat y,
    GLfloat z,
    GLfloat rotation_x,
    GLfloat rotation_y,
    GLfloat rotation_z,
    GLfloat target_size
)
{
    glPushMatrix();

    glTranslatef(x, y, z);

    glRotatef(rotation_x, 1.0f, 0.0f, 0.0f);
    glRotatef(rotation_y, 0.0f, 1.0f, 0.0f);
    glRotatef(rotation_z, 0.0f, 0.0f, 1.0f);

    draw_model_normalized(object, target_size);

    glPopMatrix();
}

// Mancha de sombra translucida, aproximada por uma elipse radial suave.
void draw_soft_shadow(
    GLfloat x,
    GLfloat y,
    GLfloat z,
    GLfloat raio_x,
    GLfloat raio_z,
    GLfloat rotacao_y,
    GLfloat opacidade
)
{
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1.0f, -1.0f);
    glDepthMask(GL_FALSE);

    glPushMatrix();
    glTranslatef(x, y, z);
    glRotatef(rotacao_y, 0.0f, 1.0f, 0.0f);

    glBegin(GL_TRIANGLE_FAN);
    glColor4f(0.015f, 0.02f, 0.035f, opacidade);
    glVertex3f(0.0f, 0.0f, 0.0f);

    const int segmentos = 48;
    const GLfloat pi = 3.14159265f;
    for (int i = 0; i <= segmentos; ++i) {
        const GLfloat angulo = 2.0f * pi * i / segmentos;
        glColor4f(0.015f, 0.02f, 0.035f, 0.0f);
        glVertex3f(
            raio_x * std::cos(angulo),
            0.0f,
            raio_z * std::sin(angulo)
        );
    }
    glEnd();

    glPopMatrix();
    glPopAttrib();
}

// ==========================================================
// DESENHAR CENARIO
// ==========================================================

void draw_scene()
{
    // Agua com textura animada.
    draw_water();

    // Sombra suave do barco sobre a superficie da agua.
    draw_soft_shadow(28.0f, 5.24f, 0.0f, 5.2f, 1.8f, 0.0f, 0.24f);

    glEnable(GL_TEXTURE_2D);

    // Ilha.
    draw_object(
        ilha, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 24.0f
    );

    // Desenhar por cima da agua depois da ilha deixa visivel a espuma na costa.
    draw_beach_foam();

    // Sombras projetadas na areia, desviadas da luz do barco.
    draw_soft_shadow(-0.8f, 5.23f, 0.0f, 3.2f, 1.45f, -12.0f, 0.22f);
    draw_soft_shadow(-6.6f, 3.33f, 2.0f, 2.3f, 0.55f, -12.0f, 0.20f);
    draw_soft_shadow(3.8f, 3.33f, 2.0f, 2.3f, 0.55f, -12.0f, 0.20f);
    draw_soft_shadow(-6.2f, 3.33f, -2.0f, 2.0f, 0.50f, -12.0f, 0.20f);
    draw_soft_shadow(3.6f, 3.33f, -2.0f, 2.0f, 0.50f, -12.0f, 0.20f);

    // Cabana.
    draw_object(
        cabana, 0.0f, 5.2f, 0.0f,
        0.0f, 0.0f, 0.0f, 5.5f
    );

    // Coqueiros.
    draw_object(
        coqueiro, -5.2f, 3.3f, 2.0f,
        0.0f, -20.0f, 0.0f, 9.0f
    );

    draw_object(
        coqueiro, 5.2f, 3.3f, 2.0f,
        0.0f, 25.0f, 0.0f, 9.0f
    );

    draw_object(
        coqueiro, -4.8f, 3.3f, -2.0f,
        0.0f, -35.0f, 0.0f, 7.5f
    );

    draw_object(
        coqueiro, 5.0f, 3.3f, -2.0f,
        0.0f, 30.0f, 0.0f, 7.5f
    );

    // Barco parado com leves oscilacoes simulando as ondas.
    draw_object(
        barco,
        barco_x,
        barco_altura + barco_flutuacao,
        barco_z,
        barco_balanço_x,
        barco_rotacao_y,
        barco_balanço_z,
        barco_tamanho
    );

    // Respingos de agua abaixo do barco.
    draw_water_particles();
}

// ==========================================================
// CEU NOTURNO, ESTRELAS E LUA
// ==========================================================

void draw_night_sky()
{
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    // O domo acompanha a camera para manter o ceu no horizonte.
    glPushMatrix();
    glTranslatef(camera_eye_x, camera_eye_y, camera_eye_z);

    const GLfloat raio = 250.0f;
    const GLfloat pi = 3.14159265f;
    const int faixas = 32;
    const int segmentos = 64;

    for (int faixa = 0; faixa < faixas; ++faixa) {
        GLfloat theta0 = pi * faixa / faixas;
        GLfloat theta1 = pi * (faixa + 1) / faixas;
        glBegin(GL_QUAD_STRIP);

        for (int segmento = 0; segmento <= segmentos; ++segmento) {
            GLfloat phi = 2.0f * pi * segmento / segmentos;

            for (int lado = 0; lado < 2; ++lado) {
                GLfloat theta = lado == 0 ? theta0 : theta1;
                GLfloat t = 1.0f - std::fabs(std::cos(theta));
                glColor3f(
                    0.004f + 0.031f * t,
                    0.007f + 0.053f * t,
                    0.025f + 0.135f * t
                );
                glVertex3f(
                    raio * std::sin(theta) * std::cos(phi),
                    raio * std::cos(theta),
                    raio * std::sin(theta) * std::sin(phi)
                );
            }
        }

        glEnd();
    }

    // Estrelas deterministicas para nao alterar o gerador das particulas.
    unsigned int semente = 928371u;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_POINT_SMOOTH);
    glPointSize(2.0f);
    glBegin(GL_POINTS);
    for (int i = 0; i < 260; ++i) {
        semente = semente * 1664525u + 1013904223u;
        GLfloat u = static_cast<GLfloat>(semente) / 4294967295.0f;
        semente = semente * 1664525u + 1013904223u;
        GLfloat v = static_cast<GLfloat>(semente) / 4294967295.0f;
        GLfloat y = 0.12f + 0.86f * u;
        GLfloat phi = 2.0f * pi * v;
        GLfloat horizontal = std::sqrt(1.0f - y * y);
        GLfloat brilho = 0.65f + 0.35f * u;
        glColor4f(brilho * 0.82f, brilho * 0.88f, brilho, 0.9f);
        glVertex3f(
            raio * horizontal * std::cos(phi),
            raio * y,
            raio * horizontal * std::sin(phi)
        );
    }
    glEnd();
    glDisable(GL_POINT_SMOOTH);

    // Lua em um disco voltado para a camera.
    GLfloat dx = -0.48f, dy = 0.58f, dz = 0.66f;
    GLfloat norma = std::sqrt(dx * dx + dy * dy + dz * dz);
    dx /= norma; dy /= norma; dz /= norma;
    GLfloat rx = -dz, rz = dx;
    GLfloat rnorma = std::sqrt(rx * rx + rz * rz);
    rx /= rnorma; rz /= rnorma;
    GLfloat ux = -dy * dx;
    GLfloat uy = dx * dx + dz * dz;
    GLfloat uz = -dy * dz;
    GLfloat cx = raio * dx, cy = raio * dy, cz = raio * dz;

    // Halo transparente e disco claro.
    for (int camada = 0; camada < 5; ++camada) {
        GLfloat r = 8.0f - camada * 1.1f;
        glColor4f(0.72f, 0.82f, 1.0f, 0.035f + camada * 0.018f);
        glBegin(GL_TRIANGLE_FAN);
        glVertex3f(cx, cy, cz);
        for (int i = 0; i <= 48; ++i) {
            GLfloat a = 2.0f * pi * i / 48.0f;
            GLfloat h = std::cos(a), v = std::sin(a);
            glVertex3f(cx + r * (rx * h + ux * v),
                       cy + r * uy * v,
                       cz + r * (rz * h + uz * v));
        }
        glEnd();
    }

    glColor4f(1.0f, 0.94f, 0.76f, 1.0f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(cx, cy, cz);
    for (int i = 0; i <= 48; ++i) {
        GLfloat a = 2.0f * pi * i / 48.0f;
        GLfloat h = std::cos(a), v = std::sin(a);
        glVertex3f(cx + 3.2f * (rx * h + ux * v),
                   cy + 3.2f * uy * v,
                   cz + 3.2f * (rz * h + uz * v));
    }
    glEnd();

    glPopMatrix();
    glPopAttrib();
}

// ==========================================================
// DESENHAR CENA E POSICIONAR A LUZ
// ==========================================================

void draw(GLFWwindow* window)
{
    int width, height;

    glfwGetFramebufferSize(window, &width, &height);

    if (height == 0)
        return;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    gluPerspective(
        kPerspectiveFieldOfViewAngle,
        static_cast<GLfloat>(width) /
            static_cast<GLfloat>(height),
        kPerspectiveNearZ,
        kPerspectiveFarZ
    );

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    gluLookAt(
        camera_eye_x,
        camera_eye_y + camera_bob_intensidade * 0.045f *
            std::sin(static_cast<GLfloat>(glfwGetTime()) * 10.0f),
        camera_eye_z,
        camera_eye_x + camera_direction_x,
        camera_eye_y + camera_direction_y + camera_bob_intensidade * 0.045f *
            std::sin(static_cast<GLfloat>(glfwGetTime()) * 10.0f),
        camera_eye_z + camera_direction_z,
        kCameraUpX,
        kCameraUpY,
        kCameraUpZ
    );

    draw_night_sky();

    glTranslatef(translate_x, translate_y, translate_z);

    glRotatef(rotate_x, 1.0f, 0.0f, 0.0f);
    glRotatef(rotate_y, 0.0f, 1.0f, 0.0f);
    glRotatef(rotate_z, 0.0f, 0.0f, 1.0f);

    glScalef(scale_x, scale_y, scale_z);

    // Posiciona a luz junto ao barco.
    glPushMatrix();

    glTranslatef(
        barco_x,
        barco_altura,
        barco_z
    );

    const GLfloat posicao_luz[] = {
        0.0f, 16.0f, 0.0f, 1.0f
    };

    glLightfv(GL_LIGHT0, GL_POSITION, posicao_luz);

    glPopMatrix();

    draw_scene();
}

// ==========================================================
// HUD
// ==========================================================

void draw_hud(GLFWwindow* window)
{
    (void)window;

    // HUD removido para nao exibir textos na cena.
}

// ==========================================================
// CARREGAR MODELOS
// ==========================================================

bool load_all_models()
{
    if (!load_model(ilha, "models/ilha.glb"))
        return false;

    if (!load_model(cabana, "models/cabana.glb"))
        return false;

    if (!load_model(coqueiro, "models/coqueiro.glb"))
        return false;

    if (!load_model(cadeira_praia, "models/cadeira_praia.glb"))
        return false;

    if (!load_model(barco, "models/barco.glb"))
        return false;

    return true;
}

// ==========================================================
// MAIN
// ==========================================================

int main()
{
    if (!glfwInit()) {
        std::cerr
            << "Falha ao inicializar GLFW."
            << std::endl;

        return EXIT_FAILURE;
    }

    GLFWwindow* window = glfwCreateWindow(
        1000,
        700,
        "TDE 1 - Ilha Tropical",
        nullptr,
        nullptr
    );

    if (!window) {
        std::cerr
            << "Falha ao criar janela."
            << std::endl;

        glfwTerminate();

        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK) {
        std::cerr
            << "Falha ao inicializar GLEW."
            << std::endl;

        glfwDestroyWindow(window);
        glfwTerminate();

        return EXIT_FAILURE;
    }

    glfwSetInputMode(
        window,
        GLFW_CURSOR,
        GLFW_CURSOR_DISABLED
    );

    double mouse_x, mouse_y;

    glfwGetCursorPos(window, &mouse_x, &mouse_y);

    mouse_read(window, mouse_x, mouse_y);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    glEnable(GL_COLOR_MATERIAL);

    glColorMaterial(
        GL_FRONT_AND_BACK,
        GL_AMBIENT_AND_DIFFUSE
    );

    glEnable(GL_NORMALIZE);

    const GLfloat material_especular[] = {
        0.35f, 0.30f, 0.20f, 1.0f
    };

    glMaterialfv(
        GL_FRONT_AND_BACK,
        GL_SPECULAR,
        material_especular
    );

    glMaterialf(
        GL_FRONT_AND_BACK,
        GL_SHININESS,
        24.0f
    );

    configure_boat_light();

    // Carrega a textura uma unica vez.
    if (!load_water_texture()) {
        std::cerr
            << "A agua sera exibida sem textura."
            << std::endl;
    }

    glClearColor(
        0.035f, 0.045f, 0.09f, 1.0f
    );

    update_camera_vectors();

    // Inicializa gerador aleatorio e relogio das particulas.
    srand(static_cast<unsigned int>(time(nullptr)));

    tempo_ultima_atualizacao =
        static_cast<GLfloat>(glfwGetTime());

    std::cout << "Carregando modelos..." << std::endl;

    if (!load_all_models()) {
        std::cerr
            << "Erro ao carregar os modelos."
            << std::endl;

        destroy_model(ilha);
        destroy_model(cabana);
        destroy_model(coqueiro);
        destroy_model(cadeira_praia);
        destroy_model(barco);

        if (agua_texture != 0)
            glDeleteTextures(1, &agua_texture);

        glfwDestroyWindow(window);
        glfwTerminate();

        return EXIT_FAILURE;
    }

    std::cout
        << "Todos os modelos foram carregados!"
        << std::endl;

    while (!glfwWindowShouldClose(window)) {
        glfwGetCursorPos(window, &mouse_x, &mouse_y);

        mouse_read(window, mouse_x, mouse_y);

        glClear(
            GL_COLOR_BUFFER_BIT |
            GL_DEPTH_BUFFER_BIT
        );

        keyboard_read(window);
        update_camera_bob();
        resize_window(window);

        // Atualiza as animacoes.
        update_boat_animation();
        update_water_animation();

        // Atualiza as particulas de agua.
        update_water_particles();

        draw(window);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    destroy_model(ilha);
    destroy_model(cabana);
    destroy_model(coqueiro);
    destroy_model(cadeira_praia);
    destroy_model(barco);

    // Libera a textura da agua.
    if (agua_texture != 0) {
        glDeleteTextures(1, &agua_texture);
        agua_texture = 0;
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return EXIT_SUCCESS;
}
