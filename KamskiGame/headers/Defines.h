#pragma once

#include <KamskiEngine/engine/deps/glm/glm.hpp>
#include "KamskiEngine/KamskiApi.h"

class Game;
inline Game* GAME;

using TextureId = u32;
using ItemBit = u32;

inline constexpr u32 MAP_SIZE = 300;
inline constexpr u32 MAP_SIZE_Y = MAP_SIZE;
inline constexpr u32 MAP_SIZE_X = MAP_SIZE;
// transform this to constexpr
inline struct {
    glm::uvec2 MIN{20, 20};
    glm::uvec2 MAX{30, 30};
} ROOM;
inline constexpr u32 MAX_WALLS = 100000;
#ifdef KAMSKI_DEBUG
inline constexpr f32 DEFAULT_CAMERA_ZOOM = 3.0f;
#else
inline constexpr f32 DEFAULT_CAMERA_ZOOM = 7.0f;
#endif
inline constexpr f32 DEFAULT_PLAYER_SPEED = 75.0f;
inline constexpr f32 DEFAULT_ENEMY_SPEED = 40.0f;
inline constexpr f32 ENEMY_DETECTION_RADIUS = 180.0f;
inline constexpr f32 QUAD_SIZE = 16.0f;
inline constexpr f32 HEALTH_BAR_HEIGHT = 2.0f;
inline constexpr f32 HEALTH_BAR_HEIGHT_OFFSET = 5.0f;
inline constexpr f32 ANIMATIONS_MULTIPLIER = 1.0f;
inline constexpr f32 ENTITIES_SIZE_MULTIPLIER = 1.0f;
//TODO: create a math library
inline constexpr f32 PI = 3.14159265f;


/* ENUMS START */
enum class TextureTag: u32
{
    NONE,

#ifdef KAMSKI_DEBUG
    // DEBUG
    CIRCLE,
    DEBUG_OVERLAY,
#endif
    
    HEALTH_POTION,
    STAMINA_POTION,
    MANA_POTION,
    
    // UI
    BUTTON,
    BUTTON_PRESSED,
    MOUSE_POINTER,
    STATS_UI,
    ITEM_HOLDER_START,
    ITEM_HOLDER_END,
    ITEM_HOLDER,
    ITEM_HOLDER_BACKGROUND,
    ITEM_HOLDER_INVENTORY,
    STATS_BACKGROUND,

    WEAPON_START,
    SWORD_0 = WEAPON_START,
    SWORD_FINAL = SWORD_0,
    SHIELD_0,
    SHIELD_FINAL = SHIELD_0,
    FORK_0,
    FORK_FINAL = FORK_0,
    KNIFE_0,
    KNIFE_FINAL = KNIFE_0,
    WEAPON_END = KNIFE_FINAL,

    /* MAP TILES, RATIO ALWAYS 1:1 SO ADD THEM ONLY AFTER THIS COMMENT */
    FLOOR_START,
    FLOOR_1 = FLOOR_START,
    FLOOR_2,
    FLOOR_3,
    FLOOR_4,
    FLOOR_5,
    FLOOR_6,
    FLOOR_7,
    FLOOR_8,
    FLOOR_HOLE,
    FLOOR_LADDER,
    FLOOR_END = FLOOR_LADDER,

    COLLISION_START,
    WALL_START = COLLISION_START,
    // WALL
    WALL = WALL_START,

    WALL_BANNER_BLUE,
    WALL_BANNER_GREEN,
    WALL_BANNER_RED,
    WALL_BANNER_YELLOW,

    WALL_COLUMN_MID,
    WALL_COLUMN_TOP,
    WALL_COLUMN_BASE,

    WALL_LEFT,
    WALL_MID,
    WALL_RIGHT,

    WALL_TOP_LEFT,
    WALL_TOP_MID,
    WALL_TOP_RIGHT,

