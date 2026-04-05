#include "MusicManager.h"
#include <SDL_mixer.h>
#include <iostream>
#include <stdexcept>

MusicManager::MusicManager() {}

void MusicManager::loadAll() {
    const std::string base = "assets/audio/";

    tracks[MusicTrack::titleScreen] =
        std::make_unique<TDT4102::Audio>(base + "1-02_Title_Screen_.mp3");
    tracks[MusicTrack::town] = std::make_unique<TDT4102::Audio>(base + "1-04. New Bark Town_.mp3");
    tracks[MusicTrack::city] = std::make_unique<TDT4102::Audio>(base + "022 Violet City.mp3");
    tracks[MusicTrack::lab] = std::make_unique<TDT4102::Audio>(base + "1-07. Elm Pokemon Lab_.mp3");
    tracks[MusicTrack::route] = std::make_unique<TDT4102::Audio>(base + "020 Route 30.mp3");
    tracks[MusicTrack::center] = std::make_unique<TDT4102::Audio>(base + "015 Pokémon Center.mp3");
    tracks[MusicTrack::mart] = std::make_unique<TDT4102::Audio>(base + "25. Poké Mart.mp3");
    tracks[MusicTrack::trainerEyesMeet] =
        std::make_unique<TDT4102::Audio>(base + "17. Trainers' Eyes Meet (Boy 1).mp3");
    tracks[MusicTrack::wildBattle] =
        std::make_unique<TDT4102::Audio>(base + "1-10. Battle! (Wild Pokemon-Johto Version)_.mp3");
    tracks[MusicTrack::wildVictory] =
        std::make_unique<TDT4102::Audio>(base + "1-11. Victory! (Wild Pokemon)_.mp3");
    tracks[MusicTrack::trainerBattle] = std::make_unique<TDT4102::Audio>(
        base + "1-18. Battle! (Trainer Battle-Johto Version)_.mp3");
    tracks[MusicTrack::trainerVictory] =
        std::make_unique<TDT4102::Audio>(base + "1-19. Victory! (Trainer Battle)_.mp3");
    tracks[MusicTrack::evolution] = std::make_unique<TDT4102::Audio>(base + "039 Evolution.mp3");
    tracks[MusicTrack::evolutionComplete] =
        std::make_unique<TDT4102::Audio>(base + "40. Congratulations! Your Pokémon Evolved!.mp3");
}

void MusicManager::play(MusicTrack track, TDT4102::AnimationWindow &window) {
    if (track == currentTrack)
        return;

    // Stop whatever is currently playing
    Mix_HaltMusic();

    currentTrack = track;
    currentPath.clear();

    if (track == MusicTrack::none)
        return;

    auto it = tracks.find(track);
    if (it != tracks.end() && it->second)
        window.play_audio(*it->second, 9999); // loop many times
}

void MusicManager::playOneShot(MusicTrack track, TDT4102::AnimationWindow &window) {
    Mix_HaltMusic();
    currentTrack = track;
    currentPath.clear();

    if (track == MusicTrack::none)
        return;

    auto it = tracks.find(track);
    if (it != tracks.end() && it->second)
        window.play_audio(*it->second, 0);
}

void MusicManager::playPath(const std::string &path, TDT4102::AnimationWindow &window) {
    if (path.empty()) {
        stop();
        return;
    }

    if (currentTrack == MusicTrack::none && currentPath == path)
        return;

    Mix_HaltMusic();
    currentTrack = MusicTrack::none;
    currentPath = path;

    auto [it, inserted] = pathTracks.try_emplace(path);
    if (inserted || !it->second)
        it->second = std::make_unique<TDT4102::Audio>(path);

    if (!it->second)
        return;

    try {
        window.play_audio(*it->second, 9999);
    } catch (const std::runtime_error &error) {
        std::cerr << "Failed to play music with path '" << path << "': " << error.what()
                  << std::endl;
    }
}

void MusicManager::stop() {
    Mix_HaltMusic();
    currentTrack = MusicTrack::none;
    currentPath.clear();
}

MusicTrack MusicManager::getCurrentTrack() const { return currentTrack; }
