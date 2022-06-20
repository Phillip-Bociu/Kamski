#define WIN32_LEAN_AND_MEAN
#include "deps/stb_image.cpp"
#include "deps/stb_truetype.cpp"
#include "KamskiEngine_win32.cpp"
#include "KamskiEngine.cpp"

typedef void GameLoadEngineFunc(API& api);
typedef void GameInitFunc();
typedef void PlayerInputFunc();
typedef void GameUpdateFunc(f64& dt);
typedef void GameRenderFunc(f64 dt);

struct Game
{
    GameLoadEngineFunc* load;
    GameInitFunc* init;
    PlayerInputFunc* input;
    GameUpdateFunc* update;
    GameRenderFunc* render;
};

LRESULT CALLBACK winProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_SIZE:
        {
            if (rendererInitialized)
            {
                RECT res = {};
                GetClientRect(window, &res);
                const u32 x = res.right;
                const u32 y = res.bottom;
                logDebug("x:%u y:%u", x, y);
                resizeViewport(x,y);
            }
        }
        break;

        case WM_ACTIVATEAPP:
        {
            if (wParam)
            {
                SetLayeredWindowAttributes(window, RGB(0, 0, 0), 255, LWA_ALPHA);
            } else
            {
                SetLayeredWindowAttributes(window, RGB(0, 0, 0), 64, LWA_ALPHA);
            }
        }
        break;

        case WM_CLOSE:
        {
            PostQuitMessage(0);
            return 0;
        }
        break;
    }

    return DefWindowProc(window, message, wParam, lParam);
}

void loadGameCode(const char* dllPath, HMODULE& gameLib, Game& gameFuncs)
{
    if (gameLib)
    {
        FreeLibrary(gameLib);
    }

    logDebug("Loading Game Code");

    CopyFile(dllPath, "GameFuncs.dll", FALSE);
    gameLib = LoadLibrary("GameFuncs.dll");

    gameFuncs.load = (GameLoadEngineFunc*)GetProcAddress(gameLib, "loadEngine");
    gameFuncs.init = (GameInitFunc*)GetProcAddress(gameLib, "gameInit");
    gameFuncs.input = (PlayerInputFunc*)GetProcAddress(gameLib, "gameInput");
    gameFuncs.update = (GameUpdateFunc*)GetProcAddress(gameLib, "gameUpdate");
    gameFuncs.render = (GameRenderFunc*)GetProcAddress(gameLib, "gameRender");
}

struct RecordingState
{
    HANDLE fileHandle;
    enum Option
    {
        NONE,
        RECORD,
        PLAYBACK
    }option;
};

