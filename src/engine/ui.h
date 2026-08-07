#pragma once

#include "core/platform.h"

namespace aether::engine {

class Renderer2D;

namespace ui {

class Backend {
public:
    virtual ~Backend() = default;

    virtual void set_text(const char* element, const char* text) = 0;
    virtual void set_visible(const char* element, bool visible) = 0;
    virtual void set_fill(const char* element, f32 ratio) = 0;
    virtual void flash(const char* text, f32 duration) = 0;
    virtual void draw(Renderer2D* renderer) {}
};

void init();
void set_backend(Backend* backend);

void set_text(const char* element, const char* text);
void set_visible(const char* element, bool visible);
void set_fill(const char* element, f32 ratio);
void flash(const char* text, f32 duration = 1.5f);
void draw(Renderer2D* renderer);

}

}
