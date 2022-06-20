#pragma once

#include <cstdint>
#include "engine/deps/glm/glm.hpp"
#include "engine/deps/stb_truetype.h"
//TODO: move this somewhere else when porting
#define NOMINMAX
#include <Windows.h>

using i8  = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using f32 = float;
using f64 = double;

using Entity = u32;
using AnimationId = u32;

#define WINDOWS_SIZE_X 1000
#define WINDOWS_SIZE_Y 1000

#ifndef KAMSKI_DLL
#define KAMSKI_DLL "KamskiGame.dll"
#endif

#ifndef KAMSKI_WINDOW_NAME
#define KAMSKI_WINDOW_NAME "Kamski"
#endif

#ifndef KAMSKI_FRAME_COUNT
#define KAMSKI_FRAME_COUNT 100
#endif

#ifdef KAMSKI_DEBUG
#define KAMSKI_BASE_ADDRESS ((void*)TB(2))
#else
#define KAMSKI_BASE_ADDRESS nullptr
#endif

#define KAMSKI_MAX_LIGHT_COUNT 8192

#ifndef KAMSKI_MEMORY_CHUNKS_MAX
#define KAMSKI_MEMORY_CHUNKS_MAX 1024
#endif

#ifndef KAMSKI_RECORDING_FRAMERATE
#define KAMSKI_RECORDING_FRAMERATE 60
#endif

#define KAMSKI_MAX_PARTICLE_COUNT 8192

#define KAMSKI_PLAYBACK_FILENAME "playback.hmi"

#if 0 && KAMSKI_DEBUG
#define KAMSKI_WINDOW_STYLE WS_EX_TOPMOST | WS_EX_LAYERED
#else
#define KAMSKI_WINDOW_STYLE 0
#endif

#ifndef KAMSKI_MAX_ANIMATION_COUNT
#define KAMSKI_MAX_ANIMATION_COUNT 256
#endif

#define KAMSKI_ANIMATION_ARENA_SIZE MB(16)

#define SCREEN_SIZE_WORLD_COORDS 1000.0f

#define KB(x) (1024ull * (u64)(x))
#define MB(x) (1024ull * KB(x))
#define GB(x) (1024ull * MB(x))
#define TB(x) (1024ull * GB(x))

#define FONT_TEX_WIDTH 1920
#define FONT_TEX_HEIGHT 1080
#define FONT_HEIGHT 100

#define BIT(X) (1 << X)
#define LINE_BIT(X) BIT(__LINE__ - X)

#define ARRAY_COUNT(array) (sizeof(array) / sizeof((array)[0]))

#ifdef assert
#undef assert
#endif

#ifdef KAMSKI_DEBUG
#define assert(expression) if (!(expression)) logError("Assertion failure: " #expression), *(volatile i32*)0=0
#else
#define assert(expression)
#endif

#ifdef KAMSKI_ENGINE
#define ENGINE_OWNED public
#else
#define ENGINE_OWNED private
#endif


#ifdef KAMSKI_DEBUG

#ifdef KAMSKI_ENGINE

#ifdef _MSC_VER

#define logDebug(format, ...) kamskiLog(format, __FILE__, __LINE__, __FUNCTION__ , KamskiLogLevel::Debug, __VA_ARGS__)
#define logInfo(format, ...) kamskiLog(format, __FILE__, __LINE__, __FUNCTION__ , KamskiLogLevel::Info, __VA_ARGS__)
#define logWarning(format, ...) kamskiLog(format, __FILE__, __LINE__, __FUNCTION__ , KamskiLogLevel::Warning, __VA_ARGS__)
#define logError(format, ...) kamskiLog(format, __FILE__, __LINE__, __FUNCTION__ , KamskiLogLevel::Error, __VA_ARGS__)

#else // _MSC_VER

#define logDebug(format, ...) kamskiLog(format, __FILE__, __LINE__, __FUNCTION__, KamskiLogLevel::Debug __VA_OPT__(,) __VA_ARGS__)
#define logInfo(format, ...) kamskiLog(format, __FILE__, __LINE__, __FUNCTION__, KamskiLogLevel::Info __VA_OPT__(,) __VA_ARGS__)
#define logWarning(format, ...) kamskiLog(format, __FILE__, __LINE__, __FUNCTION__, KamskiLogLevel::Warning __VA_OPT__(,) __VA_ARGS__)
#define logError(format, ...) kamskiLog(format, __FILE__, __LINE__, __FUNCTION__, KamskiLogLevel::Error __VA_OPT__(,) __VA_ARGS__)