    WALL_SIDE_FRONT_LEFT,
    WALL_SIDE_FRONT_RIGHT,
    WALL_SIDE_MID_LEFT,
    WALL_SIDE_MID_RIGHT,
    WALL_SIDE_TOP_LEFT,
    WALL_SIDE_TOP_RIGHT,

    WALL_CORNER_BOTTOM_LEFT,
    WALL_CORNER_BOTTOM_RIGHT,
    WALL_CORNER_FRONT_LEFT,
    WALL_CORNER_FRONT_RIGHT,
    WALL_CORNER_LEFT,
    WALL_CORNER_RIGHT,
    WALL_CORNER_TOP_LEFT,
    WALL_CORNER_TOP_RIGHT,

    WALL_INNER_CORNER_L_TOP_LEFT,
    WALL_INNER_CORNER_L_TOP_RIGHT,
    WALL_INNER_CORNER_MID_LEFT,
    WALL_INNER_CORNER_MID_RIGHT,
    WALL_INNER_CORNER_T_TOP_LEFT,
    WALL_INNER_CORNER_T_TOP_RIGHT,
    WALL_END = WALL_INNER_CORNER_T_TOP_RIGHT,
    COLLISION_END = WALL_END,

    // animations
    // ELF_M
    ELF_M_HIT_0,
    ELF_M_HIT_FINAL = ELF_M_HIT_0,

    ELF_M_IDLE_0,
    ELF_M_IDLE_1,
    ELF_M_IDLE_2,
    ELF_M_IDLE_3,
    ELF_M_IDLE_FINAL = ELF_M_IDLE_3,

    ELF_M_RUN_0,
    ELF_M_RUN_1,
    ELF_M_RUN_2,
    ELF_M_RUN_3,
    ELF_M_RUN_FINAL = ELF_M_RUN_3,

    // ELF_F
    ELF_F_HIT_0,
    ELF_F_HIT_FINAL = ELF_F_HIT_0,

    ELF_F_IDLE_0,
    ELF_F_IDLE_1,
    ELF_F_IDLE_2,
    ELF_F_IDLE_3,
    ELF_F_IDLE_FINAL = ELF_F_IDLE_3,

    ELF_F_RUN_0,
    ELF_F_RUN_1,
    ELF_F_RUN_2,
    ELF_F_RUN_3,
    ELF_F_RUN_FINAL = ELF_F_RUN_3,

    // BIG DEMON
    BIG_DEMON_IDLE_0,
    BIG_DEMON_IDLE_1,
    BIG_DEMON_IDLE_2,
    BIG_DEMON_IDLE_3,
    BIG_DEMON_IDLE_FINAL = BIG_DEMON_IDLE_3,

    BIG_DEMON_RUN_0,
    BIG_DEMON_RUN_1,
    BIG_DEMON_RUN_2,
    BIG_DEMON_RUN_3,
    BIG_DEMON_RUN_FINAL = BIG_DEMON_RUN_3,

    // BIG ZOMBIE
    BIG_ZOMBIE_HIT_0,
    BIG_ZOMBIE_HIT_FINAL = BIG_ZOMBIE_HIT_0,

    BIG_ZOMBIE_IDLE_0,
    BIG_ZOMBIE_IDLE_1,
    BIG_ZOMBIE_IDLE_2,
    BIG_ZOMBIE_IDLE_3,
    BIG_ZOMBIE_IDLE_FINAL = BIG_ZOMBIE_IDLE_3,

    BIG_ZOMBIE_RUN_0,
    BIG_ZOMBIE_RUN_1,
    BIG_ZOMBIE_RUN_2,
    BIG_ZOMBIE_RUN_3,
    BIG_ZOMBIE_RUN_FINAL = BIG_ZOMBIE_RUN_3,

    // CHEST
    CHEST_EMPTY_OPEN_0,
    CHEST_EMPTY_OPEN_1,
    CHEST_EMPTY_OPEN_2,
    CHEST_EMPTY_OPEN_FINAL = CHEST_EMPTY_OPEN_2,

