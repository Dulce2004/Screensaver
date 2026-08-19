#include "bubbles/renderer.hpp"

#include "config.hpp"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define STBI_ONLY_JPEG
#define STBI_NO_16BIT
#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#include "stb_image.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace bubbles {
namespace {

GLFWwindow* gWindow = nullptr;
bool gGlfwInitialized = false;
GLuint gBubbleShaderProgram = 0;
GLuint gVertexArray = 0;
GLuint gVertexBuffer = 0;
GLsizei gCircleVertexCount = 0;
GLint gCenterLocation = -1;
GLint gRadiusLocation = -1;
GLint gViewportLocation = -1;
GLint gColorLocation = -1;
GLuint gBackgroundShaderProgram = 0;
GLuint gBackgroundVertexArray = 0;
GLuint gBackgroundVertexBuffer = 0;
GLuint gBackgroundTexture = 0;
GLint gBackgroundTextureLocation = -1;
int gCanvasWidth = kInitialWindowWidth;
int gCanvasHeight = kInitialWindowHeight;
int gFramebufferWidth = kInitialWindowWidth;
int gFramebufferHeight = kInitialWindowHeight;
bool gRendererSessionActive = false;

void cleanupRendererImpl();

void glfwErrorCallback(int errorCode, const char* description) {
    std::cerr << "Error de GLFW (" << errorCode << "): "
              << (description != nullptr ? description : "sin descripcion") << '\n';
}

void windowSizeCallback(GLFWwindow*, int width, int height) {
    // GLFW puede reportar cero durante ciertos estados de minimizacion. Se
    // conserva el ultimo canvas valido para no introducir limites degenerados.
    if (width > 0 && height > 0) {
        gCanvasWidth = width;
        gCanvasHeight = height;
    }
}

void framebufferSizeCallback(GLFWwindow*, int width, int height) {
    gFramebufferWidth = std::max(width, 0);
    gFramebufferHeight = std::max(height, 0);
    glViewport(0, 0, gFramebufferWidth, gFramebufferHeight);
}

void keyCallback(GLFWwindow* window, int key, int, int action, int) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

GLuint compileShader(GLenum shaderType, const char* source) {
    const GLuint shader = glCreateShader(shaderType);
    if (shader == 0) {
        std::cerr << "No fue posible crear un objeto shader.\n";
        return 0;
    }

    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_TRUE) {
        return shader;
    }

    GLint logLength = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
    std::vector<GLchar> log(static_cast<std::size_t>(std::max(logLength, 1)));
    glGetShaderInfoLog(shader, static_cast<GLsizei>(log.size()), nullptr, log.data());
    std::cerr << "Error al compilar shader:\n" << log.data() << '\n';
    glDeleteShader(shader);
    return 0;
}

GLuint createShaderProgram(const char* vertexShaderSource,
                           const char* fragmentShaderSource,
                           const char* programName) {
    const GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    if (vertexShader == 0) {
        return 0;
    }

    const GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    if (fragmentShader == 0) {
        glDeleteShader(vertexShader);
        return 0;
    }

    const GLuint program = glCreateProgram();
    if (program == 0) {
        std::cerr << "No fue posible crear el programa " << programName << ".\n";
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return 0;
    }

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    // Tras enlazar, el programa ya contiene el codigo; los shaders individuales
    // se pueden liberar incluso cuando el enlace falla.
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    GLint linked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (linked == GL_TRUE) {
        return program;
    }

    GLint logLength = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
    std::vector<GLchar> log(static_cast<std::size_t>(std::max(logLength, 1)));
    glGetProgramInfoLog(program, static_cast<GLsizei>(log.size()), nullptr, log.data());
    std::cerr << "Error al enlazar el programa " << programName << ":\n"
              << log.data() << '\n';
    glDeleteProgram(program);
    return 0;
}

