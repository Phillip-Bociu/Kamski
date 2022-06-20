#include "headers/Defines.h"
#include "headers/Components.h"
#include <random>

class Game {
    public:
    
    struct Polygon
    {
        glm::vec2* vertices;
        u32 vertexCount;
    };
    
    struct Map {
        TextureTag* tiles;
        // refactor this tilesArr to be cache friendly by also storing the texture id
        // and change uvec2 to vec2 (store the real position, not the relativ position inside the tiles matrix)
        glm::uvec2* tilesArr;
        glm::uvec2 size;
        union Wall {
            struct {
                glm::vec2 leftBottom;
                glm::vec2 topRight;
            };
            glm::vec4 corners;
        }* walls;
        u32 numberOfWalls;
        glm::vec2 quadSize;
        u32 tilesArrSize;
        struct Room
        {
            glm::vec2 center;
            glm::vec2 size;
            glm::vec2 entranceLeftBottom;
            glm::vec2 entranceLeftTop;
            glm::vec2 entranceRightBottom;
            glm::vec2 entranceRightTop;
        } rooms[100];
        u32 roomCount;
        struct NavigationMesh
        {
            Polygon* polygons;
            u32 polygonCount;
        } navMesh;
    };
    
    u32 itemId;
    
    enum CombatPhase
    {
        COMBAT_PHASE_OFF,
        COMBAT_PHASE_ON
    };
    
    enum MenuPhase
    {
        MENU_PHASE_ZOOM,
        MENU_PHASE_TRANSITION
    };
    
    // MEMORY THAT SHOULDN'T BE CHANGED
    TextureId textureIdsByTag[(u32)TextureTag::COUNT];
    
    // MEMORY THAT YOU CAN RESET
    union {
        struct {
            f64 deltaTime;
            Entity playerEId;
            EntityRegistry<ComponentList<KAMSKI_COMPONENTS>> entityRegistry;
            glm::vec3 camera;
            bool isVroomOn;
            glm::vec2 startPosition;
            f32 playerRadius;
            States gameState;
            ItemSet itemSet;
            glm::vec2 buttonSize;
            f64 playerAttackTimer;
            CombatPhase combatPhase;
            f32 menuTime;
            MenuPhase menuPhase;
            u32 menuTagIndex;
            
            struct
            {
                KeyState startGame;
                KeyState walkUp;
                KeyState walkDown;
                KeyState walkLeft;
                KeyState walkRight;
                KeyState attack;
                KeyState restart;
                KeyState zoomIn;
                KeyState zoomOut;
                KeyState pauseGame;
                KeyState menuInteract;
                KeyState hp;
                KeyState stam;
                KeyState mana;
                KeyState fastRestart;
            } actionState;
            
            glm::vec2 cursorPosition;
            Map map;
            u64 seed;
        };
        char disposableMemory[];
    };
    
    Game() = delete;
    
    void gameInit();
    
    void gameInput();
    
    void gameUpdate();
    
    void gameRender();
    
    void drawEntrance()
    {
        for(u32 i = 0; i < map.roomCount; i++)
        {
            glm::vec2 entrance = map.rooms[i].entranceLeftTop;
            ENGINE.drawColoredQuad(entrance, map.quadSize, glm::vec4(1.0f,1.0f,1.0f,1.0f), 0);
            entrance = map.rooms[i].entranceLeftBottom;
            ENGINE.drawColoredQuad(entrance, map.quadSize, glm::vec4(1.0f,1.0f,1.0f,1.0f), 0);
            entrance = map.rooms[i].entranceRightTop;
            ENGINE.drawColoredQuad(entrance, map.quadSize, glm::vec4(1.0f,1.0f,1.0f,1.0f), 0);
            entrance = map.rooms[i].entranceRightBottom;
            ENGINE.drawColoredQuad(entrance, map.quadSize, glm::vec4(1.0f,1.0f,1.0f,1.0f), 0);
        }
    }
    
    void enterRoom(u32 roomIndex)
    {
        static u64 seed = std::random_device()();
        assert(roomIndex < map.roomCount);
        Map::Room& room = map.rooms[roomIndex];
        if(roomIndex == map.roomCount - 1)
        {
            Entity eId = entityRegistry.createEntity();
            
            entityRegistry.addComponent<TransformComponent>(eId, room.center, glm::vec2(180.0f, 260.0f));
            entityRegistry.addComponent<TypeComponent>(eId, BIG_DEMON);
            entityRegistry.addComponent<ColliderComponent>(eId, glm::vec2(180.0f, 260.0f));
            entityRegistry.addComponent<SpriteComponent>(eId, BIG_DEMON_IDLE);
            entityRegistry.addComponent<EntityComponent>(eId, ENTITIES_STATS[BIG_DEMON]);
            entityRegistry.addComponent<EnemyComponent>(eId, EnemyComponent::WALK, (f32)ENGINE.getGameTime());
            
            Entity healthBarId = entityRegistry.createEntity();
            
            entityRegistry.addComponent<TransformComponent>(healthBarId, glm::vec2{0.0f, 0.0f}, glm::vec2{180.0f, HEALTH_BAR_HEIGHT}, 0.0f);
            entityRegistry.addComponent<SolidColorComponent>(healthBarId, glm::vec4{1.0f, 0.0f, 0.0f, 1.0f});
            entityRegistry.addComponent<FollowComponent>(healthBarId, eId, glm::vec2{0.0f, 260.0f / 2.0f + HEALTH_BAR_HEIGHT_OFFSET});
            entityRegistry.addComponent<HealthBarComponent>(healthBarId, 180.0f, ENTITIES_STATS[BIG_DEMON].healthPoints);
            
            EntityComponent& ent = entityRegistry.getComponent<EntityComponent>(playerEId);
            ent.attackPoints = 0.1f;
            itemSet.weapons &= ~BIT(WEAPON_SWORD);
        }
        else
        {
            
            AnimationTag tag;
            EntityType type;
            
            if(ENGINE.randomU64(seed) % 2 == 0)
            {
                type = BIG_DEMON; 
                tag = BIG_DEMON_IDLE;
            }
            else
            {
                type = BIG_ZOMBIE; 
                tag = BIG_ZOMBIE_IDLE;
            }
            
            glm::vec2 roomSize = room.size;
            addEntity(glm::vec2(room.center + roomSize / 4.0f),
                      type,
                      tag);
            
            roomSize = {-room.size.x, room.size.y}; 
            
            addEntity(glm::vec2(room.center + roomSize / 4.0f),
                      type,
                      tag);
            
            roomSize = {-room.size.x, -room.size.y}; 
            
            addEntity(glm::vec2(room.center + roomSize / 4.0f),
                      type,
                      tag);
            
            roomSize = {room.size.x, -room.size.y}; 
            
            addEntity(glm::vec2(room.center + roomSize / 4.0f),
                      type,
                      tag);
        }
        
        combatPhase = COMBAT_PHASE_ON;
        
    }
    
    void handleCombatPhases()
    {
        static u32 roomIndex = 0;
        static u64 seed = std::random_device()();
        
        if(combatPhase == COMBAT_PHASE_ON)
        {
            ComponentVector<EnemyComponent>& enemies = entityRegistry.getComponentVector<EnemyComponent>();
            if(enemies.size() == 0)
            {
                //ENDS COMBAT
                // opens doors and drops item
                Map::Room room = map.rooms[roomIndex];
                
                ItemType type = ITEM_TYPE_WEAPON;//ENGINE.randomRangeU64(0, 3);
                ItemBit bit = ENGINE.randomRangeU64(seed, 0, 2);
                AnimationTag aniTag = SWORD;
                
                switch(bit)
                {
                    case WEAPON_FORK:
                    {
                        aniTag = FORK;
                    }break;
                    
                    case WEAPON_SHIELD:
                    {
                        aniTag = SHIELD;
                    }break;    
                }
                
                addItem(room.center, type, bit, aniTag);
                
                combatPhase = COMBAT_PHASE_OFF;
            }
        }
        else
        {
            TransformComponent& tr = entityRegistry.getComponent<TransformComponent>(playerEId);
            ColliderComponent& col = entityRegistry.getComponent<ColliderComponent>(playerEId);
            
            for(u32 i = 0; i < map.roomCount; i++)
            {
                if(isCollision(tr.position, map.rooms[i].entranceLeftBottom,
                               col.hitBox, map.quadSize))
                {
                    roomIndex = i;
                    enterRoom(i);
                    return;
                }
                if(isCollision(tr.position, map.rooms[i].entranceLeftTop,
                               col.hitBox, map.quadSize))
                {
                    roomIndex = i;
                    enterRoom(i);
                    return;
                }
            }
        }
    }
    
    bool isCollision(glm::vec2 posA, glm::vec2 posB,
                     glm::vec2 sizeA, glm::vec2 sizeB)
    {
        return (abs(posA.x - posB.x) <= sizeA.x / 2.0f + sizeB.x / 2.0f) &&
        (abs(posA.y - posB.y) <= sizeA.y / 2.0f + sizeB.y / 2.0f);
    }
    
    void renderMenuBackground()
    {
        // Textures to go through in the slideshow
        TextureTag tags[] = {
            TextureTag::SWORD_0,
            TextureTag::SHIELD_0,
            TextureTag::FORK_0
        };
        
        menuTime += deltaTime;
        ENGINE.addLight({}, 1.0f, glm::vec4(1.0f));
        switch(menuPhase)
        {
            case MENU_PHASE_ZOOM:
            {
                f32 zoom = (5.0f - (4.0f - menuTime)) / 5.0f;
                ENGINE.drawTexturedQuad({}, 
                                        ENGINE.getScreenSize() * zoom, 
                                        getTextureIdByTag(tags[menuTagIndex]), 
                                        0);
                if(menuTime >= 4.0f)
                {
                    menuTime = 0.0f;
                    menuPhase = MENU_PHASE_TRANSITION;
                }
            }break;
            
            case MENU_PHASE_TRANSITION:
            {
                f32 zoom = (5.0f + menuTime) / 5.0f;
                f32 zoom2 = (5.0f - (8.0f - menuTime)) / 5.0f;
                f32 transition = (4.0f - menuTime) / 4.0f;
                ENGINE.drawQuad({},
                                ENGINE.getScreenSize() * zoom, 
                                getTextureIdByTag(tags[menuTagIndex]), 
                                glm::vec4(transition), 
                                0);
                
                ENGINE.drawQuad({},
                                ENGINE.getScreenSize() * zoom2, 
                                getTextureIdByTag(tags[(menuTagIndex + 1) % ARRAY_COUNT(tags)]), 
                                glm::vec4(1.0f - transition), 
                                0);
                
                if(menuTime >= 4.0f)
                {
                    menuTime = 0.0f;
                    menuPhase = MENU_PHASE_ZOOM;
                    menuTagIndex = (menuTagIndex + 1) % ARRAY_COUNT(tags);
                }
            }break;
        }
    }
    
