#pragma once
#include <AnimationWindow.h>
#include <Audio.h>
#include <map>
#include <string>

// Identifies which music track should be playing
enum class MusicTrack {
    none,
    titleScreen,
    town,
    city,
    lab,
    route,
    center,
    mart,
    trainerEyesMeet,
    wildBattle,
    wildVictory,
    trainerBattle,
    trainerVictory,
    evolution,
    evolutionComplete,
};

class MusicManager {
  public:
    MusicManager();

    // Load all audio files up front
    void loadAll();

    // Play a track (does nothing if already playing that track)
    void play(MusicTrack track, TDT4102::AnimationWindow &window);

    // Play a track once immediately, interrupting current music.
    void playOneShot(MusicTrack track, TDT4102::AnimationWindow &window);

    // Play a looping music file by path, interrupting current music if needed.
    void playPath(const std::string &path, TDT4102::AnimationWindow &window);

    // Stop current music
    void stop();

    MusicTrack getCurrentTrack() const;

  private:
    MusicTrack currentTrack{MusicTrack::none};
    std::string currentPath;
    std::map<MusicTrack, std::unique_ptr<TDT4102::Audio>> tracks;
    std::map<std::string, std::unique_ptr<TDT4102::Audio>> pathTracks;
};