GLuint createBubbleShaderProgram() {
    // La geometria se expresa como un circulo unitario. El vertex shader la
    // escala por radio, la desplaza al centro y convierte pixeles a NDC.
    constexpr const char* vertexShaderSource = R"GLSL(
        #version 330 core
        layout (location = 0) in vec2 aUnitPosition;

        out vec2 localPosition;

        uniform vec2 uCenter;
        uniform float uRadius;
        uniform vec2 uViewport;

        void main() {
            localPosition = aUnitPosition;
            vec2 pixelPosition = uCenter + aUnitPosition * uRadius;
            vec2 normalizedPosition = (pixelPosition / uViewport) * 2.0 - 1.0;
            gl_Position = vec4(normalizedPosition, 0.0, 1.0);
        }
    )GLSL";

    constexpr const char* fragmentShaderSource = R"GLSL(
        #version 330 core
        in vec2 localPosition;
        out vec4 fragmentColor;
        uniform vec4 uColor;

        void main() {
            float distanceFromCenter = length(localPosition);

            // El interior conserva un alpha bajo para que Fondo.jpg sea visible.
            // El borde recibe mas opacidad para mantener bien definida la burbuja.
            float edge = smoothstep(0.72, 1.0, distanceFromCenter);

            // Pequeno reflejo translucido que aporta volumen sin ocultar el fondo.
            float highlightDistance = length(localPosition - vec2(-0.38, 0.40));
            float highlight = 1.0 - smoothstep(0.04, 0.24, highlightDistance);

            vec3 bubbleColor = mix(uColor.rgb, vec3(1.0),
                                   clamp(edge * 0.16 + highlight * 0.65, 0.0, 1.0));
            float bubbleAlpha = uColor.a * mix(0.42, 1.0, edge)
                                + highlight * 0.10;
            fragmentColor = vec4(bubbleColor, clamp(bubbleAlpha, 0.0, 0.85));
        }
    )GLSL";

    return createShaderProgram(vertexShaderSource, fragmentShaderSource, "de burbujas");
}

GLuint createBackgroundShaderProgram() {
    // El fondo ya esta expresado en coordenadas normalizadas [-1, 1], por lo
    // que cubre todo el canvas independientemente de su resolucion.
    constexpr const char* vertexShaderSource = R"GLSL(
        #version 330 core
        layout (location = 0) in vec2 aPosition;
        layout (location = 1) in vec2 aTextureCoordinate;

        out vec2 textureCoordinate;

        void main() {
            textureCoordinate = aTextureCoordinate;
            gl_Position = vec4(aPosition, 0.0, 1.0);
        }
    )GLSL";

    constexpr const char* fragmentShaderSource = R"GLSL(
        #version 330 core
        in vec2 textureCoordinate;
        out vec4 fragmentColor;

        uniform sampler2D uBackgroundTexture;

        void main() {
            fragmentColor = texture(uBackgroundTexture, textureCoordinate);
        }
    )GLSL";

    return createShaderProgram(vertexShaderSource, fragmentShaderSource, "de fondo");
}

bool createCircleGeometry() {
    // GL_TRIANGLE_FAN requiere el centro, seguido por los puntos del perimetro.
    // El ultimo punto repite al primero para cerrar el circulo sin una grieta.
    std::vector<float> vertices;
    vertices.reserve(static_cast<std::size_t>((kCircleSegments + 2) * 2));
    vertices.push_back(0.0F);
    vertices.push_back(0.0F);

    for (int i = 0; i <= kCircleSegments; ++i) {
        const float angle = 2.0F * kPi * static_cast<float>(i)
                            / static_cast<float>(kCircleSegments);
        vertices.push_back(std::cos(angle));
        vertices.push_back(std::sin(angle));
    }

    gCircleVertexCount = static_cast<GLsizei>(vertices.size() / 2U);

    glGenVertexArrays(1, &gVertexArray);
    glGenBuffers(1, &gVertexBuffer);
    if (gVertexArray == 0 || gVertexBuffer == 0) {
        std::cerr << "No fue posible reservar la geometria del circulo.\n";
        return false;
    }

    glBindVertexArray(gVertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, gVertexBuffer);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
                 vertices.data(),
                 GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    return glGetError() == GL_NO_ERROR;
}

