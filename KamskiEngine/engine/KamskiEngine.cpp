#include "../KamskiEngine.h"
#define SOGL_MAJOR_VERSION 4
#define SOGL_MINOR_VERSION 5
#define SOGL_IMPLEMENTATION_WIN32
#include "deps/sogl.h"
#include "deps/wglext.h"
#include "deps/stb_image.h"
#include "deps/stb_truetype.h"
#include <gl/GL.h>
#include <cstring>
#include <algorithm>

// ######## RESERVED_TYPES ########
struct Vertex
{
    glm::vec3 position;
    glm::vec4 color;
    glm::vec2 texture;
    f32 texIndex;
};

struct LightVertex
{
    glm::vec2 position;
    glm::vec4 color;
    f32 range;
    f32 distance;
};

struct Intersection
{
    glm::vec2 pos;
    f32 angle;
};

struct Light
{
    glm::vec4 color;
    glm::vec2 pos;
    f32 range;
};

struct LightBlocker
{
    glm::vec2 pos;
    glm::vec2 size;
};

struct RendererData
{
    HDC deviceContext;
    Vertex* quadBufferPtr;
    Vertex* quadBufferUIPtr;
    Light* lightBufferPtr;
    LightBlocker* lightBlockerBufferPtr;
    LightVertex* lightVertexPtr;
    stbtt_bakedchar chars[FONT_CHARACTER_COUNT];
    
    u32 quadVertexArray;
    u32 quadVertexBuffer;
    u32 quadVertexIndicesBuffer;
    u32 quadShaderPtr;
    
    u32 lightVertexBuffer;
    u32 lightShader;
    u32 mergeShader;
    u32 mergeVertexArray;
    u32 albedoFramebuffer;
    u32 albedoTexture;
    u32 lightVertexArray;
    u32 lightFramebuffer;
    u32 lightTexture;
    
    u32 indexCount;
    u32 indexCountUI;
    
    u32 texSlots[TEXTURE_SLOT_COUNT];
    u32 texSlotsUi[TEXTURE_SLOT_COUNT];
    u32 texSlotIndex;
    u32 texSlotUiIndex;
    u32 fontTexId;
    
    u32 resolutionX;
    u32 resolutionY;
    
    f32 aspectRatio;
    
    Vertex quadBuffer[MAX_VERTEX_COUNT];
    Vertex quadBufferUI[MAX_VERTEX_COUNT];
    LightVertex lightVertices[MAX_VERTEX_COUNT];
    Light lightBuffer[KAMSKI_MAX_LIGHT_COUNT];
    LightBlocker lightBlockerBuffer[KAMSKI_MAX_LIGHT_COUNT];
    
    glm::vec3 camera;
};

struct Animation
{
    u32* textureIds;
    u32 frameCount;
    f32 duration;
};

struct GameAnimationInternal
{
    AnimationId unusedIds[KAMSKI_MAX_ANIMATION_COUNT];
    Animation animations[KAMSKI_MAX_ANIMATION_COUNT];
    u32 lastUnusedIdIndex;
    Arena animationArena;
};

struct GameMemoryInternal
{
    void* permanentEngineMemory;
    u64 permanentEngineMemorySize;
    void* permanentMemory;
    u64 permanentMemorySize;
    void* transientMemory;
    u64 transientMemorySize;
    
    u64 totalSize;
    
    //TODO (phillip): maybe have an array of these, owned by each thread or at leas synced
    Arena* tempAlloc;
    GeneralAllocator* globalAlloc;
};

struct PlayerInputInternal
{
    KeyState keys['Z' - 'A' + 11 + (u32)SpecialKeyCode::ENUM_COUNT];
    KeyState mouseButtons[(u32)MouseButtonCode::ENUM_COUNT];
    f32 mouseX;
    f32 mouseY;
};

struct Particle
{
    f32 dieTime;
    f32 lifeTime;
    glm::vec2 pos;
    f32 velStart;
    f32 velEnd;
    glm::vec2 dir;
    glm::vec4 colorStart;
    glm::vec4 colorEnd;
    glm::vec2 size;
};

struct ParticleInternal
{
    u64 randomState;
    u16 front;
    u16 back;
    u32 count;
    Particle particles[KAMSKI_MAX_PARTICLE_COUNT];
};

// ########Animation########

GameAnimationInternal* animationSystemState = nullptr;

void createAnimation(AnimationId id, const u32 textureIds[], const u32 frameCount, const f32 duration)
{
    assert(animationSystemState->lastUnusedIdIndex < KAMSKI_MAX_ANIMATION_COUNT);
    
    animationSystemState->animations[id].frameCount = frameCount;
    animationSystemState->animations[id].duration = duration;
    animationSystemState->animations[id].textureIds = (u32*)animationSystemState->animationArena.alloc(frameCount * sizeof(u32), alignof(Animation));
    memcpy(animationSystemState->animations[id].textureIds, textureIds, frameCount * sizeof(u32));
}

u32 getAnimationFrame(AnimationId id, f32 startTime)
{
    const f64 duration = animationSystemState->animations[id].duration;
    const f64 currentTime = getGameTime();
    const f64 dt = currentTime - startTime;
    const u32 frameIndex = fmod(dt, duration) * (f32)animationSystemState->animations[id].frameCount / duration;

    return animationSystemState->animations[id].textureIds[frameIndex];
}


// ########I/O########

PlayerInputInternal* playerInputSystemState = nullptr;

KeyState getKeyState(const SpecialKeyCode keyCode)
{
    assert((u32)keyCode < (u32)(SpecialKeyCode::ENUM_COUNT));
    return playerInputSystemState->keys['Z' - 'A' + 11 + (u32)keyCode];
}

KeyState getKeyState(const char keyCode)
{
    assert((keyCode >= 'A' && keyCode <= 'Z') || (keyCode >= '0' && keyCode <= '9'));
    if (keyCode <= '9')
    {
        return playerInputSystemState->keys[keyCode + 'Z' - 'A' + 1 - '0'];
    }
    return playerInputSystemState->keys[keyCode - 'A'];
}

KeyState getMouseButtonState(const MouseButtonCode buttonCode)
{
    return playerInputSystemState->mouseButtons[(u32)buttonCode];
}

void setKeyState(const SpecialKeyCode keyCode, const KeyState keyState)
{

    assert((u32)keyCode < (u32)(SpecialKeyCode::ENUM_COUNT));
    const KeyState oldKeyState = getKeyState(keyCode);
    if ((oldKeyState == KeyState::HOLD && keyState == KeyState::PRESS) ||
        (oldKeyState == KeyState::NONE && keyState == KeyState::RELEASE))
    {
        return;
    }

    playerInputSystemState->keys['Z' - 'A' + 11 + (u32)keyCode] = keyState;
}

void setKeyState(const char keyCode, const KeyState keyState)
{
    assert((keyCode >= 'A' && keyCode <= 'Z') || (keyCode >= '0' && keyCode <= '9'));
    const KeyState oldKeyState = getKeyState(keyCode);
    if ((oldKeyState == KeyState::HOLD && keyState == KeyState::PRESS) ||
        (oldKeyState == KeyState::NONE && keyState == KeyState::RELEASE))
    {
        return;
    }

    if (keyCode <= '9')
    {
        playerInputSystemState->keys[keyCode + 'Z' - 'A' + 1 - '0'] = keyState;
    }
    else
    {
        playerInputSystemState->keys[(u32)keyCode - 'A'] = keyState;
    }
}

void setMouseButtonState(const MouseButtonCode buttonCode, const KeyState keyState)
{
    const KeyState oldKeyState = getMouseButtonState(buttonCode);
    if ((oldKeyState == KeyState::HOLD && keyState == KeyState::PRESS) ||
        (oldKeyState == KeyState::NONE && keyState == KeyState::RELEASE))
    {
        return;
    }
    playerInputSystemState->mouseButtons[(u32)buttonCode] = keyState;
}

void getMousePosition(f32& x, f32& y)
{
    x = playerInputSystemState->mouseX;
    y = playerInputSystemState->mouseY;
}

void setMousePosition(const f32 x, const f32 y)
{
    playerInputSystemState->mouseX = x;
    playerInputSystemState->mouseY = y;
}

void inputPass()
{
    for (u32 i = 0; i < ARRAY_COUNT(playerInputSystemState->keys); i++)
    {
        if (playerInputSystemState->keys[i] == KeyState::RELEASE)
        {
            playerInputSystemState->keys[i] = KeyState::NONE;
        }
        else if (playerInputSystemState->keys[i] == KeyState::PRESS)
        {
            playerInputSystemState->keys[i] = KeyState::HOLD;
        }
    }

    for (u32 i = 0; i < ARRAY_COUNT(playerInputSystemState->mouseButtons); i++)
    {
        if (playerInputSystemState->mouseButtons[i] == KeyState::RELEASE)
            playerInputSystemState->mouseButtons[i] = KeyState::NONE;
        else if (playerInputSystemState->mouseButtons[i] == KeyState::PRESS)
            playerInputSystemState->mouseButtons[i] = KeyState::HOLD;
    }
}

u64 getFileSize(const char* filePath)
{
    return kamskiPlatformGetFileSize(filePath);
}

void writeFile(const void* const buffer, const u64 bufferSize, const char* filePath)
{
    kamskiPlatformWriteFile(buffer, bufferSize, filePath);
}

void readWholeFile(void* buffer, const u64 bufferCapacity, const char* filePath)
{
    kamskiPlatformReadWholeFile(buffer, bufferCapacity, filePath);
}


// ########Memory########

GameMemoryInternal memorySystemState = {};

void* temporaryAlloc(u64 allocSize, const u64 alignment)
{
    return memorySystemState.tempAlloc->alloc(allocSize, alignment);
}

void* temporaryAlloc(u64 allocSize)
{
    return temporaryAlloc(allocSize, 8);
}

void* getPermanentMemory()
{
    return memorySystemState.permanentMemory;
}

void* globalAlloc(u64 allocSize)
{
    return memorySystemState.globalAlloc->alloc(allocSize);
}

void* globalAlignedAlloc(u64 allocSize, u8 alignment)
{
    return memorySystemState.globalAlloc->alloc(allocSize, alignment);
}

void globalFree(void* ptr)
{
    memorySystemState.globalAlloc->free(ptr);
}

void printGlobalAllocations(bool printFreeChunks)
{
    memorySystemState.globalAlloc->printAllocations(printFreeChunks);
}

Arena* allocArena(u64 arenaCapacity)
{
    Arena* retval = (Arena*)globalAlloc(sizeof(Arena) + arenaCapacity);
    new(retval) Arena(arenaCapacity);
    return retval;
}

void freeArena(Arena* arena)
{
    globalFree(arena);
}

GeneralAllocator::GeneralAllocator(u64 capacity):
capacity(capacity),
chunkCount(1)
{
    chunks[0].size = capacity;
    chunks[0].address = bytes;
}

void GeneralAllocator::splitChunk(const u64 index, const u64 size)
{
    for (u64 i = chunkCount; i > index + 1; i--)
    {
        chunks[i] = chunks[i - 1];
    }

    chunks[index + 1].address = (u8*)chunks[index].address + size;
    chunks[index + 1].size = chunks[index].size - size;
    chunks[index + 1].isOccupied = false;
    chunks[index].size = size;
    chunkCount++;
}

void* GeneralAllocator::alloc(u64 allocSize, u8 alignment)
{
    void* retval = nullptr;

    if (allocSize == 0)
    {
        logDebug("0 byte allocation");
    }
    else
    {
        for (u64 index = 0; index != chunkCount; index++)
        {
            const u64 i = (lastChunkIndex + index) % chunkCount;
            const u64 alignmentDistance = (alignment - ((u64)chunks[i].address % alignment)) % alignment;
            const u64 size = allocSize + alignmentDistance;

            if (chunks[i].size >= size && !chunks[i].isOccupied)
            {
                retval = (u8*)chunks[i].address + alignmentDistance;

                if (chunks[i].size != size)
                {
                    assert(chunkCount != KAMSKI_MEMORY_CHUNKS_MAX);
                    lastChunkIndex = i + 1;
                    splitChunk(i, size);
                }
                else
                {
                    lastChunkIndex = i;
                }
                chunks[i].isOccupied = true;
                break;
            }
        }
    }

    return retval;
}