    CHEST_FULL_OPEN_0,
    CHEST_FULL_OPEN_1,
    CHEST_FULL_OPEN_2,
    CHEST_FULL_OPEN_FINAL = CHEST_FULL_OPEN_2,

    CHEST_MIMIC_OPEN_0,
    CHEST_MIMIC_OPEN_1,
    CHEST_MIMIC_OPEN_2,
    CHEST_MIMIC_OPEN_FINAL = CHEST_MIMIC_OPEN_2,

    // the end
    COUNT
};

enum EntityType: u32
{
    ELF_M,
    ELF_F,
    BIG_DEMON,
    BIG_ZOMBIE,
    ENTITY_TYPE_COUNT,
    ENTITY_TYPE_PLAYER = ELF_M
};

enum AnimationTag: u32
{
    ELF_M_HIT,
    ELF_M_IDLE,
    ELF_M_RUN,
    ELF_F_HIT,
    ELF_F_IDLE,
    ELF_F_RUN,
    BIG_DEMON_IDLE,
    BIG_DEMON_RUN,
    BIG_ZOMBIE_IDLE,
    BIG_ZOMBIE_RUN,
    BIG_ZOMBIE_HIT,
    CHEST_EMPTY_OPEN,
    CHEST_FULL_OPEN,
    CHEST_MIMIC_OPEN,

    ANIMATION_TAG_WEAPON_START,
    SWORD = ANIMATION_TAG_WEAPON_START,
    SHIELD,
    FORK,
    KNIFE,
    ANIMATION_TAG_WEAPON_END = KNIFE,

    ANIMATION_TAG_COUNT
};

enum AnimationState: u32
{
    IDLE,
    RUN,
    HIT,
    ANIMATION_STATE_COUNT
};

enum ItemType: u32
{
    ITEM_TYPE_WEAPON,
    ITEM_TYPE_ARMOUR,
    ITEM_TYPE_UTILITY
};

enum Weapon: u32
{
    WEAPON_SWORD,
    WEAPON_SHIELD,
    WEAPON_FORK,
    WEAPON_KNIFE,
    WEAPON_COUNT
};

enum Armour: u32
{
    ARMOUR_HELMET,
    ARMOUR_CHESTPLATE,
    ARMOUR_PANTS,
    ARMOUR_COUNT
};

enum Utility: u32
{
    UTILITY_GRENADE,
    UTILITY_POTION,
    UTILITY_ROPE,
    UTILITY_COUNT
};
/* ENUMS END */

/* STRUCTS START */
struct EntityStats
{
    f32 movementSpeed;
    f32 healthPoints;
    f32 attackPoints;
};

struct ItemSet
{
    u32 weapons;
    u32 armours;
    u32 utility;
};
/* STRUCTS END */

inline constexpr EntityStats ENTITIES_STATS[]{
    [ELF_M]      = {DEFAULT_PLAYER_SPEED, 200.0f, 50.0f},
    [ELF_F]      = {DEFAULT_PLAYER_SPEED, 200.0f, 50.0f},
    [BIG_DEMON]  = {DEFAULT_ENEMY_SPEED, 200.0f, 25.0f},
    [BIG_ZOMBIE] = {DEFAULT_ENEMY_SPEED, 200.0f, 25.0f}
};

inline constexpr glm::vec2 HIT_BOXES[]{
    [ELF_M]      = {12.0f, 19.0f},
    [ELF_F]      = {13.0f, 16.0f},
    [BIG_DEMON]  = {18.0f, 28.0f},
    [BIG_ZOMBIE] = {18.0f, 26.0f}
};

inline constexpr glm::vec2 HIT_BOXES_WEAPONS[]{
    [WEAPON_FORK] = {15.0f, 21.0f},
    [WEAPON_KNIFE] = {8.0f, 19.5f} 
};

