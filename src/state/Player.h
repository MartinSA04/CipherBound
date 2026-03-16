#pragma once

#include "Daemon.h"
#include "DaemonDex.h"
#include "Entity.h"
#include "EventFlags.h"
#include "Inventory.h"
#include "Party.h"
#include "WalkAnimation.h"

class Player : public Entity,
               public Inventory,
               public EventFlags,
               public PartyAndPCBoxes,
               public DaemonDex {
  public:
    Player(const std::string &name, Position position);

    void update() override;

    void addDaemon(Daemon daemon) override;
};
