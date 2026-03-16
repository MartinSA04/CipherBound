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
    lab,
    route,
    wildBattle,
    wildVictory,
    trainerBattle,
    trainerVictory,
};

class MusicManager {
  public:
    MusicManager();

    // Load all audio files up front
    void loadAll();

    // Play a track (does nothing if already playing that track)
    void play(MusicTrack track, TDT4102::AnimationWindow &window);

    // Stop current music
    void stop();

    // Get the appropriate track for a map id
    static MusicTrack trackForMap(const std::string &mapId);

    MusicTrack getCurrentTrack() const;

  private:
    MusicTrack currentTrack{MusicTrack::none};
    std::map<MusicTrack, std::unique_ptr<TDT4102::Audio>> tracks;
};