inline constexpr glm::vec2 TEXTURE_SIZES[]{
    [ELF_M]      = {16.0f, 28.0f},
    [ELF_F]      = {15.0f, 18.0f},
    [BIG_DEMON]  = {32.0f, 36.0f},
    [BIG_ZOMBIE] = {32.0f, 34.0f}
};

inline constexpr glm::vec2 TEXTURE_SIZES_WEAPONS[]{
    [WEAPON_FORK] = {12.0f, 26.0f},
    [WEAPON_KNIFE] = {6.0f, 13.0f} 
};

inline constexpr f32 ANIMATION_DURATION[] = {
    [ELF_M_HIT]        = 0.2f,
    [ELF_M_IDLE]       = 0.8f,
    [ELF_M_RUN]        = 0.4f,
    [ELF_F_HIT]        = 0.3f,
    [ELF_F_IDLE]       = 0.5f,
    [ELF_F_RUN]        = 0.4f,
    [BIG_DEMON_IDLE]   = 0.4f,
    [BIG_DEMON_RUN]    = 0.4f,
    [BIG_ZOMBIE_IDLE]  = 0.4f,
    [BIG_ZOMBIE_RUN]   = 0.4f,
    [BIG_ZOMBIE_HIT]   = 0.3f,
    [CHEST_EMPTY_OPEN] = 0.5f,
    [CHEST_FULL_OPEN]  = 0.5f,
    [CHEST_MIMIC_OPEN] = 0.5f
};

// DO NOT SET THE COOLDOWN LONGER THAN THE DURATION OF ANIMATION
inline constexpr f32 ANIMATION_COOLDOWN[] = {
    [ELF_M_HIT]        = 0.2f,
    [ELF_M_IDLE]       = 0.0f,
    [ELF_M_RUN]        = 0.0f,
    [ELF_F_HIT]        = 0.0f,
    [ELF_F_IDLE]       = 0.0f,
    [ELF_F_RUN]        = 0.0f,
    [BIG_DEMON_IDLE]   = 0.0f,
    [BIG_DEMON_RUN]    = 0.0f,
    [BIG_ZOMBIE_IDLE]  = 0.0f,
    [BIG_ZOMBIE_RUN]   = 0.0f,
    [BIG_ZOMBIE_HIT]   = 0.3f,
    [CHEST_EMPTY_OPEN] = 0.0f,
    [CHEST_FULL_OPEN]  = 0.0f,
    [CHEST_MIMIC_OPEN] = 0.0f
};

enum States {
    MAIN_MENU,
    SETTINGS,
    GAME_START,
    GAME_RUNNING,
    GAME_PAUSED,
    GAME_LOST
};

// #define PROCEDURAL_MAP_GENERATION
#define KASMKI_MAX_ENTITY_COUNT 20000
#define KAMSKI_COMPONENTS TransformComponent, TypeComponent, ColliderComponent, SpriteComponent, SolidColorComponent, FollowComponent, EntityComponent, ProjectileComponent, HealthBarComponent, ItemComponent, VelocityComponent, EnemyComponent
#define ID(TAG) getTextureIdByTag(TextureTag::TAG)
#define TAG(TEXTURE) ((u32)TextureTag::TEXTURE)
#define ANIMATION_COUNT(ANIMATION) (TAG(ANIMATION##_FINAL) - TAG(ANIMATION##_0) + 1)

