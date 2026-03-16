#pragma once
#include <vector>
#include <map>
#include <set>
#include "Entity.h"
#include "Daemon.h"
#include "../data/Item.h"
#include "WalkAnimation.h"
#include "Inventory.h"
#include "EventFlags.h"
#include "Party.h"
#include "DaemonDex.h"

class Player : public Entity, public Inventory, public EventFlags, public PartyAndPCBoxes, public DaemonDex
{
public:
    Player(const std::string &name, Position position);

    void update() override;

    void addDaemon(Daemon daemon) override;
};
