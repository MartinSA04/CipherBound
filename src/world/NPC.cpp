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

void NPC::addDaemon(Daemon daemon)
{
    party.push_back(std::move(daemon));
}

std::vector<Daemon> &NPC::getParty() { return party; }
bool NPC::partyEmpty() const
{
    return party.empty();
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

void NPC::walkStep(Direction dir, int delay)
{
    facing = dir;
    moveDelay = delay;

    // Set initial pixel offset to full tile so the NPC slides to the new position
    switch (dir)
    {
    case Direction::up:
        position.y--;
        pixelOffsetX = 0;
        pixelOffsetY = TILE_SIZE;
        break;
    case Direction::down:
        position.y++;
        pixelOffsetX = 0;
        pixelOffsetY = -TILE_SIZE;
        break;
    case Direction::left:
        position.x--;
        pixelOffsetX = TILE_SIZE;
        pixelOffsetY = 0;
        break;
    case Direction::right:
        position.x++;
        pixelOffsetX = -TILE_SIZE;
        pixelOffsetY = 0;
        break;
    }
    animFramesLeft = moveDelay;
    walkFrame++;
}

void NPC::updateWalkAnimation()
{
    if (animFramesLeft <= 0)
        return;

    --animFramesLeft;

    if (animFramesLeft == moveDelay / 2)
        walkFrame++;

    if (animFramesLeft <= 0)
    {
        pixelOffsetX = 0;
        pixelOffsetY = 0;
    }
    else
    {
        double t = static_cast<double>(animFramesLeft) / moveDelay;
        int totalOffset = static_cast<int>(t * TILE_SIZE);

        switch (facing)
        {
        case Direction::up:
            pixelOffsetX = 0;
            pixelOffsetY = totalOffset;
            break;
        case Direction::down:
            pixelOffsetX = 0;
            pixelOffsetY = -totalOffset;
            break;
        case Direction::left:
            pixelOffsetX = totalOffset;
            pixelOffsetY = 0;
            break;
        case Direction::right:
            pixelOffsetX = -totalOffset;
            pixelOffsetY = 0;
            break;
        }
    }
}

bool NPC::isWalking() const { return animFramesLeft > 0; }
int NPC::getPixelOffsetX() const { return pixelOffsetX; }
int NPC::getPixelOffsetY() const { return pixelOffsetY; }
int NPC::getWalkFrame() const { return walkFrame; }
bool NPC::isHidden() const { return hidden; }
void NPC::setHidden(bool h) { hidden = h; }
