#ifndef SPRITE_SHEET_HPP
#define SPRITE_SHEET_HPP

#include "raylib.h"

class SpriteSheet {
    public:
        SpriteSheet(const char* path, int rows, int columns);
        ~SpriteSheet();

        void Draw(int row, int column, Vector2 position, float scale);

    private:
        Texture2D texture;
        int rows;
        int columns;
};
#endif