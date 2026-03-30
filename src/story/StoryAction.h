#pragma once

#include "../state/Daemon.h"
#include <string>
#include <utility>
#include <variant>
#include <vector>

class NPC;

struct StoryNoAction {};

struct StoryBlockWarpAction {
    std::string speaker;
    std::vector<std::string> lines;
};

struct StoryShowChoiceAction {
    std::vector<std::string> options;
    std::string choiceContext;
};

struct StoryStartBattleAction {
    NPC *trainer{nullptr};
};

struct StoryShowDialogueAction {
    std::string speaker;
    std::vector<std::string> lines;
};

struct StoryPromptStarterNicknameAction {
    Daemon daemon;
    std::string speaker;
    std::vector<std::string> lines;
};

struct StoryReturnToStateAction {};

struct StoryStartCutsceneAction {
    std::string cutscenePath;
};

struct StoryAction {
    using Payload = std::variant<StoryNoAction, StoryBlockWarpAction, StoryShowChoiceAction,
                                 StoryStartBattleAction, StoryShowDialogueAction,
                                 StoryPromptStarterNicknameAction, StoryReturnToStateAction,
                                 StoryStartCutsceneAction>;

    Payload payload;

    template <class T> bool is() const { return std::holds_alternative<T>(payload); }

    template <class T> const T *tryGet() const { return std::get_if<T>(&payload); }

    template <class T> T *tryGet() { return std::get_if<T>(&payload); }

    static StoryAction none() { return StoryAction{StoryNoAction{}}; }

    static StoryAction blockWarp(std::vector<std::string> lines, std::string speaker = {}) {
        return StoryAction{StoryBlockWarpAction{std::move(speaker), std::move(lines)}};
    }

    static StoryAction showChoice(std::vector<std::string> options, std::string choiceContext) {
        return StoryAction{StoryShowChoiceAction{std::move(options), std::move(choiceContext)}};
    }

    static StoryAction startBattle(NPC *trainer) {
        return StoryAction{StoryStartBattleAction{trainer}};
    }

    static StoryAction showDialogue(std::string speaker, std::vector<std::string> lines) {
        return StoryAction{StoryShowDialogueAction{std::move(speaker), std::move(lines)}};
    }

    static StoryAction promptStarterNickname(Daemon daemon, std::string speaker,
                                             std::vector<std::string> lines) {
        return StoryAction{StoryPromptStarterNicknameAction{
            std::move(daemon), std::move(speaker), std::move(lines)}};
    }

    static StoryAction returnToState() { return StoryAction{StoryReturnToStateAction{}}; }

    static StoryAction startCutscene(std::string cutscenePath) {
        return StoryAction{StoryStartCutsceneAction{std::move(cutscenePath)}};
    }
};
