// ============================================================
// physics_runtime.cpp — Physics mode facade implementation.
// Part of: angry::physics
//
// Implements runtime dispatch between backends:
//   * Starts/stops worker thread in threaded mode
//   * Forwards level registration/loading and commands
//   * Runs direct stepping only in single-threaded mode
//   * Exposes snapshot/event reads through one interface
// ============================================================

#include "physics_runtime.hpp"

namespace angry
{

// #=# Construction / Destruction #=#=#=#=#=#=#=#=#=#=#=#=#=#=#

PhysicsRuntime::PhysicsRuntime(PhysicsMode mode)
    : mode_(mode)
{
    if (mode_ == PhysicsMode::Threaded)
    {
        threadedEngine_.start();
    }
}

PhysicsRuntime::~PhysicsRuntime()
{
    if (mode_ == PhysicsMode::Threaded)
    {
        threadedEngine_.stop();
    }
}

// #=# Public API #=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#

void PhysicsRuntime::registerLevel(const LevelData& level)
{
    if (mode_ == PhysicsMode::Threaded)
    {
        threadedEngine_.registerLevel(level);
        return;
    }

    singleEngine_.register_level(level);
}

void PhysicsRuntime::loadLevel(const LevelData& level)
{
    if (mode_ == PhysicsMode::Threaded)
    {
        threadedEngine_.loadLevel(level);
        return;
    }

    singleEngine_.load_level(level);
}

void PhysicsRuntime::processCommands(ThreadSafeQueue<Command>& cmdQueue)
{
    if (mode_ == PhysicsMode::Threaded)
    {
        while (const std::optional<Command> cmd = cmdQueue.try_pop())
        {
            threadedEngine_.pushCommand(*cmd);
        }
        return;
    }

    singleEngine_.process_commands(cmdQueue);
}

void PhysicsRuntime::step(float dt)
{
    if (mode_ == PhysicsMode::Threaded)
    {
        // Worker thread owns simulation updates in threaded mode.
        (void)dt;
        return;
    }

    singleEngine_.step(dt);
}

WorldSnapshot PhysicsRuntime::getSnapshot() const
{
    if (mode_ == PhysicsMode::Threaded)
    {
        return threadedEngine_.readSnapshot();
    }

    return singleEngine_.get_snapshot();
}

std::vector<Event> PhysicsRuntime::drainEvents()
{
    if (mode_ == PhysicsMode::Threaded)
    {
        return threadedEngine_.drainEvents();
    }

    return singleEngine_.drain_events();
}

// #=# Accessors #=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=#=

PhysicsMode PhysicsRuntime::mode() const
{
    return mode_;
}

}  // namespace angry
