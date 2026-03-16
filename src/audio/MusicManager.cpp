#include "MusicManager.h"
#include <SDL_mixer.h>

MusicManager::MusicManager() {}

void MusicManager::loadAll() {
    const std::string base = "assets/audio/";

    tracks[MusicTrack::titleScreen] =
        std::make_unique<TDT4102::Audio>(base + "1-02_Title_Screen_.mp3");
    tracks[MusicTrack::town] = std::make_unique<TDT4102::Audio>(base + "1-04. New Bark Town_.mp3");
    tracks[MusicTrack::lab] = std::make_unique<TDT4102::Audio>(base + "1-07. Elm Pokemon Lab_.mp3");
    tracks[MusicTrack::route] = std::make_unique<TDT4102::Audio>(
        base + "1-04. New Bark Town_.mp3"); // reuse town for route (or swap)
    tracks[MusicTrack::wildBattle] =
        std::make_unique<TDT4102::Audio>(base + "1-10. Battle! (Wild Pokemon-Johto Version)_.mp3");
    tracks[MusicTrack::wildVictory] =
        std::make_unique<TDT4102::Audio>(base + "1-11. Victory! (Wild Pokemon)_.mp3");
    tracks[MusicTrack::trainerBattle] = std::make_unique<TDT4102::Audio>(
        base + "1-18. Battle! (Trainer Battle-Johto Version)_.mp3");
    tracks[MusicTrack::trainerVictory] =
        std::make_unique<TDT4102::Audio>(base + "1-19. Victory! (Trainer Battle)_.mp3");
}

void MusicManager::play(MusicTrack track, TDT4102::AnimationWindow &window) {
    if (track == currentTrack)
        return;

    // Stop whatever is currently playing
    Mix_HaltMusic();

    currentTrack = track;

    if (track == MusicTrack::none)
        return;

    auto it = tracks.find(track);
    if (it != tracks.end() && it->second)
        window.play_audio(*it->second, 9999); // loop many times
}

void MusicManager::stop() {
    Mix_HaltMusic();
    currentTrack = MusicTrack::none;
}

MusicTrack MusicManager::trackForMap(const std::string &mapId) {
    if (mapId == "bart_iver_lab")
        return MusicTrack::lab;
    if (mapId == "route_1")
        return MusicTrack::route;
    // Towns and houses
    return MusicTrack::town;
}

MusicTrack MusicManager::getCurrentTrack() const { return currentTrack; }