    void linkTextureIdByTag(TextureId id, TextureTag tag)
    {
        textureIdsByTag[static_cast<u32>(tag)] = id;
    }
    
    TextureId getTextureIdByTag(TextureTag tag) const
    {
        return textureIdsByTag[static_cast<u32>(tag)];
    }
    
    TextureId getTextureIdByTag(u32 tag) const
    {
        return textureIdsByTag[tag];
    }
    
    TextureId getTextureIdByItem(ItemType type, ItemBit bit)
    {
        TextureTag tag = TextureTag::NONE;
        switch (type)
        {
            case ITEM_TYPE_WEAPON:
            {
                tag = (TextureTag)((u32)TextureTag::WEAPON_START + bit);
            }break;
        }
        return getTextureIdByTag(tag);
    }
    
    void updateFollowers()
    {
        ComponentVector<FollowComponent>& followers = entityRegistry.getComponentVector<FollowComponent>();
        ComponentVector<TransformComponent>& transforms = entityRegistry.getComponentVector<TransformComponent>();
        
        for (Entity followerId: followers.iterateEntities())
        {
            const FollowComponent& follower = followers.getComponent(followerId);
            if (!entityRegistry.entityExists(follower.toFollowId))
            {
                entityRegistry.markEntityForDeletion(followerId);
                continue;
            }
            TransformComponent& followerTransform = transforms.getComponent(followerId);
            TransformComponent& toFollowTransform = transforms.getComponent(follower.toFollowId);
            followerTransform.position = toFollowTransform.position + follower.followOffset;
        }
        
        entityRegistry.removeMarkedEntities();
    }
    
    void updateHealthBars()
    {
        for (Entity healthBarId: entityRegistry.iterateEntities<HealthBarComponent, TransformComponent, FollowComponent>())
        {
            HealthBarComponent& healthBar = entityRegistry.getComponent<HealthBarComponent>(healthBarId);
            TransformComponent& healthBarComponent = entityRegistry.getComponent<TransformComponent>(healthBarId);
            FollowComponent& follower = entityRegistry.getComponent<FollowComponent>(healthBarId);
            if (!entityRegistry.hasComponent<EntityComponent>(follower.toFollowId))
            {
                entityRegistry.markEntityForDeletion(healthBarId);
                continue;
            }
            f32 healthPoints = entityRegistry.getComponent<EntityComponent>(follower.toFollowId).healthPoints;
            healthBarComponent.size.x = healthBar.maxSize * healthPoints / healthBar.maxHealth;
        }
        entityRegistry.removeMarkedEntities();
        
    }
    
    const char* getItemDescription(ItemType type, ItemBit bit)
    {
        switch (type)
        {
            case ITEM_TYPE_WEAPON:
            {
                switch (bit)
                {
                    case WEAPON_SHIELD:
                    return " Shield:\n Reduces knockback from enemies by 80% ";
                    case WEAPON_FORK:
                    return " Fork:\n Attacks now shoot forks in the cursor's direction ";
                    case WEAPON_SWORD:
                    return " Sword:\n Attacks deal extra damage ";
                }
            }break;
            
            case ITEM_TYPE_ARMOUR:
            {
                return "Armour:\nAdds 1 armor";
            }break;
            
            case ITEM_TYPE_UTILITY:
            {
                switch (bit)
                {
                    case UTILITY_GRENADE:
                    return "Grenade:\nAttacks now explode, dealing area of effect damage";
                    case UTILITY_POTION:
                    return "Potion:\nAttack speed is increased by 300%";
                    case UTILITY_ROPE:
                    return "Rope:\nNothing";
                }
            }break;
            
            
        }
    }
    
    void renderItems()
    {
        // Inventory
        glm::vec2 itemHolderBackgroundPos = ENGINE.uiTexture(glm::vec2{-260.0f, -200.0f}, glm::vec2{958.0f, 815.0f}, AnchorPoint::C, ID(ITEM_HOLDER_BACKGROUND));
        ENGINE.drawTextUI(glm::vec2{itemHolderBackgroundPos.x - 250.0f, itemHolderBackgroundPos.y + 450.0f}, 1200.0f, "Inventory");
        glm::vec2 inventorySlotPos[5*6];
        for(u32 y=0;y<5;y++)
            for(u32 x=0;x<6;x++)
        {
            inventorySlotPos[y*6 + x] = glm::vec2{itemHolderBackgroundPos.x - 375.0f + x*150.0f, itemHolderBackgroundPos.y + 300.0f - y*150.0f};
            ENGINE.drawUITex(inventorySlotPos[y*6 + x], glm::vec2{150, 150}, ID(ITEM_HOLDER_INVENTORY));
            
        }
        
        const EntityComponent& playerEntity = entityRegistry.getComponent<EntityComponent>(playerEId);
        char buffer[255];
        
        //Stats
        glm::vec2 statsBackgroundPos = ENGINE.uiTexture(glm::vec2{+500.0f, -200.0f}, glm::vec2{479.0f, 815.0f}, AnchorPoint::C, ID(ITEM_HOLDER_BACKGROUND));
        ENGINE.drawTextUI(glm::vec2{statsBackgroundPos.x - 100.0f, statsBackgroundPos.y + 450.0f}, 1200.0f, "Stats");
        sprintf(buffer, "Health: %.1f", playerEntity.healthPoints);
        ENGINE.drawTextUI(glm::vec2{statsBackgroundPos.x - 200.0f, statsBackgroundPos.y + 300.0f}, 500.0f, buffer);
        sprintf(buffer, "Speed: %.1f", playerEntity.movementSpeed);
        ENGINE.drawTextUI(glm::vec2{statsBackgroundPos.x - 200.0f, statsBackgroundPos.y + 300.0f - 75.0f*1}, 500.0, buffer);
        ENGINE.drawTextUI(glm::vec2{statsBackgroundPos.x - 200.0f, statsBackgroundPos.y + 300.0f - 75.0f*2}, 500.0f, "Stamina: ");
        sprintf(buffer, "Strength: %.1f", playerEntity.attackPoints);
        ENGINE.drawTextUI(glm::vec2{statsBackgroundPos.x - 200.0f, statsBackgroundPos.y + 300.0f - 75.0f*3}, 500.0f, buffer);
        ENGINE.drawTextUI(glm::vec2{statsBackgroundPos.x - 200.0f, statsBackgroundPos.y + 300.0f - 75.0f*4}, 500.0f, "Dexterity: ");
        ENGINE.drawUITex(glm::vec2{statsBackgroundPos.x, statsBackgroundPos.y - 250.0f}, TEXTURE_SIZES[ELF_M]*10.0f, ID(ELF_M_IDLE_0));
        
        glm::vec2 itemFrameSize = {100,100};
        u32 itemCount = 0;
        glm::vec2 buttonSize = {
            ENGINE.getScreenSize().x / 3.0f,
            ENGINE.getScreenSize().y / 6.5f
        };
        f32 fontSize = 500.0f;
        for (ItemBit i = 0; i != WEAPON_COUNT; i++)
        {
            if (hasWeapon(i))
            {
                if (ENGINE.uiButtonHover(inventorySlotPos[itemCount],
                                         itemFrameSize,
                                         AnchorPoint::C,
                                         getTextureIdByItem(ITEM_TYPE_WEAPON, i),
                                         getTextureIdByItem(ITEM_TYPE_WEAPON, i)))
                {
                    ENGINE.drawUITex(glm::vec2{cursorPosition.x + buttonSize.x/2, cursorPosition.y + buttonSize.y/2}, buttonSize, ID(BUTTON));
                    ENGINE.drawTextInsideBoxUI(glm::vec2{cursorPosition.x, cursorPosition.y + buttonSize.y},
                                               glm::vec2{cursorPosition.x + buttonSize.x, cursorPosition.y},
                                               fontSize,
                                               getItemDescription(ITEM_TYPE_WEAPON, i),
                                               30.0f);
                }
                itemCount++;
            }
        }
        
        for (ItemBit i = 0; i != ARMOUR_COUNT; i++)
        {
            if (hasArmour(i))
            {
                if (ENGINE.uiButtonHover(inventorySlotPos[itemCount],
                                         itemFrameSize,
                                         AnchorPoint::C,
                                         getTextureIdByItem(ITEM_TYPE_ARMOUR, i),
                                         getTextureIdByItem(ITEM_TYPE_ARMOUR, i)))
                {
                    ENGINE.drawUITex(glm::vec2{cursorPosition.x + buttonSize.x/2, cursorPosition.y + buttonSize.y/2}, buttonSize, ID(BUTTON));
                    ENGINE.drawTextInsideBoxUI(glm::vec2{cursorPosition.x, cursorPosition.y + buttonSize.y},
                                               glm::vec2{cursorPosition.x + buttonSize.x, cursorPosition.y},
                                               fontSize,
                                               getItemDescription(ITEM_TYPE_ARMOUR, i),
                                               30.0f);
                }
                itemCount++;
            }
        }
        
        for (ItemBit i = 0; i != UTILITY_COUNT; i++)
        {
            if (hasUtility(i))
            {
                if (ENGINE.uiButtonHover(inventorySlotPos[itemCount],
                                         itemFrameSize,
                                         AnchorPoint::C,
                                         getTextureIdByItem(ITEM_TYPE_UTILITY, i),
                                         getTextureIdByItem(ITEM_TYPE_UTILITY, i)))
                {
                    ENGINE.drawUITex(glm::vec2{cursorPosition.x + buttonSize.x/2, cursorPosition.y + buttonSize.y/2}, buttonSize, ID(BUTTON));
                    ENGINE.drawTextInsideBoxUI(glm::vec2{cursorPosition.x, cursorPosition.y + buttonSize.y},
                                               glm::vec2{cursorPosition.x + buttonSize.x, cursorPosition.y},
                                               fontSize,
                                               getItemDescription(ITEM_TYPE_UTILITY, i),
                                               30.0f);
                }
                itemCount++;
            }
        }
    }
    
    u32 hasWeapon(ItemBit weapon)
    {
        return itemSet.weapons >> weapon & 1;
    }
    
    u32 hasArmour(ItemBit armour)
    {
        return itemSet.armours >> armour & 1;
    }
    
    bool hasUtility(ItemBit utility)
    {
        return itemSet.utility & BIT(utility);
    }
    
