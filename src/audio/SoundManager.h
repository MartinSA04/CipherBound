#pragma once
#include "SoundEffects.h"
#include <AnimationWindow.h>
#include <Audio.h>
#include <map>

class SoundManager {
  public:
    SoundManager();

    // Load all sound effect files
    void loadAll();

    // Play a sound effect (fire-and-forget, no looping)
    void play(SoundEffect sfx, TDT4102::AnimationWindow &window);

  private:
    std::map<SoundEffect, std::unique_ptr<TDT4102::Audio>> effects;
};
