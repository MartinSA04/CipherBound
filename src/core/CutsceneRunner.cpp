#include "CutsceneRunner.h"
#include "CutsceneFormat.h"
#include <fstream>
#include <iostream>
#include <set>

bool CutsceneRunner::load(const std::string &path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "CutsceneRunner: cannot open " << path << "\n";
        return false;
    }

    auto parsed = CutsceneFormat::parse(file);
    for (const auto &warning : parsed.warnings)
        std::cerr << "CutsceneRunner: " << warning << "\n";

    cutscene = std::move(parsed.cutscene);
    return parsed.valid();
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