bool createBackgroundGeometry() {
    // Dos triangulos forman un quad de pantalla completa. Cada vertice contiene
    // posicion NDC (x, y) y coordenada de textura (u, v).
    constexpr std::array<float, 24> vertices = {
        -1.0F, -1.0F, 0.0F, 0.0F,
         1.0F, -1.0F, 1.0F, 0.0F,
         1.0F,  1.0F, 1.0F, 1.0F,
        -1.0F, -1.0F, 0.0F, 0.0F,
         1.0F,  1.0F, 1.0F, 1.0F,
        -1.0F,  1.0F, 0.0F, 1.0F
    };

    glGenVertexArrays(1, &gBackgroundVertexArray);
    glGenBuffers(1, &gBackgroundVertexBuffer);
    if (gBackgroundVertexArray == 0 || gBackgroundVertexBuffer == 0) {
        std::cerr << "No fue posible reservar la geometria del fondo.\n";
        return false;
    }

    glBindVertexArray(gBackgroundVertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, gBackgroundVertexBuffer);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
                 vertices.data(),
                 GL_STATIC_DRAW);

    constexpr GLsizei stride = 4 * sizeof(float);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1,
                          2,
                          GL_FLOAT,
                          GL_FALSE,
                          stride,
                          reinterpret_cast<const void*>(2 * sizeof(float)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    return glGetError() == GL_NO_ERROR;
}

bool loadBackgroundTexture(const char* imagePath) {
    if (imagePath == nullptr || imagePath[0] == '\0') {
        std::cerr << "La ruta de la imagen de fondo esta vacia.\n";
        return false;
    }

    // OpenGL interpreta el origen UV en la esquina inferior izquierda; JPEG lo
    // almacena desde arriba. stb_image invierte las filas durante la carga.
    stbi_set_flip_vertically_on_load(1);

    int imageWidth = 0;
    int imageHeight = 0;
    int originalChannels = 0;
    unsigned char* pixels = stbi_load(imagePath,
                                      &imageWidth,
                                      &imageHeight,
                                      &originalChannels,
                                      STBI_rgb_alpha);
    if (pixels == nullptr) {
        const char* reason = stbi_failure_reason();
        std::cerr << "No fue posible cargar '" << imagePath << "': "
                  << (reason != nullptr ? reason : "error desconocido") << '\n';
        return false;
    }

    GLint maximumTextureSize = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maximumTextureSize);
    if (imageWidth <= 0 || imageHeight <= 0
        || imageWidth > maximumTextureSize || imageHeight > maximumTextureSize) {
        std::cerr << "Dimensiones de textura no soportadas: "
                  << imageWidth << 'x' << imageHeight
                  << " (limite OpenGL: " << maximumTextureSize << ").\n";
        stbi_image_free(pixels);
        return false;
    }

    glGenTextures(1, &gBackgroundTexture);
    if (gBackgroundTexture == 0) {
        std::cerr << "No fue posible crear la textura del fondo.\n";
        stbi_image_free(pixels);
        return false;
    }

    glBindTexture(GL_TEXTURE_2D, gBackgroundTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Alineacion de un byte hace la subida segura para cualquier ancho. La
    // textura se normaliza a RGBA para que el shader tenga un formato estable.
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA8,
                 imageWidth,
                 imageHeight,
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 pixels);
    glGenerateMipmap(GL_TEXTURE_2D);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(pixels);

    const GLenum textureError = glGetError();
    if (textureError != GL_NO_ERROR) {
        std::cerr << "OpenGL rechazo la textura de fondo (codigo "
                  << textureError << ").\n";
        glDeleteTextures(1, &gBackgroundTexture);
        gBackgroundTexture = 0;
        return false;
    }

    std::cout << "Fondo cargado: " << imagePath << " ("
              << imageWidth << 'x' << imageHeight << ", "
              << originalChannels << " canales originales).\n";
    return true;
}

