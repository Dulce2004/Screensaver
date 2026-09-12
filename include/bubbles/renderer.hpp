/**
 * @file renderer.hpp
 * @brief Contrato del renderizador OpenGL y de su sesión RAII.
 */

#pragma once

#include "bubbles/types.hpp"

#include <optional>
#include <string>
#include <vector>

namespace bubbles {

/** @brief Parámetros usados para crear una sesión gráfica. */
struct RendererConfig {
    std::size_t bubbleCount; ///< Cantidad mostrada en el título inicial.
    int threadCount;        ///< Hilos mostrados como variante visual.
    bool vsyncEnabled;      ///< Solicita intervalo de intercambio 1 o 0.
    bool fixedWindowSize;   ///< Impide redimensionar durante una medición.
};

/** @brief Dimensiones lógicas y físicas observadas de la ventana. */
struct RendererDimensions {
    int windowWidth;       ///< Ancho lógico usado por la física.
    int windowHeight;      ///< Alto lógico usado por la física.
    int framebufferWidth;  ///< Ancho físico usado por el viewport.
    int framebufferHeight; ///< Alto físico usado por el viewport.
};

/**
 * @brief Dueño RAII no copiable de los recursos GLFW y OpenGL.
 *
 * Solo puede existir una sesión activa. El destructor llama close() para que
 * las salidas tempranas también liberen ventana, buffers, shaders y textura.
 */
class RendererSession {
public:
    /**
     * @brief Inicializa GLFW, OpenGL y los recursos de dibujo.
     * @param config Configuración de la ventana y presentación.
     * @post isReady() indica si todos los recursos quedaron disponibles.
     */
    explicit RendererSession(const RendererConfig& config);

    /** @brief Libera automáticamente los recursos que todavía posea. */
    ~RendererSession();

    RendererSession(const RendererSession&) = delete;
    RendererSession& operator=(const RendererSession&) = delete;
    RendererSession(RendererSession&&) = delete;
    RendererSession& operator=(RendererSession&&) = delete;

    /**
     * @brief Indica si la inicialización terminó correctamente.
     * @return `true` cuando la ventana y todos los recursos están listos.
     */
    bool isReady() const noexcept;

    /**
     * @brief Libera los recursos de forma idempotente.
     * @post isReady() devuelve `false` y futuras llamadas no tienen efecto.
     */
    void close() noexcept;

private:
    bool ownsResources_ = false; ///< La sesión debe ejecutar la limpieza.
    bool ready_ = false;         ///< Todos los recursos fueron inicializados.
};

/**
 * @brief Construye la etiqueta visible de una variante.
 * @param threadCount Cantidad de hilos observada.
 * @return `Secuencial 1T` o una etiqueta OpenMP con la cantidad de hilos.
 */
std::string visualVariantLabel(int threadCount);

/**
 * @brief Dibuja el fondo y las burbujas en el framebuffer actual.
 * @param bubbles Estado visual de las burbujas.
 * @pre Debe existir una RendererSession lista.
 */
void render(const std::vector<Bubble>& bubbles);

/**
 * @brief Indica si el usuario o GLFW solicitaron cerrar la ventana.
 * @return `true` si no hay ventana o si tiene activa la bandera de cierre.
 */
bool rendererWindowShouldClose();

/**
 * @brief Devuelve el reloj monotónico de GLFW.
 * @return Segundos transcurridos según glfwGetTime().
 */
double rendererTime();

/**
 * @brief Reemplaza el título de la ventana activa.
 * @param title Texto que GLFW copia para la ventana.
 */
void setRendererTitle(const std::string& title);

/** @brief Intercambia buffers y procesa los eventos pendientes. */
void presentRendererFrame();

/**
 * @brief Obtiene las dimensiones lógicas y físicas actuales.
 * @return Cuatro dimensiones en cero cuando no existe una ventana activa.
 */
RendererDimensions rendererDimensions();

/**
 * @brief Consulta la tasa de refresco del monitor de la ventana.
 * @return Refresco en Hz o std::nullopt cuando no está disponible.
 */
std::optional<int> queryWindowMonitorRefreshHz();

/**
 * @brief Comprueba que una ventana de medición conserve su estado.
 * @param expected Dimensiones que deben permanecer sin cambios.
 * @param errorMessage Motivo de cierre, minimización o redimensionamiento.
 * @retval true La ventana sigue visible, abierta y con el tamaño esperado.
 * @retval false La medición debe descartarse.
 */
bool measurementWindowIsStable(
    const RendererDimensions& expected,
    std::string& errorMessage);

}  // namespace bubbles
