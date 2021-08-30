#pragma once

namespace render::consts {
constexpr unsigned int maxFramesInFlight = 2u;


// bindings, set0 - per frame uniforms.
constexpr unsigned int perFrame_textureArrayBinding = 0u;
constexpr unsigned int perFrame_textureSamplerBinding = 1u;
constexpr unsigned int perFrame_uboBinding = 2u;
}
