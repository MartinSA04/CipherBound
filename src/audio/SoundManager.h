#pragma once
#include <string>
#include <map>
#include <AnimationWindow.h>
#include <Audio.h>

enum class SoundEffect
{
    confirm,        // SEQ_SE_DECIDE1 — menu confirm
    select,         // SEQ_SE_DP_SELECT — menu cursor move
    expTick,        // SEQ_SE_EXP — EXP gain tick
    expFull,        // SEQ_SE_EXPMAX — EXP bar full
    levelUp,        // SEQ_SE_LVUP — level up
    pcLogin,        // SEQ_SE_PC_LOGIN — open PC box
    pcLogoff,       // SEQ_SE_PC_LOGOFF — close PC box
    pcOn,           // SEQ_SE_PC_ON — interact with PC
    recovery,       // SEQ_SE_RECOVERY — healing
    save,           // SEQ_SE_SAVE — game saved
    wallHit,        // SEQ_SE_WALL_HIT — walking into wall
    pokeballEscape, // pokeball_escape — capture failed
    pokeballShake,  // pokeball_shake — ball shaking
    attack,         // attack — move hit
};

class SoundManager
{
public:
    SoundManager();

    // Load all sound effect files
    void loadAll();

    // Play a sound effect (fire-and-forget, no looping)
    void play(SoundEffect sfx, TDT4102::AnimationWindow &window);

private:
    std::map<SoundEffect, std::unique_ptr<TDT4102::Audio>> effects;
};
