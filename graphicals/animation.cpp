#include "animation.hpp"

Animation::Animation(Texture2D *texture, int rows, int cols, int frameCount) {
    this->texture = texture;
    this->rows = rows;
    this->cols = cols;
    this->frameCount = frameCount;
    this->currentFrame = 0;
    this->frameTime = 0.0f;
    this->currentTime = 0.0f;
}

void Animation::start_animation() {
    currentTime = 0.0f;
    currentCol = 0;
    finished = false;
}

void Animation::end_animation() {
    finished = true;
    currentCol = 0;
}


void Animation::update(float dt) {
    currentTime += dt;
    if (currentTime >= frameTime) {
        currentTime = 0.0f;
        currentCol++;
        if (currentCol >= cols) {
            currentCol = 0;
            if (!looping) {
                finished = true;
            }
        }
    }
}

void Animation::draw(Vector2 position, float scale) {
    Rectangle sourceRect = GetCurrentFrame();
    DrawTexturePro(*texture, sourceRect, Rectangle{position.x, position.y, sourceRect.width * scale, sourceRect.height * scale}, Vector2{0, 0}, 0.0f, WHITE);
}

bool Animation::is_finished() {
    return finished;
}

void Animation::set_looping(bool looping) {
    this->looping = looping;
}