/**
 * @file
 * @brief Shared on-screen text-entry panel used by multiple modes.
 * @ingroup app_core
 */

#pragma once
#include "../../ui/GameUI.h"
#include "../../ui/InputManager.h"
#include <string>

/// Result of activating the currently selected key on the name-entry panel.
enum class NameEntryAction {
    none,      ///< No state change occurred.
    edited,    ///< The entered text changed.
    auxiliary, ///< The configurable auxiliary key was activated.
    submit,    ///< The DONE key was activated.
};

/// Rendering options for the shared name-entry panel.
struct NameEntryRenderOptions {
    std::string fieldLabel{"Name"}; ///< Label displayed above the text field.
    std::string footerPrimary;      ///< Primary help text at the bottom of the screen.
    std::string footerSecondary;    ///< Secondary help text at the bottom of the screen.
    int nameBoxY{120};              ///< Top position of the text field.
};

/// Shared keyboard-style text-entry panel for names and nicknames.
class NameEntryPanel {
  public:
    explicit NameEntryPanel(std::string auxiliaryKeyLabel = "BACK", int maxLength = 12);

    /// Resets the panel text and cursor selection.
    void reset(std::string initialText = {});
    /// Updates the label shown on the auxiliary key.
    void setAuxiliaryKeyLabel(std::string label);
    /// Replaces the current text without changing selection.
    void setText(std::string value);

    /// Returns the current raw text.
    const std::string &getText() const;
    /// Returns the current text with trailing spaces trimmed.
    std::string normalizedText() const;
    /// Returns trimmed text, or the fallback when the text would be empty.
    std::string normalizedTextOr(const std::string &fallback) const;

    /// Moves the selected key based on directional input.
    void navigate(const InputManager &input);
    /// Deletes the last character, if any.
    NameEntryAction backspace();
    /// Activates the currently selected key.
    NameEntryAction activateSelectedKey();

    /// Draws the shared text field and on-screen keyboard.
    void render(GameUI &ui, const NameEntryRenderOptions &options) const;

  private:
    std::string keyLabel(int index) const;
    bool appendCharacter(char c);
    bool appendSpace();

    std::string auxiliaryKeyLabel;
    std::string text;
    int maxLength{12};
    int selectedKey{0};
};