bool initializeRendererImpl(const RendererConfig& config) {
    if (config.bubbleCount == 0 || config.bubbleCount > kMaximumBubbleCount) {
        std::cerr << "El modo visual no tiene una cantidad valida de burbujas.\n";
        return false;
    }

    glfwSetErrorCallback(glfwErrorCallback);
    if (glfwInit() != GLFW_TRUE) {
        std::cerr << "No fue posible inicializar GLFW.\n";
        return false;
    }
    gGlfwInitialized = true;

    // Se solicita un contexto moderno y portable. El antialiasing multisample
    // suaviza visualmente el perimetro poligonal de las burbujas.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_RESIZABLE, config.fixedWindowSize ? GLFW_FALSE : GLFW_TRUE);

    const std::string initialTitle =
        "Burbujas OpenGL - " + visualVariantLabel(config.threadCount);
    gWindow = glfwCreateWindow(kInitialWindowWidth,
                               kInitialWindowHeight,
                               initialTitle.c_str(),
                               nullptr,
                               nullptr);
    if (gWindow == nullptr) {
        std::cerr << "No fue posible crear la ventana OpenGL 3.3.\n";
        return false;
    }

    if (config.fixedWindowSize) {
        glfwSetWindowSizeLimits(gWindow,
                                kInitialWindowWidth,
                                kInitialWindowHeight,
                                kInitialWindowWidth,
                                kInitialWindowHeight);
    } else {
        glfwSetWindowSizeLimits(gWindow,
                                kMinimumWindowWidth,
                                kMinimumWindowHeight,
                                GLFW_DONT_CARE,
                                GLFW_DONT_CARE);
    }
    glfwSetWindowSizeCallback(gWindow, windowSizeCallback);
    glfwSetFramebufferSizeCallback(gWindow, framebufferSizeCallback);
    glfwSetKeyCallback(gWindow, keyCallback);
    glfwMakeContextCurrent(gWindow);

    glewExperimental = GL_TRUE;
    const GLenum glewStatus = glewInit();
    if (glewStatus != GLEW_OK) {
        std::cerr << "No fue posible inicializar GLEW: "
                  << reinterpret_cast<const char*>(glewGetErrorString(glewStatus)) << '\n';
        return false;
    }
    // GLEW puede generar GL_INVALID_ENUM al probar extensiones en un contexto
    // Core. Se descarta unicamente ese estado inicial conocido.
    while (glGetError() != GL_NO_ERROR) {
    }

    glfwGetWindowSize(gWindow, &gCanvasWidth, &gCanvasHeight);
    glfwGetFramebufferSize(gWindow, &gFramebufferWidth, &gFramebufferHeight);
    glViewport(0, 0, gFramebufferWidth, gFramebufferHeight);

    gBubbleShaderProgram = createBubbleShaderProgram();
    if (gBubbleShaderProgram == 0) {
        return false;
    }

    gCenterLocation = glGetUniformLocation(gBubbleShaderProgram, "uCenter");
    gRadiusLocation = glGetUniformLocation(gBubbleShaderProgram, "uRadius");
    gViewportLocation = glGetUniformLocation(gBubbleShaderProgram, "uViewport");
    gColorLocation = glGetUniformLocation(gBubbleShaderProgram, "uColor");
    if (gCenterLocation < 0 || gRadiusLocation < 0
        || gViewportLocation < 0 || gColorLocation < 0) {
        std::cerr << "No fue posible localizar los uniforms de las burbujas.\n";
        return false;
    }

    gBackgroundShaderProgram = createBackgroundShaderProgram();
    if (gBackgroundShaderProgram == 0) {
        return false;
    }
    gBackgroundTextureLocation =
        glGetUniformLocation(gBackgroundShaderProgram, "uBackgroundTexture");
    if (gBackgroundTextureLocation < 0) {
        std::cerr << "No fue posible localizar el sampler de la imagen de fondo.\n";
        return false;
    }

    if (!createCircleGeometry() || !createBackgroundGeometry()) {
        return false;
    }

    if (!loadBackgroundTexture(kBackgroundImagePath)) {
        return false;
    }

    // El sampler del fondo siempre lee de la unidad de textura cero.
    glUseProgram(gBackgroundShaderProgram);
    glUniform1i(gBackgroundTextureLocation, 0);
    glUseProgram(0);

    glDisable(GL_DEPTH_TEST);  // Todos los elementos son 2D.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_MULTISAMPLE);
    // El modo visual normal conserva VSync ON. La prueba suplementaria fija el
    // valor antes de init() y lo registra en cada fila, sin mezclar condiciones.
    glfwSwapInterval(config.vsyncEnabled ? 1 : 0);
    return true;
}

}  // namespace

