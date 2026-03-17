#include "CutsceneRunner.h"
#include "StringUtils.h"
#include "TextParse.h"
#include <fstream>
#include <iostream>
#include <set>

using StringUtils::parseDirection;
using StringUtils::splitSemicolon;
using StringUtils::trimRightInPlace;

namespace {

enum class CutsceneSection { none, header, steps };

std::optional<CutsceneStep> parseCutsceneStep(std::string_view line) {
    const auto parts = TextParse::splitView(line, '|');
    if (parts.empty())
        return std::nullopt;

    const std::string_view cmd = parts[0];
    CutsceneStep step{};

    if (cmd == "move" && parts.size() >= 4) {
        const auto coords = TextParse::parseFixedIntFields<2>(parts, 2);
        if (!coords.has_value())
            return std::nullopt;
        step.type = CutsceneStep::Type::move;
        step.target = std::string(parts[1]);
        step.x = (*coords)[0];
        step.y = (*coords)[1];
        return step;
    }

    if (cmd == "walk" && parts.size() >= 3) {
        step.type = CutsceneStep::Type::walk;
        step.target = std::string(parts[1]);
        step.direction = parseDirection(std::string(parts[2]));
        return step;
    }

    if (cmd == "face" && parts.size() >= 3) {
        step.type = CutsceneStep::Type::face;
        step.target = std::string(parts[1]);
        step.direction = parseDirection(std::string(parts[2]));
        return step;
    }

    if (cmd == "say" && parts.size() >= 3) {
        step.type = CutsceneStep::Type::say;
        step.speaker = std::string(parts[1]);
        step.lines = splitSemicolon(std::string(parts[2]));
        return step;
    }

    if (cmd == "wait" && parts.size() >= 2) {
        const auto frames = TextParse::parseInt(parts[1]);
        if (!frames.has_value())
            return std::nullopt;
        step.type = CutsceneStep::Type::wait;
        step.frames = *frames;
        return step;
    }

    if (cmd == "sync") {
        step.type = CutsceneStep::Type::sync;
        return step;
    }

    if (cmd == "flag" && parts.size() >= 2) {
        step.type = CutsceneStep::Type::flag;
        step.flagName = std::string(parts[1]);
        return step;
    }

    if (cmd == "hide" && parts.size() >= 2) {
        step.type = CutsceneStep::Type::hide;
        step.target = std::string(parts[1]);
        return step;
    }

    if (cmd == "show" && parts.size() >= 2) {
        step.type = CutsceneStep::Type::show;
        step.target = std::string(parts[1]);
        return step;
    }

    return std::nullopt;
}

} // namespace

bool CutsceneRunner::load(const std::string &path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "CutsceneRunner: cannot open " << path << "\n";
        return false;
    }

    cutscene.steps.clear();
    cutscene.id.clear();

    CutsceneSection section = CutsceneSection::none;

    std::string line;
    while (std::getline(file, line)) {
        trimRightInPlace(line);

        if (line.empty() || line[0] == '#')
            continue;

        if (line == "[header]") {
            section = CutsceneSection::header;
            continue;
        }
        if (line == "[steps]") {
            section = CutsceneSection::steps;
            continue;
        }

        if (section == CutsceneSection::header) {
            const auto keyVal = TextParse::splitOnce(line, '|');
            if (keyVal.has_value() && keyVal->first == "id")
                cutscene.id = std::string(keyVal->second);
        } else if (section == CutsceneSection::steps) {
            const auto step = parseCutsceneStep(line);
            if (!step.has_value()) {
                std::cerr << "CutsceneRunner: invalid step '" << line << "'\n";
                continue;
            }
            cutscene.steps.push_back(*step);
        }
    }

    return !cutscene.id.empty();
}

// ---- playback ----

void CutsceneRunner::start() {
    currentStep = 0;
    finished = false;
    pendingMoves.clear();
    waitFrames = 0;
    inDialogue = false;
}

bool CutsceneRunner::isFinished() const { return finished; }
bool CutsceneRunner::isShowingDialogue() const { return inDialogue; }
const std::string &CutsceneRunner::getId() const { return cutscene.id; }

bool CutsceneRunner::update(World &world, GameUI &ui, bool confirmPressed, SoundManager &sound) {
    if (finished)
        return false;

    // Update all NPC walk animations on the current map
    for (auto &npc : world.getNPCs(world.getCurrentMapId()))
        npc->updateAnimation();

    // If we're in a dialogue, wait for dismissal
    if (inDialogue) {
        if (ui.updateTypewriter(confirmPressed)) {
            sound.play(SoundEffect::select, ui.getRenderer().getWindow());
            if (!ui.advanceDialogueLine()) {
                // Dialogue finished
                inDialogue = false;
                ++currentStep;
                processSteps(world, ui);
            }
        }
        return true;
    }

    // If we're waiting frames
    if (waitFrames > 0) {
        tickMovements(world);
        --waitFrames;
        if (waitFrames <= 0) {
            ++currentStep;
            processSteps(world, ui);
        }
        return true;
    }

    // If we're syncing (waiting for all moves to finish)
    if (currentStep < cutscene.steps.size() &&
        cutscene.steps[currentStep].type == CutsceneStep::Type::sync) {
        tickMovements(world);
        if (allMovesComplete(world)) {
            pendingMoves.clear();
            ++currentStep;
            processSteps(world, ui);
        }
        return true;
    }

    // Otherwise process steps
    processSteps(world, ui);
    return !finished;
}

