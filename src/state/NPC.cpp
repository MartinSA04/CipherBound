#include "NPC.h"
#include "../ui/Renderer.h"
#include <cmath>

const std::vector<std::string> NPC::emptyLines = {};

NPC::NPC(const std::string &id, const std::string &name, Position position, NPCType type)
    : Entity(name, position), id(id), type(type), defeated(false), sightRange(4)
{
}

void NPC::update()
{
    // NPC AI (turn toward player, patrol, etc.) — future use
}

void NPC::addDaemon(Daemon daemon)
{
    party.push_back(std::move(daemon));
}

const std::string &NPC::getId() const { return id; }
NPCType NPC::getType() const { return type; }
bool NPC::isTrainerType() const { return type == NPCType::trainer || type == NPCType::gymLeader; }
bool NPC::isDefeated() const { return defeated; }
void NPC::setDefeated(bool d) { defeated = d; }

const std::vector<std::string> &NPC::getDialogueLines(const std::set<std::string> &playerFlags) const
{
    // Check stages in order; first match wins
    for (const auto &stage : dialogueStages)
    {
        if (stage.requiredFlag.empty() || playerFlags.count(stage.requiredFlag))
            return stage.lines;
    }
    return emptyLines;
}

void NPC::addDialogueStage(const std::string &requiredFlag, const std::vector<std::string> &lines)
{
    dialogueStages.push_back({requiredFlag, lines});
}

int NPC::getSightRange() const { return sightRange; }
void NPC::setSightRange(int range) { sightRange = range; }

bool NPC::canSeePlayer(Position playerPos) const
{
    // Only see in the direction the NPC is facing, within sight range
    int dx = playerPos.x - position.x;
    int dy = playerPos.y - position.y;

    switch (facing)
    {
    case Direction::up:
        return dx == 0 && dy < 0 && dy >= -sightRange;
    case Direction::down:
        return dx == 0 && dy > 0 && dy <= sightRange;
    case Direction::left:
        return dy == 0 && dx < 0 && dx >= -sightRange;
    case Direction::right:
        return dy == 0 && dx > 0 && dx <= sightRange;
    }
    return false;
}

bool NPC::willFight() const
{
    if (isDefeated())
        return false;
    if (getType() != NPCType::trainer && getType() != NPCType::gymLeader)
        return false;
    if (partyEmpty())
        return false;
    return true;
}

bool NPC::isHidden() const { return hidden; }
void NPC::setHidden(bool h) { hidden = h; }