void GeneralAllocator::free(void* ptr)
{
    if (!ptr)
        return;
    for (u64 i = 0; i < chunkCount; i++)
    {
        if (chunks[i].address == ptr)
        {
            assert(chunks[i].isOccupied);
            chunks[i].isOccupied = false;

            if (i != 0 && !chunks[i - 1].isOccupied)
            {
                if (i != chunkCount - 1 && !chunks[i + 1].isOccupied)
                {
                    chunks[i-1].size += chunks[i].size + chunks[i+1].size;
                    for (u64 j = i; j < chunkCount - 2; j++)
                    {
                        chunks[j] = chunks[j + 2];
                    }
                    chunkCount -= 2;
                } else
                {
                    chunks[i-1].size += chunks[i].size;
                    for (u64 j = i; j < chunkCount - 1; j++)
                    {
                        chunks[j] = chunks[j + 1];
                    }
                    chunkCount--;
                }
                lastChunkIndex = i-1;
            } else if (i < chunkCount - 2 && !chunks[i + 1].isOccupied)
            {
                chunks[i].size += chunks[i + 1].size;
                for (u64 j = i + 1; j < chunkCount - 1; j++)
                {
                    chunks[j] = chunks[j + 1];
                }
                chunkCount--;
                lastChunkIndex = i;
            }
            break;
        }
    }
}

void GeneralAllocator::printAllocations(bool printFreeChunks) const
{
    const char* suffixes[] = {"B", "KB", "MB", "GB"};
    if (printFreeChunks)
    {
        for (u64 i = 0; i < chunkCount; i++)
        {
            u64 suffixIndex = 0;
            f64 size = chunks[i].size;
            while (size >= 1024.0f)
            {
                size /= 1024.0f;
                suffixIndex ++;
            }
            if (chunks[i].isOccupied)
            {
                logInfo("%f%s allocated at %p", size, suffixes[suffixIndex], chunks[i].address);
            }
            else
            {
                logInfo("%f%s free at %p", size, suffixes[suffixIndex], chunks[i].address);
            }
        }
    }
    else
    {
        for (u64 i = 0; i < chunkCount; i++)
        {
            u64 suffixIndex = 0;
            f64 size = chunks[i].size;
            while (size >= 1024.0f)
            {
                size /= 1024.0f;
                suffixIndex ++;
            }
            if (chunks[i].isOccupied)
            {
                logInfo("%f%s allocated at %p", size, suffixes[suffixIndex], chunks[i].address);
            }
        }
    }
}

// ########Particles########

ParticleInternal* particleSystemState = nullptr;

void emitParticles(u32 particleCount,
                   glm::vec2 pos,
                   f32 velocityStart,
                   f32 velocityEnd,
                   glm::vec2 dir,
                   glm::vec4 colorStart,
                   glm::vec4 colorEnd,
                   glm::vec2 size,
                   float spread,
                   float lifeTime)
{
    particleSystemState->count = std::min<u32>(KAMSKI_MAX_PARTICLE_COUNT, particleSystemState->count + particleCount);
    f32 sizeLen = sqrtf(size.x * size.x + size.y * size.y);

    for (u32 i = 0; i < particleCount; i++)
    {
        Particle* current = &particleSystemState->particles[particleSystemState->back];
        if (particleSystemState->back == particleSystemState->front && particleSystemState->count == KAMSKI_MAX_PARTICLE_COUNT)
        {
            particleSystemState->front = (particleSystemState->front + 1) % KAMSKI_MAX_PARTICLE_COUNT;
        }
        particleSystemState->back = (particleSystemState->back + 1) % KAMSKI_MAX_PARTICLE_COUNT;

        f32 angleSkew = randomF32(particleSystemState->randomState, -3.14159f / 16.0f * spread, 3.14159f / 16.0f * spread);
        f32 speedSkew = randomF32(particleSystemState->randomState, velocityStart / 4.0f,  velocityStart * 4.0f);
        f32 sizeSkew = randomF32(particleSystemState->randomState, -sizeLen / 2.0f, sizeLen / 2.0f);

        const f32 cosVal = cosf(angleSkew);
        const f32 sinVal = sinf(angleSkew);

        glm::vec2 newDir;

        newDir.x = dir.x * cosVal - dir.y * sinVal;
        newDir.y = dir.x * sinVal + dir.y * cosVal;

        current->pos = pos;
        current->velStart = velocityStart + speedSkew;
        current->velEnd = velocityEnd;
        current->dir = newDir;
        current->colorStart = colorStart;
        current->colorEnd = colorEnd;
        current->size = size + sizeSkew;
        current->dieTime = getGameTime() + lifeTime;
        current->lifeTime = lifeTime;
    }

}

// ########Particles########

void renderParticle(Particle p)
{
    f32 t = (p.dieTime - getGameTime()) / p.lifeTime;
    glm::vec4 color = p.colorStart * t + p.colorEnd * (1.0f - t);
    drawColoredQuad(p.pos, p.size, color, 0);
   // logDebug("%f, %f", p.size.x, p.size.y);
}

void drawParticles()
{
    if (particleSystemState->front > particleSystemState->back)
    {
        for (u32 i = particleSystemState->front; i != KAMSKI_MAX_PARTICLE_COUNT; i++)
        {
            renderParticle(particleSystemState->particles[i]);
        }

        for (u32 i = 0; i != particleSystemState->back; i++)
        {
            renderParticle(particleSystemState->particles[i]);
        }
    }
    else
    {
        for (u32 i = particleSystemState->front; i != particleSystemState->back; i++)
        {
            renderParticle(particleSystemState->particles[i]);
        }
    }
}

static void simulateParticle(Particle& p, f32 dt)
{
    f32 t = (p.dieTime - getGameTime()) / p.lifeTime;
    f32 vel = p.velEnd * (1.0f - t) + p.velStart * t;
    p.pos += p.dir * vel * dt;
}

void simulateParticles(f32 dt)
{
    for (;;)
    {
        if (particleSystemState->particles[particleSystemState->front].dieTime > getGameTime() || (particleSystemState->count == 0))
        {
            break;
        }
        particleSystemState->front = (particleSystemState->front + 1) % KAMSKI_MAX_PARTICLE_COUNT;
        particleSystemState->count--;
    }

    if (particleSystemState->front > particleSystemState->back)
    {
        for (u32 i = particleSystemState->front; i != KAMSKI_MAX_PARTICLE_COUNT; i++)
        {
            simulateParticle(particleSystemState->particles[i], dt);
        }

        for (u32 i = 0; i != particleSystemState->back; i++)
        {
            simulateParticle(particleSystemState->particles[i], dt);
        }
    }
    else
    {
        for (u32 i = particleSystemState->front; i != particleSystemState->back; i++)
        {
            simulateParticle(particleSystemState->particles[i], dt);
        }
    }
}

// ########Random########

u64 randomU64(u64& state)
{
    u64 word = ((state >> ((state >> 59u) + 5u)) ^ state) * 12605985483714917081ull;
    state = (word >> 43u) ^ word;
    return state;
}

u64 randomU64(u64& state, u64 min, u64 max)
{
    return min + randomU64(state) % (max - min + 1);
}

f64 randomF64(u64& state)
{
    return (f64)randomU64(state) / (f64)(~0ull);
}

f64 randomF64(u64& state, const f64 min, const f64 max)
{
    f64 x = (f64)randomU64(state) / (f64)(~0ull);
    return  (1.0 - x) * min + x * max;
}

f32 randomF32(u64& state)
{
    return (f32)randomU64(state) / (f32)(~0ull);
}

f32 randomF32(u64& state, const f32 min, const f32 max)
{
    f32 x = (f32)randomU64(state) / (f32)(~0ull);
    return  (1.0f - x) * min + x * max;
}

u64 localStateRandomU64(u64 state)
{
    u64 word = ((state >> ((state >> 59u) + 5u)) ^ state) * 12605985483714917081ull;
    return (word >> 43u) ^ word;
}

f64 localStateRandomF64(u64 state)
{
    return (f64)localStateRandomU64(state) / (f64)(~0ull);
}

f64 localStateRandomF64(u64 state, const f64 min, const f64 max)
{
    f64 x = (f64)localStateRandomU64(state) / (f64)(~0ull);
    return  (1.0 - x) * min + x * max;
}

f32 localStateRandomF32(u64 state)
{
    return (f32)localStateRandomU64(state) / (f32)(~0ull);
}

f32 localStateRandomF32(u64 state, const f32 min, const f32 max)
{
    f32 x = (f32)localStateRandomU64(state) / (f32)(~0ull);
    return  (1.0f - x) * min + x * max;
}

// ########Renderer########

#define DECLARE_WGL_EXT_FUNC(returnType, name, ...) typedef returnType (WINAPI *name##FUNC)(__VA_ARGS__);\
name##FUNC name = (name##FUNC)0;
#define LOAD_WGL_EXT_FUNC(name) name = (name##FUNC) wglGetProcAddress(#name)

DECLARE_WGL_EXT_FUNC(BOOL, wglChoosePixelFormatARB, HDC hdc, const i32* piAttribIList, const FLOAT* pfAttribFList, UINT nMaxFormats, i32* piFormats, UINT* nNumFormats);
DECLARE_WGL_EXT_FUNC(HGLRC, wglCreateContextAttribsARB, HDC hDC, HGLRC hshareContext, const i32* attribList);
DECLARE_WGL_EXT_FUNC(BOOL, wglSwapIntervalEXT, i32 interval);

RendererData* rData = nullptr;
bool rendererInitialized = false;

//TODO: move into math library when it exists
glm::vec2 rotateVec2(const glm::vec2 v, f32 radians)
{
    glm::vec2 retval;
    const f32 cosVal = cosf(radians);
    const f32 sinVal = sinf(radians);

    retval.x = v.x * cosVal - v.y * sinVal;
    retval.y = v.x * sinVal + v.y * cosVal;

    return retval;
}

void worldPosToOpenGLPos(f32& x, f32& y)
{
    const f32 screenWorldSizeY = SCREEN_SIZE_WORLD_COORDS;
    const f32 screenWorldSizeX = screenWorldSizeY * rData->aspectRatio;

    x /= screenWorldSizeX;
    y /= screenWorldSizeY;
}

void openGLPosToWorldPos(f32& x, f32& y)
{
    const f32 screenWorldSizeY = SCREEN_SIZE_WORLD_COORDS;
    const f32 screenWorldSizeX = screenWorldSizeY * rData->aspectRatio;

    x *= screenWorldSizeX;
    y *= screenWorldSizeY;
}

void setResolution(u32 x, u32 y)
{
    rData->resolutionX = x;
    rData->resolutionY = y;
}

glm::vec2 getScreenSize()
{
    return 2.0f * glm::vec2{SCREEN_SIZE_WORLD_COORDS * rData->aspectRatio, SCREEN_SIZE_WORLD_COORDS};
}

void beginTriangleFan(glm::vec3 camera)
{
    rData->aspectRatio = (f32)rData->resolutionX / (f32)rData->resolutionY;
    rData->camera = camera;
    worldPosToOpenGLPos(camera.x, camera.y);
    rData->quadBufferPtr = rData->quadBuffer;
    
    glUseProgram(rData->quadShaderPtr);
    glUniform3f(glGetUniformLocation(rData->quadShaderPtr, "camera"), camera.x, camera.y, camera.z);
}

void addFanVertex(glm::vec2 pos, u64 index)
{
    u64 seed = index * 782349238;
    worldPosToOpenGLPos(pos.x, pos.y);
    rData->quadBufferPtr->position = { pos, 0.0f };
    rData->quadBufferPtr->texture = {0.0f, 0.0f};
    rData->quadBufferPtr->color = glm::vec4(localStateRandomF32(seed), localStateRandomF32(seed * 23487), localStateRandomF32(seed * 49234), 0.3f);
    rData->quadBufferPtr->texIndex = -1.0f;
    rData->quadBufferPtr++;
}

