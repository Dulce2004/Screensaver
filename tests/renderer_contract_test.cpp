#include "bubbles/renderer.hpp"

#include <type_traits>

static_assert(
    std::is_constructible_v<bubbles::RendererSession,
                            const bubbles::RendererConfig&>,
    "Una sesion grafica debe inicializarse con configuracion explicita.");
static_assert(
    !std::is_copy_constructible_v<bubbles::RendererSession>,
    "La sesion grafica no debe duplicar el ownership de OpenGL.");
static_assert(
    !std::is_move_constructible_v<bubbles::RendererSession>,
    "La direccion estable de la sesion evita transferencias ambiguas.");

int main() {
    return 0;
}
