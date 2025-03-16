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
    
    // Initialize player
    Vector2 startPos = { (screenWidth - 192 * 2) / 2.0f, (screenHeight - 192 * 2) / 2.0f };
    Player player;
    InitPlayer(&player, "SproutLandsPack/Characters/BasicCharakterSpritesheet.png", startPos, 2.0f);
    
    // Create map manager
    MapManager* mapManager = CreateMapManager("Tiled/Tiledmaps/field.tmj");
    
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
        
        // Update player first
        UpdatePlayer(&player, &mapManager->currentMap, dt);
        
        // Update map manager (includes entity updates)
        UpdateMapManager(mapManager, &player, dt);
        
        // Check for collisions between entities
        CheckCollisions(mapManager->entityManager);
        
        // Remove any dead entities
        RemoveDeadEntities(mapManager->entityManager);
        
        // Update camera to follow player
        camera.target = (Vector2){ 
            player.physics.position.x + (player.sprite.frameWidth * player.physics.scale) / 2.0f,
            player.physics.position.y + (player.sprite.frameHeight * player.physics.scale) / 2.0f
        };
        
        BeginDrawing();
            ClearBackground((Color){200, 255, 200, 255});
            
            BeginMode2D(camera);
                // Render map and entities
                RenderMapManager(mapManager, player.physics.scale);
                
                // Draw player
                DrawPlayer(&player);
            EndMode2D();
            
            // Draw UI
            DrawText(TextFormat("Current Map: %s", mapManager->currentMapName), 
                    10, 10, 20, BLACK);
        EndDrawing();
    }
    
    // Cleanup
    DestroyMapManager(mapManager);
    UnloadPlayer(&player);
    CloseWindow();
    
    return 0;
}