RendererSession::RendererSession(const RendererConfig& config) {
    if (gRendererSessionActive) {
        std::cerr << "Ya existe una sesion grafica activa.\n";
        return;
    }

    gRendererSessionActive = true;
    ownsResources_ = true;
    try {
        ready_ = initializeRendererImpl(config);
    } catch (...) {
        cleanupRendererImpl();
        gRendererSessionActive = false;
        ownsResources_ = false;
        throw;
    }
}

RendererSession::~RendererSession() {
    close();
}

bool RendererSession::isReady() const noexcept {
    return ready_;
}

void RendererSession::close() noexcept {
    if (!ownsResources_) {
        return;
    }
    cleanupRendererImpl();
    gRendererSessionActive = false;
    ownsResources_ = false;
    ready_ = false;
}

std::string visualVariantLabel(int threadCount) {
#ifdef BUBBLES_ENABLE_OPENMP
    std::ostringstream label;
    label << "OpenMP / " << threadCount
          << (threadCount == 1 ? " hilo" : " hilos");
    return label.str();
#else
    (void)threadCount;
    return "Secuencial";
#endif
}

void render(const std::vector<Bubble>& bubbles) {
    if (gFramebufferWidth <= 0 || gFramebufferHeight <= 0
        || gCanvasWidth <= 0 || gCanvasHeight <= 0) {
        return;  // Una ventana minimizada no tiene una superficie dibujable valida.
    }

    // El color de limpieza solo se apreciaria si el quad de fondo no cubriera
    // algun pixel por un error de driver o durante un cambio de tamano.
    glClearColor(0.025F, 0.045F, 0.095F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT);

    // 1) La imagen estatica se dibuja primero y ocupa todo el viewport.
    glUseProgram(gBackgroundShaderProgram);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gBackgroundTexture);
    glBindVertexArray(gBackgroundVertexArray);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // 2) Las burbujas RGBA se componen encima. El blending configurado en
    // init() permite ver el fondo y otras burbujas a traves de su alpha.
    glUseProgram(gBubbleShaderProgram);
    glBindVertexArray(gVertexArray);
    glUniform2f(gViewportLocation,
                static_cast<float>(gCanvasWidth),
                static_cast<float>(gCanvasHeight));

    for (const Bubble& bubble : bubbles) {
        glUniform2f(gCenterLocation, bubble.position.x, bubble.position.y);
        glUniform1f(gRadiusLocation, bubble.radius);
        glUniform4fv(gColorLocation, 1, bubble.color.data());
        glDrawArrays(GL_TRIANGLE_FAN, 0, gCircleVertexCount);
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
}

namespace {

void cleanupRendererImpl() {
    if (gWindow != nullptr) {
        glfwMakeContextCurrent(gWindow);
        if (gBackgroundTexture != 0) {
            glDeleteTextures(1, &gBackgroundTexture);
            gBackgroundTexture = 0;
        }
        if (gBackgroundVertexBuffer != 0) {
            glDeleteBuffers(1, &gBackgroundVertexBuffer);
            gBackgroundVertexBuffer = 0;
        }
        if (gBackgroundVertexArray != 0) {
            glDeleteVertexArrays(1, &gBackgroundVertexArray);
            gBackgroundVertexArray = 0;
        }
        if (gBackgroundShaderProgram != 0) {
            glDeleteProgram(gBackgroundShaderProgram);
            gBackgroundShaderProgram = 0;
        }
        if (gVertexBuffer != 0) {
            glDeleteBuffers(1, &gVertexBuffer);
            gVertexBuffer = 0;
        }
        if (gVertexArray != 0) {
            glDeleteVertexArrays(1, &gVertexArray);
            gVertexArray = 0;
        }
        if (gBubbleShaderProgram != 0) {
            glDeleteProgram(gBubbleShaderProgram);
            gBubbleShaderProgram = 0;
        }

        glfwDestroyWindow(gWindow);
        gWindow = nullptr;
    }

    if (gGlfwInitialized) {
        glfwTerminate();
        gGlfwInitialized = false;
    }
}

}  // namespace