void endTriangleFan()
{
    const i64 size = (u8*)rData->quadBufferPtr - (u8*)rData->quadBuffer;
    assert(size / sizeof(Vertex) >= 3);
    glUseProgram(rData->quadShaderPtr);
    glBindVertexArray(rData->quadVertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, rData->quadVertexBuffer);
    glBufferSubData(GL_ARRAY_BUFFER, 0, size, rData->quadBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    glDrawArrays(GL_TRIANGLE_FAN, 0, size / sizeof(Vertex));
}

void addLight(glm::vec2 position, f32 radius, const glm::vec4& color)
{
    glm::vec2 size = {radius, radius};
    //
    //if ((position.x + size.x / 2.0f - rData->camera.x) * rData->camera.z < -SCREEN_SIZE_WORLD_COORDS * rData->aspectRatio ||
    //( position.x - size.x / 2.0f - rData->camera.x) * rData->camera.z >  SCREEN_SIZE_WORLD_COORDS * rData->aspectRatio ||
    //( position.y + size.y / 2.0f - rData->camera.y) * rData->camera.z < -SCREEN_SIZE_WORLD_COORDS ||
    //( position.y - size.y / 2.0f - rData->camera.y) * rData->camera.z >  SCREEN_SIZE_WORLD_COORDS)
    //return;
    //
    rData->lightBufferPtr->color = color;
    rData->lightBufferPtr->pos = position;
    rData->lightBufferPtr->range = radius;
    rData->lightBufferPtr++;
}


void addLightBlocker(glm::vec2 position, glm::vec2 size)
{
    if ((position.x + size.x / 2.0f - rData->camera.x)  * rData->camera.z / 2 < -SCREEN_SIZE_WORLD_COORDS * rData->aspectRatio ||
        ( position.x - size.x / 2.0f - rData->camera.x) * rData->camera.z / 2 >  SCREEN_SIZE_WORLD_COORDS * rData->aspectRatio ||
        ( position.y + size.y / 2.0f - rData->camera.y) * rData->camera.z / 2 < -SCREEN_SIZE_WORLD_COORDS ||
        ( position.y - size.y / 2.0f - rData->camera.y) * rData->camera.z / 2 >  SCREEN_SIZE_WORLD_COORDS)
        return;

    rData->lightBlockerBufferPtr->pos = position;
    rData->lightBlockerBufferPtr->size = size;
    rData->lightBlockerBufferPtr++;
}


f32 rayCast(glm::vec2 rayPos,
            glm::vec2 rayDir,
            glm::vec2 segPos,
            glm::vec2 segDir)
{

    f32 segParam = (rayDir.x * (segPos.y - rayPos.y) + rayDir.y * (rayPos.x - segPos.x)) / (segDir.x * rayDir.y - segDir.y * rayDir.x);
    f32 rayParam = (segPos.x + segDir.x * segParam - rayPos.x) / rayDir.x;

    if (rayParam > 0 && segParam > 0 && segParam < 1)
        return rayParam;
    else
        return INFINITY;
}

f32 phaseIntersection(glm::vec2 rayPos,
                      glm::vec2 rayDir,
                      glm::vec2 segPos,
                      glm::vec2 segDir)
{
    f32 retval = rayCast(rayPos,
                         rayDir,
                         segPos,
                         segDir);
    return retval;
}

Intersection intersect(LightBlocker* lightBlockers,
                       u32 lightBlockerCount,
                       glm::vec2 rayPos,
                       glm::vec2 rayDir)
{
    Intersection retval = {};
    f32 rayMin = INFINITY;

    f32 rayParam;
    glm::vec2 segPos;
    glm::vec2 segDir;
    glm::vec2 screenSize = getScreenSize();

    // Clockwise segment intersection
    for (u32 i = 0; i < lightBlockerCount; i++)
    {
        // BOTTOM-LEFT (-X, -Y) Phase -> DIR = UP
        segPos = lightBlockers[i].pos + glm::vec2{ -lightBlockers[i].size.x / 2.0f, -lightBlockers[i].size.y / 2.0f};
        segDir = glm::vec2(0.0f, +lightBlockers[i].size.y);

        rayParam = phaseIntersection(rayPos,
                                     rayDir,
                                     segPos,
                                     segDir);

        rayMin = std::min(rayParam, rayMin);

        // BOTTOM-RIGHT (+X, -Y) Phase -> DIR = LEFT
        segPos = lightBlockers[i].pos + glm::vec2{ +lightBlockers[i].size.x / 2.0f, -lightBlockers[i].size.y / 2.0f};
        segDir = glm::vec2(-lightBlockers[i].size.x, 0.0f);

        rayParam = phaseIntersection(rayPos,
                                     rayDir,
                                     segPos,
                                     segDir);

        rayMin = std::min(rayParam, rayMin);

        // TOP-LEFT (-X, +Y) Phase -> DIR = RIGHT
        segPos = lightBlockers[i].pos + glm::vec2{ -lightBlockers[i].size.x / 2.0f, +lightBlockers[i].size.y / 2.0f};
        segDir = glm::vec2(lightBlockers[i].size.x, 0.0f);

        rayParam = phaseIntersection(rayPos,
                                     rayDir,
                                     segPos,
                                     segDir);

        rayMin = std::min(rayParam, rayMin);

        // TOP-RIGHT (+X, +Y) Phase -> DIR = DOWN
        segPos = lightBlockers[i].pos + glm::vec2{ +lightBlockers[i].size.x / 2.0f, +lightBlockers[i].size.y / 2.0f};
        segDir = glm::vec2(0.0f, -lightBlockers[i].size.y);

        rayParam = phaseIntersection(rayPos,
                                     rayDir,
                                     segPos,
                                     segDir);

        rayMin = std::min(rayParam, rayMin);
    }

    retval.pos = rayPos + rayDir * rayMin;
    //rayDir = glm::normalize(rayDir);
    retval.angle = atan2(rayDir.y, rayDir.x);
    return retval;
}



void renderLights()
{
    rData->lightVertexPtr = rData->lightVertices;

    u32 lightCount = rData->lightBufferPtr - rData->lightBuffer;
    u32 lightBlockerCount = rData->lightBlockerBufferPtr - rData->lightBlockerBuffer;

    if (lightCount == 0)
    {
        return;
    }
    glm::vec2 screenSize = getScreenSize();

    Intersection* intersections = (Intersection*)temporaryAlloc(MB(8));
    u32 intersectionCount = 0;
    addLightBlocker({rData->camera.x, rData->camera.y}, getScreenSize());

    lightBlockerCount++;

    for (u32 lightIndex = 0; lightIndex != lightCount; lightIndex++)
    {
        const glm::vec2 rayPos = rData->lightBuffer[lightIndex].pos;
        glm::vec2 rayDir;

        for (u32 blockerIndex = 0; blockerIndex != lightBlockerCount; blockerIndex++)
        {
            // Normalizing this is maybe not necessary
            rayDir = glm::normalize(rData->lightBlockerBuffer[blockerIndex].pos + glm::vec2{-rData->lightBlockerBuffer[blockerIndex].size.x / 2.0f , -rData->lightBlockerBuffer[blockerIndex].size.y / 2.0f} - rayPos);
            intersections[intersectionCount] = intersect(rData->lightBlockerBuffer, lightBlockerCount, rayPos, rayDir);
            worldPosToOpenGLPos(intersections[intersectionCount].pos.x, intersections[intersectionCount].pos.y);
            intersectionCount++;

            rayDir = rotateVec2(rayDir, 0.00001f);
            intersections[intersectionCount] = intersect(rData->lightBlockerBuffer, lightBlockerCount, rayPos, rayDir);
            worldPosToOpenGLPos(intersections[intersectionCount].pos.x, intersections[intersectionCount].pos.y);
            intersectionCount++;

            rayDir = rotateVec2(rayDir, -0.00002f);
            intersections[intersectionCount] = intersect(rData->lightBlockerBuffer, lightBlockerCount, rayPos, rayDir);
            worldPosToOpenGLPos(intersections[intersectionCount].pos.x, intersections[intersectionCount].pos.y);
            intersectionCount++;

            rayDir = (rData->lightBlockerBuffer[blockerIndex].pos + glm::vec2{ rData->lightBlockerBuffer[blockerIndex].size.x / 2.0f , -rData->lightBlockerBuffer[blockerIndex].size.y / 2.0f} - rayPos);
            intersections[intersectionCount] = intersect(rData->lightBlockerBuffer, lightBlockerCount, rayPos, rayDir);
            worldPosToOpenGLPos(intersections[intersectionCount].pos.x, intersections[intersectionCount].pos.y);
            intersectionCount++;

            rayDir = rotateVec2(rayDir, 0.00001f);
            intersections[intersectionCount] = intersect(rData->lightBlockerBuffer, lightBlockerCount, rayPos, rayDir);
            worldPosToOpenGLPos(intersections[intersectionCount].pos.x, intersections[intersectionCount].pos.y);
            intersectionCount++;

            rayDir = rotateVec2(rayDir, -0.00002f);
            intersections[intersectionCount] = intersect(rData->lightBlockerBuffer, lightBlockerCount, rayPos, rayDir);
            worldPosToOpenGLPos(intersections[intersectionCount].pos.x, intersections[intersectionCount].pos.y);
            intersectionCount++;

            rayDir = (rData->lightBlockerBuffer[blockerIndex].pos + glm::vec2{ rData->lightBlockerBuffer[blockerIndex].size.x / 2.0f ,  rData->lightBlockerBuffer[blockerIndex].size.y / 2.0f} - rayPos);
            intersections[intersectionCount] = intersect(rData->lightBlockerBuffer, lightBlockerCount, rayPos, rayDir);
            worldPosToOpenGLPos(intersections[intersectionCount].pos.x, intersections[intersectionCount].pos.y);
            intersectionCount++;

            rayDir = rotateVec2(rayDir, 0.00001f);
            intersections[intersectionCount] = intersect(rData->lightBlockerBuffer, lightBlockerCount, rayPos, rayDir);
            worldPosToOpenGLPos(intersections[intersectionCount].pos.x, intersections[intersectionCount].pos.y);
            intersectionCount++;

            rayDir = rotateVec2(rayDir, -0.00002f);
            intersections[intersectionCount] = intersect(rData->lightBlockerBuffer, lightBlockerCount, rayPos, rayDir);
            worldPosToOpenGLPos(intersections[intersectionCount].pos.x, intersections[intersectionCount].pos.y);
            intersectionCount++;

            rayDir = (rData->lightBlockerBuffer[blockerIndex].pos + glm::vec2{-rData->lightBlockerBuffer[blockerIndex].size.x / 2.0f ,  rData->lightBlockerBuffer[blockerIndex].size.y / 2.0f} - rayPos);
            intersections[intersectionCount] = intersect(rData->lightBlockerBuffer, lightBlockerCount, rayPos, rayDir);
            worldPosToOpenGLPos(intersections[intersectionCount].pos.x, intersections[intersectionCount].pos.y);
            intersectionCount++;

            rayDir = rotateVec2(rayDir, 0.00001f);
            intersections[intersectionCount] = intersect(rData->lightBlockerBuffer, lightBlockerCount, rayPos, rayDir);
            worldPosToOpenGLPos(intersections[intersectionCount].pos.x, intersections[intersectionCount].pos.y);
            intersectionCount++;

            rayDir = rotateVec2(rayDir, -0.00002f);
            intersections[intersectionCount] = intersect(rData->lightBlockerBuffer, lightBlockerCount, rayPos, rayDir);
            worldPosToOpenGLPos(intersections[intersectionCount].pos.x, intersections[intersectionCount].pos.y);
            intersectionCount++;
        }

        std::sort(intersections, intersections + intersectionCount, [](const Intersection& a, const Intersection& b)
                  {
                      return a.angle < b.angle;
                  });

        glm::vec2 glPos = rayPos;
        worldPosToOpenGLPos(glPos.x, glPos.y);
        f32 stub;

        for (u32 i = 0; i != intersectionCount - 1; i++)
        {
            rData->lightVertexPtr->position = intersections[i].pos;
            rData->lightVertexPtr->color = rData->lightBuffer[lightIndex].color;
            rData->lightVertexPtr->range = rData->lightBuffer[lightIndex].range;
            rData->lightVertexPtr->distance = glm::distance(glPos, intersections[i].pos);
            worldPosToOpenGLPos(rData->lightVertexPtr->range, stub);
            rData->lightVertexPtr++;

            rData->lightVertexPtr->position = glPos;
            rData->lightVertexPtr->color = rData->lightBuffer[lightIndex].color;
            rData->lightVertexPtr->range = rData->lightBuffer[lightIndex].range;
            rData->lightVertexPtr->distance = 0;
            worldPosToOpenGLPos(rData->lightVertexPtr->range, stub);
            rData->lightVertexPtr++;

            rData->lightVertexPtr->position = intersections[i + 1].pos;
            rData->lightVertexPtr->color = rData->lightBuffer[lightIndex].color;
            rData->lightVertexPtr->range = rData->lightBuffer[lightIndex].range;
            rData->lightVertexPtr->distance = glm::distance(glPos, intersections[i + 1].pos);
            worldPosToOpenGLPos(rData->lightVertexPtr->range, stub);
            rData->lightVertexPtr++;
        }

        rData->lightVertexPtr->position = intersections[intersectionCount - 1].pos;
        rData->lightVertexPtr->color = rData->lightBuffer[lightIndex].color;
        rData->lightVertexPtr->range = rData->lightBuffer[lightIndex].range;
        rData->lightVertexPtr->distance = glm::distance(glPos, intersections[intersectionCount - 1].pos);
        worldPosToOpenGLPos(rData->lightVertexPtr->range, stub);
        rData->lightVertexPtr++;

        rData->lightVertexPtr->position = glPos;
        rData->lightVertexPtr->color = rData->lightBuffer[lightIndex].color;
        rData->lightVertexPtr->range = rData->lightBuffer[lightIndex].range;
        rData->lightVertexPtr->distance = 0;
        worldPosToOpenGLPos(rData->lightVertexPtr->range, stub);
        rData->lightVertexPtr++;

        rData->lightVertexPtr->position = intersections[0].pos;
        rData->lightVertexPtr->color = rData->lightBuffer[lightIndex].color;
        rData->lightVertexPtr->range = rData->lightBuffer[lightIndex].range;
        rData->lightVertexPtr->distance = glm::distance(glPos, intersections[0].pos);
        worldPosToOpenGLPos(rData->lightVertexPtr->range, stub);
        rData->lightVertexPtr++;
    }

    u64 lightVertexBufferCount = rData->lightVertexPtr - rData->lightVertices;
    assert(lightVertexBufferCount < MAX_VERTEX_COUNT);
    glUseProgram(rData->lightShader);
    glBindVertexArray(rData->lightVertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, rData->lightVertexBuffer);
    glBufferSubData(GL_ARRAY_BUFFER, 0, lightVertexBufferCount * sizeof(LightVertex), rData->lightVertices);
    glBindFramebuffer(GL_FRAMEBUFFER, rData->lightFramebuffer);
    glDrawArrays(GL_TRIANGLES, 0, lightVertexBufferCount);
}

void setBlurWholeScreen(bool value)
{
    glUseProgram(rData->mergeShader);
    glUniform1f(glGetUniformLocation(rData->mergeShader, "blurWholeScreen"), (f32)value);
}

void resizeViewport(u32 x, u32 y)
{

    logDebug("%f %f");
    glViewport(0, 0,
               x, y);

    rData->resolutionX = x;
    rData->resolutionY = y;

    glDeleteFramebuffers(1, &rData->lightFramebuffer);
    glDeleteFramebuffers(1, &rData->albedoFramebuffer);

    glDeleteTextures(1, &rData->lightTexture);
    glDeleteTextures(1, &rData->albedoTexture);

    glCreateTextures(GL_TEXTURE_2D, 1, &rData->lightTexture);
    glBindTexture(GL_TEXTURE_2D, rData->lightTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rData->resolutionX, rData->resolutionY, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    glBindTexture(GL_TEXTURE_2D, 0);

    glCreateFramebuffers(1, &rData->lightFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, rData->lightFramebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rData->lightTexture, 0);

    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glCreateTextures(GL_TEXTURE_2D, 1, &rData->albedoTexture);
    glBindTexture(GL_TEXTURE_2D, rData->albedoTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, rData->resolutionX, rData->resolutionY, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);

    glCreateFramebuffers(1, &rData->albedoFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, rData->albedoFramebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rData->albedoTexture, 0);

    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

u32 loadTexture(const char* textureFilePath)
{
    u32 retval = 0;
    glCreateTextures(GL_TEXTURE_2D, 1, &retval);
    glBindTexture(GL_TEXTURE_2D, retval);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    i32 width, height, channels;
    stbi_set_flip_vertically_on_load(true);

    u8* image = stbi_load(textureFilePath, &width, &height, &channels, 0);
    switch (channels)
    {
        case 1:
        {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_R8, GL_UNSIGNED_BYTE, image);
        }
        break;

        case 2:
        {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RG, GL_UNSIGNED_BYTE, image);
        }
        break;

        case 3:
        {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        }
        break;

        case 4:
        {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        }
        break;

    }
    stbi_image_free(image);
    glBindTexture(GL_TEXTURE_2D, 0);
    return retval;
}

void beginBatch(glm::vec3 camera)
{
    rData->aspectRatio = (f32)rData->resolutionX / (f32)rData->resolutionY;

    rData->camera = camera;
    worldPosToOpenGLPos(camera.x, camera.y);
    rData->quadBufferPtr = rData->quadBuffer;
    rData->lightBufferPtr = rData->lightBuffer;
    rData->lightBlockerBufferPtr = rData->lightBlockerBuffer;
    rData->lightVertexPtr = rData->lightVertices;

    glUseProgram(rData->quadShaderPtr);
    glUniform3f(glGetUniformLocation(rData->quadShaderPtr, "camera"), camera.x, camera.y, camera.z);
    glUseProgram(rData->lightShader);
    glUniform3f(glGetUniformLocation(rData->lightShader, "camera"), camera.x, camera.y, camera.z);
    glUseProgram(0);
}

void mergeFramebuffers()
{
    glUseProgram(rData->mergeShader);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindVertexArray(rData->mergeVertexArray);

    glBindTextureUnit(0, rData->albedoTexture);
    glBindTextureUnit(1, rData->lightTexture);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rData->quadVertexIndicesBuffer);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
}

