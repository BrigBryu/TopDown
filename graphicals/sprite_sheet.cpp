#include "sprite_sheet.hpp"

SpriteSheet::SpriteSheet(const char* path, int rows, int columns) {
    texture = LoadTexture(path);
    this->rows = rows;
    this->columns = columns;
}

SpriteSheet::~SpriteSheet() {
    UnloadTexture(texture);
}

void SpriteSheet::Draw(int row, int column, Vector2 position, float scale) {
    int frameWidth = texture.width / columns;
    int frameHeight = texture.height / rows;

    Rectangle sourceRect = {
        column * frameWidth,// x position
        row * frameHeight,// y position
        frameWidth,// width
        frameHeight// height
    };

    Rectangle destRect = {
        position.x,// screen x position
        position.y,// screen y position
        sourceRect.width * scale,// scaled width
        sourceRect.height * scale// scaled height
    };

    // Draw the portion of the texture defined by sourceRect into destRect
    DrawTexturePro(texture, sourceRect, destRect, Vector2{0, 0}, 0.0f, WHITE);
}