    void itemPickupSystem()
    {
        TransformComponent& playerSprite = entityRegistry.getComponent<TransformComponent>(playerEId);
        ColliderComponent& playerCollider = entityRegistry.getComponent<ColliderComponent>(playerEId);
        for (Entity eId : entityRegistry.iterateEntities<ItemComponent, TransformComponent>())
        {
            TransformComponent& itemSprite = entityRegistry.getComponent<TransformComponent>(eId);
            ColliderComponent& itemCollider = entityRegistry.getComponent<ColliderComponent>(eId);
            if (isCollision(playerSprite.position, itemSprite.position,
                            playerCollider.hitBox, itemCollider.hitBox))
            {
                ItemComponent& item = entityRegistry.getComponent<ItemComponent>(eId);
                //logInfo("Before pickup: %d %d %d", itemSet.weapons, itemSet.armours, itemSet.utility);
                switch (item.itemType)
                {
                    case ITEM_TYPE_WEAPON:
                    {
                        itemSet.weapons |= BIT(item.itemId);
                    }break;
                    
                    case ITEM_TYPE_ARMOUR:
                    {
                        itemSet.armours |= BIT(item.itemId);
                    }break;
                    
                    case ITEM_TYPE_UTILITY:
                    {
                        itemSet.utility|= BIT(item.itemId);
                    }break;
                }
                //logInfo("After pickup: %d %d %d", itemSet.weapons, itemSet.armours, itemSet.utility);
                entityRegistry.markEntityForDeletion(eId);
            }
        }
        
        entityRegistry.removeMarkedEntities();
    }
    
    void addPlayer(glm::vec2 position, EntityType playerType, AnimationTag animationTag)
    {
        Entity eId = entityRegistry.createEntity();
        playerEId = eId;
        
        camera = {position.x, position.y, DEFAULT_CAMERA_ZOOM};
        
        playerRadius = calcRadiusOfEntity(playerType);
        
        entityRegistry.addComponent<TransformComponent>(eId, position, TEXTURE_SIZES[playerType], 0.0f);
        entityRegistry.addComponent<TypeComponent>(eId, playerType);
        entityRegistry.addComponent<ColliderComponent>(eId, HIT_BOXES[playerType]);
        entityRegistry.addComponent<SpriteComponent>(eId, animationTag);
        entityRegistry.addComponent<EntityComponent>(eId, ENTITIES_STATS[playerType]);
        entityRegistry.addComponent<VelocityComponent>(eId, glm::vec2{}, glm::vec2{});
    }
    
    void addEntity(glm::vec2 position, EntityType enemyType, AnimationTag animationTag)
    {
        Entity eId = entityRegistry.createEntity();
        
        entityRegistry.addComponent<TransformComponent>(eId, position, TEXTURE_SIZES[enemyType], 0.0f);
        entityRegistry.addComponent<TypeComponent>(eId, enemyType);
        entityRegistry.addComponent<ColliderComponent>(eId, HIT_BOXES[enemyType]);
        entityRegistry.addComponent<SpriteComponent>(eId, animationTag);
        entityRegistry.addComponent<EntityComponent>(eId, ENTITIES_STATS[enemyType]);
        entityRegistry.addComponent<EnemyComponent>(eId, EnemyComponent::WALK, (f32)ENGINE.getGameTime());
        
        Entity healthBarId = entityRegistry.createEntity();
        
        entityRegistry.addComponent<TransformComponent>(healthBarId, glm::vec2{0.0f, 0.0f}, glm::vec2{HIT_BOXES[enemyType].x, HEALTH_BAR_HEIGHT}, 0.0f);
        entityRegistry.addComponent<SolidColorComponent>(healthBarId, glm::vec4{1.0f, 0.0f, 0.0f, 1.0f});
        entityRegistry.addComponent<FollowComponent>(healthBarId, eId, glm::vec2{0.0f, HIT_BOXES[enemyType].y / 2.0f + HEALTH_BAR_HEIGHT_OFFSET});
        entityRegistry.addComponent<HealthBarComponent>(healthBarId, HIT_BOXES[enemyType].x, ENTITIES_STATS[enemyType].healthPoints);
    }
    
    void addItem(glm::vec2 position, ItemType type, ItemBit bit, AnimationTag animationTag)
    {
        Entity itemEId = entityRegistry.createEntity();
        itemId = itemEId;
        entityRegistry.addComponent<TransformComponent>(itemEId, position, glm::vec2{25.0f, 25.0f}, 0.0f);
        entityRegistry.addComponent<ItemComponent>(itemEId, type, bit);
        entityRegistry.addComponent<SpriteComponent>(itemEId, animationTag);
        entityRegistry.addComponent<ColliderComponent>(itemEId, glm::vec2{25.0f, 25.0f});
    }
    
    bool isAnimationLocked(const SpriteComponent& sprite)
    {
        return ENGINE.getGameTime() < sprite.endTime;
    }
    
    void setAnimation(SpriteComponent& sprite, AnimationTag tag)
    {
        if (sprite.animation == tag || isAnimationLocked(sprite))
            return;
        sprite.animation = tag;
        sprite.startTime = ENGINE.getGameTime();
        sprite.endTime = sprite.startTime + ANIMATION_COOLDOWN[tag];
    }
    
    void updatePlayerPosition()
    {
        if (actionState.zoomIn == KeyState::HOLD)
        {
            camera.z += camera.z * deltaTime;
        }
        
        if (actionState.zoomOut == KeyState::HOLD)
        {
            camera.z -= camera.z * deltaTime;
        }
        
        glm::vec2 direction{0.0f, 0.0f};
        if (actionState.walkUp == KeyState::HOLD || actionState.walkUp == KeyState::PRESS)
            direction += glm::vec2{0.0f, +1.0f};
        if (actionState.walkDown == KeyState::HOLD || actionState.walkDown == KeyState::PRESS)
            direction += glm::vec2{0.0f, -1.0f};
        if (actionState.walkLeft == KeyState::HOLD || actionState.walkLeft == KeyState::PRESS)
            direction += glm::vec2{-1.0f, 0.0f};
        if (actionState.walkRight == KeyState::HOLD || actionState.walkRight == KeyState::PRESS)
            direction += glm::vec2{+1.0f, 0.0f};
        
        TransformComponent& playerTransform = entityRegistry.getComponent<TransformComponent>(playerEId);
        EntityComponent& playerEntity = entityRegistry.getComponent<EntityComponent>(playerEId);
        VelocityComponent& v = entityRegistry.getComponent<VelocityComponent>(playerEId);
        v.targetVel = direction * playerEntity.movementSpeed;
        
        SpriteComponent& playerSprite = entityRegistry.getComponent<SpriteComponent>(playerEId);
        if (v.targetVel.x != 0.0f || v.targetVel.y != 0.0f)
        {
            setAnimation(playerSprite, ELF_M_RUN);
        }
        else
        {
            setAnimation(playerSprite, ELF_M_IDLE);
        }
        
        playerTransform.position = resolveBasePositionCollision(playerTransform.position - v.vel * (f32)deltaTime, playerTransform.position, ENTITY_TYPE_PLAYER);
        
        f32 len = glm::length(cursorPosition);
        len = fmax(len, 50);
        glm::vec2 offset = glm::normalize(cursorPosition) * len;
        
        camera.x = playerTransform.position.x + offset.x / 20.0f;
        camera.y = playerTransform.position.y + offset.y / 20.0f;
    }
    
    void renderCursor()
    {
        ENGINE.uiTexture(cursorPosition, {50.0f,50.0f}, AnchorPoint::C, getTextureIdByTag(TextureTag::MOUSE_POINTER));
    }
    
    void updatePlayerHealth()
    {
        TransformComponent& playerTransform = entityRegistry.getComponent<TransformComponent>(playerEId);
        ColliderComponent& playerCollider = entityRegistry.getComponent<ColliderComponent>(playerEId);
        EntityComponent& playerEntity = entityRegistry.getComponent<EntityComponent>(playerEId);
        VelocityComponent& playerVel = entityRegistry.getComponent<VelocityComponent>(playerEId);
        
        for (Entity enemyId: entityRegistry.iterateEntities<TransformComponent, EntityComponent, ColliderComponent>())
        {
            if (entityRegistry.getComponent<TypeComponent>(enemyId).entityType == ENTITY_TYPE_PLAYER)
                continue;
            
            TransformComponent& enemyTransform = entityRegistry.getComponent<TransformComponent>(enemyId);
            ColliderComponent& enemyCollider = entityRegistry.getComponent<ColliderComponent>(playerEId);
            if (isCollision(playerTransform.position, enemyTransform.position,
                            playerCollider.hitBox, enemyCollider.hitBox))
            {
                f32 armourCount = (f32)(hasArmour(ARMOUR_CHESTPLATE) + hasArmour(ARMOUR_HELMET) + hasArmour(ARMOUR_PANTS));
                f32 enemyAttackPoints = entityRegistry.getComponent<EntityComponent>(enemyId).attackPoints;
                playerEntity.healthPoints -= enemyAttackPoints / (1.0f + armourCount);
                
                glm::vec2 dir = glm::normalize(playerTransform.position - enemyTransform.position);
                if (hasWeapon(WEAPON_SHIELD))
                    playerVel.vel += dir * 100.0f;
                else
                    playerVel.vel += dir * 500.0f;
                
                if (playerEntity.healthPoints <= 0.0f)
                {
                    gameState = GAME_LOST;
                    return;
                }
            }
        }
    }
    
    void updatePlayerAttack()
    {
        if (playerAttackTimer > 0.0)
        {
            playerAttackTimer -= deltaTime;
            return;
        }
        if (actionState.attack != KeyState::PRESS && actionState.attack != KeyState::HOLD)
        {
            return;
        }
        SpriteComponent& playerSprite = entityRegistry.getComponent<SpriteComponent>(playerEId);
        setAnimation(playerSprite, ELF_M_HIT);
        // if (hasWeapon(WEAPON_FORK))
        {
            playerAttackTimer = 0.4 - 0.3 * (f64)hasUtility(UTILITY_POTION);
            // cursorPosition is not affected by the camera position in calculations.
            //To convert cursorPosition to actual world Position you have to add the camera position to it
            EntityComponent& playerEntity = entityRegistry.getComponent<EntityComponent>(playerEId);
            
            Entity projectileId = entityRegistry.createEntity();
            glm::vec2 playerPos = entityRegistry.getComponent<TransformComponent>(playerEId).position;
            glm::vec2 directionVector = glm::normalize(cursorPosition + glm::vec2{camera.x, camera.y} - playerPos);
            
            entityRegistry.addComponent<ProjectileComponent>(projectileId, directionVector, 400.0f,
                                                             playerEntity.attackPoints + (50.0f * (f32)hasWeapon(WEAPON_SWORD)), false);
            
            entityRegistry.addComponent<TransformComponent>(projectileId, playerPos, TEXTURE_SIZES_WEAPONS[WEAPON_FORK],
                                                            (f32)(atan2(directionVector.y, directionVector.x) - PI / 2.0f));
            
            // TO-DO collision with hitbox
            // entityRegistry.addComponent<ColliderComponent>();
            
            entityRegistry.addComponent<SpriteComponent>(projectileId, FORK);
        }
    }
    