#endif //_MSC_VER

#else // KAMSKI_ENGINE

#ifdef _MSC_VER

#define logDebug(format, ...) ENGINE.kamskiLog(format, __FILE__, __LINE__, __FUNCTION__ , KamskiLogLevel::Debug, __VA_ARGS__)
#define logInfo(format, ...) ENGINE.kamskiLog(format, __FILE__, __LINE__, __FUNCTION__ , KamskiLogLevel::Info, __VA_ARGS__)
#define logWarning(format, ...) ENGINE.kamskiLog(format, __FILE__, __LINE__, __FUNCTION__ , KamskiLogLevel::Warning, __VA_ARGS__)
#define logError(format, ...) ENGINE.kamskiLog(format, __FILE__, __LINE__, __FUNCTION__ , KamskiLogLevel::Error, __VA_ARGS__)

#else // _MSC_VER

#define logDebug(format, ...) ENGINE.kamskiLog(format, __FILE__, __LINE__, __FUNCTION__, KamskiLogLevel::Debug __VA_OPT__(,) __VA_ARGS__)
#define logInfo(format, ...) ENGINE.kamskiLog(format, __FILE__, __LINE__, __FUNCTION__, KamskiLogLevel::Info __VA_OPT__(,) __VA_ARGS__)
#define logWarning(format, ...) ENGINE.kamskiLog(format, __FILE__, __LINE__, __FUNCTION__, KamskiLogLevel::Warning __VA_OPT__(,) __VA_ARGS__)
#define logError(format, ...) ENGINE.kamskiLog(format, __FILE__, __LINE__, __FUNCTION__, KamskiLogLevel::Error __VA_OPT__(,) __VA_ARGS__)

#endif // _MSC_VER

#endif // KAMSKI_ENGINE

#else // KAMSKI_DEBUG

#define logDebug(format, ...)
#define logInfo(format, ...)
#define logWarning(format, ...)
#define logError(format, ...)

#endif // KAMSKI_DEBUG


// ######## TYPES ########


// ######## Logger ########

enum class KamskiLogLevel
{
    Error,
    Warning,
    Info,
    Debug
};

// ######## I/O ########

enum class KeyState : u8
{
    NONE,
    HOLD,
    PRESS,
    RELEASE,
    ENUM_COUNT
};

enum class SpecialKeyCode
{
    SPACE,
    RETURN,
    ESCAPE,
    SHIFT,
    CTRL,
    ALT,
    ENUM_COUNT
};

enum class MouseButtonCode
{
    LEFT_CLICK,
    RIGHT_CLICK,
    MIDDLE_CLICK,
    MOUSE_4,
    MOUSE_5,
    MOUSE_BUTTON_CODE_MAX,
    ENUM_COUNT
};

// ######## Memory ########

class GeneralAllocator
{
    public:
    GeneralAllocator(u64 capacity);
    // Allocates [allocSize] bytes with [alignment]-byte alignment
    void* alloc(u64 allocSize, u8 alignment = 8);
    // Frees pointer
    void free(void* ptr);

    ENGINE_OWNED:
    void printAllocations(bool printFreeChunks) const;
    void splitChunk(u64 index, u64 size);

    struct MemoryChunk
    {
        void* address;
        u64 size;
        bool isOccupied;
    };

    u64 capacity;
    u64 lastChunkIndex;
    u64 chunkCount;
    MemoryChunk chunks[KAMSKI_MEMORY_CHUNKS_MAX];
    u8 bytes[];
};

class Arena
{
    public:
    Arena(const u64 capacity):
    size(0), capacity(capacity)
    {
    }

    // Allocates [allocSize] bytes with [alignment]-byte alignment
    void* alloc(u64 allocSize, const u8 alignment = 8)
    {
        const u64 alignmentDistance = (alignment - (size % alignment)) % alignment;
        allocSize += alignmentDistance;

        if (size + allocSize <= capacity)
        {
            void* address = &bytes[size + alignmentDistance];
            size += allocSize;
            return address;
        }

        return nullptr;
    }