void endBatch()
{
    flush();
    renderLights();
    mergeFramebuffers();
    glUseProgram(rData->quadShaderPtr);
    glUniform3f(glGetUniformLocation(rData->quadShaderPtr, "camera"), 0, 0, 1);
    flushUI();
}

void flush()
{
    const i64 size = (u8*)rData->quadBufferPtr - (u8*)rData->quadBuffer;
    glUseProgram(rData->quadShaderPtr);
    glBindVertexArray(rData->quadVertexArray);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rData->quadVertexIndicesBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, rData->quadVertexBuffer);
    glBufferSubData(GL_ARRAY_BUFFER, 0, size, rData->quadBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, rData->albedoFramebuffer);

    for (u32 i = 0; i < rData->texSlotIndex; i++)
        glBindTextureUnit(i, rData->texSlots[i]);

    glDrawElements(GL_TRIANGLES, rData->indexCount, GL_UNSIGNED_INT, nullptr);

    rData->indexCount = 0;
    rData->texSlotIndex = 0;
}

void swapClear()
{
    const float ambient = 0.1f;
    SwapBuffers(rData->deviceContext);

    glBindFramebuffer(GL_FRAMEBUFFER, rData->lightFramebuffer);
    glClearColor(ambient, ambient, ambient, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glClearColor(0, 0, 0, 1.0);
    glBindFramebuffer(GL_FRAMEBUFFER, rData->albedoFramebuffer);
    glClear(GL_COLOR_BUFFER_BIT);

    glClearColor(0, 0, 0, 1.0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT);

}

void drawTexturedQuad(glm::vec2 position, glm::vec2 size, const u32 texId, f32 rotation)
{
    glm::vec2 cullSize = glm::abs(size);
    if (( position.x + cullSize.x / 2.0f - rData->camera.x) * rData->camera.z < -SCREEN_SIZE_WORLD_COORDS * rData->aspectRatio ||
        ( position.x - cullSize.x / 2.0f - rData->camera.x) * rData->camera.z >  SCREEN_SIZE_WORLD_COORDS * rData->aspectRatio ||
        ( position.y + cullSize.y / 2.0f - rData->camera.y) * rData->camera.z < -SCREEN_SIZE_WORLD_COORDS ||
        ( position.y - cullSize.y / 2.0f - rData->camera.y) * rData->camera.z >  SCREEN_SIZE_WORLD_COORDS)
        return;


    if (rData->indexCount >= MAX_INDEX_COUNT)
    {
        flush();
        rData->quadBufferPtr = rData->quadBuffer;
    }

    f32 texIndex = -1.0f;

    for (u32 i = 0; i < rData->texSlotIndex; i++)
    {
        if (rData->texSlots[i] == texId)
        {
            texIndex = (f32)i;
            break;
        }
    }

    if (texIndex == -1.0f)
    {
        if (rData->texSlotIndex == 32)
        {
            flush();
            rData->quadBufferPtr = rData->quadBuffer;
        }

        rData->texSlots[rData->texSlotIndex] = texId;
        texIndex = (f32)rData->texSlotIndex;
        rData->texSlotIndex++;
    }

    glm::vec2 pos1 = glm::vec2{ -size.x, -size.y} / 2.0f;
    glm::vec2 pos2 = glm::vec2{  size.x, -size.y} / 2.0f;
    glm::vec2 pos3 = glm::vec2{  size.x,  size.y} / 2.0f;
    glm::vec2 pos4 = glm::vec2{ -size.x,  size.y} / 2.0f;

    pos1 = rotateVec2(pos1, rotation) + glm::vec2{position.x, position.y};
    pos2 = rotateVec2(pos2, rotation) + glm::vec2{position.x, position.y};
    pos3 = rotateVec2(pos3, rotation) + glm::vec2{position.x, position.y};
    pos4 = rotateVec2(pos4, rotation) + glm::vec2{position.x, position.y};

    worldPosToOpenGLPos(pos1.x, pos1.y);
    worldPosToOpenGLPos(pos2.x, pos2.y);
    worldPosToOpenGLPos(pos3.x, pos3.y);
    worldPosToOpenGLPos(pos4.x, pos4.y);

    rData->quadBufferPtr->position = { pos1, 0.0f };
    rData->quadBufferPtr->color = {1.0f, 1.0f, 1.0f, 1.0f};
    rData->quadBufferPtr->texture = {0.0f, 0.0f};
    rData->quadBufferPtr->texIndex = texIndex;
    rData->quadBufferPtr++;

    rData->quadBufferPtr->position = { pos2, 0.0f };
    rData->quadBufferPtr->color = {1.0f, 1.0f, 1.0f, 1.0f};
    rData->quadBufferPtr->texture = { 1.0f, 0.0f };
    rData->quadBufferPtr->texIndex = texIndex;
    rData->quadBufferPtr++;

    rData->quadBufferPtr->position = { pos3, 0.0f };
    rData->quadBufferPtr->color = {1.0f, 1.0f, 1.0f, 1.0f};
    rData->quadBufferPtr->texture = { 1.0f, 1.0f };
    rData->quadBufferPtr->texIndex = texIndex;
    rData->quadBufferPtr++;

    rData->quadBufferPtr->position = { pos4, 0.0f };
    rData->quadBufferPtr->color = {1.0f, 1.0f, 1.0f, 1.0f};
    rData->quadBufferPtr->texture = { 0.0f, 1.0f };
    rData->quadBufferPtr->texIndex = texIndex;

    rData->quadBufferPtr++;

    rData->indexCount += 6;
}

void drawQuad(glm::vec2 position, glm::vec2 size, const u32 texId, const glm::vec4& color, f32 rotation)
{
    glm::vec2 cullSize = glm::abs(size);
    if (( position.x + cullSize.x / 2.0f - rData->camera.x) * rData->camera.z < -SCREEN_SIZE_WORLD_COORDS * rData->aspectRatio ||
        ( position.x - cullSize.x / 2.0f - rData->camera.x) * rData->camera.z >  SCREEN_SIZE_WORLD_COORDS * rData->aspectRatio ||
        ( position.y + cullSize.y / 2.0f - rData->camera.y) * rData->camera.z < -SCREEN_SIZE_WORLD_COORDS ||
        ( position.y - cullSize.y / 2.0f - rData->camera.y) * rData->camera.z >  SCREEN_SIZE_WORLD_COORDS)
        return;
    
    if (rData->indexCount >= MAX_INDEX_COUNT)
    {
        flush();
        rData->quadBufferPtr = rData->quadBuffer;
    }
    
    f32 texIndex = -1.0f;
    
    for (u32 i = 0; i < rData->texSlotIndex; i++)
    {
        if (rData->texSlots[i] == texId)
        {
            texIndex = (f32)i;
            break;
        }
    }
    
    if (texIndex == -1.0f)
    {
        if (rData->texSlotIndex == 32)
        {
            flush();
            rData->quadBufferPtr = rData->quadBuffer;
        }
        
        rData->texSlots[rData->texSlotIndex] = texId;
        texIndex = (f32)rData->texSlotIndex;
        rData->texSlotIndex++;
    }
    
    glm::vec2 pos1 = glm::vec2{ -size.x, -size.y} / 2.0f;
    glm::vec2 pos2 = glm::vec2{  size.x, -size.y} / 2.0f;
    glm::vec2 pos3 = glm::vec2{  size.x,  size.y} / 2.0f;
    glm::vec2 pos4 = glm::vec2{ -size.x,  size.y} / 2.0f;
    
    pos1 = rotateVec2(pos1, rotation) + glm::vec2{position.x, position.y};
    pos2 = rotateVec2(pos2, rotation) + glm::vec2{position.x, position.y};
    pos3 = rotateVec2(pos3, rotation) + glm::vec2{position.x, position.y};
    pos4 = rotateVec2(pos4, rotation) + glm::vec2{position.x, position.y};
    
    worldPosToOpenGLPos(pos1.x, pos1.y);
    worldPosToOpenGLPos(pos2.x, pos2.y);
    worldPosToOpenGLPos(pos3.x, pos3.y);
    worldPosToOpenGLPos(pos4.x, pos4.y);
    
    rData->quadBufferPtr->position = { pos1, 0.0f };
    rData->quadBufferPtr->color = color;
    rData->quadBufferPtr->texture = {0.0f, 0.0f};
    rData->quadBufferPtr->texIndex = texIndex;
    rData->quadBufferPtr++;
    
    rData->quadBufferPtr->position = { pos2, 0.0f };
    rData->quadBufferPtr->color = color;
    rData->quadBufferPtr->texture = { 1.0f, 0.0f };
    rData->quadBufferPtr->texIndex = texIndex;
    rData->quadBufferPtr++;
    
    rData->quadBufferPtr->position = { pos3, 0.0f };
    rData->quadBufferPtr->color = color;
    rData->quadBufferPtr->texture = { 1.0f, 1.0f };
    rData->quadBufferPtr->texIndex = texIndex;
    rData->quadBufferPtr++;
    
    rData->quadBufferPtr->position = { pos4, 0.0f };
    rData->quadBufferPtr->color = color;
    rData->quadBufferPtr->texture = { 0.0f, 1.0f };
    rData->quadBufferPtr->texIndex = texIndex;
    
    rData->quadBufferPtr++;
    
    rData->indexCount += 6;
}


void drawQuadUI(glm::vec2 position, glm::vec2 size, const glm::vec4& color, f32 rotation)
{
    
    if (rData->indexCountUI >= MAX_INDEX_COUNT)
    {
        flushUI();
        rData->quadBufferUIPtr = rData->quadBufferUI;
    }
    
    const f32 texIndex = -1.0f;
    
    glm::vec2 pos1 = glm::vec2{ -size.x, -size.y} / 2.0f;
    glm::vec2 pos2 = glm::vec2{  size.x, -size.y} / 2.0f;
    glm::vec2 pos3 = glm::vec2{  size.x,  size.y} / 2.0f;
    glm::vec2 pos4 = glm::vec2{ -size.x,  size.y} / 2.0f;
    
    pos1 = rotateVec2(pos1, rotation) + glm::vec2{position.x, position.y};
    pos2 = rotateVec2(pos2, rotation) + glm::vec2{position.x, position.y};
    pos3 = rotateVec2(pos3, rotation) + glm::vec2{position.x, position.y};
    pos4 = rotateVec2(pos4, rotation) + glm::vec2{position.x, position.y};
    
    worldPosToOpenGLPos(pos1.x, pos1.y);
    worldPosToOpenGLPos(pos2.x, pos2.y);
    worldPosToOpenGLPos(pos3.x, pos3.y);
    worldPosToOpenGLPos(pos4.x, pos4.y);
    
    rData->quadBufferUIPtr->position = { pos1, 0.0f };
    rData->quadBufferUIPtr->texture = {0.0f, 0.0f};
    rData->quadBufferUIPtr->color = color;
    rData->quadBufferUIPtr->texIndex = texIndex;
    
    rData->quadBufferUIPtr++;
    rData->quadBufferUIPtr->position = { pos2, 0.0f };
    rData->quadBufferUIPtr->texture = { 1.0f, 0.0f };
    rData->quadBufferUIPtr->color = color;
    rData->quadBufferUIPtr->texIndex = texIndex;
    rData->quadBufferUIPtr++;
    
    rData->quadBufferUIPtr->position = { pos3, 0.0f };
    rData->quadBufferUIPtr->texture = { 1.0f, 1.0f };
    rData->quadBufferUIPtr->color = color;
    rData->quadBufferUIPtr->texIndex = texIndex;
    
    rData->quadBufferUIPtr++;
    
    rData->quadBufferUIPtr->position = { pos4, 0.0f };
    rData->quadBufferUIPtr->texture = { 0.0f, 1.0f };
    rData->quadBufferUIPtr->color = color;
    rData->quadBufferUIPtr->texIndex = texIndex;
    
    rData->quadBufferUIPtr++;
    
    rData->indexCountUI += 6;
}

void drawColoredQuad(glm::vec2 position, glm::vec2 size, const glm::vec4& color, f32 rotation)
{
    glm::vec2 cullSize = glm::abs(size);
    if (( position.x + cullSize.x / 2.0f - rData->camera.x) * rData->camera.z < -SCREEN_SIZE_WORLD_COORDS * rData->aspectRatio ||
        ( position.x - cullSize.x / 2.0f - rData->camera.x) * rData->camera.z >  SCREEN_SIZE_WORLD_COORDS * rData->aspectRatio ||
        ( position.y + cullSize.y / 2.0f - rData->camera.y) * rData->camera.z < -SCREEN_SIZE_WORLD_COORDS ||
        ( position.y - cullSize.y / 2.0f - rData->camera.y) * rData->camera.z >  SCREEN_SIZE_WORLD_COORDS)
        return;
    
    if (rData->indexCount >= MAX_INDEX_COUNT)
    {
        flush();
        rData->quadBufferPtr = rData->quadBuffer;
    }

    const f32 texIndex = -1.0f;

    glm::vec2 pos1 = glm::vec2{ -size.x, -size.y} / 2.0f;
    glm::vec2 pos2 = glm::vec2{  size.x, -size.y} / 2.0f;
    glm::vec2 pos3 = glm::vec2{  size.x,  size.y} / 2.0f;
    glm::vec2 pos4 = glm::vec2{ -size.x,  size.y} / 2.0f;

    pos1 = rotateVec2(pos1, rotation) + glm::vec2{position.x, position.y};
    pos2 = rotateVec2(pos2, rotation) + glm::vec2{position.x, position.y};
    pos3 = rotateVec2(pos3, rotation) + glm::vec2{position.x, position.y};
    pos4 = rotateVec2(pos4, rotation) + glm::vec2{position.x, position.y};

    worldPosToOpenGLPos(pos1.x, pos1.y);
    worldPosToOpenGLPos(pos2.x, pos2.y);
    worldPosToOpenGLPos(pos3.x, pos3.y);
    worldPosToOpenGLPos(pos4.x, pos4.y);

    rData->quadBufferPtr->position = { pos1, 0.0f };
    rData->quadBufferPtr->texture = {0.0f, 0.0f};
    rData->quadBufferPtr->color = color;
    rData->quadBufferPtr->texIndex = texIndex;

    rData->quadBufferPtr++;
    rData->quadBufferPtr->position = { pos2, 0.0f };
    rData->quadBufferPtr->texture = { 1.0f, 0.0f };
    rData->quadBufferPtr->color = color;
    rData->quadBufferPtr->texIndex = texIndex;
    rData->quadBufferPtr++;

    rData->quadBufferPtr->position = { pos3, 0.0f };
    rData->quadBufferPtr->texture = { 1.0f, 1.0f };
    rData->quadBufferPtr->color = color;
    rData->quadBufferPtr->texIndex = texIndex;

    rData->quadBufferPtr++;

    rData->quadBufferPtr->position = { pos4, 0.0f };
    rData->quadBufferPtr->texture = { 0.0f, 1.0f };
    rData->quadBufferPtr->color = color;
    rData->quadBufferPtr->texIndex = texIndex;

    rData->quadBufferPtr++;

    rData->indexCount += 6;
}

void loadFont(const char* path, u32 &fontTexId, stbtt_bakedchar *chars)
{
    u64 fileSize = getFileSize(path);
    Arena* arena = allocArena(fileSize + FONT_TEX_WIDTH * FONT_TEX_HEIGHT * 5);
    u8* fileBuffer = (u8*)arena->alloc(fileSize, 1);
    readWholeFile(fileBuffer, fileSize, path);
    u8* bitMap = (u8*)arena->alloc(FONT_TEX_WIDTH*FONT_TEX_HEIGHT, 1);

    stbtt_BakeFontBitmap(fileBuffer,
                         0,
                         FONT_HEIGHT,
                         bitMap,
                         FONT_TEX_WIDTH,
                         FONT_TEX_HEIGHT,
                         ' ',
                         96,
                         chars);

    u8* image = (u8*)arena->alloc(4 * FONT_TEX_WIDTH * FONT_TEX_HEIGHT, 1);
    u32 pixel = 0;
    for (u64 i = 0; i < FONT_TEX_WIDTH * FONT_TEX_HEIGHT * 4; i += 4, pixel++)
    {
        image[i] = 255 * (bitMap[pixel] != 0);
        image[i+1] = 255 * (bitMap[pixel] != 0);
        image[i+2] = 255 * (bitMap[pixel] != 0);
        image[i+3] = 255 * (bitMap[pixel] != 0);
    }

    glCreateTextures(GL_TEXTURE_2D, 1, &fontTexId);
    glBindTexture(GL_TEXTURE_2D, fontTexId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, FONT_TEX_WIDTH, FONT_TEX_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

    freeArena(arena);
}

void drawCharacter(glm::vec2 position, const f32 scale, const char character)
{
    f32 rotation = 0.0f;

    if (rData->indexCount >= MAX_INDEX_COUNT)
    {
        flush();
        rData->quadBufferPtr = rData->quadBuffer;
    }

    f32 texIndex = -1.0f;

    for (u32 i = 0; i < rData->texSlotIndex; i++)
    {
        if (rData->texSlots[i] == rData->fontTexId)
        {
            texIndex = (f32)i;
            break;
        }
    }

    if (texIndex == -1.0f)
    {
        if (rData->texSlotIndex == 32)
        {
            flush();
            rData->quadBufferPtr = rData->quadBuffer;
        }

        rData->texSlots[rData->texSlotIndex] = rData->fontTexId;
        texIndex = (f32)rData->texSlotIndex;
        rData->texSlotIndex++;
    }

    f32 x0Refactored = rData->chars[character - ' '].x0 / (f32)FONT_TEX_WIDTH;
    f32 x1Refactored = rData->chars[character - ' '].x1 / (f32)FONT_TEX_WIDTH;
    f32 y0Refactored = rData->chars[character - ' '].y0 / (f32)FONT_TEX_HEIGHT;
    f32 y1Refactored = rData->chars[character - ' '].y1 / (f32)FONT_TEX_HEIGHT;

    f32 charWidth = (rData->chars[character - ' '].x1 - rData->chars[character - ' '].x0)/(f32)FONT_TEX_WIDTH * scale;
    f32 charHeight = (rData->chars[character - ' '].y1 - rData->chars[character - ' '].y0)/(f32)FONT_TEX_HEIGHT * scale;

    glm::vec2 pos1 = glm::vec2{ -charWidth, +charHeight};
    glm::vec2 pos2 = glm::vec2{ charWidth, +charHeight};
    glm::vec2 pos3 = glm::vec2{ charWidth, -charHeight};
    glm::vec2 pos4 = glm::vec2{ -charWidth, -charHeight};

    pos1 = rotateVec2(pos1, rotation) + glm::vec2{position.x, position.y};
    pos2 = rotateVec2(pos2, rotation) + glm::vec2{position.x, position.y};
    pos3 = rotateVec2(pos3, rotation) + glm::vec2{position.x, position.y};
    pos4 = rotateVec2(pos4, rotation) + glm::vec2{position.x, position.y};

    worldPosToOpenGLPos(pos1.x, pos1.y);
    worldPosToOpenGLPos(pos2.x, pos2.y);
    worldPosToOpenGLPos(pos3.x, pos3.y);
    worldPosToOpenGLPos(pos4.x, pos4.y);

    rData->quadBufferPtr->position = { pos1, 0.0f };
    rData->quadBufferPtr->texture = { x0Refactored, y0Refactored };
    rData->quadBufferPtr->texIndex = texIndex;
    rData->quadBufferPtr++;

    rData->quadBufferPtr->position = { pos2, 0.0f };
    rData->quadBufferPtr->texture = { x1Refactored, y0Refactored };
    rData->quadBufferPtr->texIndex = texIndex;
    rData->quadBufferPtr++;

    rData->quadBufferPtr->position = { pos3, 0.0f };
    rData->quadBufferPtr->texture = { x1Refactored, y1Refactored };
    rData->quadBufferPtr->texIndex = texIndex;

    rData->quadBufferPtr++;

    rData->quadBufferPtr->position = { pos4, 0.0f };
    rData->quadBufferPtr->texture = { x0Refactored, y1Refactored };
    rData->quadBufferPtr->texIndex = texIndex;

    rData->quadBufferPtr++;

    rData->indexCount += 6;
}

void drawCharacterUI(glm::vec2 position, const f32 scale, const char character)
{
    f32 rotation = 0.0f;

    if (rData->indexCountUI >= MAX_INDEX_COUNT)
    {
        flushUI();
        rData->quadBufferUIPtr = rData->quadBufferUI;
    }

    f32 texIndex = -1.0f;

    for (u32 i = 0; i < rData->texSlotUiIndex; i++)
    {
        if (rData->texSlotsUi[i] == rData->fontTexId)
        {
            texIndex = (f32)i;
            break;
        }
    }

    if (texIndex == -1.0f)
    {
        if (rData->texSlotUiIndex == 32)
        {
            flushUI();
            rData->quadBufferUIPtr = rData->quadBufferUI;
        }

        rData->texSlotsUi[rData->texSlotUiIndex] = rData->fontTexId;
        texIndex = (f32)rData->texSlotUiIndex;
        rData->texSlotUiIndex++;
    }

    f32 x0Refactored = rData->chars[character - ' '].x0 / (f32)FONT_TEX_WIDTH;
    f32 x1Refactored = rData->chars[character - ' '].x1 / (f32)FONT_TEX_WIDTH;
    f32 y0Refactored = rData->chars[character - ' '].y0 / (f32)FONT_TEX_HEIGHT;
    f32 y1Refactored = rData->chars[character - ' '].y1 / (f32)FONT_TEX_HEIGHT;

    f32 charWidth = (rData->chars[character - ' '].x1 - rData->chars[character - ' '].x0)/(f32)FONT_TEX_WIDTH * scale;
    f32 charHeight = (rData->chars[character - ' '].y1 - rData->chars[character - ' '].y0)/(f32)FONT_TEX_HEIGHT * scale;

    glm::vec2 pos1 = glm::vec2{ -charWidth, +charHeight};
    glm::vec2 pos2 = glm::vec2{ charWidth, +charHeight};
    glm::vec2 pos3 = glm::vec2{ charWidth, -charHeight};
    glm::vec2 pos4 = glm::vec2{ -charWidth, -charHeight};

    pos1 = rotateVec2(pos1, rotation) + glm::vec2{position.x, position.y};
    pos2 = rotateVec2(pos2, rotation) + glm::vec2{position.x, position.y};
    pos3 = rotateVec2(pos3, rotation) + glm::vec2{position.x, position.y};
    pos4 = rotateVec2(pos4, rotation) + glm::vec2{position.x, position.y};

    worldPosToOpenGLPos(pos1.x, pos1.y);
    worldPosToOpenGLPos(pos2.x, pos2.y);
    worldPosToOpenGLPos(pos3.x, pos3.y);
    worldPosToOpenGLPos(pos4.x, pos4.y);

    rData->quadBufferUIPtr->position = { pos1, 0.0f };
    rData->quadBufferUIPtr->color= {1.0f, 1.0f, 1.0f, 1.0f};
    rData->quadBufferUIPtr->texture = { x0Refactored, y0Refactored };
    rData->quadBufferUIPtr->texIndex = texIndex;
    rData->quadBufferUIPtr++;

    rData->quadBufferUIPtr->position = { pos2, 0.0f };
    rData->quadBufferUIPtr->color= {1.0f, 1.0f, 1.0f, 1.0f};
    rData->quadBufferUIPtr->texture = { x1Refactored, y0Refactored };
    rData->quadBufferUIPtr->texIndex = texIndex;
    rData->quadBufferUIPtr++;

    rData->quadBufferUIPtr->position = { pos3, 0.0f };
    rData->quadBufferUIPtr->color= {1.0f, 1.0f, 1.0f, 1.0f};
    rData->quadBufferUIPtr->texture = { x1Refactored, y1Refactored };
    rData->quadBufferUIPtr->texIndex = texIndex;

    rData->quadBufferUIPtr++;

    rData->quadBufferUIPtr->position = { pos4, 0.0f };
rData->quadBufferUIPtr->color= {1.0f, 1.0f, 1.0f, 1.0f};
rData->quadBufferUIPtr->texture = { x0Refactored, y1Refactored };
rData->quadBufferUIPtr->texIndex = texIndex;

rData->quadBufferUIPtr++;

rData->indexCountUI += 6;
}

void drawText(glm::vec2 position, const f32 scale, const char* text)
{
    glm::vec2 characterPosition = position;
    for (i32 i=0;i<strlen(text);i++)
        if (text[i] == ' ')
        characterPosition.x += scale*0.05f;
    else if (text[i] > ' ')
    {
        f32 delta = (rData->chars[text[i] - ' '].y1 - rData->chars[text[i] - ' '].y0 + rData->chars[text[i] - ' '].yoff*2)/(f32)FONT_TEX_HEIGHT*scale;
        f32 delta2 = (rData->chars[text[i] - ' '].x1 - rData->chars[text[i] - ' '].x0)/(FONT_TEX_WIDTH*0.85f)*scale;
        characterPosition.x += delta2;
        characterPosition.y -= delta;
        drawCharacter(characterPosition, scale, text[i]);
        characterPosition.y += delta;
        characterPosition.x += delta2;
    }
}

void drawText(const glm::vec3& position, const f32 scale, const char* text)
{
    glm::vec3 characterPosition = position;
    for (i32 i=0;i<strlen(text);i++)
        if (text[i] == ' ')
        characterPosition.x += scale*0.05f;
    else if (text[i] > ' ')
    {
        f32 delta = (rData->chars[text[i] - ' '].y1 - rData->chars[text[i] - ' '].y0 + rData->chars[text[i] - ' '].yoff*2)/(f32)FONT_TEX_HEIGHT*scale;
        f32 delta2 = (rData->chars[text[i] - ' '].x1 - rData->chars[text[i] - ' '].x0)/(FONT_TEX_WIDTH*0.85f)*scale;
        characterPosition.x += delta2;
        characterPosition.y -= delta;
        drawCharacter(characterPosition, scale, text[i]);
        characterPosition.y += delta;
        characterPosition.x += delta2;
    }
}

void drawTextUI(glm::vec2 position, const f32 scale, const char* text)
{
    glm::vec2 characterPosition = position;
    for (i32 i=0;i<strlen(text);i++)
        if (text[i] == ' ')
        characterPosition.x += scale*0.05f;
    else if (text[i] > ' ')
    {
        f32 delta = (rData->chars[text[i] - ' '].y1 - rData->chars[text[i] - ' '].y0 + rData->chars[text[i] - ' '].yoff*2)/(f32)FONT_TEX_HEIGHT*scale;
        f32 delta2 = (rData->chars[text[i] - ' '].x1 - rData->chars[text[i] - ' '].x0)/(FONT_TEX_WIDTH*0.85f)*scale;
        characterPosition.x += delta2;
        characterPosition.y -= delta;
        drawCharacterUI(characterPosition, scale, text[i]);
        characterPosition.y += delta;
        characterPosition.x += delta2;
    }
}

void drawTextInsideBox(glm::vec2 topLeftCorner, glm::vec2 bottomRightCorner, const f32 scale, const char* text)
{
    glm::vec2 characterPosition = topLeftCorner;
    characterPosition.y -= 1.25f * scale * FONT_HEIGHT / (f32)FONT_TEX_HEIGHT;
    if (characterPosition.y >= bottomRightCorner.y)
    {
        for (u32 i = 0; i < strlen(text); i++)
        {
            if (text[i] == ' ')
            {
                characterPosition.x += scale * 0.05f;
            }
            else if (text[i] == '\n')
            {
                characterPosition.x = topLeftCorner.x;
                characterPosition.y -= 1.5f * scale * FONT_HEIGHT / (f32)FONT_TEX_HEIGHT;
                if (characterPosition.y <= bottomRightCorner.y)
                    break;
            }
            else if (text[i] > ' ')
            {
                f32 delta = (rData->chars[text[i] - ' '].y1 - rData->chars[text[i] - ' '].y0 + rData->chars[text[i] - ' '].yoff * 2) / (f32)FONT_TEX_HEIGHT * scale;
                f32 delta2 = (rData->chars[text[i] - ' '].x1 - rData->chars[text[i] - ' '].x0) / (FONT_TEX_WIDTH * 0.85f) * scale;
                characterPosition.x += delta2;
                if (characterPosition.x >= bottomRightCorner.x)
                {
                    characterPosition.x = topLeftCorner.x + delta2;
                    characterPosition.y -= 1.5f * scale * FONT_HEIGHT / (f32)FONT_TEX_HEIGHT;
                    if (characterPosition.y <= bottomRightCorner.y)
                        break;
                }
                characterPosition.y -= delta;
                drawCharacter(characterPosition, scale, text[i]);
                characterPosition.y += delta;
                characterPosition.x += delta2;
            }
        }
    }
}

void drawTextInsideBoxUI(glm::vec2 topLeftCorner, glm::vec2 bottomRightCorner, const f32 scale, const char* text, f32 padding)
{
    topLeftCorner.x += padding;
    bottomRightCorner.x -= padding;
    topLeftCorner.y -= padding;
    bottomRightCorner.y += padding;
    glm::vec2 characterPosition = topLeftCorner;
    characterPosition.y -= 1.25f * scale * FONT_HEIGHT / (f32)FONT_TEX_HEIGHT;
    if (characterPosition.y >= bottomRightCorner.y)
    {
        for (u32 i = 0; i < strlen(text); i++)
        {
            if (text[i] == ' ')
            {
                characterPosition.x += scale * 0.05f;
            }
            else if (text[i] == '\n')
            {
                characterPosition.x = topLeftCorner.x;
                characterPosition.y -= 1.5f * scale * FONT_HEIGHT / (f32)FONT_TEX_HEIGHT;
                if (characterPosition.y < bottomRightCorner.y)
                    break;
            }
            else if (text[i] > ' ')
            {
                f32 delta = (rData->chars[text[i] - ' '].y1 - rData->chars[text[i] - ' '].y0 + rData->chars[text[i] - ' '].yoff * 2) / (f32)FONT_TEX_HEIGHT * scale;
                f32 delta2 = (rData->chars[text[i] - ' '].x1 - rData->chars[text[i] - ' '].x0) / (FONT_TEX_WIDTH * 0.85f) * scale;
                characterPosition.x += delta2;
                if (characterPosition.x >= bottomRightCorner.x)
                {
                    characterPosition.x = topLeftCorner.x + delta2;
                    characterPosition.y -= 1.5f * scale * FONT_HEIGHT / (f32)FONT_TEX_HEIGHT;
                    if (characterPosition.y < bottomRightCorner.y)
                        break;
                }
                characterPosition.y -= delta;
                drawCharacterUI(characterPosition, scale, text[i]);
                characterPosition.y += delta;
                characterPosition.x += delta2;
            }
        }
    }
}

glm::vec2 getTextSizeInsideBox(glm::vec2 boxSize, const f32 scale, const char* text)
{
    glm::vec2 textSize = {0.0f, 0.0f};
    glm::vec2 characterPosition = {0.0f, 0.0f};
    characterPosition.y -= 1.25f * scale * FONT_HEIGHT / (f32)FONT_TEX_HEIGHT;
    if (characterPosition.y >= -boxSize.y)
    {
        textSize.y = -characterPosition.y;
        for (u32 i = 0; i < strlen(text); i++)
        {
            if (text[i] == ' ')
            {
                characterPosition.x += scale * 0.05f;
                if (characterPosition.x > textSize.x)
                    textSize.x = characterPosition.x;
            }
            else if (text[i] == '\n')
            {
                characterPosition.x = 0.0f;
                characterPosition.y -= 1.5f * scale * FONT_HEIGHT / (f32)FONT_TEX_HEIGHT;
                if (characterPosition.y < -boxSize.y)
                    break;
                textSize.y = -characterPosition.y;
            }
            else if (text[i] > ' ')
            {
                f32 delta = (rData->chars[text[i] - ' '].y1 - rData->chars[text[i] - ' '].y0 + rData->chars[text[i] - ' '].yoff * 2) / (f32)FONT_TEX_HEIGHT * scale;
                f32 delta2 = (rData->chars[text[i] - ' '].x1 - rData->chars[text[i] - ' '].x0) / (FONT_TEX_WIDTH * 0.85f) * scale;
                characterPosition.x += delta2;
                if (characterPosition.x > textSize.x)
                    textSize.x = characterPosition.x;
                if (characterPosition.x >= boxSize.x)
                {
                    characterPosition.x = 0.0f + delta2;
                    characterPosition.y -= 1.5f * scale * FONT_HEIGHT / (f32)FONT_TEX_HEIGHT;
                    if (characterPosition.y < -boxSize.y)
                        break;
                    textSize.y = -characterPosition.y;
                }
                characterPosition.x += delta2;
                if (characterPosition.x > textSize.x)
                    textSize.x = characterPosition.x;
            }
        }
    }
    return textSize + 0.0001f;
}

void flushUI()
{
    const i64 size = (u8*)rData->quadBufferUIPtr - (u8*)rData->quadBufferUI;
    glBindBuffer(GL_ARRAY_BUFFER, rData->quadVertexBuffer);
    glBufferSubData(GL_ARRAY_BUFFER, 0, size, rData->quadBufferUI);
    glUseProgram(rData->quadShaderPtr);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    for (u32 i = 0; i < rData->texSlotUiIndex ; i++)
        glBindTextureUnit(i, rData->texSlotsUi[i]);

    glBindVertexArray(rData->quadVertexArray);
    glDrawElements(GL_TRIANGLES, rData->indexCountUI, GL_UNSIGNED_INT, nullptr);

    rData->indexCountUI = 0;
    rData->texSlotUiIndex = 0;
    rData->quadBufferUIPtr = rData->quadBufferUI;
}

void drawUITex(glm::vec2 position, glm::vec2 size, const u32 texId)
{
    const f32 aspectRatio = (f32)rData->resolutionX / (f32)rData->resolutionY;

    const f32 screenWorldSizeY = SCREEN_SIZE_WORLD_COORDS;
    const f32 screenWorldSizeX = screenWorldSizeY * aspectRatio;

    if (rData->indexCountUI >= MAX_INDEX_COUNT)
    {
        flushUI();
        rData->quadBufferUIPtr = rData->quadBufferUI;
    }

    f32 texIndex = -1.0f;

    for (u32 i = 0; i < rData->texSlotUiIndex; i++)
    {
        if (rData->texSlotsUi[i] == texId)
        {
            texIndex = (f32)i;
            break;
        }
    }

    if (texIndex == -1.0f)
    {
        if (rData->texSlotUiIndex == 32)
        {
            flushUI();
            rData->quadBufferUIPtr = rData->quadBufferUI;
        }

        rData->texSlotsUi[rData->texSlotUiIndex] = texId;
        texIndex = (f32)rData->texSlotUiIndex;
        rData->texSlotUiIndex++;

    }

    glm::vec2 pos1 = glm::vec2{ -size.x, -size.y} / 2.0f;
    glm::vec2 pos2 = glm::vec2{  size.x, -size.y} / 2.0f;
    glm::vec2 pos3 = glm::vec2{  size.x,  size.y} / 2.0f;
    glm::vec2 pos4 = glm::vec2{ -size.x,  size.y} / 2.0f;

    pos1 += glm::vec2{position.x, position.y};
    pos2 += glm::vec2{position.x, position.y};
    pos3 += glm::vec2{position.x, position.y};
    pos4 += glm::vec2{position.x, position.y};

    worldPosToOpenGLPos(pos1.x, pos1.y);
    worldPosToOpenGLPos(pos2.x, pos2.y);
    worldPosToOpenGLPos(pos3.x, pos3.y);
    worldPosToOpenGLPos(pos4.x, pos4.y);

    rData->quadBufferUIPtr->position = { pos1, 0.0f };
    rData->quadBufferUIPtr->color= {1.0f, 1.0f, 1.0f, 1.0f};
    rData->quadBufferUIPtr->texture = {0.0f, 0.0f};
    rData->quadBufferUIPtr->texIndex = texIndex;
    rData->quadBufferUIPtr++;

    rData->quadBufferUIPtr->position = { pos2, 0.0f };
    rData->quadBufferUIPtr->color= {1.0f, 1.0f, 1.0f, 1.0f};
    rData->quadBufferUIPtr->texture = { 1.0f, 0.0f };
    rData->quadBufferUIPtr->texIndex = texIndex;
    rData->quadBufferUIPtr++;

    rData->quadBufferUIPtr->position = { pos3, 0.0f };
    rData->quadBufferUIPtr->color= {1.0f, 1.0f, 1.0f, 1.0f};
    rData->quadBufferUIPtr->texture = { 1.0f, 1.0f };
    rData->quadBufferUIPtr->texIndex = texIndex;

    rData->quadBufferUIPtr++;

    rData->quadBufferUIPtr->position = { pos4, 0.0f };
    rData->quadBufferUIPtr->color= {1.0f, 1.0f, 1.0f, 1.0f};
    rData->quadBufferUIPtr->texture = { 0.0f, 1.0f };
    rData->quadBufferUIPtr->texIndex = texIndex;

    rData->quadBufferUIPtr++;

    rData->indexCountUI += 6;
}
u32 loadShader(const char* vertexFilePath, const char* fragmentFilePath)
{

    u64 vsSourceSize = kamskiPlatformGetFileSize(vertexFilePath);
    char* vsSource = (char*)temporaryAlloc(vsSourceSize);
    readWholeFile(vsSource, vsSourceSize, vertexFilePath);

    u64 fsSourceSize = kamskiPlatformGetFileSize(fragmentFilePath);
    char* fsSource = (char*)temporaryAlloc(fsSourceSize);
    readWholeFile(fsSource, fsSourceSize, fragmentFilePath);

    const u32 shader = glCreateShader(GL_VERTEX_SHADER);
    const i32 shaderSize = (i32)vsSourceSize;
    glShaderSource(shader, 1, &vsSource, &shaderSize);
    glCompileShader(shader);

    i32 compileSuccess;

    glGetShaderiv(shader, GL_COMPILE_STATUS, &compileSuccess);
    if (!compileSuccess)
    {
        char compileStatus[512];
        glGetShaderInfoLog(shader, 512, nullptr, compileStatus);
        logError("Vertex Compile Error [%s]: %s",vertexFilePath, compileStatus);
    }

    const u32 fShader = glCreateShader(GL_FRAGMENT_SHADER);
    const i32 fShaderSize = (i32)fsSourceSize;
    glShaderSource(fShader, 1, &fsSource, &fShaderSize);
    glCompileShader(fShader);

    glGetShaderiv(fShader, GL_COMPILE_STATUS, &compileSuccess);
    if (!compileSuccess)
    {
        char compileStatus[512];
        glGetShaderInfoLog(fShader, 512, nullptr, compileStatus);
        logError("Fragment Compile Error [%s]: %s", fragmentFilePath, compileStatus);
    }

    u32 retval = glCreateProgram();
    glAttachShader(retval, shader);
    glAttachShader(retval, fShader);
    glLinkProgram(retval);
    glGetProgramiv(retval, GL_LINK_STATUS, &compileSuccess);
    glDeleteShader(shader);
    glDeleteShader(fShader);
    if (!compileSuccess)
    {
        char compileStatus[512];
        glGetProgramInfoLog(retval, 512, nullptr, compileStatus);
        logError("Program Link Error [%s, %s]: %s", vertexFilePath, fragmentFilePath, compileStatus);
    }
    return retval;
}

void initRenderer(HWND window)
{
    const HWND dummyWindow = CreateWindow("KamskiWindowClass", "DUMMY", WS_OVERLAPPEDWINDOW, 0, 0, 1, 1, NULL, NULL, GetModuleHandle(0), NULL);

    if (!dummyWindow) {
        logError("Could not create window!");
        ExitProcess(1);
    }

    const HDC dummyContext = GetDC(dummyWindow);

    PIXELFORMATDESCRIPTOR pfd = { 0 };
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
    pfd.iPixelType = PFD_TYPE_RGBA,
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;

    i32 pixelFormat = ChoosePixelFormat(dummyContext, &pfd);
    SetPixelFormat(dummyContext, pixelFormat, &pfd);
    const HGLRC dummyGL = wglCreateContext(dummyContext);
    wglMakeCurrent(dummyContext, dummyGL);

    LOAD_WGL_EXT_FUNC(wglChoosePixelFormatARB);
    LOAD_WGL_EXT_FUNC(wglCreateContextAttribsARB);
    LOAD_WGL_EXT_FUNC(wglSwapIntervalEXT);

    if (!wglChoosePixelFormatARB || !wglCreateContextAttribsARB) {
        logError("Could not load wgl ARB functions!");
        ExitProcess(EXIT_FAILURE);
    }

    wglMakeCurrent(nullptr, nullptr);
    wglDeleteContext(dummyGL);
    DestroyWindow(dummyWindow);

    const HDC deviceContext = GetDC(window);

    constexpr i32 pixelAttribList[] = {
        WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
        WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
        WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
        WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
        WGL_COLOR_BITS_ARB, 32,
        WGL_DEPTH_BITS_ARB, 24,
        WGL_STENCIL_BITS_ARB, 8,
        WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
        WGL_SAMPLE_BUFFERS_ARB, GL_TRUE,
        WGL_SAMPLES_ARB, 4,
        0
    };

    UINT numFormats;
    const BOOL success = wglChoosePixelFormatARB(deviceContext, pixelAttribList, nullptr, 1, &pixelFormat, &numFormats);

    if (!success || numFormats == 0) {
        logError("Could not load ARB pixel format!");
        ExitProcess(EXIT_FAILURE);
    }

    DescribePixelFormat(deviceContext, pixelFormat, sizeof(pfd), &pfd);
    SetPixelFormat(deviceContext, pixelFormat, &pfd);

    constexpr i32 contextAttribList[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
        WGL_CONTEXT_MINOR_VERSION_ARB, 5,
        WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        0
    };

    const HGLRC gl = wglCreateContextAttribsARB(deviceContext, nullptr, contextAttribList);

    if (!gl) {
        logError("Could not load ARB GL context!");
        ExitProcess(EXIT_FAILURE);
    }

    wglMakeCurrent(deviceContext, gl);

    // loading opengl functions
    if (!sogl_loadOpenGL()) {
        const char** failures = sogl_getFailures();
        while (*failures) {
            char debugMessage[256];
            snprintf(debugMessage, 256, "SOGL WIN32 EXAMPLE: Failed to load function %s\n", *failures);
            logError(debugMessage);
            failures++;
        }
    }

    glCreateTextures(GL_TEXTURE_2D, 1, &rData->lightTexture);
    glBindTexture(GL_TEXTURE_2D, rData->lightTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, rData->resolutionX, rData->resolutionY, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);


    glCreateFramebuffers(1, &rData->lightFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, rData->lightFramebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rData->lightTexture, 0);

    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    glCreateTextures(GL_TEXTURE_2D, 1, &rData->albedoTexture);
    glBindTexture(GL_TEXTURE_2D, rData->albedoTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, rData->resolutionX, rData->resolutionY, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D, 0);


    glCreateFramebuffers(1, &rData->albedoFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, rData->albedoFramebuffer);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rData->albedoTexture, 0);

    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    rData->quadShaderPtr = loadShader("shaders/vertex.shader", "shaders/fragment.shader");
    rData->lightShader = loadShader("shaders/lightingVertex.shader", "shaders/lightingFragment.shader");
    rData->mergeShader = loadShader("shaders/mergeVertex.shader", "shaders/mergeFragment.shader");

    // Enable texture overlapping
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    u32* indices = (u32*)globalAlloc(MAX_INDEX_COUNT * sizeof(u32));
    u32 offset = 0;

    for (i32 i = 0; i < MAX_INDEX_COUNT; i+=6)
    {
        indices[i] = 0 + offset;
        indices[i+1] = 1 + offset;
        indices[i+2] = 2 + offset;

        indices[i+3] = 2 + offset;
        indices[i+4] = 3 + offset;
        indices[i+5] = 0 + offset;

        offset += 4;
    }

    glCreateBuffers(1, &rData->quadVertexIndicesBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rData->quadVertexIndicesBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, MAX_INDEX_COUNT * sizeof(u32), indices, GL_STATIC_DRAW);
    // Merge vertex array

    glCreateVertexArrays(1, &rData->mergeVertexArray);
    glBindVertexArray(rData->mergeVertexArray);

    //Albedo vertex array

    glCreateVertexArrays(1, &rData->quadVertexArray);
    glBindVertexArray(rData->quadVertexArray);

    glCreateBuffers(1, &rData->quadVertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, rData->quadVertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, MAX_VERTEX_COUNT * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexArrayAttrib(rData->quadVertexArray, 0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex,position));

    glEnableVertexArrayAttrib(rData->quadVertexArray, 1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, color));

    glEnableVertexArrayAttrib(rData->quadVertexArray, 2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, texture));

    glEnableVertexArrayAttrib(rData->quadVertexArray, 3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, texIndex));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    // Lightmap vertex array

    glCreateVertexArrays(1, &rData->lightVertexArray);
    glBindVertexArray(rData->lightVertexArray);

    glCreateBuffers(1, &rData->lightVertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, rData->lightVertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, MAX_VERTEX_COUNT * 24 * sizeof(LightVertex), nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexArrayAttrib(rData->lightVertexArray, 0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(LightVertex), (const void*)offsetof(LightVertex, position));

    glEnableVertexArrayAttrib(rData->lightVertexArray, 1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(LightVertex), (const void*)offsetof(LightVertex, color));

    glEnableVertexArrayAttrib(rData->lightVertexArray, 2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(LightVertex), (const void*)offsetof(LightVertex, range));

    glEnableVertexArrayAttrib(rData->lightVertexArray, 3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(LightVertex), (const void*)offsetof(LightVertex, distance));
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Enable texture overlapping
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    i32 samplers[32];
    for (i32 i = 0; i < 32; i++) {
        samplers[i] = i;
    }

    glUseProgram(rData->quadShaderPtr);
    glUniform1iv(glGetUniformLocation(rData->quadShaderPtr, "textures"), 32, samplers);
    glUseProgram(rData->mergeShader);
    u32 loc = glGetUniformLocation(rData->mergeShader, "framebuffers");
    glUniform1iv(loc, 2, samplers);

    for (auto& textureSlot : rData->texSlots) {
        textureSlot = 0;
    }

    setBlurWholeScreen(false);

    rData->deviceContext = deviceContext;
    rData->quadBufferUIPtr = rData->quadBufferUI;
//    wglSwapIntervalEXT(0);

    loadFont("fonts\\CompassPro.ttf", rData->fontTexId, rData->chars);
    globalFree(indices);
    rendererInitialized = true;
}