    void updatePlayer()
    {
        updatePlayerPosition();
        updatePlayerAttack();
        updatePlayerHealth();
        const SpriteComponent& player = entityRegistry.getComponent<SpriteComponent>(playerEId);
    }
    
    /* Custom collision */
    void updateEnemies()
    {
        f64 time = ENGINE.getGameTime();
        for (EnemyComponent& enemy : entityRegistry.iterateComponents<EnemyComponent>())
        {
            if (enemy.phase == EnemyComponent::WALK && time - enemy.startTime >= 5.0)
            {
                enemy.phase = EnemyComponent::SHOOT;
                enemy.startTime = time;
            }
            else if (enemy.phase == EnemyComponent::WAIT && time - enemy.startTime >= 1.0)
            {
                enemy.phase = EnemyComponent::WALK;
                enemy.startTime = time;
            }
        }
        
        const TransformComponent& playerTransform = entityRegistry.getComponent<TransformComponent>(playerEId);
        
        for (Entity enemyEntityId: entityRegistry.iterateEntities<TransformComponent, EntityComponent>())
        {
            TransformComponent& enemyTransform = entityRegistry.getComponent<TransformComponent>(enemyEntityId);
            glm::vec2 enemyVector{
                enemyTransform.position.x - playerTransform.position.x,
                enemyTransform.position.y - playerTransform.position.y
            };
            
            EntityType entityType = entityRegistry.getComponent<TypeComponent>(enemyEntityId).entityType;
            SpriteComponent& enemySprite = entityRegistry.getComponent<SpriteComponent>(enemyEntityId);
            
            if (glm::length(enemyVector) > ENEMY_DETECTION_RADIUS)
            {
                switch (entityType)
                {
                    case BIG_DEMON:
                    {
                        setAnimation(enemySprite, BIG_DEMON_IDLE);
                        break;
                    }
                    case BIG_ZOMBIE:
                    {
                        setAnimation(enemySprite, BIG_ZOMBIE_IDLE);
                        break;
                    }
                }
                continue;
            }
            
            switch (entityType)
            {
                case BIG_DEMON:
                {
                    setAnimation(enemySprite, BIG_DEMON_RUN);
                    break;
                }
                case BIG_ZOMBIE:
                {
                    setAnimation(enemySprite, BIG_ZOMBIE_RUN);
                    break;
                }
            }
            
            if (enemyEntityId == playerEId)
            {
                continue;
            }
            
            glm::vec2 normalizedEnemyVector = glm::normalize(enemyVector);
            EntityComponent& enemyEntity = entityRegistry.getComponent<EntityComponent>(enemyEntityId);
            EnemyComponent& enemy = entityRegistry.getComponent<EnemyComponent>(enemyEntityId);
            ColliderComponent& enemyCollider = entityRegistry.getComponent<ColliderComponent>(enemyEntityId);
            if (enemy.phase == EnemyComponent::WALK)
            {
                f32 distance = (f32)deltaTime * enemyEntity.movementSpeed;
                
                glm::vec2 nextPosition{
                    enemyTransform.position.x - normalizedEnemyVector.x * distance,
                    enemyTransform.position.y - normalizedEnemyVector.y * distance
                };
                EntityType enemyType = entityRegistry.getComponent<TypeComponent>(enemyEntityId).entityType;
                glm::vec2 newPosition = resolveBasePositionCollision(enemyTransform.position, nextPosition, enemyType);
                
                // collision with player
                // const glm::vec2 vectorBetween{
                //     playerTransform.position.x - newPosition.x,
                //     playerTransform.position.y - newPosition.y
                // };
                
                // const f32 distanceBetween = glm::length(vectorBetween);
                
                enemyTransform.position.x = newPosition.x;
                enemyTransform.position.y = newPosition.y;
            }
            else if (enemy.phase == EnemyComponent::SHOOT)
            {
                switch (entityType)
                {
                    case BIG_DEMON:
                    {
                        break;
                    }
                    case BIG_ZOMBIE:
                    {
                        setAnimation(enemySprite, BIG_ZOMBIE_HIT);
                        break;
                    }
                }
                enemy.phase = EnemyComponent::WAIT;
                Entity proj = entityRegistry.createEntity();
                glm::vec2 dir = glm::normalize(playerTransform.position - enemyTransform.position);
                entityRegistry.addComponent<ProjectileComponent>(proj,
                                                                 dir,
                                                                 250.0f,
                                                                 enemyEntity.attackPoints,
                                                                 true);
                entityRegistry.addComponent<TransformComponent>(proj,
                                                                enemyTransform.position,
                                                                HIT_BOXES_WEAPONS[WEAPON_KNIFE],
                                                                (f32)(atan2(dir.y, dir.x) - PI / 2.0f));
                entityRegistry.addComponent<SpriteComponent>(proj, KNIFE);
            }
        }
    }
    
    void renderSprites()
    {
        u32 cnt = 0;
        ComponentVector<SpriteComponent>& sprites = entityRegistry.getComponentVector<SpriteComponent>();
        Entity* entityIds = (Entity*)(ENGINE.temporaryAlloc(sprites.size() * sizeof(Entity)));
        
        for (Entity entityId: sprites.iterateEntities())
        {
            entityIds[cnt++] = entityId;
        }
        
        std::sort(entityIds, entityIds + cnt, [this](const Entity A, const Entity B)
                  {
                      const f32 aY = entityRegistry.getComponent<TransformComponent>(A).position.y;
                      const f32 bY = entityRegistry.getComponent<TransformComponent>(B).position.y;
                      return aY > bY;
                  });
        
        const SpriteComponent& playerSprite = entityRegistry.getComponent<SpriteComponent>(playerEId);
        TextureId playerTextureId = ENGINE.getAnimationFrame(playerSprite.animation, playerSprite.startTime);
        const TransformComponent& playerTransform = entityRegistry.getComponent<TransformComponent>(playerEId);
        for (u32 i = 0; i < cnt; ++i)
        {
            const SpriteComponent& entitySprite = entityRegistry.getComponent<SpriteComponent>(entityIds[i]);
            TextureId textureId = ENGINE.getAnimationFrame(entitySprite.animation, entitySprite.startTime);
            TransformComponent entityTransform = entityRegistry.getComponent<TransformComponent>(entityIds[i]);
            if (playerTextureId == textureId)
            {
                if (cursorPosition.x < 0)
                {
                    entityTransform.size.x = -abs(entityTransform.size.x);
                }
                else
                {
                    entityTransform.size.x = abs(entityTransform.size.x);
                }
            }
            // it's not item and it's not the player
            else if (entityRegistry.hasComponent<TypeComponent>(entityIds[i]) && entityIds[i] != playerEId)
            {
                if (playerTransform.position.x < entityTransform.position.x)
                {
                    entityTransform.size.x = -abs(entityTransform.size.x);
                }
                else
                {
                    entityTransform.size.x = abs(entityTransform.size.x);
                }
            }
            // draw texture
            ENGINE.drawTexturedQuad(
                                    entityTransform.position,
                                    ENTITIES_SIZE_MULTIPLIER * entityTransform.size,
                                    textureId,
                                    entityTransform.rotation
                                    );
        }
        // draw light
        ENGINE.addLight(playerTransform.position, 100.0f, {1.0f, 1.0f, 1.0f, 1.0f});
        
        for (Entity colorId: entityRegistry.iterateEntities<SolidColorComponent, TransformComponent>())
        {
            TransformComponent& colorTransform = entityRegistry.getComponent<TransformComponent>(colorId);
            glm::vec4 color = entityRegistry.getComponent<SolidColorComponent>(colorId).color;
            ENGINE.drawColoredQuad(colorTransform.position, colorTransform.size, color, colorTransform.rotation);
        }
        
        const EntityComponent& playerEntity = entityRegistry.getComponent<EntityComponent>(playerEId);
        f32 maxBarLength = 923.0f;
        f32 healthBarLength = maxBarLength/200.0f*playerEntity.healthPoints;
        
        // Stats bars
        glm::vec2 hudBarsPos = {0.0f,0.0f};
        hudBarsPos = ENGINE.uiTexture(glm::vec2{20,-20}, glm::vec2{1218, 196}, AnchorPoint::NW, ID(STATS_UI));
        ENGINE.drawQuadUI(glm::vec2{hudBarsPos.x + 72.0f - (maxBarLength - healthBarLength)/2, hudBarsPos.y + 64.0f}, glm::vec2{healthBarLength,22}, glm::vec4{0.9f,0.1f,0.3f,0.8f}, 0.0f);
        ENGINE.drawQuadUI(glm::vec2{hudBarsPos.x + 72.0f, hudBarsPos.y + 0.0f}, glm::vec2{920,22}, glm::vec4{0.1f,0.9f,0.3f,0.8f}, 0.0f);
        ENGINE.drawQuadUI(glm::vec2{hudBarsPos.x - 238.0f, hudBarsPos.y + -64.0f}, glm::vec2{305,22}, glm::vec4{0.4f,0.4f,0.9f,0.8f}, 0.0f);
        
        // Quick access items
        glm::vec2 quickItemsSlotPos[8];
        glm::vec2 hudItemHolderPos = ENGINE.uiTexture(glm::vec2{20, 50}, glm::vec2{45, 84}, AnchorPoint::SW, ID(ITEM_HOLDER_START));
        u32 i=0;
        for(i=0;i<3;i++)
        {
            quickItemsSlotPos[i] = glm::vec2{195.0f/2 + hudItemHolderPos.x + i*150, hudItemHolderPos.y};
            ENGINE.drawUITex(quickItemsSlotPos[i], glm::vec2{150, 144}, ID(ITEM_HOLDER)); 
        }
        ENGINE.drawUITex(glm::vec2{45.0f + hudItemHolderPos.x + i*150, hudItemHolderPos.y}, glm::vec2{45, 84}, ID(ITEM_HOLDER_END));
        
        //items
        ENGINE.drawUITex(quickItemsSlotPos[0], glm::vec2{120,120}, ID(HEALTH_POTION));
        ENGINE.drawUITex(quickItemsSlotPos[1], glm::vec2{120,120}, ID(STAMINA_POTION));
        ENGINE.drawUITex(quickItemsSlotPos[2], glm::vec2{120,120}, ID(MANA_POTION));
    }
    
    void velocitySystem()
    {
        for (Entity vEid: entityRegistry.iterateEntities<VelocityComponent>())
        {
            VelocityComponent& v = entityRegistry.getComponent<VelocityComponent>(vEid);
            TransformComponent& t = entityRegistry.getComponent<TransformComponent>(vEid);
            v.vel += (v.targetVel - v.vel) * 20.0f * (f32)deltaTime;
            t.position += v.vel * (f32)deltaTime;
        }
    }
    