void CutsceneRunner::processSteps(World &world, GameUI &ui) {
    // Process immediate steps until we hit a blocking one or run out
    while (currentStep < cutscene.steps.size()) {
        auto &step = cutscene.steps[currentStep];

        switch (step.type) {
        case CutsceneStep::Type::move: {
            // Register a pending move (non-blocking)
            pendingMoves.push_back({step.target, {step.x, step.y}});
            ++currentStep;
            break;
        }

        case CutsceneStep::Type::walk: {
            // Single tile step — register as a move to the adjacent tile
            Position cur = getEntityPosition(world, step.target);
            Position dest = cur;
            switch (step.direction) {
            case Direction::up:
                dest.y--;
                break;
            case Direction::down:
                dest.y++;
                break;
            case Direction::left:
                dest.x--;
                break;
            case Direction::right:
                dest.x++;
                break;
            }
            pendingMoves.push_back({step.target, dest});
            ++currentStep;
            break;
        }

        case CutsceneStep::Type::face: {
            // Immediate: set facing
            if (step.target == "player") {
                world.getPlayer().setFacing(step.direction);
            } else {
                NPC *npc =
                    world.findNPCAt(world.getCurrentMapId(), getEntityPosition(world, step.target));
                // Also search by ID if position lookup fails
                if (!npc) {
                    for (auto &n : world.getNPCs(world.getCurrentMapId())) {
                        if (n->getId() == step.target) {
                            npc = n.get();
                            break;
                        }
                    }
                }
                if (npc)
                    npc->setFacing(step.direction);
            }
            ++currentStep;
            break;
        }

        case CutsceneStep::Type::say: {
            // Blocking: show dialogue
            ui.startDialogue(step.speaker, step.lines);
            inDialogue = true;
            return; // Wait for dialogue dismissal
        }

        case CutsceneStep::Type::wait: {
            // Blocking: wait N frames
            waitFrames = step.frames;
            return;
        }

        case CutsceneStep::Type::sync: {
            // Blocking: wait for all pending moves to complete
            // The main update loop handles ticking and checking
            return;
        }

        case CutsceneStep::Type::flag: {
            world.getPlayer().setFlag(step.flagName);
            ++currentStep;
            break;
        }

        case CutsceneStep::Type::hide: {
            for (auto &n : world.getNPCs(world.getCurrentMapId())) {
                if (n->getId() == step.target) {
                    n->setHidden(true);
                    break;
                }
            }
            ++currentStep;
            break;
        }

        case CutsceneStep::Type::show: {
            for (auto &n : world.getNPCs(world.getCurrentMapId())) {
                if (n->getId() == step.target) {
                    n->setHidden(false);
                    break;
                }
            }
            ++currentStep;
            break;
        }
        }
    }

    // Ran out of steps — cutscene is done
    finished = true;
}

// ---- movement helpers ----

Position CutsceneRunner::getEntityPosition(const World &world, const std::string &targetId) const {
    if (targetId == "player")
        return world.getPlayer().getPosition();

    for (const auto &n : world.getNPCs(world.getCurrentMapId())) {
        if (n->getId() == targetId)
            return n->getPosition();
    }
    return {0, 0};
}

bool CutsceneRunner::isEntityWalking(const World &world, const std::string &targetId) const {
    if (targetId == "player")
        return world.getPlayer().isMoving();

    for (const auto &n : world.getNPCs(world.getCurrentMapId())) {
        if (n->getId() == targetId)
            return n->isMoving();
    }
    return false;
}

void CutsceneRunner::stepEntityToward(World &world, const std::string &targetId, Position dest) {
    Position cur = getEntityPosition(world, targetId);

    // Decide direction: prefer X first, then Y
    Direction dir = Direction::down;
    if (cur.x < dest.x)
        dir = Direction::right;
    else if (cur.x > dest.x)
        dir = Direction::left;
    else if (cur.y < dest.y)
        dir = Direction::down;
    else if (cur.y > dest.y)
        dir = Direction::up;
    else
        return; // already there

    if (targetId == "player") {
        Player &player = world.getPlayer();
        Map &map = world.getMap(world.getCurrentMapId());
        map.setOccupied(player.getPosition(), false);
        player.move(dir);
        map.setOccupied(player.getPosition(), true);
    } else {
        for (auto &n : world.getNPCs(world.getCurrentMapId())) {
            if (n->getId() == targetId) {
                n->setMoveDelay(24);
                n->move(dir);
                break;
            }
        }
    }
}

void CutsceneRunner::tickMovements(World &world) {
    // For each entity, only process the FIRST pending move that isn't complete
    // yet. This chains multiple moves for the same target sequentially.
    std::set<std::string> stepped; // entities we've already handled this tick

    for (auto &pm : pendingMoves) {
        if (stepped.count(pm.targetId))
            continue; // Already processing an earlier move for this entity

        if (isEntityWalking(world, pm.targetId)) {
            stepped.insert(pm.targetId); // Still animating, skip later moves
            continue;
        }

        Position cur = getEntityPosition(world, pm.targetId);
        if (cur == pm.destination)
            continue; // This move is done, check the next one for this entity

        stepEntityToward(world, pm.targetId, pm.destination);
        stepped.insert(pm.targetId);
    }
}

bool CutsceneRunner::allMovesComplete(const World &world) const {
    for (const auto &pm : pendingMoves) {
        if (isEntityWalking(world, pm.targetId))
            return false;
        Position cur = getEntityPosition(world, pm.targetId);
        if (cur != pm.destination)
            return false;
    }
    return true;
}
