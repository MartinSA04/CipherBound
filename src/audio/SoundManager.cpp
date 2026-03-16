#include "SoundManager.h"
#include <SDL_mixer.h>

SoundManager::SoundManager() {}

void SoundManager::loadAll() {
    const std::string base = "assets/audio/sound_effects/";

    effects[SoundEffect::confirm] = std::make_unique<TDT4102::Audio>(base + "SEQ_SE_DECIDE1.wav");
    effects[SoundEffect::select] = std::make_unique<TDT4102::Audio>(base + "SEQ_SE_DP_SELECT.wav");
    effects[SoundEffect::expTick] = std::make_unique<TDT4102::Audio>(base + "SEQ_SE_EXP.wav");
    effects[SoundEffect::expFull] = std::make_unique<TDT4102::Audio>(base + "SEQ_SE_EXPMAX.wav");
    effects[SoundEffect::levelUp] = std::make_unique<TDT4102::Audio>(base + "SEQ_SE_LVUP.wav");
    effects[SoundEffect::pcLogin] = std::make_unique<TDT4102::Audio>(base + "SEQ_SE_PC_LOGIN.wav");
    effects[SoundEffect::pcLogoff] =
        std::make_unique<TDT4102::Audio>(base + "SEQ_SE_PC_LOGOFF.wav");
    effects[SoundEffect::pcOn] = std::make_unique<TDT4102::Audio>(base + "SEQ_SE_PC_ON.wav");
    effects[SoundEffect::recovery] = std::make_unique<TDT4102::Audio>(base + "SEQ_SE_RECOVERY.wav");
    effects[SoundEffect::save] = std::make_unique<TDT4102::Audio>(base + "SEQ_SE_SAVE.wav");
    effects[SoundEffect::wallHit] = std::make_unique<TDT4102::Audio>(base + "SEQ_SE_WALL_HIT.wav");
    effects[SoundEffect::pokeballEscape] =
        std::make_unique<TDT4102::Audio>(base + "pokeball_escape.wav");
    effects[SoundEffect::pokeballShake] =
        std::make_unique<TDT4102::Audio>(base + "pokeball_shake.wav");
    effects[SoundEffect::attack] = std::make_unique<TDT4102::Audio>(base + "attack.wav");
}

void SoundManager::play(SoundEffect sfx, TDT4102::AnimationWindow &window) {
    auto it = effects.find(sfx);
    if (it != effects.end() && it->second) {
        // Set all SFX channels to ~75% volume (96 out of 128)
        Mix_Volume(-1, 96);
        window.play_audio(*it->second, 0);
    }
}
