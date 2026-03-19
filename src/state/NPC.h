/**
 * @file
 * @brief NPC model used for dialogue, trainer battles, shops, and scripted interactions.
 * @ingroup world_state
 */

#pragma once
#include "Daemon.h"
#include "Entity.h"
#include "player/Party.h"
#include <set>
#include <string>
#include <vector>

/// Runtime NPC role used by story flow, interaction logic, and battle setup.
enum class NPCType {
    trainer,      ///< Standard trainer battle NPC.
    gymLeader,    ///< Stronger trainer with gym-leader semantics.
    shopkeeper,   ///< NPC that opens the shop screen.
    healer,       ///< NPC that heals the player's party.
    questGiver,   ///< NPC with quest/story progression logic.
    wildEncounter,///< NPC-like trigger for forced wild encounters.
    normal,       ///< Generic dialogue-only NPC.
    pc,           ///< Computer terminal for PC box access.
};

/// Conditional dialogue stage selected by player event flags.
struct DialogueStage {
    std::string requiredFlag;        ///< Required player flag, or empty for default dialogue.
    std::vector<std::string> lines;  ///< Dialogue lines shown when the stage matches.
};

/**
 * @brief Overworld NPC with optional trainer party, dialogue stages, and visibility state.
 * @ingroup world_state
 */
class NPC : public Entity {
  public:
    /// Constructs an NPC at a world position with an optional sprite type override.
    NPC(const std::string &id, const std::string &name, Position position, NPCType type,
        std::string spriteType = {});

    /// Advances movement/animation inherited from `Entity`.
    void update() override;

    /// Appends a daemon to the trainer party.
    void addDaemon(Daemon daemon);
    /// Returns mutable access to a party daemon.
    Daemon &getDaemon(int index);
    /// Returns immutable access to a party daemon.
    const Daemon &getDaemon(int index) const;
    /// Returns the full trainer party.
    const std::vector<Daemon> &getParty() const;
    /// Returns the party size.
    int partySize() const;
    /// Returns whether the trainer party is empty.
    bool partyEmpty() const;
    /// Clears the trainer party.
    void clearParty();

    /// Returns the stable NPC id used by story and save systems.
    const std::string &getId() const;
    /// Returns the sprite type used for rendering.
    const std::string &getSpriteType() const;
    /// Returns the NPC role.
    NPCType getType() const;
    /// Returns whether this NPC can initiate trainer-battle flow.
    bool isTrainerType() const;
    /// Returns whether the NPC has been defeated.
    bool isDefeated() const;
    /// Sets the defeated flag.
    void setDefeated(bool defeated);

    /// Returns the best matching dialogue lines for the current player flags.
    const std::vector<std::string> &
    getDialogueLines(const std::set<std::string> &playerFlags) const;
    /// Adds a dialogue stage checked in insertion order.
    void addDialogueStage(const std::string &requiredFlag, const std::vector<std::string> &lines);

    /// Returns the trainer sight range in tiles.
    int getSightRange() const;
    /// Sets the trainer sight range in tiles.
    void setSightRange(int range);

    /// Returns whether the NPC can currently see the player from its facing direction.
    bool canSeePlayer(Position playerPos) const;
    /// Returns whether the NPC is eligible to fight the player.
    bool willFight() const;

    /// Returns whether the NPC is hidden from the overworld.
    bool isHidden() const;
    /// Sets whether the NPC is hidden from the overworld.
    void setHidden(bool h);

  private:
    std::string id;                 ///< Unique identifier for story and save-state lookups.
    std::string spriteType;         ///< Render sprite key.
    NPCType type;                   ///< Runtime NPC role.
    bool defeated;                  ///< Whether the NPC has already been defeated.
    int sightRange;                 ///< Trainer detection range.
    std::vector<DialogueStage> dialogueStages; ///< Ordered dialogue stages.
    Party party;                    ///< Trainer party.

    bool hidden{false};             ///< Whether the NPC is currently hidden.

    static const std::vector<std::string> emptyLines; ///< Shared empty dialogue result.
};