void startRecording(RecordingState& recordingState)
{
    assert(recordingState.option == RecordingState::NONE);
    recordingState.fileHandle = CreateFileA(KAMSKI_PLAYBACK_FILENAME, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    recordingState.option = RecordingState::RECORD;
}

void endRecording(RecordingState& recordingState)
{
    CloseHandle(recordingState.fileHandle);
    recordingState.option = RecordingState::NONE;
}

void startPlayback(RecordingState& recordingState)
{
    assert(recordingState.option == RecordingState::NONE);
    recordingState.fileHandle = CreateFileA(KAMSKI_PLAYBACK_FILENAME, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    recordingState.option = RecordingState::PLAYBACK;
}

void endPlayback(RecordingState& recordingState)
{
    endRecording(recordingState);
}

void recordInput(RecordingState& recordingState, const PlayerInputInternal& input)
{
    DWORD bytesWritten;
    WriteFile(recordingState.fileHandle, &input, sizeof(input), &bytesWritten, 0);
}

bool playbackInput(RecordingState& recordingState, PlayerInputInternal& input)
{
    bool retval = false;
    assert(recordingState.option == RecordingState::PLAYBACK);
    DWORD bytesRead;
    if (ReadFile(recordingState.fileHandle, &input, sizeof(input), &bytesRead, 0))
    {
        if (bytesRead == 0)
        {
            endPlayback(recordingState);
            startPlayback(recordingState);
            retval = true;
        }
    }
    return retval;
}


void initAPI(API& api)
{
    api = {};

    api.kamskiLog = kamskiLog;
    api.beginBatch = beginBatch;
    api.endBatch = endBatch;
    api.swapClear = swapClear;
    api.drawTexturedQuad = drawTexturedQuad ;
    api.drawColoredQuad = drawColoredQuad ;
    api.drawQuad = drawQuad;
    api.drawCharacter = drawCharacter;
    api.drawText = drawText;
    api.drawTextInsideBox = drawTextInsideBox;
    api.addLight = addLight;
    api.addLightBlocker = addLightBlocker;
    api.loadTexture = loadTexture;
    api.getScreenSize = getScreenSize;
    api.temporaryAlignedAlloc = temporaryAlloc;
    api.temporaryAlloc = temporaryAlloc;
    api.getPermanentMemory = getPermanentMemory;
    api.globalAlloc = globalAlloc;
    api.globalAlignedAlloc = globalAlignedAlloc;
    api.globalFree = globalFree;
    api.allocArena = allocArena;
    api.freeArena = freeArena;
    api.printGlobalAllocations = printGlobalAllocations;
    api.readWholeFile = readWholeFile;
    api.writeFile = writeFile;
    api.getFileSize = getFileSize;
    api.getSpecialKeyState = getKeyState;
    api.getKeyState = getKeyState;
    api.getMouseButtonState = getMouseButtonState;
    api.getMousePosition = getMousePosition;
    api.setSpecialKeyState = setKeyState;
    api.setKeyState = setKeyState;
    api.setMouseButtonState = setMouseButtonState;
    api.createAnimation = createAnimation;
    api.getAnimationFrame = getAnimationFrame;
    api.randomU64 = randomU64;
    api.randomRangeU64 = randomU64;
    api.randomF64 = randomF64;
    api.randomRangeF64 = randomF64;
    api.randomF32 = randomF32;
    api.randomRangeF32 = randomF32;
    api.localStateRandomU64 = localStateRandomU64;
    api.localStateRandomF64 = localStateRandomF64;
    api.localStateRandomRangeF64 = localStateRandomF64;
    api.localStateRandomF32 = localStateRandomF32;
    api.localStateRandomRangeF32 = localStateRandomF32;
    api.emitParticles = emitParticles;
    api.drawParticles = drawParticles;
    api.beginTriangleFan = beginTriangleFan;
    api.addFanVertex = addFanVertex;
    api.endTriangleFan = endTriangleFan; 
    api.simulateParticles = simulateParticles;
    api.drawTextInsideBoxUI = drawTextInsideBoxUI;
    api.drawTextUI = drawTextUI;
    api.drawQuadUI = drawQuadUI;
    api.drawUITex = drawUITex;
    api.uiPlainColor = uiPlainColor;
    api.uiTexture = uiTexture;
    api.uiText = uiText;
    api.uiButtonHover = uiButtonHover;
    api.uiButtonHoverText = uiButtonHoverText;
    api.uiButtonText = uiButtonText;
    api.uiButton = uiButton;
    api.exit = exit;
    api.setBlurWholeScreen = setBlurWholeScreen;
    api.getGameTime = getGameTime;
#ifdef  KAMSKI_DEBUG

    void** functionPtrChecker = (void**)&api.kamskiLog;
    void** functionPtrCheckerEnd = (void**)&api.uiButton + 1;
    for (;functionPtrChecker != functionPtrCheckerEnd; functionPtrChecker++)
    {
        if (*functionPtrChecker == nullptr)
        {
            logError("The %dnd function ptr", functionPtrChecker - (void**)&api.kamskiLog);
            Sleep(12000);
            ExitProcess(1);
        }
    }
#endif

}

struct PermanentEngineMemoryPartition
{
    Win32State win32State;
    RendererData rendererMemory;
    PlayerInputInternal playerInputMemory;
    ParticleInternal particleMemory;
    GameAnimationInternal animationMemory;
};

int CALLBACK WinMain(HINSTANCE instance,
                     HINSTANCE prevInstance,
                     LPSTR cmdLine,
                     int showWindow)
{
    // Log Test
#ifdef KAMSKI_DEBUG
    AllocConsole();
#endif

    memorySystemState.permanentEngineMemorySize = sizeof(PermanentEngineMemoryPartition) + KAMSKI_ANIMATION_ARENA_SIZE;

    memorySystemState.permanentMemorySize = MB(64);
    memorySystemState.transientMemorySize = GB(2);
    memorySystemState.totalSize = memorySystemState.permanentEngineMemorySize + memorySystemState.permanentMemorySize + memorySystemState.transientMemorySize;

    memorySystemState.permanentEngineMemory = VirtualAlloc(KAMSKI_BASE_ADDRESS,
                                                           memorySystemState.permanentEngineMemorySize + memorySystemState.permanentMemorySize + memorySystemState.transientMemorySize,
                                                           MEM_COMMIT | MEM_RESERVE,
                                                           PAGE_READWRITE);

    if (KAMSKI_BASE_ADDRESS && memorySystemState.permanentEngineMemory != KAMSKI_BASE_ADDRESS)
    {
        logError("Base address differs!");
    }

    PermanentEngineMemoryPartition* partition = (PermanentEngineMemoryPartition*)memorySystemState.permanentEngineMemory;

    rData = &partition->rendererMemory;
    playerInputSystemState = &partition->playerInputMemory;
    animationSystemState = &partition->animationMemory;
    particleSystemState = &partition->particleMemory;
    particleSystemState->randomState = kamskiPlatformGetTime() * 1000000.0;
    win32State = &partition->win32State;

    memorySystemState.permanentMemory = (u8*)memorySystemState.permanentEngineMemory + memorySystemState.permanentEngineMemorySize;
    memorySystemState.transientMemory = ((u8*)memorySystemState.permanentMemory) + memorySystemState.permanentMemorySize;
    memorySystemState.tempAlloc = new(memorySystemState.transientMemory) Arena(MB(128));
    memorySystemState.globalAlloc = new (((u8*)memorySystemState.transientMemory) + MB(128) + sizeof(Arena)) GeneralAllocator(memorySystemState.transientMemorySize - MB(128) + sizeof(Arena));

    void* gameState = memorySystemState.permanentMemory;

    API api;
    RecordingState recordingState = {};

    initAPI(api);
    api.gameState = gameState;

    animationSystemState->animationArena = Arena(KAMSKI_ANIMATION_ARENA_SIZE);
    animationSystemState->lastUnusedIdIndex = KAMSKI_MAX_ANIMATION_COUNT - 1;

    Game gameFuncs;

    HMODULE gameLib = nullptr;

    HANDLE fileHandle = CreateFileA(KAMSKI_DLL,
                                    GENERIC_READ,
                                    FILE_SHARE_READ,
                                    nullptr,
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL,
                                    nullptr);

    FILETIME lastFileTime;
    GetFileTime(fileHandle,
                nullptr,
                nullptr,
                &lastFileTime);

    CloseHandle(fileHandle);

    loadGameCode(KAMSKI_DLL, gameLib, gameFuncs);

    // Win32 window creation

    WNDCLASSEX winClass = {0};
    winClass.cbSize = sizeof(winClass);
    winClass.style = CS_HREDRAW | CS_VREDRAW;
    winClass.lpfnWndProc = winProc;
    winClass.hInstance = instance;
    winClass.hIcon = LoadIcon(instance, IDI_APPLICATION);
    winClass.hIconSm = LoadIcon(instance, IDI_APPLICATION);
    winClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    winClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    winClass.lpszClassName = "KamskiWindowClass";

    if (!RegisterClassEx(&winClass))
    {
        logError("Could not register window class\n");
        return EXIT_FAILURE;
    }

    // Legacy code from Lazy Engine, do not touch it!
    RECT res{100, 100, 100 + WINDOWS_SIZE_X, 100 + WINDOWS_SIZE_Y};
    AdjustWindowRect(&res, WS_OVERLAPPEDWINDOW, FALSE);

    // Creating actual window
    win32State->window = CreateWindowExA(KAMSKI_WINDOW_STYLE,
                                         winClass.lpszClassName,
                                         KAMSKI_WINDOW_NAME,
                                         WS_OVERLAPPEDWINDOW,
                                         CW_USEDEFAULT, CW_USEDEFAULT,
                                         res.right - res.left, res.bottom - res.top,
                                         NULL,
                                         NULL,
                                         instance,
                                         NULL);

    GetClientRect(win32State->window, &res);
    rData->resolutionX = res.right;
    rData->resolutionY = res.bottom;
    rData->aspectRatio = (f32)rData->resolutionX / (f32)rData->resolutionY;

    if (!win32State->window)
    {
        logError("Could not create window!");
        return EXIT_FAILURE;
    }

    // Making the window visible
    ShowCursor(false);
    ShowWindow(win32State->window, showWindow);
    initRenderer(win32State->window);

    gameFuncs.load(api);
    gameFuncs.init();

    bool isRunning = true;
    f64 lastFrameTime = kamskiPlatformGetTime();
    f64 thisFrameTime = 0;

    f64 frameTimes[KAMSKI_FRAME_COUNT] = {};
    u8 frameCount = 0;
    printGlobalAllocations(false);

    // game loop

    while (isRunning)
    {
        thisFrameTime = kamskiPlatformGetTime();
        f64 dt = thisFrameTime - lastFrameTime;
        lastFrameTime = thisFrameTime;

        fileHandle = CreateFileA(KAMSKI_DLL,
                                 GENERIC_READ,
                                 FILE_SHARE_READ,
                                 nullptr,
                                 OPEN_EXISTING,
                                 FILE_ATTRIBUTE_NORMAL,
                                 nullptr);

        FILETIME fileTime;
        GetFileTime(fileHandle,
                    nullptr,
                    nullptr,
                    &fileTime);
        CloseHandle(fileHandle);

        if (CompareFileTime(&fileTime, &lastFileTime) == 1)
        {
            loadGameCode(KAMSKI_DLL, gameLib, gameFuncs);
            gameFuncs.load(api);
            lastFileTime = fileTime;
        }

        // Input
        MSG msg;
        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
            switch (msg.message)
            {
                case WM_QUIT:
                {
                    ShowWindow(win32State->window, SW_HIDE);
                    isRunning = false;
                }break;

                case WM_LBUTTONUP:
                {
                    setMouseButtonState(MouseButtonCode::LEFT_CLICK, KeyState::RELEASE);
                }
                break;

                case WM_LBUTTONDOWN:
                {
                    setMouseButtonState(MouseButtonCode::LEFT_CLICK, KeyState::PRESS);
                }
                break;

                case WM_MBUTTONUP:
                {
                    setMouseButtonState(MouseButtonCode::MIDDLE_CLICK, KeyState::RELEASE);
                }
                break;

                case WM_MBUTTONDOWN:
                {
                    setMouseButtonState(MouseButtonCode::MIDDLE_CLICK, KeyState::PRESS);
                }
                break;

                case WM_RBUTTONUP:
                {
                    setMouseButtonState(MouseButtonCode::RIGHT_CLICK, KeyState::RELEASE);
                }
                break;

                case WM_RBUTTONDOWN:
                {
                    setMouseButtonState(MouseButtonCode::RIGHT_CLICK, KeyState::PRESS);
                }
                break;

                case WM_MOUSEMOVE:
                {
                    f32 x = 2.0f * (f32)LOWORD(msg.lParam) / (f32)rData->resolutionX - 1.0f;
                    f32 y = 2.0f * (f32)(rData->resolutionY - HIWORD(msg.lParam)) / (f32)rData->resolutionY - 1.0f;
                    openGLPosToWorldPos(x, y);
                    setMousePosition(x, y);
                }
                break;

                case WM_KEYDOWN:
                case WM_SYSKEYDOWN:
                case WM_KEYUP:
                case WM_SYSKEYUP:
                {

                    KeyState ks = (msg.lParam >> 31) ? KeyState::RELEASE : KeyState::PRESS;

                    if ((msg.lParam & (1 << 29)) && msg.wParam == VK_F4)
                    {
                        ShowWindow(win32State->window, SW_HIDE);
                        isRunning = false;
                        break;
                    }

#ifdef KAMSKI_DEBUG
                    else if (ks == KeyState::PRESS && msg.wParam == VK_F1)
                    {
                        if (recordingState.option == RecordingState::NONE)
                        {
                            startRecording(recordingState);
                            DWORD bytesWritten;
                            WriteFile(recordingState.fileHandle, memorySystemState.permanentEngineMemory, memorySystemState.totalSize, &bytesWritten, 0);
                            lastFrameTime = kamskiPlatformGetTime();
                        }
                        else
                        {
                            endRecording(recordingState);
                        }
                    }else if (ks == KeyState::PRESS && msg.wParam == VK_F2)
                    {
                        if (recordingState.option == RecordingState::NONE)
                        {
                            startPlayback(recordingState);
                            DWORD bytesRead;
                            RendererData* temp = new RendererData;
                            *temp = partition->rendererMemory;
                            ReadFile(recordingState.fileHandle, memorySystemState.permanentEngineMemory, memorySystemState.totalSize, &bytesRead, 0);
                            partition->rendererMemory = *temp;
                            delete temp;
                        }
                        else
                        {
                            endPlayback(recordingState);
                            for (KeyState& key : playerInputSystemState->keys)
                            {
                                key = KeyState::RELEASE;
                            }

                            for (KeyState& key : playerInputSystemState->mouseButtons)
                            {
                                key = KeyState::RELEASE;
                            }
                        }
                    }
#endif
                    if ((msg.wParam >= 'A' && msg.wParam <= 'Z') || (msg.wParam >= '0' && msg.wParam <= '9'))
                    {
                        setKeyState((char)msg.wParam, ks);
                    }
                    else switch (msg.wParam)
                    {
                        case VK_SPACE:
                        {
                            setKeyState(SpecialKeyCode::SPACE, ks);
                        }
                        break;

                        case VK_RETURN:
                        {
                            setKeyState(SpecialKeyCode::RETURN, ks);
                        }
                        break;

                        case VK_CONTROL:
                        {
                            setKeyState(SpecialKeyCode::CTRL, ks);
                        }
                        break;

                        case VK_SHIFT:
                        {
                            setKeyState(SpecialKeyCode::SHIFT, ks);
                        }
                        break;

                        case VK_MENU:
                        {
                            setKeyState(SpecialKeyCode::ALT, ks);
                        }
                        break;

                    }


                }break;

                default:
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }break;
            }


        }
#ifdef KAMSKI_DEBUG
        if (recordingState.option == RecordingState::RECORD)
        {
            recordInput(recordingState, *playerInputSystemState);
        } else if (recordingState.option == RecordingState::PLAYBACK)
        {
            if (playbackInput(recordingState, *playerInputSystemState))
            {
                DWORD bytesRead;
                RendererData* temp = new RendererData;
                *temp = partition->rendererMemory;
                ReadFile(recordingState.fileHandle, memorySystemState.permanentEngineMemory, memorySystemState.totalSize, &bytesRead, 0);
                partition->rendererMemory = *temp;
                delete temp;
            }
        }
#endif

        f64 gameDt = dt;
        // Input
        gameFuncs.input();
        // Update
        gameFuncs.update(gameDt);
        // Render
        gameFuncs.render(gameDt);

        inputPass();
        stepTime(gameDt);

        memorySystemState.tempAlloc->size = 0;
#ifdef KAMSKI_DEBUG
        if (recordingState.option != RecordingState::NONE)
        {
            f64 now = kamskiPlatformGetTime();
            f64 sleepTime = std::max(1000.0 * (1.0 / ((f64)KAMSKI_RECORDING_FRAMERATE) - (now - thisFrameTime)), 0.0);
            Sleep(sleepTime / 2.0f);
        }

        frameTimes[frameCount] = dt;
        frameCount = (frameCount + 1) % KAMSKI_FRAME_COUNT;
        f64 avg = 0;
        for (u8 i = 0; i < KAMSKI_FRAME_COUNT; i++)
        {
            avg += frameTimes[i];
        }

        avg = avg / (f64)KAMSKI_FRAME_COUNT;

        char title[64];
        sprintf(title, "frameTime:%fms, FPS:%f", avg, 1.0 / avg);
        SetWindowTextA(win32State->window, title);
#endif
    }

    return 0;
}
