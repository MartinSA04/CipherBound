#pragma once
#include "../data/Move.h"
#include "../data/Species.h"
#include <array>
#include <string>

struct MoveSlot {
    int moveId; // can be -1 for None
    int currentPP;
    int maxPP;
};

class Daemon {
  public:
    Daemon(const Species &species, int level);

    // Rehydrate from saved data
    Daemon(const Species &species, int level, int exp, int currentHP, const std::string &nickname, StatusEffect status,
           const BaseStats &ivs, const BaseStats &evs, const std::array<MoveSlot, 4> &moves);

    const std::string &getNickname() const;
    void setNickname(const std::string &name);

    int getLevel() const;
    int getExp() const;
    int getExpNeeded() const;
    void addExp(int amount);
    bool checkLevelUp();

    int getCurrentHP() const;
    int getMaxHP() const;
    void takeDamage(int amount);
    void heal(int amount);
    void fullHeal();
    bool isFainted() const;

    int getStat(int statIndex) const; // 0=hp,1=atk,2=def,3=spa,4=spd,5=spe
    const Species &getSpecies() const;
    StatusEffect getStatus() const;
    void setStatus(StatusEffect status);
    void clearStatus();

    const std::array<MoveSlot, 4> &getMoves() const;
    bool useMove(int slot); // Deduct 1 PP, returns false if no PP left
    bool learnMove(int moveId, int slot);

    int getSpeciesId() const;

    const BaseStats &getIVs() const;
    const BaseStats &getEVs() const;

  private:
    std::string nickname;
    int speciesId;
    int level;
    int exp;
    int currentHP;
    StatusEffect status;

    BaseStats ivs; // individual values
    BaseStats evs; // effort values

    std::array<MoveSlot, 4> moves;

    int calculateStat(int base, int iv, int ev, bool isHP) const;

    const Species *speciesRef;
};
