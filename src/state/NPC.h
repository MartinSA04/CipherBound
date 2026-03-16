#pragma once
#include "Daemon.h"
#include "Entity.h"
#include "player/Party.h"
#include <set>
#include <string>
#include <vector>

enum class NPCType {
    trainer,
    gymLeader,
    shopkeeper,
    healer,
    questGiver,
    wildEncounter,
    normal, // Generic NPC with dialogue only
    pc,     // Computer terminal (PC box system)
};

// A dialogue stage: if the player has the required flag (or flag is empty),
// use these lines. Stages are checked in order; first match wins.
struct DialogueStage {
    std::string requiredFlag; // empty = always matches (default)
    std::vector<std::string> lines;
};

class NPC : public Entity {
  public:
    NPC(const std::string &id, const std::string &name, Position position, NPCType type);

    void update() override;

    void addDaemon(Daemon daemon);
    Daemon &getDaemon(int index);
    const Daemon &getDaemon(int index) const;
    const std::vector<Daemon> &getParty() const;
    int partySize() const;
    bool partyEmpty() const;
    void clearParty();

    const std::string &getId() const;
    NPCType getType() const;
    bool isTrainerType() const;
    bool isDefeated() const;
    void setDefeated(bool defeated);

    // Dialogue — returns best matching stage for the player's flags
    const std::vector<std::string> &
    getDialogueLines(const std::set<std::string> &playerFlags) const;
    void addDialogueStage(const std::string &requiredFlag, const std::vector<std::string> &lines);

    // Sight range for trainer battles
    int getSightRange() const;
    void setSightRange(int range);

    bool canSeePlayer(Position playerPos) const;
    bool willFight() const;

    // Visibility (cutscene hide/show)
    bool isHidden() const;
    void setHidden(bool h);

  private:
    std::string id; // Unique identifier for event flags
    NPCType type;
    bool defeated;
    int sightRange;
    std::vector<DialogueStage> dialogueStages;
    Party party;

    bool hidden{false};

    static const std::vector<std::string> emptyLines;
};
