/*
 * Copyright (C) 2017 Open Source Robotics Foundation
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

#include <functional>
#include <string>

#include <gazebo/common/Events.hh>
#include <gazebo/common/Plugin.hh>
#include <gazebo/physics/PhysicsIface.hh>
#include <gazebo/physics/Joint.hh>
#include <gazebo/physics/Model.hh>
#include <gazebo/physics/World.hh>
#include "ConveyorBeltPlugin.hh"
#include <iostream>

using namespace gazebo;

GZ_REGISTER_MODEL_PLUGIN(ConveyorBeltPlugin)

/////////////////////////////////////////////////
ConveyorBeltPlugin::~ConveyorBeltPlugin()
{
}

/////////////////////////////////////////////////
void ConveyorBeltPlugin::Load(physics::ModelPtr _model, sdf::ElementPtr _sdf)
{
  std::cout << "Conveyor online" << std::endl;

  // Read the power of the belt.
  if (_sdf->HasElement("power"))
  {
    this->beltPower = _sdf->Get<double>("power");
  }

  // Read and set the joint that controls the belt.
  std::string jointName = "belt_joint";
  if (_sdf->HasElement("joint"))
  {
    jointName = _sdf->Get<std::string>("joint");
  }

  std::cout << "joint name: " << jointName << std::endl;
  std::cout << "Conveyor model name: " << _model->GetName() << std::endl;
  std::cout << "Child in the model: " << _model->GetChild("conveyor_belt") << std::endl;
  this->joint = _model->GetJoint(jointName);
  if (!this->joint)
  {
    std::cerr << "Joint [" << jointName << "] not found, belt disabled" << std::endl;;
    return;
  }

  // Read and set the belt's link.
  std::string linkName = "belt_link";
  if (_sdf->HasElement("link"))
    linkName = _sdf->Get<std::string>("link");
  std::cout << "Using link name of: [" << linkName << "]" << std::endl;

  auto worldPtr = gazebo::physics::get_world();
  this->link = boost::static_pointer_cast<physics::Link>(
    worldPtr->EntityByName(linkName));

  if (!this->link)
  {
    std::cerr << "Link not found" << std::endl;
    return;
  }

  // Set the point where the link will be moved to its starting pose.
  this->limit = this->joint->UpperLimit(0) - 0.9;

  // Initialize Gazebo transport
  this->gzNode = transport::NodePtr(new transport::Node());
  this->gzNode->Init();


  // Listen to the update event that is broadcasted every simulation iteration.
  this->updateConnection = event::Events::ConnectWorldUpdateEnd(
    std::bind(&ConveyorBeltPlugin::OnUpdate, this));

  std::cout << "Using belt power of: " << this->beltPower << "%" << std::endl;
  this->SetPower(this->beltPower);
}

/////////////////////////////////////////////////
void ConveyorBeltPlugin::OnUpdate()
{
  this->joint->SetVelocity(0, this->beltVelocity);
  // Reset the belt.
  if (this->joint->Position(0) >= this->limit)
  {
    // Warning: Megahack!!
    // We should use "this->joint->SetPosition(0, 0)" here but I found that
    // this line occasionally freezes the joint. I tracked the problem and
    // found an incorrect value in childLinkPose within
    // Joint::SetPositionMaximal(). This workaround makes sure that the right
    // numbers are always used in our scenario.
    this->joint->SetPosition(0, 0);
    //const ignition::math::Pose3d childLinkPose(1.20997, 2.5998, 0.8126, 0, 0, -1.57);
    //const ignition::math::Pose3d newChildLinkPose(1.20997, 2.98, 0.8126, 0, 0, -1.57);
    //this->link->MoveFrame(childLinkPose, newChildLinkPose);
  }
}

/////////////////////////////////////////////////
double ConveyorBeltPlugin::Power() const
{
  if (!this->joint || !this->link)
    return 0.0;

  return this->beltPower;
}

/////////////////////////////////////////////////
void ConveyorBeltPlugin::SetPower(const double _power)
{
  if (!this->joint || !this->link)
    return;

  if (_power < 0 || _power > 100)
  {
    std::cerr << "Incorrect power value [" << _power << "]" << std::endl;
    std::cerr << "Accepted values are in the [0-100] range" << std::endl;
    return;
  }

  this->beltPower = _power;

  // Convert the power (percentage) to a velocity.
  this->beltVelocity = this->kMaxBeltLinVel * this->beltPower / 100.0;
  std::cout << "Received power of: " << _power << ", setting velocity to: " << this->beltVelocity << std::endl;
}
