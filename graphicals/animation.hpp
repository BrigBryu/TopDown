#ifndef ANIMATION_HPP
#define ANIMATION_HPP

#include "raylib.h"
#include "sprite_sheet.hpp"

class Animation {
    public:
        Animation(SpriteSheet *spriteSheet, int row, int colStart, int rowEnd, int colEnd, int frameCount);
        void update(float dt);
        void draw(Vector2 position, float scale);
        bool is_finished();
        void set_looping(bool looping);
        void start_animation();
    private:
        SpriteSheet *spriteSheet;
        int row, colEnd, frameCount;
        int currentCol;
        float frameTime;
        float currentTime;
        bool looping;
        bool finished;
};
#endif