    void startGame()
    {
        seed = std::random_device()();
        initMap(MAP_SIZE_X, MAP_SIZE_Y);
        addPlayer(startPosition, ENTITY_TYPE_PLAYER, ELF_M_IDLE);
        updateFollowers();
        
        glm::vec2 playerPos = entityRegistry.getComponent<TransformComponent>(playerEId).position;
        
        ENGINE.printGlobalAllocations(false);
        gameState = GAME_RUNNING;
    }
    
    void moveProjectiles()
    {
        for (Entity projectileId: entityRegistry.iterateEntities<TransformComponent, ProjectileComponent>())
        {
            TransformComponent& projectileSprite = entityRegistry.getComponent<TransformComponent>(projectileId);
            ProjectileComponent& projectile = entityRegistry.getComponent<ProjectileComponent>(projectileId);
            projectileSprite.position += projectile.direction * projectile.speed * (f32)deltaTime;
            
            if (!isOnScreen(projectileSprite.position))
            {
                entityRegistry.markEntityForDeletion(projectileId);
                continue;
            }
            
            for (Entity enemyId: entityRegistry.iterateEntities<ColliderComponent, TransformComponent, EntityComponent, TypeComponent>())
            {
                TypeComponent& enemyCollider = entityRegistry.getComponent<TypeComponent>(enemyId);
                if ((enemyCollider.entityType != ENTITY_TYPE_PLAYER && projectile.isEnemy) ||
                    (enemyCollider.entityType == ENTITY_TYPE_PLAYER && !projectile.isEnemy))
                    continue;
                
                TransformComponent& enemySprite = entityRegistry.getComponent<TransformComponent>(enemyId);
                f32 distanceBetween = glm::distance(enemySprite.position, projectileSprite.position);
                if (distanceBetween <= std::min(enemySprite.size.x, enemySprite.size.y) / 2)
                    //if (isCollision(enemySprite, projectileSprite))
                {
                    ProjectileComponent& proj = entityRegistry.getComponent<ProjectileComponent>(projectileId);
                    // subtract shooter's attack from enemy's health
                    EntityComponent& entityStats = entityRegistry.getComponent<EntityComponent>(enemyId);
                    entityStats.healthPoints -= projectile.damage;
                    
                    if (entityStats.healthPoints <= 0.0f)
                    {
                        if (enemyId == playerEId)
                        {
                            gameState = GAME_LOST;
                            return;
                        }
                        // DO NOT MOVE THIS LINE, PLAYER SHOULDN'T BE MARKED FOR DELETION
                        entityRegistry.markEntityForDeletion(enemyId);
                    }
                    
                    //Emit Particles
                    ENGINE.emitParticles(20,
                                         enemySprite.position,
                                         50.0f,
                                         0.0f,
                                         proj.direction,
                                         glm::vec4(0.7f, 0, 0, 1.0f),
                                         glm::vec4(0.4f, 0, 0, 0.0f),
                                         glm::vec2(4.0f),
                                         3.0f,
                                         0.5f);
                    // delete projectile
                    entityRegistry.markEntityForDeletion(projectileId);
                }
            }
        }
        entityRegistry.removeMarkedEntities();
        
    }
    
    glm::vec2 getCameraUnits() const
    {
        return ENGINE.getScreenSize() / camera.z;
    }
    
    glm::vec2 getCameraLeftTopCorner() const
    {
        return {camera.x - getCameraUnits().x / 2.0f, camera.y + getCameraUnits().y / 2.0f};
    }
    
    glm::vec2 getCameraRightTopCorner() const
    {
        return {camera.x + getCameraUnits().x / 2.0f, camera.y + getCameraUnits().y / 2.0f};
    }
    
    glm::vec2 getCameraLeftBottomCorner() const
    {
        return {camera.x - getCameraUnits().x / 2.0f, camera.y - getCameraUnits().y / 2.0f};
    }
    
    glm::vec2 getCameraRightBottomCorner() const
    {
        return {camera.x + getCameraUnits().x / 2.0f, camera.y - getCameraUnits().y / 2.0f};
    }
    
    bool isOnScreen(glm::vec2 pos) const {
        glm::vec2 leftBottomCorner = getCameraLeftBottomCorner();
        glm::vec2 rightTopCorner = getCameraRightTopCorner();
        return pos.x >= leftBottomCorner.x && pos.x <= rightTopCorner.x &&
            pos.y >= leftBottomCorner.y && pos.y <= rightTopCorner.y;
    }
    
    void playerToggleVroom()
    {
        EntityComponent& player = entityRegistry.getComponent<EntityComponent>(playerEId);
        isVroomOn = !isVroomOn;
        if (isVroomOn)
            player.movementSpeed = 10.0f * DEFAULT_PLAYER_SPEED;
        else
            player.movementSpeed = DEFAULT_PLAYER_SPEED;
    }
    
    glm::uvec2 getTileByPosition(glm::vec2 position) const
    {
        // we substract map.quadSize / 2 because we draw map.tiles from the center
        return (glm::uvec2)(position / map.quadSize);
    }
    
    glm::vec2 getPlayerSpritePosition() const
    {
        return entityRegistry.getComponent<TransformComponent>(playerEId).position;
    }
    
    // position used for collisions
    glm::vec2 getPlayerBasePosition() const
    {
        assert(HIT_BOXES[ENTITY_TYPE_PLAYER].x <= HIT_BOXES[ENTITY_TYPE_PLAYER].y && "Ratia texturii (x/y) playerului este prea mare");
        glm::vec2 spritePos = getPlayerSpritePosition();
        return {spritePos.x, spritePos.y - (HIT_BOXES[ENTITY_TYPE_PLAYER].y - HIT_BOXES[ENTITY_TYPE_PLAYER].x) / 2};
    }
    
    // position used for collisions
    glm::vec2 getBasePosition(glm::vec2 spritePos, EntityType type) const
    {
        assert(HIT_BOXES[type].x <= HIT_BOXES[type].y && "Ratia texturii (x/y) este prea mare");
        return {spritePos.x, spritePos.y - (HIT_BOXES[type].y - HIT_BOXES[type].x) / 2.0f};
    }
    
    glm::vec2 playerBaseToSpritePosition(glm::vec2 basePosition) const
    {
        return {basePosition.x, basePosition.y + (HIT_BOXES[ENTITY_TYPE_PLAYER].y - HIT_BOXES[ENTITY_TYPE_PLAYER].x) / 2.0f};
    }
    
    glm::vec2 baseToSpritePosition(glm::vec2 basePosition, EntityType type) const
    {
        return {basePosition.x, basePosition.y + (HIT_BOXES[type].y - HIT_BOXES[type].x) / 2.0f};
    }
    
    f32 calcRadiusOfEntity(EntityType type)
    {
        return HIT_BOXES[type].x / 2.0f;
    }
    
    void resolveCollision(glm::vec2 oldPos, glm::vec2& pos, u32 radius)
    {
        glm::uvec2 tile = getTileByPosition(pos);
        assert(tile.x > 0 && tile.y > 0);
        glm::vec2 normDirection = pos - oldPos;
        for (u32 i = tile.y - 1; i <= tile.y + 1; ++i)
        {
            for (u32 j = tile.x - 1; j <= tile.x + 1; ++j)
            {
                u32 tileIndex = i * map.size.x + j;
                if (map.tiles[tileIndex] >= TextureTag::COLLISION_START &&
                    map.tiles[tileIndex] <= TextureTag::COLLISION_END)
                {
                    glm::uvec2 tile{j,i};
                    glm::vec2 leftBottomCorner = getLeftBottomCornerByTile(tile);
                    glm::vec2 nearestPoint{
                        std::max(leftBottomCorner.x, std::min(pos.x, leftBottomCorner.x + map.quadSize.x)),
                        std::max(leftBottomCorner.y, std::min(pos.y, leftBottomCorner.y + map.quadSize.y))
                    };
                    glm::vec2 rayToNearest = nearestPoint - pos;
                    //assert((rayToNearest.x || rayToNearest.y) && "rayToNearest nu poate fi 0 deaorece va crapa la normalizare");
                    f32 overlap = radius - glm::length(rayToNearest);
                    if (std::isnan(overlap))
                        overlap = 0;
                    //assert(!std::isnan(overlap) && "Overlap e NaN, incearca sa egalezi cu 0");
                    if (overlap > 0)
                    {
                        if (glm::length(rayToNearest) != 0.0f)
                            pos -= glm::normalize(rayToNearest) * overlap;
                        else
                            pos -= glm::normalize(normDirection) * overlap;
                    }
                }
            }
        }
    }
    
    glm::vec2 Game::resolveBasePositionCollision(glm::vec2 oldPosition, glm::vec2 position, EntityType type)
    {
        glm::vec2 basePos = getBasePosition(position, type);
        resolveCollision(getBasePosition(oldPosition, type), basePos, calcRadiusOfEntity(type));
        return baseToSpritePosition(basePos, type);
    }
    
    bool checkFillTile(bool* mat, u32 tileIndex) const
    {
        // return !mat[tileIndex] && map.tiles[tileIndex] >= ID(WALL) && map.tiles[tileIndex] <= ID(WALL_CORNER_TOP_RIGHT);
        return map.tiles[tileIndex] >= TextureTag::WALL && map.tiles[tileIndex] <= TextureTag::WALL_CORNER_TOP_RIGHT;
    }
    
    bool fillUtilX(bool* mat, i32 tileX, i32 tileY)
    {
        assert(tileX >= 0 && tileX < map.size.x &&
               tileY >= 0 && tileY < map.size.y);
        bool ok = false;
        i32 j = tileX - 1;
        while (j >= 0 && checkFillTile(mat, tileY * map.size.x + j))
        {
            mat[tileY * map.size.x + j] = true;
            --j;
            ok = true;
        }
        ++tileX;
        while (tileX < map.size.x && checkFillTile(mat, tileY * map.size.x + tileX))
        {
            mat[tileY * map.size.x + tileX] = true;
            ++tileX;
            ok = true;
        }
        if (ok)
        {
            ++j;
            map.walls[map.numberOfWalls++] = {
                getLeftBottomCornerByTile({j, tileY}),
                getLeftBottomCornerByTile({tileX, tileY + 1})
            };
        }
        return ok;
    }
    
    bool fillUtilY(bool* mat, i32 tileX, i32 tileY)
    {
        assert(tileX >= 0 && tileX < map.size.x &&
               tileY >= 0 && tileY < map.size.y);
        bool ok = false;
        i32 i = tileY - 1;
        while (i >= 0 && checkFillTile(mat, i * map.size.x + tileX))
        {
            mat[i * map.size.x + tileX] = true;
            --i;
            ok = true;
        }
        ++tileY;
        while (tileY < map.size.y && checkFillTile(mat, tileY * map.size.x + tileX))
        {
            mat[tileY * map.size.x + tileX] = true;
            ++tileY;
            ok = true;
        }
        if (ok)
        {
            ++i;
            map.walls[map.numberOfWalls++] = {
                getLeftBottomCornerByTile({tileX, i}),
                getLeftBottomCornerByTile({tileX + 1, tileY})
            };
        }
        return ok;
    }
    
