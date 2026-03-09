#pragma once
#include <vector>
#include <string>
#include <set>
#include "Entity.h"
#include "Creature.h"

enum class NPCType
{
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
struct DialogueStage
{
    std::string requiredFlag; // empty = always matches (default)
    std::vector<std::string> lines;
};

class NPC : public Entity
{
public:
    NPC(const std::string &id, const std::string &name, Position position, NPCType type);

    void update() override;

    const std::string &getId() const;
    NPCType getType() const;
    bool isTrainerType() const;
    bool isDefeated() const;
    void setDefeated(bool defeated);

    // Dialogue — returns best matching stage for the player's flags
    const std::vector<std::string> &getDialogueLines(const std::set<std::string> &playerFlags) const;
    void addDialogueStage(const std::string &requiredFlag, const std::vector<std::string> &lines);

    // Trainer party
    void addCreature(Creature creature);
    std::vector<Creature> &getParty();
    bool partyEmpty() const;

    // Sight range for trainer battles
    int getSightRange() const;
    void setSightRange(int range);

    bool canSeePlayer(Position playerPos) const;
    bool willFight() const;

    // Movement animation (used by cutscenes)
    void walkStep(Direction dir, int delay = 12);
    void updateWalkAnimation();
    bool isWalking() const;
    int getPixelOffsetX() const;
    int getPixelOffsetY() const;

    // Visibility (cutscene hide/show)
    bool isHidden() const;
    void setHidden(bool h);

private:
    std::string id; // Unique identifier for event flags
    NPCType type;
    bool defeated;
    int sightRange;
    std::vector<DialogueStage> dialogueStages;
    std::vector<Creature> party;

    // Movement animation state
    int pixelOffsetX{0};
    int pixelOffsetY{0};
    int animFramesLeft{0};
    int moveDelay{12};

    bool hidden{false};

    static const std::vector<std::string> emptyLines;
};