// always use this macro inside a {}
#define CREATE_ANIMATION_ARRAY(ANIMATION)\
    static TextureId arr[ANIMATION_COUNT(ANIMATION)];\
    for (u32 i = 0; i < ANIMATION_COUNT(ANIMATION); ++i) {\
        arr[i] = getTextureIdByTag(TAG(ANIMATION##_0) + i);\
    }\
    frameCount = ANIMATION_COUNT(ANIMATION);\
    resultedArr = arr;\
    break

/* TEXTURES */
// concatenating assets' dir with filename
#define TEXTURE_ASSET(filename) ("assets/"filename)

#define UI_ASSET(filename) TEXTURE_ASSET("ui/"filename)

#define ITEM_ASSET(filename) TEXTURE_ASSET("items/"filename)
#define WEAPON_ASSET(filename) ITEM_ASSET("weapons/"filename)

#define MAP_ASSET(filename) TEXTURE_ASSET("map/"filename)
#define WALL_ASSET(filename) MAP_ASSET("wall/"filename)
#define FLOOR_ASSET(filename) MAP_ASSET("floor/"filename)
#define CHEST_ASSET(filename) MAP_ASSET("chest/"filename)

#define MOB_ASSET(filename) TEXTURE_ASSET("mobs/"filename)

inline const char* TEXTURE_PATHS[] = {
    // [TAG(NONE)]                          = TEXTURE_ASSET("empty.png"),
    [TAG(NONE)]                          = TEXTURE_ASSET("debug.png"),
#ifdef KAMSKI_DEBUG
    [TAG(CIRCLE)]                        = TEXTURE_ASSET("circle.png"),
    [TAG(DEBUG_OVERLAY)]                 = TEXTURE_ASSET("debug_overlay.png"),
#endif
    [TAG(HEALTH_POTION)]                 = ITEM_ASSET("health_potion.png"),
    [TAG(STAMINA_POTION)]                = ITEM_ASSET("stamina_potion.png"),
    [TAG(MANA_POTION)]                   = ITEM_ASSET("mana_potion.png"),
    
    [TAG(BUTTON)]                        = UI_ASSET("button.png"),
    [TAG(BUTTON_PRESSED)]                = UI_ASSET("button_press.png"),
    [TAG(MOUSE_POINTER)]                 = UI_ASSET("mouse_pointer.png"),
    [TAG(STATS_UI)]                      = UI_ASSET("stats_ui.png"),
    [TAG(ITEM_HOLDER_START)]             = UI_ASSET("item_holder_start.png"),
    [TAG(ITEM_HOLDER_END)]               = UI_ASSET("item_holder_end.png"),
    [TAG(ITEM_HOLDER)]                   = UI_ASSET("item_holder.png"),
    [TAG(ITEM_HOLDER_BACKGROUND)]        = UI_ASSET("item_holder_background.png"),
    [TAG(ITEM_HOLDER_INVENTORY)]         = UI_ASSET("item_holder_inventory.png"),
    [TAG(STATS_BACKGROUND)]              = UI_ASSET("stats_background.png"),

    [TAG(SWORD_0)]                       = WEAPON_ASSET("weapon_sword.png"),
    [TAG(SHIELD_0)]                      = WEAPON_ASSET("weapon_shield.png"),
    [TAG(KNIFE_0)]                       = WEAPON_ASSET("weapon_knife.png"),
    [TAG(FORK_0)]                        = WEAPON_ASSET("weapon_fork.png"),

    [TAG(FLOOR_1)]                       = FLOOR_ASSET("floor_1.png"),
    [TAG(FLOOR_2)]                       = FLOOR_ASSET("floor_2.png"),
    [TAG(FLOOR_3)]                       = FLOOR_ASSET("floor_3.png"),
    [TAG(FLOOR_4)]                       = FLOOR_ASSET("floor_4.png"),
    [TAG(FLOOR_5)]                       = FLOOR_ASSET("floor_5.png"),
    [TAG(FLOOR_6)]                       = FLOOR_ASSET("floor_6.png"),
    [TAG(FLOOR_7)]                       = FLOOR_ASSET("floor_7.png"),
    [TAG(FLOOR_8)]                       = FLOOR_ASSET("floor_8.png"),
    [TAG(FLOOR_HOLE)]                    = FLOOR_ASSET("floor_hole.png"),
    [TAG(FLOOR_LADDER)]                  = FLOOR_ASSET("floor_ladder.png"),

    [TAG(WALL)]                          = WALL_ASSET("wall.png"),

    [TAG(WALL_BANNER_BLUE)]              = WALL_ASSET("wall_banner_blue.png"),
    [TAG(WALL_BANNER_GREEN)]             = WALL_ASSET("wall_banner_green.png"),
    [TAG(WALL_BANNER_RED)]               = WALL_ASSET("wall_banner_red.png"),
    [TAG(WALL_BANNER_YELLOW)]            = WALL_ASSET("wall_banner_yellow.png"),

    [TAG(WALL_COLUMN_MID)]               = WALL_ASSET("column/wall_column_mid.png"),
    [TAG(WALL_COLUMN_TOP)]               = WALL_ASSET("column/wall_column_top.png"),
    [TAG(WALL_COLUMN_BASE)]              = WALL_ASSET("column/wall_column_base.png"),

    [TAG(WALL_LEFT)]                     = WALL_ASSET("wall_left.png"),
    [TAG(WALL_MID)]                      = WALL_ASSET("wall_mid.png"),
    [TAG(WALL_RIGHT)]                    = WALL_ASSET("wall_right.png"),

    [TAG(WALL_TOP_LEFT)]                 = WALL_ASSET("top/wall_top_left.png"),
    [TAG(WALL_TOP_MID)]                  = WALL_ASSET("top/wall_top_mid.png"),
    [TAG(WALL_TOP_RIGHT)]                = WALL_ASSET("top/wall_top_right.png"),

    [TAG(WALL_SIDE_FRONT_LEFT)]          = WALL_ASSET("side/wall_side_front_left.png"),
    [TAG(WALL_SIDE_FRONT_RIGHT)]         = WALL_ASSET("side/wall_side_front_right.png"),
    [TAG(WALL_SIDE_MID_LEFT)]            = WALL_ASSET("side/wall_side_mid_left.png"),
    [TAG(WALL_SIDE_MID_RIGHT)]           = WALL_ASSET("side/wall_side_mid_right.png"),
    [TAG(WALL_SIDE_TOP_LEFT)]            = WALL_ASSET("side/wall_side_top_left.png"),
    [TAG(WALL_SIDE_TOP_RIGHT)]           = WALL_ASSET("side/wall_side_top_right.png"),

    [TAG(WALL_CORNER_BOTTOM_LEFT)]       = WALL_ASSET("corner/wall_corner_bottom_left.png"),
    [TAG(WALL_CORNER_BOTTOM_RIGHT)]      = WALL_ASSET("corner/wall_corner_bottom_right.png"),
    [TAG(WALL_CORNER_FRONT_LEFT)]        = WALL_ASSET("corner/wall_corner_front_left.png"),
    [TAG(WALL_CORNER_FRONT_RIGHT)]       = WALL_ASSET("corner/wall_corner_front_right.png"),
    [TAG(WALL_CORNER_LEFT)]              = WALL_ASSET("corner/wall_corner_left.png"),
    [TAG(WALL_CORNER_RIGHT)]             = WALL_ASSET("corner/wall_corner_right.png"),
    [TAG(WALL_CORNER_TOP_LEFT)]          = WALL_ASSET("corner/wall_corner_top_left.png"),
    [TAG(WALL_CORNER_TOP_RIGHT)]         = WALL_ASSET("corner/wall_corner_top_right.png"),

    [TAG(WALL_INNER_CORNER_L_TOP_LEFT)]  = WALL_ASSET("inner_corner/wall_inner_corner_l_top_left.png"),
    [TAG(WALL_INNER_CORNER_L_TOP_RIGHT)] = WALL_ASSET("inner_corner/wall_inner_corner_l_top_right.png"),
    [TAG(WALL_INNER_CORNER_MID_LEFT)]    = WALL_ASSET("inner_corner/wall_inner_corner_mid_left.png"),
    [TAG(WALL_INNER_CORNER_MID_RIGHT)]   = WALL_ASSET("inner_corner/wall_inner_corner_mid_right.png"),
    [TAG(WALL_INNER_CORNER_T_TOP_LEFT)]  = WALL_ASSET("inner_corner/wall_inner_corner_t_top_left.png"),
    [TAG(WALL_INNER_CORNER_T_TOP_RIGHT)] = WALL_ASSET("inner_corner/wall_inner_corner_t_top_right.png"),

#define ELF_M_ASSET(filename) MOB_ASSET("elf_m/"filename)
    [TAG(ELF_M_HIT_0)]                   = ELF_M_ASSET("hit/elf_m_hit_anim_f0.png"),

    [TAG(ELF_M_IDLE_0)]                  = ELF_M_ASSET("idle/elf_m_idle_anim_f0.png"),
    [TAG(ELF_M_IDLE_1)]                  = ELF_M_ASSET("idle/elf_m_idle_anim_f1.png"),
    [TAG(ELF_M_IDLE_2)]                  = ELF_M_ASSET("idle/elf_m_idle_anim_f2.png"),
    [TAG(ELF_M_IDLE_3)]                  = ELF_M_ASSET("idle/elf_m_idle_anim_f3.png"),

    [TAG(ELF_M_RUN_0)]                   = ELF_M_ASSET("run/elf_m_run_anim_f0.png"),
    [TAG(ELF_M_RUN_1)]                   = ELF_M_ASSET("run/elf_m_run_anim_f1.png"),
    [TAG(ELF_M_RUN_2)]                   = ELF_M_ASSET("run/elf_m_run_anim_f2.png"),
    [TAG(ELF_M_RUN_3)]                   = ELF_M_ASSET("run/elf_m_run_anim_f3.png"),
#undef ELF_M_ASSET

#define ELF_F_ASSET(filename) MOB_ASSET("elf_m/"filename)
    [TAG(ELF_F_HIT_0)]                   = ELF_F_ASSET("hit/elf_f_hit_anim_f0.png"),

    [TAG(ELF_F_IDLE_0)]                  = ELF_F_ASSET("idle/elf_f_idle_anim_f0.png"),
    [TAG(ELF_F_IDLE_1)]                  = ELF_F_ASSET("idle/elf_f_idle_anim_f1.png"),
    [TAG(ELF_F_IDLE_2)]                  = ELF_F_ASSET("idle/elf_f_idle_anim_f2.png"),
    [TAG(ELF_F_IDLE_3)]                  = ELF_F_ASSET("idle/elf_f_idle_anim_f3.png"),

    [TAG(ELF_F_RUN_0)]                   = ELF_F_ASSET("run/elf_f_run_anim_f0.png"),
    [TAG(ELF_F_RUN_1)]                   = ELF_F_ASSET("run/elf_f_run_anim_f1.png"),
    [TAG(ELF_F_RUN_2)]                   = ELF_F_ASSET("run/elf_f_run_anim_f2.png"),
    [TAG(ELF_F_RUN_3)]                   = ELF_F_ASSET("run/elf_f_run_anim_f3.png"),
#undef ELF_F_ASSET

#define BIG_DEMON_ASSET(filename) MOB_ASSET("big_demon/"filename)
    [TAG(BIG_DEMON_IDLE_0)]              = BIG_DEMON_ASSET("idle/big_demon_idle_anim_f0.png"),
    [TAG(BIG_DEMON_IDLE_1)]              = BIG_DEMON_ASSET("idle/big_demon_idle_anim_f1.png"),
    [TAG(BIG_DEMON_IDLE_2)]              = BIG_DEMON_ASSET("idle/big_demon_idle_anim_f2.png"),
    [TAG(BIG_DEMON_IDLE_3)]              = BIG_DEMON_ASSET("idle/big_demon_idle_anim_f3.png"),

    [TAG(BIG_DEMON_RUN_0)]               = BIG_DEMON_ASSET("run/big_demon_run_anim_f0.png"),
    [TAG(BIG_DEMON_RUN_1)]               = BIG_DEMON_ASSET("run/big_demon_run_anim_f1.png"),
    [TAG(BIG_DEMON_RUN_2)]               = BIG_DEMON_ASSET("run/big_demon_run_anim_f2.png"),
    [TAG(BIG_DEMON_RUN_3)]               = BIG_DEMON_ASSET("run/big_demon_run_anim_f3.png"),
#undef BIG_DEMON_ASSET

#define BIG_ZOMBIE_ASSET(filename) MOB_ASSET("big_zombie/"filename)
    [TAG(BIG_ZOMBIE_HIT_0)]              = BIG_ZOMBIE_ASSET("hit/big_zombie_hit_anim_f0.png"),

    [TAG(BIG_ZOMBIE_IDLE_0)]             = BIG_ZOMBIE_ASSET("idle/big_zombie_idle_anim_f0.png"),
    [TAG(BIG_ZOMBIE_IDLE_1)]             = BIG_ZOMBIE_ASSET("idle/big_zombie_idle_anim_f1.png"),
    [TAG(BIG_ZOMBIE_IDLE_2)]             = BIG_ZOMBIE_ASSET("idle/big_zombie_idle_anim_f2.png"),
    [TAG(BIG_ZOMBIE_IDLE_3)]             = BIG_ZOMBIE_ASSET("idle/big_zombie_idle_anim_f3.png"),

    [TAG(BIG_ZOMBIE_RUN_0)]              = BIG_ZOMBIE_ASSET("run/big_zombie_run_anim_f0.png"),
    [TAG(BIG_ZOMBIE_RUN_1)]              = BIG_ZOMBIE_ASSET("run/big_zombie_run_anim_f1.png"),
    [TAG(BIG_ZOMBIE_RUN_2)]              = BIG_ZOMBIE_ASSET("run/big_zombie_run_anim_f2.png"),
    [TAG(BIG_ZOMBIE_RUN_3)]              = BIG_ZOMBIE_ASSET("run/big_zombie_run_anim_f3.png"),
#undef BIG_ZOMBIE_ASSET

    [TAG(CHEST_EMPTY_OPEN_0)]            = CHEST_ASSET("empty/chest_empty_open_anim_f0.png"),
    [TAG(CHEST_EMPTY_OPEN_1)]            = CHEST_ASSET("empty/chest_empty_open_anim_f1.png"),
    [TAG(CHEST_EMPTY_OPEN_2)]            = CHEST_ASSET("empty/chest_empty_open_anim_f2.png"),

    [TAG(CHEST_FULL_OPEN_0)]             = CHEST_ASSET("full/chest_full_open_anim_f0.png"),
    [TAG(CHEST_FULL_OPEN_1)]             = CHEST_ASSET("full/chest_full_open_anim_f1.png"),
    [TAG(CHEST_FULL_OPEN_2)]             = CHEST_ASSET("full/chest_full_open_anim_f2.png"),

    [TAG(CHEST_MIMIC_OPEN_0)]            = CHEST_ASSET("mimic/chest_full_open_anim_f0.png"),
    [TAG(CHEST_MIMIC_OPEN_1)]            = CHEST_ASSET("mimic/chest_full_open_anim_f1.png"),
    [TAG(CHEST_MIMIC_OPEN_2)]            = CHEST_ASSET("mimic/chest_full_open_anim_f2.png"),
};
#undef TEXTURE_ASSET
#undef ITEM_ASSET
#undef WEAPON_ASSET
#undef MAP_ASSET
#undef WALL_ASSET
#undef FLOOR_ASSET
#undef CHEST_ASSET
#undef UI_ASSET
#undef MOB_ASSET
/* END OF TEXTURES */

inline constexpr u32 TEXTURE_COUNT = ARRAY_COUNT(TEXTURE_PATHS);