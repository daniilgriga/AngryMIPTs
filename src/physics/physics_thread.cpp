#include "physics_thread.hpp"

namespace angry
{

PhysicsThread::~PhysicsThread()
{
    stop();
}

void PhysicsThread::start()
{
    std::lock_guard<std::mutex> lock(mutex_);
    running_ = true;
}

void PhysicsThread::stop()
{
    std::lock_guard<std::mutex> lock(mutex_);
    running_ = false;
}

bool PhysicsThread::isRunning() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return running_;
}

void PhysicsThread::registerLevel(const LevelData& level)
{
    std::lock_guard<std::mutex> lock(mutex_);
    engine_.registerLevel(level);
}

void PhysicsThread::loadLevel(const LevelData& level)
{
    std::lock_guard<std::mutex> lock(mutex_);
    engine_.loadLevel(level);
}

void PhysicsThread::pushCommand(const Command& cmd)
{
    commandQueue_.push(cmd);
}

void PhysicsThread::tickSingleThread(float dt)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!running_)
    {
        return;
    }

    engine_.processCommands(commandQueue_);
    engine_.step(dt);
}

WorldSnapshot PhysicsThread::readSnapshot() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return engine_.getSnapshot();
}

std::vector<Event> PhysicsThread::drainEvents()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return engine_.drainEvents();
}

}  // namespace angry
