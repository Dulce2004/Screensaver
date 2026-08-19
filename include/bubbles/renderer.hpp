#pragma once

#include "bubbles/types.hpp"

#include <optional>
#include <string>
#include <vector>

namespace bubbles {

struct RendererConfig {
    std::size_t bubbleCount;
    int threadCount;
    bool vsyncEnabled;
    bool fixedWindowSize;
};

struct RendererDimensions {
    int windowWidth;
    int windowHeight;
    int framebufferWidth;
    int framebufferHeight;
};

class RendererSession {
public:
    explicit RendererSession(const RendererConfig& config);
    ~RendererSession();

    RendererSession(const RendererSession&) = delete;
    RendererSession& operator=(const RendererSession&) = delete;
    RendererSession(RendererSession&&) = delete;
    RendererSession& operator=(RendererSession&&) = delete;

    bool isReady() const noexcept;
    void close() noexcept;

private:
    bool ownsResources_ = false;
    bool ready_ = false;
};

std::string visualVariantLabel(int threadCount);
void render(const std::vector<Bubble>& bubbles);
bool rendererWindowShouldClose();
double rendererTime();
void setRendererTitle(const std::string& title);
void presentRendererFrame();
RendererDimensions rendererDimensions();
std::optional<int> queryWindowMonitorRefreshHz();
bool measurementWindowIsStable(
    const RendererDimensions& expected,
    std::string& errorMessage);

}  // namespace bubbles