    // size has no reason to be private since a setter and getter would be implemented anyway
    u64 size;
    ENGINE_OWNED:
    u64 capacity;
    u8 bytes[];
};

// UI

enum class AnchorPoint
{
    NW = 0, N, NE, W, C, E, SW, S, SE
};

#ifdef KAMSKI_ENGINE

inline constexpr u64 MAX_QUAD_COUNT = 100000;
inline constexpr u64 MAX_VERTEX_COUNT = MAX_QUAD_COUNT * 4;
inline constexpr u64 MAX_INDEX_COUNT = MAX_QUAD_COUNT * 6;
inline constexpr u64 FONT_CHARACTER_COUNT = 96;
inline constexpr u64 TEXTURE_SLOT_COUNT = 32;

// ######## FUNCTIONS ########
// ######## Renderer ########

void resizeViewport(u32 x, u32 y);
bool isInitialized();
void setResolution(u32 x, u32 y);
void openGLPosToWorldPos(f32& x, f32& y);
void worldPosToOpenGLPos(f32& x, f32& y);
u32 loadTexture(const char* textureFilePath);
void beginBatch(glm::vec3 camera);
void endBatch();
void mergeFramebuffers();
void renderLights();
void addLight(glm::vec2 position, f32 radius, const glm::vec4& color);
void addLightBlocker(glm::vec2 position, glm::vec2 size);
void flush();
void flushUI();
void swapClear();
void drawTexturedQuad(glm::vec2 position, glm::vec2 size, const u32 texId, f32 rotation);
void drawColoredQuad(glm::vec2 position, glm::vec2 size, const glm::vec4& color, f32 rotation);
void drawQuad(glm::vec2 position, glm::vec2 size, f32 texId, const glm::vec4& color, f32 rotation);
void loadFont(const char* path, u32 &fontTexId, stbtt_bakedchar* chars);
void drawCharacter(glm::vec2 position, const f32 scale, const char character);
void drawText(glm::vec2 position, const f32 scale, const char* text);
void drawTextInsideBox(glm::vec2 position1, glm::vec2 position2, const f32 scale, const char* text);
glm::vec2 getScreenSize();

// ######## Random ########

u64 randomU64(u64& state);
u64 randomU64(u64& state, u64 min, u64 max);
f64 randomF64(u64& state);
f64 randomF64(u64& state, f64 min, f64 max);
f32 randomF32(u64& state);
f32 randomF32(u64& state, f32 min, f32 max);
u64 localStateRandomU64(u64 state);
f64 localStateRandomF64(u64 state);
f64 localStateRandomF64(u64 state, f64 min, f64 max);
f32 localStateRandomF32(u64 state);
f32 localStateRandomF32(u64 state, f32 min, f32 max);

// ######## Logger ########

void kamskiLog(const char* format,
               const char* fileName,
               int lineNumber,
               const char* funcName,
               KamskiLogLevel logLevel,
               ...);

// ######## IO ########

KeyState getKeyState(const SpecialKeyCode keyCode);
KeyState getKeyState(const char keyCode);
KeyState getMouseButtonState(const MouseButtonCode buttonCode);
void getMousePosition(f32& x, f32& y);
void setKeyState(const SpecialKeyCode keyCode, const KeyState state);
void setKeyState(const char keyCode, const KeyState state);
void setMouseButtonState(const MouseButtonCode buttonCode, const KeyState state);
void setMousePosition(f32 x, f32 y);
void inputPass();
void readWholeFile(void* const buffer, u64 bufferCapacity, const char* filePath);
void writeFile(const void* const buffer, const u64 bufferSize, const char* filePath);
u64 getFileSize(const char* filePath);

// ######## Memory ########

void*  temporaryAlloc(u64 allocSize, const u64 alignment);
void*  temporaryAlloc(u64 allocSize);
void*  getPermanentMemory();
void*  globalAlloc(u64 allocSize);
void*  globalAlignedAlloc(u64 allocSize, u8 alignment);
void   globalFree(void* ptr);
void   printGlobalAllocations(bool printFreeChunks = true);
Arena* allocArena(u64 arenaSize);
void   freeArena(Arena* arena);

