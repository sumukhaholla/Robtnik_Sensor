/*
 * Copyright 2013 Open Source Robotics Foundation
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
/*
 * Desc: Syncronizes shutters across multiple cameras
 * Author: John Hsu
 * Date: 10 June 2013
 */

#include <string>

#include <gazebo/sensors/Sensor.hh>
#include <gazebo/sensors/MultiCameraSensor.hh>
#include <gazebo/sensors/SensorTypes.hh>

#include "gazebo_plugins/gazebo_ros_multicamera.h"

namespace gazebo
{
// Register this plugin with the simulator
GZ_REGISTER_SENSOR_PLUGIN(GazeboRosMultiCamera)

////////////////////////////////////////////////////////////////////////////////
// Constructor
GazeboRosMultiCamera::GazeboRosMultiCamera()
{
}

////////////////////////////////////////////////////////////////////////////////
// Destructor
GazeboRosMultiCamera::~GazeboRosMultiCamera()
{
}

void GazeboRosMultiCamera::Load(sensors::SensorPtr _parent,
  sdf::ElementPtr _sdf)
{
  MultiCameraPlugin::Load(_parent, _sdf);

  // Make sure the ROS node for Gazebo has already been initialized
  if (!ros::isInitialized())
  {
    ROS_FATAL_STREAM_NAMED("multicamera", "A ROS node for Gazebo has not been initialized, unable to load plugin. "
      << "Load the Gazebo system plugin 'libgazebo_ros_new_api_plugin.so' in the gazebo_ros_new package)");
    return;
  }

  // initialize shared_ptr members
  this->image_connect_count_ = boost::shared_ptr<int>(new int(0));
  this->image_connect_count_lock_ = boost::shared_ptr<boost::mutex>(new boost::mutex);
  this->was_active_ = boost::shared_ptr<bool>(new bool(false));

  // copying from CameraPlugin into GazeboRosCameraUtils
  for (unsigned i = 0; i < this->camera.size(); ++i)
  {
    GazeboRosCameraUtils* util = new GazeboRosCameraUtils();
    util->parentSensor_ = this->parentSensor;
    util->width_   = this->width[i];
    util->height_  = this->height[i];
    util->depth_   = this->depth[i];
    util->format_  = this->format[i];
    util->camera_  = this->camera[i];
    // Set up a shared connection counter
    util->image_connect_count_ = this->image_connect_count_;
    util->image_connect_count_lock_ = this->image_connect_count_lock_;
    util->was_active_ = this->was_active_;
    if (this->camera[i]->Name().find("left") != std::string::npos)
    {
      // FIXME: hardcoded, left hack_baseline_ 0
      util->Load(_parent, _sdf, "/left", 0.0);
    }
    else if (this->camera[i]->Name().find("right") != std::string::npos)
    {
      double hackBaseline = 0.0;
      if (_sdf->HasElement("hackBaseline"))
        hackBaseline = _sdf->Get<double>("hackBaseline");
      util->Load(_parent, _sdf, "/right", hackBaseline);
    }
    this->utils.push_back(util);
  }
}

////////////////////////////////////////////////////////////////////////////////

void GazeboRosMultiCamera::OnNewFrame(const unsigned char *_image,
    GazeboRosCameraUtils* util)
{
  common::Time sensor_update_time = util->parentSensor_->LastMeasurementTime();

  if (util->parentSensor_->IsActive())
  {
    if (sensor_update_time - util->last_update_time_ >= util->update_period_)
    {
      util->PutCameraData(_image, sensor_update_time);
      util->PublishCameraInfo(sensor_update_time);
      util->last_update_time_ = sensor_update_time;
    }
  }
}

// Update the controller
void GazeboRosMultiCamera::OnNewFrameLeft(const unsigned char *_image,
    unsigned int _width, unsigned int _height, unsigned int _depth,
    const std::string &_format)
{
  OnNewFrame(_image, this->utils[0]);
}

////////////////////////////////////////////////////////////////////////////////
// Update the controller
void GazeboRosMultiCamera::OnNewFrameRight(const unsigned char *_image,
    unsigned int _width, unsigned int _height, unsigned int _depth,
    const std::string &_format)
{
  OnNewFrame(_image, this->utils[1]);
}
}
