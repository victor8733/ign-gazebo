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

#include "ServerPrivate.hh"

#include <functional>
#include <ignition/common/Console.hh>
#include <ignition/math/Pose3.hh>
#include <sdf/Model.hh>
#include <sdf/Root.hh>

using namespace ignition;
using namespace gazebo;

/////////////////////////////////////////////////
ServerPrivate::ServerPrivate()
{
  this->sigHandler.AddCallback(
      std::bind(&ServerPrivate::OnSignal, this, std::placeholders::_1));
}

/////////////////////////////////////////////////
ServerPrivate::~ServerPrivate()
{
  if (this->runThread.joinable())
  {
    this->running = false;
    this->runThread.join();
  }
}

/////////////////////////////////////////////////
void ServerPrivate::UpdateSystems()
{
  /// \todo(nkoenig) Update systems
}

/////////////////////////////////////////////////
bool ServerPrivate::Run(const uint64_t _iterations,
    std::optional<std::condition_variable *> _cond)
{
  this->runMutex.lock();
  this->running = true;
  if (_cond)
    _cond.value()->notify_all();
  this->runMutex.unlock();

  // Execute all the systems until we are told to stop, or the number of
  // iterations is reached.
  for (uint64_t startingIterations = this->iterations;
       this->running && (_iterations == 0 ||
                         this->iterations < _iterations + startingIterations);
       ++this->iterations)
  {
    this->UpdateSystems();
  }

  this->running = false;
  return true;
}

//////////////////////////////////////////////////
void ServerPrivate::OnSignal(int _sig)
{
  igndbg << "Server received signal[" << _sig  << "]\n";
  this->running = false;
}

//////////////////////////////////////////////////
void ServerPrivate::EraseEntities()
{
  this->entities.clear();
}

//////////////////////////////////////////////////
void ServerPrivate::CreateEntities(const sdf::Root &_root)
{
  for (uint64_t i = 0; i < _root.ModelCount(); ++i)
  {
    /// \todo(nkoenig) This should use free entity slots.
    EntityId entityId = this->entities.size();
    this->entities.push_back(Entity(entityId));

    // Get the SDF model
    const sdf::Model *model = _root.ModelByIndex(i);

    // Create the pose component for the model.
    ComponentKey compKey = this->componentMgr.CreateComponent(model->Pose());
    this->entityComponents[entityId].push_back(compKey);

    /// \todo(nkoenig) Notify systems that entities have been created.
  }
}