// TIME

f64 gameTime = 0.0;

f64 getGameTime()
{
    return gameTime;
}

void stepTime(f64 dt)
{
    gameTime += dt;
}

// UI

void uiAnchoredPosConverter(glm::vec2& truePosition, glm::vec2 position, glm::vec2 size, AnchorPoint anchorPoint)
{
    glm::vec2 screenWorldSize = getScreenSize()/2.0f;
    switch (anchorPoint)
    {
        case AnchorPoint::N:
        {
            truePosition = glm::vec2{position.x , position.y + screenWorldSize.y - size.y/2};
        }break;

        case AnchorPoint::NW:
        {
            truePosition = glm::vec2{position.x - screenWorldSize.x  + size.x/2, position.y + screenWorldSize.y  - size.y/2};
        }break;

        case AnchorPoint::NE:
        {
            truePosition = glm::vec2{position.x + screenWorldSize.x  - size.x/2, position.y + screenWorldSize.y  - size.y/2};
        }break;

        case AnchorPoint::W:
        {
            truePosition = glm::vec2{position.x - screenWorldSize.x  + size.x/2, position.y};
        }break;

        case AnchorPoint::E:
        {
            truePosition = glm::vec2{position.x + screenWorldSize.x  - size.x/2, position.y};
        }break;

        case AnchorPoint::SW:
        {
            truePosition = glm::vec2{position.x - screenWorldSize.x  + size.x/2, position.y - screenWorldSize.y  + size.y/2};
        }break;

        case AnchorPoint::S:
        {
            truePosition = glm::vec2{position.x, position.y - screenWorldSize.y  + size.y/2};
        }break;

        case AnchorPoint::SE:
        {
            truePosition = glm::vec2{position.x + screenWorldSize.x - size.x/2, position.y - screenWorldSize.y  + size.y/2};
        }break;

        default:
        truePosition = glm::vec2{position.x, position.y};
    }
}

