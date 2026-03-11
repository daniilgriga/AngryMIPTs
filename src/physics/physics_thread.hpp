#pragma once

#include "physics_engine.hpp"

#include "../shared/command.hpp"
#include "../shared/event.hpp"
#include "../shared/level_data.hpp"
#include "../shared/thread_safe_queue.hpp"
#include "../shared/world_snapshot.hpp"

#include <mutex>
#include <vector>

namespace angry
{

// Stage 1 skeleton: lifecycle + API surface, no worker thread yet.
class PhysicsThread
{
public:
    PhysicsThread() = default;
    ~PhysicsThread();

    void start();
    void stop();
    bool isRunning() const;

    void registerLevel(const LevelData& level);
    void loadLevel(const LevelData& level);
    void pushCommand(const Command& cmd);

    // Temporary adapter for single-thread integration while worker loop is not introduced yet.
    void tickSingleThread(float dt);

    WorldSnapshot readSnapshot() const;
    std::vector<Event> drainEvents();

private:
    mutable std::mutex mutex_;
    PhysicsEngine engine_;
    ThreadSafeQueue<Command> commandQueue_;
    bool running_ = false;
};

}  // namespace angry