    void fill()
    {
        bool* mat = (bool*)ENGINE.globalAlloc(sizeof(bool) * map.size.x * map.size.y);
        memset(mat, false, sizeof(bool) * map.size.x * map.size.y);
        for (i32 i = 0; i < map.size.y; ++i)
        {
            for (i32 j = 0; j < map.size.x; ++j)
            {
                i32 tileIndex = i * map.size.x + j;
                if (!mat[tileIndex] && checkFillTile(mat, tileIndex)) {
                    mat[tileIndex] = true;
                    glm::uvec2* idkTile =  map.tilesArr;
                    if (!fillUtilX(mat, j, i) && !fillUtilY(mat, j, i))
                    {
                        u32 idk2 = map.numberOfWalls;
                        TextureTag idkTag =  map.tiles[tileIndex];
                        map.walls[map.numberOfWalls++] = {
                            getLeftBottomCornerByTile({j, i}),
                            getLeftBottomCornerByTile({j + 1, i + 1})
                        };
                    }
                }
            }
        }
        ENGINE.globalFree(mat);
    }
    
    void initMap(const u32 sizeX, const u32 sizeY)
    {
        map.size.x = sizeX;
        map.size.y = sizeY;
        map.tiles = (TextureTag*)ENGINE.globalAlloc(sizeof(TextureTag) * sizeY * sizeX);
        map.walls = (Map::Wall*)ENGINE.globalAlloc(sizeof(Map::Wall) * MAX_WALLS);
        map.numberOfWalls = 0;
        for (u32 i = 0; i < sizeY * sizeX; ++i)
        {
            map.tiles[i] = TextureTag::NONE;
        }
        map.quadSize = {QUAD_SIZE, QUAD_SIZE};
#ifdef PROCEDURAL_MAP_GENERATION
        generateMapLevel();
#else
        loadMapFromFile();
#endif
        fill();
        glm::vec2* vertexArr;
        u32 vertexArrSize;
        createPolygonOfWalkableSurface(vertexArr, vertexArrSize);
        // logInfo("vertexArrSize: %u", vertexArrSize);
        
        for (u32 i = 0; i < vertexArrSize; ++i)
        {
            glm::uvec2 tile = getTileByPosition(vertexArr[i]);
            map.tiles[tile.y * map.size.x + tile.x] = TextureTag::FLOOR_HOLE;
            logInfo("(%u,%u)", tile.x, tile.y);
        }
        triangulatePolygon(vertexArr, vertexArrSize);
    }
    
    f32 sign(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3)
    {
        return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
    }
    
    bool isPointInsideTriangle(glm::vec2 pt, glm::vec2 v1, glm::vec2 v2, glm::vec2 v3)
    {
        f32 d1 = sign(pt, v1, v2);
        f32 d2 = sign(pt, v2, v3);
        f32 d3 = sign(pt, v3, v1);
        
        bool has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
        bool has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);
        
