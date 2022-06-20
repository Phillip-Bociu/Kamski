#include "Game.cpp"

extern "C"
__declspec(dllexport)
void loadEngine(const API& api)
{
    /* INIT ENGINE SUBSYSTEMS */
    GAME = (Game*)api.gameState;
    ENGINE = api;
}

void Game::gameInit()
{
    logDebug("%f %f", buttonSize.x, buttonSize.y);
    for (u32 i = 0; i < TEXTURE_COUNT; ++i)
    {
        logDebug("%s", TEXTURE_PATHS[i]);
        TextureId textureId = ENGINE.loadTexture(TEXTURE_PATHS[i]);
        logDebug("%u", textureId);
        linkTextureIdByTag(textureId, (TextureTag)i);
    }
    for (u32 i = 0; i < ANIMATION_TAG_COUNT; ++i)
    {
        linkAnimationByTag((AnimationTag)i);
    }
    GAME->gameState = MAIN_MENU;
    logInfo("[+] Init - done!");
}

void Game::gameInput()
{
    // set action keys
    actionState.fastRestart = ENGINE.getKeyState('O');
    actionState.startGame = ENGINE.getKeyState('R');
    actionState.walkUp = ENGINE.getKeyState('W');
    actionState.walkLeft = ENGINE.getKeyState('A');
    actionState.walkDown = ENGINE.getKeyState('S');
    actionState.walkRight = ENGINE.getKeyState('D');
    actionState.zoomIn = ENGINE.getKeyState('Q');
    actionState.zoomOut = ENGINE.getKeyState('E');
    actionState.pauseGame = ENGINE.getKeyState('P');
    actionState.attack = ENGINE.getSpecialKeyState(SpecialKeyCode::SPACE);
    actionState.restart = ENGINE.getSpecialKeyState(SpecialKeyCode::RETURN);
    actionState.menuInteract = ENGINE.getMouseButtonState(MouseButtonCode::LEFT_CLICK);
    actionState.hp = ENGINE.getKeyState('1');
    actionState.stam = ENGINE.getKeyState('2');
    actionState.mana= ENGINE.getKeyState('3');
    if (ENGINE.getSpecialKeyState(SpecialKeyCode::CTRL) == KeyState::PRESS)
    {
        playerToggleVroom();
    }
    // set cursor position
    ENGINE.getMousePosition(cursorPosition.x, cursorPosition.y);
}

void Game::gameUpdate()
{
    switch (gameState)
    {
        case MAIN_MENU:
        ENGINE.setBlurWholeScreen(false);
        camera.z = 1.0f;
        if (menuButton({0.0f, -ENGINE.getScreenSize().y / 5.0f}, "EXIT"))
        {
            ENGINE.exit(EXIT_SUCCESS);
        }
        //else if (menuButton({0.0f, 0.0f}, "SETTINGS"))
        //{
            //gameState = SETTINGS;
        //}
        else if (menuButton({0.0f, +ENGINE.getScreenSize().y / 5.0f}, "PLAY"))
        {
            gameState = GAME_START;
        }
        break;
        
        case SETTINGS:
        if (menuButton({0.0f, 0.0f}, "GO BACK"))
        {
            gameState = MAIN_MENU;
        }
        break;
        
        case GAME_PAUSED:
        ENGINE.setBlurWholeScreen(true);
        deltaTime = 0;
        if (actionState.pauseGame == KeyState::PRESS)
        {
            gameState = GAME_RUNNING;
            ENGINE.setBlurWholeScreen(false);
        }
        break;
        
        [[unlikely]]
        case GAME_START:
        {
            ENGINE.setBlurWholeScreen(false);
            memset((char*)this + offsetof(Game, disposableMemory),
                   0,
                   sizeof(Game) - offsetof(Game, disposableMemory));
            startGame();
            break;
            
            [[likely]]
                case GAME_RUNNING:
            
            if(actionState.fastRestart == KeyState::PRESS)
            {
                gameState = GAME_START;
                break;
            }
            if (actionState.pauseGame == KeyState::PRESS)
            {
                gameState = GAME_PAUSED;
                break;
            }
            
            if (actionState.restart == KeyState::PRESS)
            {
                ENGINE.globalFree(map.tiles);
                gameState = GAME_LOST;
                break;
            }
            
            ENGINE.simulateParticles(deltaTime);
            
            TransformComponent& tr = entityRegistry.getComponent<TransformComponent>(playerEId);
            glm::uvec2 tileCheck = getTileByPosition(tr.position);
            logError("PlayerPos: %u %u", tileCheck.x, tileCheck.y);
            
            EntityComponent& ent = entityRegistry.getComponent<EntityComponent>(playerEId);
            if(actionState.hp == KeyState::PRESS)
            {
                ent.healthPoints += 20;
            }
            updateFollowers();
            
            itemPickupSystem();
            
            velocitySystem();
            handleCombatPhases();
            updateEnemies();
            updateHealthBars();
            moveProjectiles();
            updatePlayer();
            break;
        }
        [[unlikely]]
            case GAME_LOST:
        ENGINE.setBlurWholeScreen(true);
        if (menuButton({0.0f, -ENGINE.getScreenSize().y / 5.0f}, "EXIT"))
        {
            ENGINE.exit(EXIT_SUCCESS);
        }
        else if (menuButton({0.0f, 0.0f}, "GO TO MENU"))
        {
            gameState = MAIN_MENU;
        }
        else if (menuButton({0.0f, +ENGINE.getScreenSize().y / 5.0f}, "PLAY AGAIN"))
        {
            gameState = GAME_START;
        }
        break;
    }

}

void Game::gameRender()
{
    ENGINE.beginBatch(camera);
    switch (gameState) {
        case GAME_PAUSED:
        renderItems();
        case GAME_RUNNING:
        case GAME_LOST:
        renderMap();
        renderSprites();
        ENGINE.drawParticles();
        break;
        case MAIN_MENU:
        case SETTINGS:
        renderMenuBackground();
        break;
    }
    renderCursor();
    ENGINE.endBatch();
    u32 count = map.navMesh.polygonCount;
    static bool showTriangles = false;
    
    if(actionState.stam == KeyState::PRESS)
        showTriangles = !showTriangles;
    
    if(showTriangles)
    {
        for(u32 i = 0; i < map.navMesh.polygonCount; i++)
        {
            ENGINE.beginTriangleFan(camera);
            u32 vertexCount = map.navMesh.polygons[i].vertexCount;
            for(u32 vInd = 0; vInd != map.navMesh.polygons[i].vertexCount; vInd++)
            {
                ENGINE.addFanVertex(map.navMesh.polygons[i].vertices[vInd], i);
            }
            ENGINE.endTriangleFan();
        }
    }
    ENGINE.swapClear();
}
