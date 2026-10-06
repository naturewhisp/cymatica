// CYMATICA M0 smoke application: window, input, test shader and audio tone.
// Not gameplay code: no fixed-step simulation exists yet (M1).
#include "audio_engine.h"
#include "raylib.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

// Chladni-inspired nodal pattern (spec section 17.3), m != n.
constexpr const char* kChladniFragmentShader = R"(#version 330
in vec2 fragTexCoord;
out vec4 finalColor;
uniform float uTime;
void main() {
    vec2 p = fragTexCoord;              // normalized [0,1]^2
    const float n = 3.0;
    const float m = 5.0;
    const float kPi = 3.141592653589793;
    float phase = uTime * 0.4;
    float f = cos(n * kPi * p.x + phase) * cos(m * kPi * p.y)
            - cos(m * kPi * p.x) * cos(n * kPi * p.y + phase);
    float nodal = 1.0 - smoothstep(0.0, 0.07, abs(f));
    vec3 plate = vec3(0.015, 0.025, 0.045);
    vec3 line = vec3(0.0, 0.85, 0.95);
    finalColor = vec4(mix(plate, line, nodal), 1.0);
}
)";

constexpr float kBaseFreqHz = 220.0f;
constexpr float kAltFreqHz = 440.0f;
constexpr float kAmplitude = 0.08f;

} // namespace

int main(int argc, char** argv) {
    double smokeSeconds = 0.0; // 0 = run until the window is closed
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--smoke-seconds") == 0 && i + 1 < argc) {
            smokeSeconds = std::atof(argv[++i]);
        }
    }

    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    InitWindow(1280, 720, "CYMATICA - Milestone 0");
    if (!IsWindowReady()) {
        std::fprintf(stderr, "[cymatica_game] window: FAIL\n");
        return 1;
    }
    SetTargetFPS(60); // frame limiter in addition to the VSync hint

    Shader shader = LoadShaderFromMemory(nullptr, kChladniFragmentShader);
    const bool shaderOk = IsShaderValid(shader);
    const int timeLoc = GetShaderLocation(shader, "uTime");

    cymatica::audio::AudioEngine audio;
    const bool audioOk = audio.init({48000, 2, kBaseFreqHz}) && audio.startTone(kBaseFreqHz, kAmplitude);

    std::uint64_t frames = 0;
    const double start = GetTime();
    float freq = kBaseFreqHz;

    while (!WindowShouldClose()) {
        const double elapsed = GetTime() - start;
        if (smokeSeconds > 0.0 && elapsed >= smokeSeconds) break;

        const float wanted = IsKeyDown(KEY_SPACE) ? kAltFreqHz : kBaseFreqHz;
        if (wanted != freq) {
            freq = wanted;
            audio.setToneFrequency(freq);
        }

        const float t = static_cast<float>(elapsed);
        if (timeLoc >= 0) SetShaderValue(shader, timeLoc, &t, SHADER_UNIFORM_FLOAT);

        BeginDrawing();
        ClearBackground(BLACK);
        if (shaderOk) {
            BeginShaderMode(shader);
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), WHITE);
            EndShaderMode();
        }
        DrawText("CYMATICA - Milestone 0 smoke test", 24, 24, 24, RAYWHITE);
        DrawText(TextFormat("Shader: %s | Audio: %s | Tone: %.0f Hz | Device rate: %u Hz | FPS: %d",
                            shaderOk ? "OK" : "FAIL", audioOk ? "OK" : "FAIL", freq, audio.sampleRate(),
                            GetFPS()),
                 24, 60, 18, (shaderOk && audioOk) ? GREEN : RED);
        DrawText("Hold SPACE: 440 Hz | ESC: exit", 24, GetScreenHeight() - 36, 18, YELLOW);
        EndDrawing();
        ++frames;
    }

    const double total = GetTime() - start;
    const std::uint64_t audioFrames = audio.framesRendered();
    audio.shutdown();
    if (shaderOk) UnloadShader(shader);
    CloseWindow();

    std::printf("[cymatica_game] frames=%llu seconds=%.2f avg_fps=%.1f audio_frames=%llu shader=%s audio=%s\n",
                static_cast<unsigned long long>(frames), total, total > 0.0 ? frames / total : 0.0,
                static_cast<unsigned long long>(audioFrames), shaderOk ? "OK" : "FAIL",
                audioOk ? "OK" : "FAIL");
    return (shaderOk && audioOk) ? 0 : 1;
}
