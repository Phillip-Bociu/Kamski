#pragma once

#include "Defines.h"

using EntityComponent = EntityStats;

struct TransformComponent
{
    glm::vec2 position;
    glm::vec2 size;
    f32 rotation;
};

struct TypeComponent
{
    EntityType entityType;
};

struct ColliderComponent
{
    glm::vec2 hitBox;
};

struct SpriteComponent
{
    AnimationTag animation;
    f64 startTime;
    f64 endTime;
};

struct SolidColorComponent
{
    glm::vec4 color;
};

struct EnemyComponent
{
    enum Phase
    {
        WALK,
        SHOOT,
        WAIT
    }phase;
    f32 startTime;
};

struct VelocityComponent
{
    glm::vec2 targetVel;
    glm::vec2 vel;
};

struct FollowComponent
{
    Entity toFollowId;
    glm::vec2 followOffset;
};

struct ProjectileComponent
{
    glm::vec2 direction;
    f32 speed;
    f32 damage;
    bool isEnemy;
};

struct HealthBarComponent
{
    f32 maxSize;
    f32 maxHealth;
};

struct ItemComponent
{
    // Weapon, Armor, Utility
    ItemType itemType;
    // Bow sword, shield, helmet, etc.
    ItemBit itemId;
};