// ######## Particles ########

void emitParticles(u32 particleCount,
                   glm::vec2 pos,
                   f32 velocityStart,
                   f32 velocityEnd,
                   glm::vec2 dir,
                   glm::vec4 colorStart,
                   glm::vec4 colorEnd,
                   glm::vec2 size,
                   float spread);

void drawParticles();
void simulateParticles(f32 dt);

// ######## Animation ########

void createAnimation(AnimationId id, const u32 textureIds[], u32 frameCount, f32 duration);
u32 getAnimationFrame(AnimationId animationId, f32 startTime);

// ######## Time ########

f64 getGameTime();
void stepTime(f64 dt);

// ######## Platform ########

void kamskiPlatformReadWholeFile(void* buffer, u64 bufferSize, const char* filePath);
void kamskiPlatformWriteFile(const void* const buffer, u64 bufferSize, const char* filePath);
u64 kamskiPlatformGetFileSize(const char* filePath);
f64 kamskiPlatformGetTime();

// ######## UI ########

void drawTextUI(glm::vec2 position, const f32 scale, const char* text);
void drawTextInsideBoxUI(glm::vec2 topLeftCorner, glm::vec2 bottomRightCorner, const f32 scale, const char* text, f32 padding);
void drawQuadUI(glm::vec2 position, glm::vec2 size, const glm::vec4& color, f32 rotation);
void drawUITex(glm::vec2 position, glm::vec2 size, const u32 texId);
glm::vec2 uiPlainColor(enum AnchorPoint anchorPoint, glm::vec2 position, glm::vec2 size, const glm::vec4& color, f32 rotation);
glm::vec2 uiTexture(glm::vec2 position, glm::vec2 size, AnchorPoint anchorPoint, const u32 texId);
glm::vec2 uiText(AnchorPoint anchorPoint, glm::vec2 position, const f32 fontSize, const char* text);
bool uiButtonHover(glm::vec2 position, glm::vec2 size, AnchorPoint anchorPoint, const u32 texId, const u32 texIdHover);
bool uiButtonHoverText(glm::vec2 position, glm::vec2 size, AnchorPoint anchorPoint, const u32 texId, const u32 texIdHover, const f32 fontSize, const char* text);
bool uiButtonText(glm::vec2 position, glm::vec2 size, AnchorPoint anchorPoint, const u32 texId, const f32 fontSize, const char* text);
bool uiButton(glm::vec2 position, glm::vec2 size, AnchorPoint anchorPoint, const u32 texId);
void uiAnchoredPosConverter(glm::vec2& truePosition, glm::vec2 position, glm::vec2 size, AnchorPoint anchorPoint);


#endif