glm::vec2 uiPlainColor(AnchorPoint anchorPoint, glm::vec2 position, glm::vec2 size, const glm::vec4& color, f32 rotation)
{
    glm::vec2 truePosition = {0.0f, 0.0f};
    uiAnchoredPosConverter(truePosition, position, size, anchorPoint);
    drawQuadUI(truePosition, size, color, rotation);
    return truePosition;
}

glm::vec2 uiText(AnchorPoint anchorPoint, glm::vec2 position, const f32 fontSize, const char* text)
{
    glm::vec2 truePosition = {0.0f, 0.0f};
    uiAnchoredPosConverter(truePosition, glm::vec2{position.x, position.y}, glm::vec2{0.0f, fontSize/4.5f}, anchorPoint);
    drawTextUI(truePosition, fontSize, text);
    return truePosition;
}

glm::vec2 uiTexture(glm::vec2 position, glm::vec2 size, AnchorPoint anchorPoint, const u32 texId)
{
    glm::vec2 truePosition = {0.0f, 0.0f};
    uiAnchoredPosConverter(truePosition, glm::vec2{position.x, position.y}, size, anchorPoint);
    drawUITex(truePosition, size, texId);
    return truePosition;
}

bool uiButtonHover(glm::vec2 position, glm::vec2 size, AnchorPoint anchorPoint, const u32 texId, const u32 texIdHover)
{
    glm::vec2 truePosition = {0.0f, 0.0f};
    uiAnchoredPosConverter(truePosition, glm::vec2{position.x, position.y}, size, anchorPoint);
    glm::vec2 mousePos;
    getMousePosition(mousePos.x, mousePos.y);
    if (mousePos.x >= truePosition.x - size.x/2 && mousePos.x <= truePosition.x + size.x/2 && mousePos.y >= truePosition.y - size.y/2 && mousePos.y <= truePosition.y + size.y/2)
    {
        drawUITex(position, size, texIdHover);
        //if(getMouseButtonState(MouseButtonCode::LEFT_CLICK) == KeyState::PRESS)
        return true;
    }
    else
        drawUITex(position, size, texId);

    return false;
}