        return !(has_neg && has_pos);
    }
    
    bool isAngleReflexive(glm::vec2 a, glm::vec2 b, glm::vec2 c)
    {
        f32 val = (b.y - a.y) * (c.x - b.x) -
        (b.x - a.x) * (c.y - b.y);
        return val < 0.0f;
    }
    
    bool isClockwise(glm::vec2 a, glm::vec2 b, glm::vec2 c)
    {
        f32 val = (b.x - a.x) * (c.y - a.y) -
        (c.x - a.x) * (b.y - a.y);
        return val < 0.0f;
    }
    
    bool isPointInsidePolygon(glm::vec2 point, Polygon poly)
    {
        glm::vec2& a = poly.vertices[0];
        for (u32 i = 1; i + 1 < poly.vertexCount; ++i)
        {
            glm::vec2& b = poly.vertices[i];
            glm::vec2& c = poly.vertices[i + 1];
            if (isPointInsideTriangle(point, a, b, c))
            {
                return true;
            }
        }
        return false;
    }
    
    i32 clampIndex(i32 ind, i32 left, i32 right)
    {
        if (ind < left)
            return right;
        if (ind > right)
            return left;
        return ind;
    }
    
    f32 computeAngle(glm::vec2 a, glm::vec2 b, glm::vec2 c)
    {
        glm::vec2 v1{
            b.x - a.x,
            b.y - a.y
        };
        glm::vec2 v2{
            b.x - c.x,
            b.y - c.y
        };
        return std::acos(glm::dot(v1, v2) / (glm::length(v1) * glm::length(v2)));
    }
    
    void triangulatePolygon(glm::vec2* points, u32 pointCount)
    {
        u32 trianglesCount = 0;
        Polygon* triangles = (Polygon*)ENGINE.temporaryAlloc(10000 * sizeof(Polygon));
        
        struct Node
        {
            glm::vec2 vertex;
            Node* prev;
            Node* next;
        };
        
        struct CircularList
        {
            Node* head;
            u32 nodeCount;
            
            void addNode(glm::vec2 vertex)
            {
                if(!head)
                {
                    head = (Node*)ENGINE.temporaryAlloc(sizeof(Node));
                    head->vertex = vertex;
                    head->prev = head;
                    head->next = head;
                } else
                {
                    Node* temp = (Node*)ENGINE.temporaryAlloc(sizeof(Node));
                    
                    temp->vertex = vertex;
                    temp->prev = head->prev;
                    temp->next = head;
                    
                    head->prev->next = temp;
                    head->prev = temp;
                }
                
                nodeCount++;
            }
            
            void removeNode(Node* node)
            {
                node->prev->next = node->next;
                node->next->prev = node->prev;
                nodeCount--;
                
                if(!nodeCount)
                {
                    head = nullptr;
                } else if(head == node)
                {
                    head = node->next;
                }
            }
        };
        
        CircularList list = {};
        
        for(u32 i = 0; i < pointCount; i++)
        {
            list.addNode(points[i]);
        }
        
        Node* ptr = list.head;
        
        for(; list.nodeCount > 2; ptr = ptr->next)
        {
            if(!isClockwise(ptr->prev->vertex, ptr->vertex, ptr->next->vertex))
            {
                bool shouldBreak = false;
                for(u32 j = 0; j < pointCount; j++)
                {
                    if(points[j] == ptr->prev->vertex || 
                       points[j] == ptr->next->vertex ||
                       points[j] == ptr->vertex)
                        continue;
                    
                    if(isPointInsideTriangle(points[j], ptr->prev->vertex, ptr->vertex, ptr->next->vertex))
                    {
                        logInfo("points[j] = (%f, %f)", points[j].x, points[j].y);
                        logInfo("prev = (%f, %f)", ptr->prev->vertex.x, ptr->prev->vertex.y);
                        logInfo("this = (%f, %f)", ptr->vertex.x, ptr->vertex.y);
                        logInfo("next = (%f, %f)", ptr->next->vertex.x, ptr->next->vertex.y);
                        shouldBreak = true;
                        break;
                    }
                }
                
                if(!shouldBreak)
                {
                    glm::vec2* triangle = (glm::vec2*)ENGINE.globalAlloc(sizeof(glm::vec2) * 3);
                    
                    triangle[0] = ptr->prev->vertex;
                    triangle[1] = ptr->vertex;
                    triangle[2] = ptr->next->vertex;
                    
                    triangles[trianglesCount].vertices = triangle; 
                    triangles[trianglesCount].vertexCount = 3; 
                    
                    trianglesCount++;
                    
                    list.removeNode(ptr);
                }
            }
        }
        
        map.navMesh.polygons = (Polygon*)ENGINE.globalAlloc(trianglesCount * sizeof(Polygon));
        map.navMesh.polygonCount = trianglesCount;
        memcpy(map.navMesh.polygons, triangles, trianglesCount * sizeof(Polygon));
        
        // DEBUG PART
        logInfo("Triangles:");
        for (u32 i = 0; i < map.navMesh.polygonCount; ++i)
        {
            Polygon& poly = map.navMesh.polygons[i];
            glm::uvec2 tile[3];
            assert(poly.vertexCount == 3);
            for (u32 j = 0; j < poly.vertexCount; ++j)
            {
                tile[j] = getTileByPosition(poly.vertices[j]);
            }
            logInfo("{(%u,%u), (%u,%u), (%u,%u)}",
                    tile[0].x,
                    tile[0].y,
                    tile[1].x,
                    tile[1].y,
                    tile[2].x,
                    tile[2].y
                    );
        }
    }
    
    glm::vec2 getCenterPositionByTile(glm::uvec2 tile) const
    {
        return glm::vec2((f32)tile.x, (f32)tile.y) * map.quadSize + map.quadSize / 2.0f;
    }
    
    glm::vec2 getLeftBottomCornerByTile(glm::uvec2 tile) const
    {
        return glm::vec2((f32)tile.x, (f32)tile.y) * map.quadSize;
    }
    
    void renderMap() const
    {
        u32 idk = map.tilesArrSize;
        for (u32 i = 0; i < map.tilesArrSize; ++i)
        {
            glm::uvec2* t = &map.tilesArr[i];
            glm::uvec2 tile = map.tilesArr[i];
            glm::vec2 position = getCenterPositionByTile(tile);
            u32 tileIndex = tile.y * map.size.x + tile.x;
            ENGINE.drawTexturedQuad(position, map.quadSize, getTextureIdByTag(map.tiles[tileIndex]), 0);
        }
        for (u32 i = 0; i < map.numberOfWalls; ++i)
        {
            glm::vec2 center = (map.walls[i].leftBottom + map.walls[i].topRight) / 2.0f;
            glm::vec2 size = map.walls[i].topRight - map.walls[i].leftBottom;
            
            ENGINE.addLightBlocker(center, size);
            // ENGINE.drawTexturedQuad(center, size, ID(DEBUG_OVERLAY), 0);
        }
        
    }
    
    glm::vec2 getQuadSize() const
    {
        return map.quadSize;
    }
    
    TextureTag randomFloor(u64& seed)
    {
        return (TextureTag)(TAG(FLOOR_1) + ENGINE.randomRangeU64(seed, 0, 2));
    }
    
    void generateMapLevel()
    {
        assert(map.size.x >= ROOM.MAX.x && map.size.y >= ROOM.MAX.y);
        
        if (ROOM.MIN.x % 2)
        {
            ROOM.MIN.x += 1;
        }
        
        if (ROOM.MAX.x % 2)
        {
            ROOM.MAX.x += 1;
        }
        
        if (ROOM.MIN.y % 2)
        {
            ROOM.MIN.y -= 1;
        }
        
        if (ROOM.MAX.y % 2)
        {
            ROOM.MAX.y -= 1;
        }
        
        assert(ROOM.MIN.x <= ROOM.MAX.x && ROOM.MIN.y <= ROOM.MAX.y);
        
        // transform to constexpr
        const glm::uvec2 ROOM_AVERAGE{
            (ROOM.MIN.x + ROOM.MAX.x) / 2,
            (ROOM.MIN.y + ROOM.MAX.y) / 2
        };
        
        u32 leftVerticalAxis = 1;
        u32 rightVerticalAxis = map.size.x - 1;
        map.roomCount = 0;
        for (u32 prevYPos = map.size.y / 2 - ROOM_AVERAGE.y, prevWidth, prevHeight, prevLeftVerticalAxis, roomRight; ; ++map.roomCount)
        {
            u32 height = (u32)ENGINE.randomRangeU64(seed, ROOM.MIN.y, ROOM.MAX.y) & (~1u);
            u32 width = (u32)ENGINE.randomRangeU64(seed, ROOM.MIN.x, ROOM.MAX.x) & (~1u);
            
            if (leftVerticalAxis + width > rightVerticalAxis)
            {
                // // right
                // map.walls[map.numberOfWalls++] = {
                //     getLeftBottomCornerByTile({prevLeftVerticalAxis + prevWidth - 1, prevYPos}),
                //     getLeftBottomCornerByTile({prevLeftVerticalAxis + prevWidth, prevYPos + prevHeight})
                // };
                break;
            }
            
            const u32 yPos = (u32)ENGINE.randomRangeU64(seed, std::max(0, (i32)prevYPos - (i32)ROOM.MIN.y), std::min(map.size.y - height, prevYPos + ROOM.MIN.y));
            for (u32 i = 1; i + 1 < height; ++i)
            {
                for (u32 j = 1; j + 1 < width; ++j)
                {
                    map.tiles[leftVerticalAxis + (yPos + i) * map.size.x + j] = randomFloor(seed);
                }
            }
            glm::uvec2 tileLeftBottom{ leftVerticalAxis + 1, yPos + 1 };
            glm::uvec2 tileRightTop{ leftVerticalAxis + width, yPos + height };
            glm::vec2 cornerLeftBottom = getLeftBottomCornerByTile(tileLeftBottom);
            glm::vec2 cornerRightTop = getLeftBottomCornerByTile(tileRightTop);
            map.rooms[map.roomCount].center = (cornerLeftBottom + cornerRightTop) / 2.0f;
            map.rooms[map.roomCount].size = cornerRightTop - cornerLeftBottom;
            // WALLS
            map.tiles[leftVerticalAxis + yPos * map.size.x] =  TextureTag::WALL_LEFT;
            map.tiles[leftVerticalAxis + yPos * map.size.x + width - 1] = TextureTag::WALL_LEFT;
            map.tiles[leftVerticalAxis + (yPos + height - 1) * map.size.x] = TextureTag::WALL_LEFT;
            map.tiles[leftVerticalAxis + (yPos + height - 1) * map.size.x + width - 1] = TextureTag::WALL_LEFT;
            
            
            for (u32 i = 1; i + 1 < height; ++i)
            {
                map.tiles[leftVerticalAxis + (yPos + i) * map.size.x] = TextureTag::WALL_LEFT;
                map.tiles[leftVerticalAxis + (yPos + i) * map.size.x + width - 1] = TextureTag::WALL_LEFT;
            }
            
            for (u32 j = 1; j + 1 < width; ++j)
            {
                map.tiles[leftVerticalAxis + yPos * map.size.x + j] = TextureTag::WALL_LEFT;
                map.tiles[leftVerticalAxis + (yPos + height - 1) * map.size.x + j] = TextureTag::WALL_LEFT;
            }
            
            const u32 prevRoomRight = roomRight;
            roomRight = leftVerticalAxis + width - 1;
            prevLeftVerticalAxis = leftVerticalAxis;
            leftVerticalAxis += width + (u32)ENGINE.randomRangeU64(seed, ROOM.MIN.x / 2, std::max(ROOM.MIN.x, ROOM.MAX.x / 2)) & (~1u);
            
            if (map.roomCount != 0)
            {
                // MIDDLE TUNNEL
                u32 startY = prevYPos + prevHeight / 2;
                u32 stopY = yPos + height / 2;
                if (startY >= stopY)
                    std::swap(startY, stopY);
                
                startY -= 2;
                stopY += 1;
                
                u32 baseX = (prevRoomRight + prevLeftVerticalAxis) / 2 + 1;
                
                map.tiles[startY * map.size.x + baseX - 2] = TextureTag::WALL_LEFT;
                map.tiles[startY * map.size.x + baseX - 1] = TextureTag::WALL_LEFT;
                map.tiles[startY * map.size.x + baseX]     = TextureTag::WALL_LEFT;
                map.tiles[startY * map.size.x + baseX + 1] = TextureTag::WALL_LEFT;
                for (u32 i = startY + 1; i < stopY; ++i)
                {
                    map.tiles[i * map.size.x + baseX - 2] = TextureTag::WALL_LEFT;
                    map.tiles[i * map.size.x + baseX - 1] = randomFloor(seed);
                    map.tiles[i * map.size.x + baseX]     = randomFloor(seed);
                    map.tiles[i * map.size.x + baseX + 1] = TextureTag::WALL_LEFT;
                }
                
                map.tiles[stopY * map.size.x + baseX - 2] = TextureTag::WALL_LEFT;
                map.tiles[stopY * map.size.x + baseX - 1] = TextureTag::WALL_LEFT;
                map.tiles[stopY * map.size.x + baseX]     = TextureTag::WALL_LEFT;
                map.tiles[stopY * map.size.x + baseX + 1] = TextureTag::WALL_LEFT;
                
                // LEFT TUNNEL
                u32 startX = prevRoomRight;
                u32 stopX = (prevRoomRight + prevLeftVerticalAxis) / 2 - 1;
                u32 baseY = prevYPos + prevHeight / 2;
                // entrance - left room
                glm::uvec2 tileLeftBottomEntrance{baseY - 1, startX };
                glm::uvec2 tileLeftTopEntrance{ baseY + 1, startX + 1 };
                // left end of tunnel, right entrance of previous room
                map.rooms[map.roomCount - 1].entranceRightBottom = getCenterPositionByTile({tileLeftBottomEntrance.y, tileLeftBottomEntrance.x});
                map.rooms[map.roomCount - 1].entranceRightTop = getCenterPositionByTile({tileLeftTopEntrance.y - 1, tileLeftTopEntrance.x - 1});
                
                map.tiles[(baseY - 2) * map.size.x + startX] = TextureTag::WALL_LEFT;
                //map.walls[map.numberOfWalls++] = getLeftBottomCornerByTile({startX, baseY - 2});
                map.tiles[(baseY - 1) * map.size.x + startX] = randomFloor(seed);
                map.tiles[ baseY      * map.size.x + startX] = randomFloor(seed);
                map.tiles[(baseY + 1) * map.size.x + startX] = TextureTag::WALL_LEFT;
                // left tunnel
                for (u32 i = startX + 1; i < stopX; ++i)
                {
                    map.tiles[(baseY - 2) * map.size.x + i] = TextureTag::WALL_LEFT;
                    map.tiles[(baseY - 1) * map.size.x + i] = randomFloor(seed);
                    map.tiles[ baseY      * map.size.x + i] = randomFloor(seed);
                    map.tiles[(baseY + 1) * map.size.x + i] = TextureTag::WALL_LEFT;
                }
                // end of left tunnel
                map.tiles[(baseY - 1) * map.size.x + stopX] = randomFloor(seed);
                map.tiles[ baseY      * map.size.x + stopX] = randomFloor(seed);
                
                // RIGHT TUNNEL
                baseY = yPos + height / 2;
                startX = (prevRoomRight + prevLeftVerticalAxis) / 2 + 3;
                stopX = prevLeftVerticalAxis;
                // entrance - right room
                glm::uvec2 tileRightBottomEntrance{baseY - 1, stopX };
                glm::uvec2 tileRightTopEntrance{ baseY + 1, stopX + 1 };
                // right end of tunnel, left entrance of current room
                map.rooms[map.roomCount].entranceLeftBottom = getCenterPositionByTile({tileRightBottomEntrance.y, tileRightBottomEntrance.x});
                map.rooms[map.roomCount].entranceLeftTop = getCenterPositionByTile({tileRightTopEntrance.y - 1, tileRightTopEntrance.x - 1});
                
                map.tiles[(baseY - 2) * map.size.x + stopX] = TextureTag::WALL_LEFT;
                map.tiles[(baseY - 1) * map.size.x + stopX] = randomFloor(seed);
                map.tiles[ baseY      * map.size.x + stopX] = randomFloor(seed);
                map.tiles[(baseY + 1) * map.size.x + stopX] = TextureTag::WALL_LEFT;
                // right tunnel
                for (u32 i = startX; i < stopX; ++i)
                {
                    map.tiles[(baseY - 2) * map.size.x + i] = TextureTag::WALL_LEFT;
                    map.tiles[(baseY - 1) * map.size.x + i] = randomFloor(seed);
                    map.tiles[ baseY      * map.size.x + i] = randomFloor(seed);
                    map.tiles[(baseY + 1) * map.size.x + i] = TextureTag::WALL_LEFT;
                }
                // end of right tunnel
                map.tiles[(baseY - 1) * map.size.x + startX - 1] = randomFloor(seed);
                map.tiles[ baseY      * map.size.x + startX - 1] = randomFloor(seed);
            }
            else
            {
                glm::vec2 cornerLeftBottom = glm::vec2((f32)prevLeftVerticalAxis, (f32)yPos);
                cornerLeftBottom *= map.quadSize;
                startPosition = playerBaseToSpritePosition(cornerLeftBottom + map.quadSize * glm::vec2{(f32)width / 2.0f, (f32)height / 2.0f});
            }
            
            prevWidth = width;
            prevHeight = height;
            prevYPos = yPos;
        }
        initMapTilesArr();
    }
    
    void initMapTilesArr()
    {
        map.tilesArrSize = 0;
        for (u32 i = 0; i < map.size.y; ++i)
        {
            for (u32 j = 0; j < map.size.x; ++j)
            {
                if (map.tiles[i * map.size.x + j] != TextureTag::NONE)
                    ++map.tilesArrSize;
            }
        }
        map.tilesArr = (glm::uvec2*)ENGINE.globalAlloc(map.tilesArrSize * sizeof(glm::uvec2));
        glm::uvec2* debug = map.tilesArr;
        u32 cnt = 0;
        for (u32 i = 0; i < map.size.y; ++i)
        {
            for (u32 j = 0; j < map.size.x; ++j)
            {
                if (map.tiles[i * map.size.x + j] != TextureTag::NONE)
                    map.tilesArr[cnt++] = glm::uvec2{j,i};
            }
        }
    }
    
    void loadMapFromFile(const char* path="map.txt")
    {
        map.size = {200,100};
        u32 bufferSize = (map.size.x + 2) * map.size.y + 5;
        char* buffer = (char*)ENGINE.temporaryAlloc(bufferSize);
        memset(buffer, 0, bufferSize);
        ENGINE.readWholeFile(buffer, bufferSize, path);
        
        for (u32 i = 0; i < bufferSize; ++i)
        {
            if (buffer[i] == '\n' || buffer[i] == '\r')
            {
                memmove(buffer + i, buffer + i + 1, bufferSize - i - 1);
                --bufferSize;
                --i;
            }
        }
        for (u32 i = 0; i < bufferSize; ++i)
        {
            assert(buffer[i] != '\n' && buffer[i] != '\r');
        }
        for (u32 i = 0; i < map.size.y; ++i)
        {
            for (u32 j = 0; j < map.size.x; ++j)
            {
                u32 x = j;
                u32 y = map.size.y - i - 1;
                glm::uvec2 tile = {x,y};
                u32 ind = y * map.size.x + x;
                u32 bufferInd = i * map.size.x + j;
                switch (buffer[bufferInd])
                {
                    case '.':
                    map.tiles[ind] = TextureTag::NONE;
                    break;
                    
                    case 'X':
                    map.tiles[ind] = TextureTag::WALL_LEFT;
                    break;
                    
                    case 'O':
                    AnimationTag tag;
                    EntityType type;
                    if(ENGINE.randomU64(seed) % 2 == 0)
                    {
                        type = BIG_DEMON; 
                        tag = BIG_DEMON_IDLE;
                    }
                    else
                    {
                        type = BIG_ZOMBIE;
                        tag = BIG_ZOMBIE_IDLE;
                    }
                    addEntity(getCenterPositionByTile(tile), type, tag);
                    map.tiles[ind] = randomFloor(seed);
                    break;
                    
                    case '@':
                    startPosition = playerBaseToSpritePosition(getCenterPositionByTile(tile));
                    case '_':
                    map.tiles[ind] = randomFloor(seed);
                    break;
                }
            }
        }
        initMapTilesArr();
    }
    
    bool isFloor(TextureTag tag)
    {
        return tag >= TextureTag::FLOOR_START && tag <= TextureTag::FLOOR_END;
    }
    
    bool isWall(TextureTag tag)
    {
        return tag >= TextureTag::WALL_START && tag <= TextureTag::WALL_END;
    }
    
    void createPolygonOfWalkableSurface(glm::vec2*& vertexArr, u32& vertexArrSize)
    {
        constexpr glm::ivec2 dirIfLastRight[]
        {
            {0, -1}, // Down
            {+1, 0}, // Right
            {0, +1}, // Up
            {-1, 0}, // Left
        };
        constexpr char dirCharIfLastRight[]
        {
            'D',
            'R',
            'U',
            'L',
        };
        
        constexpr glm::ivec2 dirIfLastUp[]
        {
            {+1, 0}, // Right
            {0, +1}, // Up
            {-1, 0}, // Left
            {0, -1}, // Down
        };
        constexpr char dirCharIfLastUp[]
        {
            'R',
            'U',
            'L',
            'D',
        };
        
        constexpr glm::ivec2 dirIfLastLeft[]
        {
            {0, +1}, // Up
            {-1, 0}, // Left
            {0, -1}, // Down
            {+1, 0}, // Right
        };
        constexpr char dirCharIfLastLeft[]
        {
            'U',
            'L',
            'D',
            'R',
        };
        
        constexpr glm::ivec2 dirIfLastDown[]
        {
            {-1, 0}, // Left
            {0, -1}, // Down
            {+1, 0}, // Right
            {0, +1}, // Up
        };
        constexpr char dirCharIfLastDown[]
        {
            'L',
            'D',
            'R',
            'U',
        };
        constexpr u32 dirSize = ARRAY_COUNT(dirIfLastRight);
        
        printf("\n");
        for (u32 y = 0; y < map.size.y; ++y)
        {
            for (u32 x = 0; x < map.size.x; ++x)
            {
                u32 ind = (map.size.y - y - 1) * map.size.x + x;
                if (isFloor(map.tiles[ind]))
                {
                    printf("%c", '_');
                }
                else if (isWall(map.tiles[ind]))
                {
                    printf("%c", 'X');
                }
                else
                {
                    printf("%c", '.');
                }
            }
            printf("\n");
        }
        printf("\n");
        
        glm::uvec2 startPos;
        for (u32 y = 0; y < map.size.y; ++y)
        {
            for (u32 x = 0; x < map.size.x; ++x)
            {
                u32 ind = y * map.size.x + x;
                if (isFloor(map.tiles[ind]))
                {
                    startPos = {x,y};
                    goto LOOP_END;
                }
            }
        }
        LOOP_END:
        vertexArrSize = 0;
        vertexArr = (glm::vec2*)ENGINE.temporaryAlloc(10000 * sizeof(glm::vec2));
        {
            glm::uvec2 pos = startPos;
            vertexArr[vertexArrSize++] = getCenterPositionByTile(pos);
            // logInfo("(%u, %u)", pos.x, pos.y);
            // make sure lastDir is Right (we iterate the matrix bottom-up and left-right)
            char lastDir = 'R';
            do
            {
                assert(
                       pos.x > 1 &&
                       pos.y > 1 &&
                       pos.x < map.size.x - 2 &&
                       pos.y < map.size.y - 2
                       );
                
                const glm::ivec2* dir;
                const char* dirChar;
                switch (lastDir)
                {
                    case 'R':
                    dir = dirIfLastRight;
                    dirChar = dirCharIfLastRight;
                    break;
                    
                    case 'U':
                    dir = dirIfLastUp;
                    dirChar = dirCharIfLastUp;
                    break;
                    
                    case 'L':
                    dir = dirIfLastLeft;
                    dirChar = dirCharIfLastLeft;
                    break;
                    
                    case 'D':
                    dir = dirIfLastDown;
                    dirChar = dirCharIfLastDown;
                    break;
                }
                
                for (u32 i = 0; i < dirSize; ++i)
                {
                    glm::uvec2 newPos = (glm::ivec2)pos + dir[i];
                    u32 tileInd = newPos.y * map.size.x + newPos.x;
                    if (isFloor(map.tiles[tileInd]))
                    {
                        // logInfo("(%u, %u)", pos.x, pos.y);
                        if (lastDir != dirChar[i]) // we change direction
                        {
                            // get the center of the tile (take into account the collision with the wall)
                            glm::vec2 realPos = getCenterPositionByTile(pos);
                            vertexArr[vertexArrSize++] = realPos;
                        }
                        pos = newPos;
                        lastDir = dirChar[i];
                        break;
                    }
                }
            }
            while (pos != startPos);
        } 
    }
    
    bool menuButton(glm::vec2 pos, const char* text)
    {
        buttonSize = {
            ENGINE.getScreenSize().x / 3.0f,
            ENGINE.getScreenSize().y / 6.5f
        };
        return ENGINE.uiButtonHoverText(pos,
                                        buttonSize,
                                        AnchorPoint::C,
                                        ID(BUTTON),
                                        ID(BUTTON_PRESSED),
                                        1000.0f,
                                        text) &&
            actionState.menuInteract == KeyState::PRESS;
    }
    
    void linkAnimationByTag(AnimationTag tag)
    {
        TextureId* resultedArr = nullptr;
        u32 frameCount = 0;
        switch (tag)
        {
            case ELF_M_HIT:
            {
                CREATE_ANIMATION_ARRAY(ELF_M_HIT);
            }
            
            case ELF_M_IDLE:
            {
                CREATE_ANIMATION_ARRAY(ELF_M_IDLE);
            }
            
            case ELF_M_RUN:
            {
                CREATE_ANIMATION_ARRAY(ELF_M_RUN);
            }
            
            case ELF_F_HIT:
            {
                CREATE_ANIMATION_ARRAY(ELF_F_HIT);
            }
            
            case ELF_F_IDLE:
            CREATE_ANIMATION_ARRAY(ELF_F_IDLE);
            
            case ELF_F_RUN:
            {
                CREATE_ANIMATION_ARRAY(ELF_F_RUN);
            }
            
            case BIG_DEMON_IDLE:
            {
                CREATE_ANIMATION_ARRAY(BIG_DEMON_IDLE);
            }
            
            case BIG_DEMON_RUN:
            {
                CREATE_ANIMATION_ARRAY(BIG_DEMON_RUN);
            }
            
            case BIG_ZOMBIE_HIT:
            {
                CREATE_ANIMATION_ARRAY(BIG_ZOMBIE_HIT);
            }
            
            case BIG_ZOMBIE_IDLE:
            {
                CREATE_ANIMATION_ARRAY(BIG_ZOMBIE_IDLE);
            }
            
            case BIG_ZOMBIE_RUN:
            {
                CREATE_ANIMATION_ARRAY(BIG_ZOMBIE_RUN);
            }
            
            case CHEST_EMPTY_OPEN:
            {
                CREATE_ANIMATION_ARRAY(CHEST_EMPTY_OPEN);
            }
            
            case CHEST_FULL_OPEN:
            {
                CREATE_ANIMATION_ARRAY(CHEST_FULL_OPEN);
            }
            
            case CHEST_MIMIC_OPEN:
            {
                CREATE_ANIMATION_ARRAY(CHEST_MIMIC_OPEN);
            }
            
            case SWORD:
            {
                CREATE_ANIMATION_ARRAY(SWORD);
            }
            
            case SHIELD:
            {
                CREATE_ANIMATION_ARRAY(SHIELD);
            }
            
            case KNIFE:
            {
                CREATE_ANIMATION_ARRAY(KNIFE);
            }
            
            case FORK:
            {
                CREATE_ANIMATION_ARRAY(FORK);
            }
        }
        ENGINE.createAnimation(tag, resultedArr, frameCount, ANIMATION_DURATION[tag] * ANIMATIONS_MULTIPLIER);
    }
};

extern "C"
__declspec(dllexport)
void gameInit() {
    GAME->gameInit();
}

extern "C"
__declspec(dllexport)
void gameInput() {
    GAME->gameInput();
}

extern "C"
__declspec(dllexport)
void gameUpdate(f64& deltaTime) {
    GAME->deltaTime = deltaTime;
    GAME->gameUpdate();
    deltaTime = GAME->deltaTime;
}

extern "C"
__declspec(dllexport)
void gameRender(f64 deltaTime) {
    GAME->gameRender();
}
