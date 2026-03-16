#pragma once
#include "../Daemon.h"
#include <vector>

class Party {
  public:
    virtual ~Party() = default;
    virtual void addDaemon(Daemon daemon);
    Daemon &getDaemon(int index);
    const Daemon &getDaemon(int index) const;
    const std::vector<Daemon> &getParty() const;
    int partySize() const;
    bool partyEmpty() const;
    void swapDaemon(int indexA, int indexB);

    void clearParty();

  protected:
    std::vector<Daemon> party;
};

class PartyAndPCBoxes : public Party {

  public:
    void addDaemonToBox(Daemon daemon);

    // PC Box storage
    static constexpr int BOX_SIZE = 30;
    static constexpr int NUM_BOXES = 8;
    void depositDaemon(int partyIndex);
    void withdrawDaemon(int boxIndex, int slot);
    bool canDeposit() const;
    bool canWithdraw() const;

    const std::vector<Daemon> &getBox(int boxIndex) const;
    int getBoxCount(int boxIndex) const;
    int getCurrentBox() const;
    void setCurrentBox(int box);
    void clearPCBoxes();

  private:
    std::vector<std::vector<Daemon>> pcBoxes{NUM_BOXES}; // PC storage boxes
    int currentBox{0};                                   // Currently selected box
};
