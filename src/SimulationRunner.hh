/*
 * Copyright (C) 2018 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/
#ifndef IGNITION_GAZEBO_SIMULATIONRUNNER_HH_
#define IGNITION_GAZEBO_SIMULATIONRUNNER_HH_

#include <atomic>
#include <chrono>
#include <functional>
#include <list>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include <ignition/common/WorkerPool.hh>
#include <ignition/transport/Node.hh>

#include <sdf/World.hh>

#include "ignition/gazebo/config.hh"
#include "ignition/gazebo/EntityComponentManager.hh"
#include "ignition/gazebo/Export.hh"
#include "ignition/gazebo/System.hh"
#include "ignition/gazebo/SystemPluginPtr.hh"
#include "ignition/gazebo/Types.hh"

using namespace std::chrono_literals;

namespace ignition
{
  namespace gazebo
  {
    // Inline bracket to help doxygen filtering.
    inline namespace IGNITION_GAZEBO_VERSION_NAMESPACE {
    // Forward declarations.
    class SimulationRunnerPrivate;

    /// \brief Class to hold systems internally
    class SystemInternal
    {
      /// \brief Constructor
      public: explicit SystemInternal(const SystemPluginPtr &_systemPlugin)
              : systemPlugin(_systemPlugin),
                system(systemPlugin->QueryInterface<System>()),
                preupdate(systemPlugin->QueryInterface<ISystemPreUpdate>()),
                update(systemPlugin->QueryInterface<ISystemUpdate>()),
                postupdate(systemPlugin->QueryInterface<ISystemPostUpdate>())
      {
      }

      /// \brief Plugin object. This manages the lifecycle of the instantiated
      /// class as well as the shared library.
      public: SystemPluginPtr systemPlugin;

      /// \brief Access this system via the `System` interface
      public: System *system = nullptr;

      /// \brief Access this system via the ISystemPreUpdate interface
      /// Will be nullptr if the System doesn't implement this interface.
      public: ISystemPreUpdate *preupdate = nullptr;

      /// \brief Access this system via the ISystemUpdate interface
      /// Will be nullptr if the System doesn't implement this interface.
      public: ISystemUpdate *update = nullptr;

      /// \brief Access this system via the ISystemPostUpdate interface
      /// Will be nullptr if the System doesn't implement this interface.
      public: ISystemPostUpdate *postupdate = nullptr;

      /// \brief Vector of queries and callbacks
      public: std::vector<EntityQueryCallback> updates;
    };

    class IGNITION_GAZEBO_VISIBLE SimulationRunner
    {
      /// \brief Constructor
      /// \param[in] _world Pointer to the SDF world.
      /// \param[in] _systems Systems to be loaded
      public: explicit SimulationRunner(const sdf::World *_world,
                const std::vector<SystemPluginPtr> &_systems);

      /// \brief Destructor.
      public: virtual ~SimulationRunner();

      /// \brief Stop running
      public: void Stop();

      /// \brief Run the simulationrunner.
      /// \param[in] _iterations Number of iterations.
      /// \return True if the operation completed successfully.
      public: bool Run(const uint64_t _iterations);

      /// \brief Add system after the simulation runner has been instantiated
      /// \param[in] _system System to be added
      public: void AddSystem(const SystemPluginPtr &_system);

      /// \brief Update all the systems
      public: void UpdateSystems();

      /// \brief Publish current world statistics.
      public: void PublishStats();

      /// \brief Create all entities that exist in the sdf::World object.
      /// \param[in] _world SDF world object.
      public: void CreateEntities(const sdf::World *_world);

      /// \brief Get whether this is running. When running is true,
      /// then simulation is stepping forward.
      /// \return True if the server is running.
      public: bool Running() const;

      /// \brief Get the number of iterations the server has executed.
      /// \return The current iteration count.
      public: uint64_t IterationCount() const;

      /// \brief Get the number of entities on the runner.
      /// \return Entity count.
      public: size_t EntityCount() const;

      /// \brief Get the number of systems on the runner.
      /// \return System count.
      public: size_t SystemCount() const;

      /// \brief Set the update period. The update period is the wall-clock
      /// time between updates of all systems. Note that even if systems
      /// are being updated, this doesn't mean sim time is increasing.
      /// \param[in] _updatePeriod Duration between updates.
      public: void SetUpdatePeriod(
                  const std::chrono::steady_clock::duration &_updatePeriod);

      /// \brief Get the update period.
      /// \return The update period.
      public: const std::chrono::steady_clock::duration &UpdatePeriod() const;

      /// \brief Set the paused state.
      /// \param[in] _paused True to pause the simulation runner.
      public: void SetPaused(const bool _paused);

      /// \brief Get the pause state.
      /// \return True if the simulation runner is paused, false otherwise.
      public: bool Paused() const;

      /// \brief Get the EntityComponentManager
      /// \return Reference to the entity component manager.
      public: const EntityComponentManager &EntityCompMgr() const;

      /// \brief Get the current info object.
      /// \return Current info.
      public: const UpdateInfo &CurrentInfo() const;

      /// \brief Get the step size;
      /// \return Step size.
      public: const ignition::math::clock::duration &StepSize() const;

      /// \brief Set the step size;
      /// \param[in] _step Step size.
      public: void SetStepSize(const ignition::math::clock::duration &_step);

      /// \brief World control service callback. This function stores the
      /// the request which will then be processed by the ProcessMessages
      /// function.
      /// \param[in] _req Request from client, currently handling play / pause
      /// and multistep.
      /// \param[out] _res Response to client, true if successful.
      /// \return True for success
      private: bool OnWorldControl(const msgs::WorldControl &_req,
                                         msgs::Boolean &_res);

      /// \brief Calculate real time factor and populate currentInfo.
      private: void UpdateCurrentInfo();

      /// \brief Process all buffered messages. Ths function is called at
      /// the end of an update iteration.
      private: void ProcessMessages();

      /// \brief Process world control service messages.
      private: void ProcessWorldControl();

      /// \brief This is used to indicate that Run has been called, and the
      /// server is in the run state.
      private: std::atomic<bool> running{false};

      /// \brief All the systems.
      private: std::vector<SystemInternal> systems;

      /// \brief Systems implementing PreUpdate
      private: std::vector<ISystemPreUpdate*> systemsPreupdate;

      /// \brief Systems implementing Update
      private: std::vector<ISystemUpdate*> systemsUpdate;

      /// \brief Systems implementing PostUpdate
      private: std::vector<ISystemPostUpdate*> systemsPostupdate;

      /// \brief Manager of all components.
      private: EntityComponentManager entityCompMgr;

      /// \brief A pool of worker threads.
      private: common::WorkerPool workerPool{2};

      /// \brief Wall time of the previous update.
      private: std::chrono::steady_clock::time_point prevUpdateRealTime;

      /// \brief A duration used to account for inaccuracies associated with
      /// sleep durations.
      private: std::chrono::steady_clock::duration sleepOffset{0};

      /// \brief This is the rate at which the systems are updated.
      /// The default update rate is 500hz, which is a period of 2ms.
      private: std::chrono::steady_clock::duration updatePeriod{2ms};

      /// \brief List of simulation times used to compute averages.
      private: std::list<std::chrono::steady_clock::duration> simTimes;

      /// \brief List of real times used to compute averages.
      private: std::list<std::chrono::steady_clock::duration> realTimes;

      /// \brief Node for communication.
      private: ignition::transport::Node node;

      /// \brief World statistics publisher.
      private: ignition::transport::Node::Publisher statsPub;

      /// \brief Name of world being simulated.
      private: std::string worldName;

      /// \brief Stopwatch to keep track of wall time.
      private: ignition::math::Stopwatch realTimeWatch;

      /// \brief Step size
      private: ignition::math::clock::duration stepSize{10ms};

      /// \brief The real time factor calculated based on sim and real time
      /// averages.
      private: double realTimeFactor{0.0};

      /// \brief Number of simulation steps requested that haven't been
      /// executed yet.
      private: unsigned int pendingSimIterations{0};

      /// \brief Keeps the latest simulation info.
      private: UpdateInfo currentInfo;

      /// \brief Buffer of world control messages.
      private: std::list<msgs::WorldControl> worldControlMsgs;

      /// \brief Mutex to protect message buffers.
      private: std::mutex msgBufferMutex;
    };
    }
  }
}
#endif