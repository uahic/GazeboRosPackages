/*********************************************************************
* Software License Agreement (BSD License)
*
*  Copyright (c) 2008, Willow Garage, Inc.
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above
*     copyright notice, this list of conditions and the following
*     disclaimer in the documentation and/or other materials provided
*     with the distribution.
*   * Neither the name of Willow Garage, Inc. nor the names of its
*     contributors may be used to endorse or promote products derived
*     from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
*  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
*  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
*  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
********************************************************************/

#ifndef ROSBAG_PLAYER_H
#define ROSBAG_PLAYER_H

#include <sys/stat.h>
#if !defined(_MSC_VER)
  #include <termios.h>
  #include <unistd.h>
#endif
#include <time.h>

#include <queue>
#include <string>

#include <ros/ros.h>
#include <ros/time.h>
#include <std_srvs/SetBool.h>

#include "rosbag/bag.h"

#include <topic_tools/shape_shifter.h>

#include "rosbag/time_translator.h"
#include "rosbag/macros.h"

#include <boost/thread/mutex.hpp>

namespace rosbag {


ros::AdvertiseOptions createAdvertiseOptions(MessageInstance const& msg, uint32_t queue_size, const std::string& prefix = "");

ROSBAG_DECL ros::AdvertiseOptions createAdvertiseOptions(const ConnectionInfo* c, uint32_t queue_size, const std::string& prefix = "");


struct ROSBAG_DECL PlayerOptions
{
    PlayerOptions();

    void check();

    std::string prefix;
    bool     quiet;
    bool     start_paused;
    bool     at_once;
    bool     bag_time;
    double   bag_time_frequency;
    double   time_scale;
    int      queue_size;
    ros::WallDuration advertise_sleep;
    bool     try_future;
    bool     has_time;
    bool     loop;
    float    time;
    bool     has_duration;
    float    duration;
    bool     keep_alive;
    bool     wait_for_subscribers;
    std::string rate_control_topic;
    float    rate_control_max_delay;
    ros::Duration skip_empty;

    std::vector<std::string> bags;
    std::vector<std::string> topics;
    std::vector<std::string> pause_topics;
};


class ROSBAG_DECL TimePublisher {
public:
    TimePublisher();

    void setPublishFrequency(double publish_frequency);
    
    void setTimeScale(double time_scale);

    void setHorizon(const ros::Time& horizon);

    void setWCHorizon(const ros::WallTime& horizon);

    void setTime(const ros::Time& time);

    ros::Time const& getTime() const;

    // NRP: added message time, use the real timestamp
    void setMessageTime(const ros::Time& time);

    ros::Time const& getMessageTime() const;

    void runClock(const ros::WallDuration& duration);

    void runStalledClock(const ros::WallDuration& duration);

    void stepClock();

    bool horizonReached();

private:
    bool do_publish_;
    
    double publish_frequency_;
    double time_scale_;
    
    ros::NodeHandle node_handle_;
    ros::Publisher time_pub_;
    
    ros::WallDuration wall_step_;
    
    ros::WallTime next_pub_;

    ros::WallTime wc_horizon_;
    ros::Time horizon_;
    ros::Time current_;
    ros::Time message_;
};



class ROSBAG_DECL Player
{
public:
    Player(PlayerOptions const& options);
    ~Player();

    void publish();

		// NRP: mechanism to stop playback in the middle of a bag
		void shutdown();

    // NRP: run the playback until a certain time
    void runFor(const int &sec, const int& nsec);

private:
    int readCharFromStdin();
    void setupTerminal();
    void restoreTerminal();

    void updateRateTopicTime(const ros::MessageEvent<topic_tools::ShapeShifter const>& msg_event);

    void doPublish(rosbag::MessageInstance const& m);

    void doKeepAlive();

    void printTime();

    bool pauseCallback(std_srvs::SetBool::Request &req, std_srvs::SetBool::Response &res);

    void processPause(const bool paused, ros::WallTime &horizon);

    void waitForSubscribers() const;

private:

    // NRP: mutex to avoid shutdown race conditions
		boost::mutex _shutdown_mutex;

    typedef std::map<std::string, ros::Publisher> PublisherMap;

    PlayerOptions options_;

    ros::NodeHandle node_handle_;

    ros::ServiceServer pause_service_;

    bool paused_;
    bool delayed_;

    bool pause_for_topics_;

    bool pause_change_requested_;

    bool requested_pause_state_;

    ros::Subscriber rate_control_sub_;
    ros::Time last_rate_control_;

    ros::WallTime paused_time_;

    std::vector<boost::shared_ptr<Bag> >  bags_;
    PublisherMap publishers_;

    // Terminal
    bool    terminal_modified_;
#if defined(_MSC_VER)
    HANDLE input_handle;
    DWORD stdin_set;
#else
    termios orig_flags_;
    fd_set  stdin_fdset_;
#endif
    int     maxfd_;

    TimeTranslator time_translator_;
    TimePublisher time_publisher_;

    ros::Time start_time_;
    ros::Duration bag_length_;
};


} // namespace rosbag

#endif