bool uiButtonHoverText(glm::vec2 position, glm::vec2 size, AnchorPoint anchorPoint, const u32 texId, const u32 texIdHover, const f32 fontSize, const char* text)
{
    glm::vec2 truePosition = {0.0f, 0.0f};
    uiAnchoredPosConverter(truePosition, glm::vec2{position.x, position.y}, size, anchorPoint);
    glm::vec2 mousePos;
    getMousePosition(mousePos.x, mousePos.y);
    if (mousePos.x >= truePosition.x - size.x/2 && mousePos.x <= truePosition.x + size.x/2 && mousePos.y >= truePosition.y - size.y/2 && mousePos.y <= truePosition.y + size.y/2)
    {
        truePosition.y -= size.y*0.085f;
        drawUITex(truePosition, size, texIdHover);
        glm::vec2 textSize = getTextSizeInsideBox(size, fontSize, text);
        drawTextInsideBoxUI(glm::vec2{truePosition.x - textSize.x/2, truePosition.y + textSize.y/2}, glm::vec2{truePosition.x + textSize.x/2, truePosition.y - textSize.y/2}, fontSize, text, 0.0f);

        if (getMouseButtonState(MouseButtonCode::LEFT_CLICK) == KeyState::PRESS)
            return true;
    }
    else
    {
        drawUITex(truePosition, size, texId);
        glm::vec2 textSize = getTextSizeInsideBox(size, fontSize, text);
        drawTextInsideBoxUI(glm::vec2{truePosition.x - textSize.x/2, truePosition.y + textSize.y/2}, glm::vec2{truePosition.x + textSize.x/2, truePosition.y - textSize.y/2}, fontSize, text, 0.0f);
    }

    return false;
}

bool uiButton(glm::vec2 position, glm::vec2 size, AnchorPoint anchorPoint, const u32 texId)
{
    return uiButtonHover(position, size, anchorPoint, texId, texId);
}

bool uiButtonText(glm::vec2 position, glm::vec2 size, AnchorPoint anchorPoint, const u32 texId, const f32 fontSize, const char* text)
{
    return uiButtonHoverText(position, size, anchorPoint, texId, texId, fontSize, text);
}