struct API
{
    void* gameState;
    void (*kamskiLog)(const char* format,
                      const char* fileName,
                      int lineNumber,
                      const char* funcName,
                      KamskiLogLevel logLevel,
                      ...);
    void (*beginBatch)(glm::vec3 camera);
    void (*endBatch)();
    void (*swapClear)();
    void (*drawTexturedQuad)(glm::vec2 position, glm::vec2 size, u32 texId, f32 rotation);
    void (*drawColoredQuad)(glm::vec2 position, glm::vec2 size, const glm::vec4& color, f32 rotation);
    void (*drawQuad)(glm::vec2 position, glm::vec2 size, u32 texId, const glm::vec4& color, f32 rotation);
    void (*drawCharacter)(glm::vec2 position, const f32 scale, const char character);
    void (*drawText)(glm::vec2 position, const f32 scale, const char* text);
    void (*drawTextInsideBox)(glm::vec2 position1, glm::vec2 position2, const f32 scale, const char* text);
    void (*addLight)(glm::vec2 position, f32 radius, const glm::vec4& color);
    void (*addLightBlocker)(glm::vec2 position, glm::vec2 size);
    u32  (*loadTexture)(const char* textureFilePath);
    glm::vec2 (*getScreenSize)();
    void*  (*temporaryAlignedAlloc)(u64 allocSize, const u64 alignment);
    void*  (*temporaryAlloc)(u64 allocSize);
    void*  (*getPermanentMemory)();
    void*  (*globalAlloc)(u64 allocSize);
    void*  (*globalAlignedAlloc)(u64 allocSize, u8 alignment);
    void   (*globalFree)(void* ptr);
    Arena* (*allocArena)(u64 arenaCapacity);
    void   (*freeArena)(Arena* arena);
    void   (*printGlobalAllocations)(bool printFreeChunks);
    void (*readWholeFile)(void* const buffer, const u64 bufferCapacity, const char* filePath);
    void (*writeFile)(const void* const buffer, const u64 bufferSize, const char* filePath);
    u64 (*getFileSize)(const char* filePath);
    KeyState (*getSpecialKeyState)(const SpecialKeyCode keyCode);
    KeyState (*getKeyState)(const char keyCode);
    KeyState (*getMouseButtonState)(const MouseButtonCode buttonCode);
    void (*getMousePosition)(f32& x, f32& y);
    void (*setSpecialKeyState)(const SpecialKeyCode keyCode, const KeyState state);
    void (*setKeyState)(const char keyCode, const KeyState state);
    void (*setMouseButtonState)(const MouseButtonCode buttonCode, const KeyState state);
    void (*createAnimation)(AnimationId id, const u32 textureIds[], u32 frameCount, f32 duration);
    u32 (*getAnimationFrame)(AnimationId animationId, f32 startTime);
    u64 (*randomU64)(u64& state);
    u64 (*randomRangeU64)(u64& state, u64 min, u64 max);
    f64 (*randomF64)(u64& state);
    f64 (*randomRangeF64)(u64& state, f64 min, f64 max);
    f32 (*randomF32)(u64& state);
    f32 (*randomRangeF32)(u64& state, f32 min, f32 max);
    u64 (*localStateRandomU64)(u64 state);
    f64 (*localStateRandomF64)(u64 state);
    f64 (*localStateRandomRangeF64)(u64 state, f64 min, f64 max);
    f32 (*localStateRandomF32)(u64 state);
    f32 (*localStateRandomRangeF32)(u64 state, f32 min, f32 max);
    void (*emitParticles)(u32 particleCount,
                          glm::vec2 pos,
                          f32 velocityStart,
                          f32 velocityEnd,
                          glm::vec2 dir,
                          glm::vec4 colorStart,
                          glm::vec4 colorEnd,
                          glm::vec2 size,
                          float spread,
                          float lifeTime);
    void (*drawParticles)();
    void (*endTriangleFan)();
    void (*beginTriangleFan)(glm::vec3 camera);
    void (*addFanVertex)(glm::vec2 pos, u64 index);
    void (*simulateParticles)(f32 dt);
    void (*drawUITex)(glm::vec2 position, glm::vec2 size, const u32 texId);
    void (*drawTextUI)(glm::vec2 position, const f32 scale, const char* text);
    void (*drawQuadUI)(glm::vec2 position, glm::vec2 size, const glm::vec4& color, f32 rotation);
    glm::vec2 (*uiPlainColor)(enum AnchorPoint anchorPoint, glm::vec2 position, glm::vec2 size, const glm::vec4& color, f32 rotation);
    glm::vec2 (*uiTexture)(glm::vec2 position, glm::vec2 size, AnchorPoint anchorPoint, const u32 texId);
    glm::vec2 (*uiText)(AnchorPoint anchorPoint, glm::vec2 position, const f32 fontSize, const char* text);
    void (*drawTextInsideBoxUI)(glm::vec2 topLeftCorner, glm::vec2 bottomRightCorner, const f32 scale, const char* text, f32 padding);
    bool (*uiButtonHover)(glm::vec2 position, glm::vec2 size, AnchorPoint anchorPoint, const u32 texId, const u32 texIdHover);
    bool (*uiButtonHoverText)(glm::vec2 position, glm::vec2 size, AnchorPoint anchorPoint, const u32 texId, const u32 texIdHover, const f32 fontSize, const char* text);
    bool (*uiButtonText)(glm::vec2 position, glm::vec2 size, AnchorPoint anchorPoint, const u32 texId, const f32 fontSize, const char* text);            
    bool (*uiButton)(glm::vec2 position, glm::vec2 size, AnchorPoint anchorPoint, const u32 texId);
    void (*exit)(u32 code);
    void (*setBlurWholeScreen)(bool value);
    f64 (*getGameTime)();

};
