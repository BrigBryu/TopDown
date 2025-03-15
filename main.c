#include "raylib.h"
#include "map_manager.h"
#include "player.h"
#include "entity_manager.h"
#include "monster.h"
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    const int screenWidth = 800;
    const int screenHeight = 600;
    InitWindow(screenWidth, screenHeight, "Map Manager Demo");
    SetTargetFPS(60);
    
    // Create entity manager
    EntityManager* entityManager = CreateEntityManager();
    
    // Initialize player
    Vector2 startPos = { (screenWidth - 192 * 2) / 2.0f, (screenHeight - 192 * 2) / 2.0f };
    Player player;
    InitPlayer(&player, "SproutLandsPack/Characters/BasicCharakterSpritesheet.png", startPos, 2.0f);
    
    // Create map manager
    MapManager* mapManager = CreateMapManager("Tiled/Tiledmaps/field.tmj");
    
    // Add some monsters using the same texture as the player
    const char* monsterTexture = "SproutLandsPack/Characters/BasicCharakterSpritesheet.png";
    
    // Add a slime at a position offset from the player
    Entity* slime = CreateSlime((Vector2){startPos.x + 100, startPos.y + 100}, 2.0f);
    InitEntitySprite(&slime->sprite, monsterTexture, 4, 4, 0.1f);
    AddEntity(entityManager, slime);
    
    // Add a bat at another position
    Entity* bat = CreateBat((Vector2){startPos.x - 100, startPos.y - 100}, 2.0f);
    InitEntitySprite(&bat->sprite, monsterTexture, 4, 4, 0.1f);
    AddEntity(entityManager, bat);
    
    // Add a skeleton at another position
    Entity* skeleton = CreateSkeleton((Vector2){startPos.x + 150, startPos.y - 150}, 2.0f);
    InitEntitySprite(&skeleton->sprite, monsterTexture, 4, 4, 0.1f);
    AddEntity(entityManager, skeleton);
    
    // Camera setup
    Camera2D camera = { 0 };
    camera.target = (Vector2){ 
        player.physics.position.x + (player.sprite.frameWidth * player.physics.scale) / 2.0f,
        player.physics.position.y + (player.sprite.frameHeight * player.physics.scale) / 2.0f
    };
    camera.offset = (Vector2){ screenWidth / 2.0f, screenHeight / 2.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;
    
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        
        // Toggle entity movement with 'M' key
        if (IsKeyPressed(KEY_M)) {
            ENTITIES_CAN_MOVE = !ENTITIES_CAN_MOVE;
            TraceLog(LOG_INFO, "Entity movement %s", ENTITIES_CAN_MOVE ? "enabled" : "disabled");
        }
        
        // Update map and player
        UpdateMapManager(mapManager, &player, dt);
        
        // Update all entities
        UpdateEntities(entityManager, &mapManager->currentMap, dt);
        
        // Check for collisions between entities
        CheckCollisions(entityManager);
        
        // Remove any dead entities
        RemoveDeadEntities(entityManager);
        
        // Update camera to follow player
        camera.target = (Vector2){ 
            player.physics.position.x + (player.sprite.frameWidth * player.physics.scale) / 2.0f,
            player.physics.position.y + (player.sprite.frameHeight * player.physics.scale) / 2.0f
        };
        
        BeginDrawing();
            ClearBackground((Color){200, 255, 200, 255});
            
            BeginMode2D(camera);
                // Render map
                RenderMapManager(mapManager, player.physics.scale);
                
                // Draw all entities
                DrawEntities(entityManager);
                
                // Draw player
                DrawPlayer(&player);
            EndMode2D();
            
            // Draw movement status
            DrawText(ENTITIES_CAN_MOVE ? "Movement: ON (M)" : "Movement: OFF (M)", 
                     10, 10, 20, ENTITIES_CAN_MOVE ? GREEN : RED);
        EndDrawing();
    }
    
    // Cleanup
    DestroyEntityManager(entityManager);
    DestroyMapManager(mapManager);
    UnloadPlayer(&player);
    CloseWindow();
    
    return 0;
}