bool rendererWindowShouldClose() {
    return gWindow == nullptr || glfwWindowShouldClose(gWindow) == GLFW_TRUE;
}

double rendererTime() {
    return glfwGetTime();
}

void setRendererTitle(const std::string& title) {
    if (gWindow != nullptr) {
        glfwSetWindowTitle(gWindow, title.c_str());
    }
}

void presentRendererFrame() {
    if (gWindow != nullptr) {
        glfwSwapBuffers(gWindow);
        glfwPollEvents();
    }
}

RendererDimensions rendererDimensions() {
    RendererDimensions dimensions{};
    if (gWindow != nullptr) {
        glfwGetWindowSize(gWindow, &dimensions.windowWidth, &dimensions.windowHeight);
        glfwGetFramebufferSize(
            gWindow, &dimensions.framebufferWidth, &dimensions.framebufferHeight);
    }
    return dimensions;
}

std::optional<int> queryWindowMonitorRefreshHz() {
    if (gWindow == nullptr) {
        return std::nullopt;
    }

    // Una ventana en modo ventana no tiene monitor asociado por GLFW. Se elige
    // el monitor con mayor area de interseccion, que representa donde realmente
    // aparece el canvas. Si GLFW no ofrece un modo valido, el CSV queda vacio.
    int windowX = 0;
    int windowY = 0;
    int windowWidth = 0;
    int windowHeight = 0;
    glfwGetWindowPos(gWindow, &windowX, &windowY);
    glfwGetWindowSize(gWindow, &windowWidth, &windowHeight);

    int monitorCount = 0;
    GLFWmonitor** monitors = glfwGetMonitors(&monitorCount);
    GLFWmonitor* bestMonitor = nullptr;
    long long bestOverlap = 0;
    for (int index = 0; index < monitorCount; ++index) {
        GLFWmonitor* monitor = monitors[index];
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        if (mode == nullptr) {
            continue;
        }
        int monitorX = 0;
        int monitorY = 0;
        glfwGetMonitorPos(monitor, &monitorX, &monitorY);
        const int overlapWidth = std::max(
            0,
            std::min(windowX + windowWidth, monitorX + mode->width)
                - std::max(windowX, monitorX));
        const int overlapHeight = std::max(
            0,
            std::min(windowY + windowHeight, monitorY + mode->height)
                - std::max(windowY, monitorY));
        const long long overlap =
            static_cast<long long>(overlapWidth) * overlapHeight;
        if (overlap > bestOverlap) {
            bestOverlap = overlap;
            bestMonitor = monitor;
        }
    }

    const GLFWvidmode* bestMode =
        bestMonitor != nullptr ? glfwGetVideoMode(bestMonitor) : nullptr;
    if (bestMode == nullptr || bestMode->refreshRate <= 0) {
        return std::nullopt;
    }
    return bestMode->refreshRate;
}

bool measurementWindowIsStable(const RendererDimensions& expected,
                               std::string& errorMessage) {
    if (rendererWindowShouldClose()) {
        errorMessage = "La ventana se cerro antes de completar la medicion.";
        return false;
    }
    if (glfwGetWindowAttrib(gWindow, GLFW_ICONIFIED) == GLFW_TRUE) {
        errorMessage = "La ventana fue minimizada; la medicion se descarta.";
        return false;
    }

    int windowWidth = 0;
    int windowHeight = 0;
    int framebufferWidth = 0;
    int framebufferHeight = 0;
    glfwGetWindowSize(gWindow, &windowWidth, &windowHeight);
    glfwGetFramebufferSize(gWindow, &framebufferWidth, &framebufferHeight);
    if (windowWidth != expected.windowWidth || windowHeight != expected.windowHeight
        || framebufferWidth != expected.framebufferWidth
        || framebufferHeight != expected.framebufferHeight) {
        errorMessage =
            "La resolucion cambio durante la prueba; la medicion se descarta.";
        return false;
    }
    if (framebufferWidth <= 0 || framebufferHeight <= 0) {
        errorMessage = "El framebuffer dejo de ser dibujable.";
        return false;
    }
    return true;
}

}  // namespace bubbles
