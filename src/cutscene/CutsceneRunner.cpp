#include "CutsceneRunner.h"
#include "CutsceneLoader.h"
#include "CutsceneWorldOps.h"
#include "../common/StringUtils.h"
#include "../audio/SoundManager.h"
#include "../ui/GameUI.h"
#include <filesystem>
#include <iostream>

bool CutsceneRunner::load(const std::string &path) {
    CutsceneLoader::LoadResult loaded = CutsceneLoader::load(path);
    if (!loaded.opened) {
        std::cerr << "CutsceneRunner: cannot open " << path
                  << " (cwd=" << std::filesystem::current_path().string() << ")\n";
        return false;
    }

    for (const auto &warning : loaded.warnings)
        std::cerr << "CutsceneRunner: " << warning << "\n";

    if (!loaded.valid()) {
        std::cerr << "CutsceneRunner: invalid cutscene '" << loaded.resolvedPath.string()
                  << "'\n";
        return false;
    }

    cutscene = std::move(loaded.cutscene);
    return true;
}

bool CutsceneRunner::load(Cutscene cutsceneData) {
    cutscene = std::move(cutsceneData);
    return !cutscene.id.empty();
}

void CutsceneRunner::start() { playback.reset(); }

bool CutsceneRunner::isFinished() const { return playback.isFinished(); }

bool CutsceneRunner::isShowingDialogue() const { return playback.isShowingDialogue(); }

const std::string &CutsceneRunner::getId() const { return cutscene.id; }

bool CutsceneRunner::update(World &world, GameUI &ui, bool confirmPressed, SoundManager &sound) {
    if (playback.isFinished())
        return false;

    CutsceneWorldOps::updateAnimations(world);

    if (playback.isShowingDialogue()) {
        if (ui.updateTypewriter(confirmPressed)) {
            sound.play(SoundEffect::select, ui.getRenderer().getWindow());
            if (!ui.advanceDialogueLine()) {
                playback.endDialogue();
                playback.advanceStep();
                processSteps(world, ui);
            }
        }
        return true;
    }

    if (playback.isWaiting()) {
        CutsceneWorldOps::tickMovements(world, playback.pendingMoves());
        if (playback.tickWait()) {
            playback.advanceStep();
            processSteps(world, ui);
        }
        return true;
    }

    if (playback.stepIndex() < cutscene.steps.size() &&
        cutscene.steps[playback.stepIndex()].type == CutsceneStep::Type::sync) {
        CutsceneWorldOps::tickMovements(world, playback.pendingMoves());
        if (CutsceneWorldOps::allMovesComplete(world, playback.pendingMoves())) {
            playback.clearPendingMoves();
            playback.advanceStep();
            processSteps(world, ui);
        }
        return true;
    }

    processSteps(world, ui);
    return !playback.isFinished();
}

void CutsceneRunner::processSteps(World &world, GameUI &ui) {
    while (playback.stepIndex() < cutscene.steps.size()) {
        const auto &step = cutscene.steps[playback.stepIndex()];

        switch (step.type) {
        case CutsceneStep::Type::move:
            playback.queueMove(step.target, {step.x, step.y});
            playback.advanceStep();
            break;

        case CutsceneStep::Type::walk:
            if (const auto current = CutsceneWorldOps::tryGetEntityPosition(world, step.target);
                current.has_value()) {
                playback.queueMove(step.target,
                                   CutsceneWorldOps::adjacentDestination(*current, step.direction));
            }
            playback.advanceStep();
            break;

        case CutsceneStep::Type::face:
            CutsceneWorldOps::setFacing(world, step.target, step.direction);
            playback.advanceStep();
            break;

        case CutsceneStep::Type::say:
            ui.startDialogue(StringUtils::substitutePlayerName(step.speaker, world.getPlayer().getName()),
                             StringUtils::substitutePlayerName(step.lines,
                                                               world.getPlayer().getName()));
            playback.beginDialogue();
            return;

        case CutsceneStep::Type::wait:
            playback.beginWait(step.frames);
            return;

        case CutsceneStep::Type::sync:
            return;

        case CutsceneStep::Type::flag:
            world.getPlayer().setFlag(step.flagName);
            playback.advanceStep();
            break;

        case CutsceneStep::Type::hide:
            CutsceneWorldOps::setHidden(world, step.target, true);
            playback.advanceStep();
            break;

        case CutsceneStep::Type::show:
            CutsceneWorldOps::setHidden(world, step.target, false);
            playback.advanceStep();
            break;
        }
    }

    playback.finish();
}
