/***********************************************************************************
 * Revised BSD License                                                             *
 * Copyright (c) 2014, Markus Bader <markus.bader@tuwien.ac.at>                    *
 * All rights reserved.                                                            *
 *                                                                                 *
 * Redistribution and use in source and binary forms, with or without              *
 * modification, are permitted provided that the following conditions are met:     *
 *     * Redistributions of source code must retain the above copyright            *
 *       notice, this list of conditions and the following disclaimer.             *
 *     * Redistributions in binary form must reproduce the above copyright         *
 *       notice, this list of conditions and the following disclaimer in the       *
 *       documentation and/or other materials provided with the distribution.      *
 *     * Neither the name of the Vienna University of Technology nor the           *
 *       names of its contributors may be used to endorse or promote products      *
 *       derived from this software without specific prior written permission.     *
 *                                                                                 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND *
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED   *
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE          *
 * DISCLAIMED. IN NO EVENT SHALL Markus Bader BE LIABLE FOR ANY                    *
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES      *
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;    *
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND     *
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT      *
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS   *
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                    *                       *
 ***********************************************************************************/

#include "rawlog_play_node.h"
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <mrpt_bridge/pose.h>
#include <mrpt_bridge/laser_scan.h>
#include <mrpt_bridge/time.h>
#include <mrpt/base.h>
#include <mrpt/slam.h>
#include <mrpt/gui.h>


int main(int argc, char **argv) {

    ros::init(argc, argv, "DataLogger");
    ros::NodeHandle n;
    RawlogPlayNode my_node(n);
    my_node.init();
    my_node.loop();
    return 0;
}

RawlogPlayNode::~RawlogPlayNode() {
}

RawlogPlayNode::RawlogPlayNode(ros::NodeHandle &n) :
    RawlogPlay(new RawlogPlayNode::ParametersNode()), n_(n), loop_count_(0) {

}

RawlogPlayNode::ParametersNode *RawlogPlayNode::param() {
    return (RawlogPlayNode::ParametersNode*) param_;
}

void RawlogPlayNode::init() {

    if(!mrpt::utils::fileExists(param_->rawlog_file)) {
        ROS_ERROR("raw_file: %s does not exit", param_->rawlog_file.c_str());
    }
    rawlog_stream_.open(param_->rawlog_file);
    pub_laser_ = n_.advertise<sensor_msgs::LaserScan>("scan", 1000);

}

bool RawlogPlayNode::nextEntry() {
    mrpt::slam::CActionCollectionPtr action;
    mrpt::slam::CSensoryFramePtr     observations;
    mrpt::slam::CObservationPtr      obs;

    if(!mrpt::slam::CRawlog::getActionObservationPairOrObservation( rawlog_stream_, action, observations, obs, entry_)) {
        ROS_INFO("end of stream!");
        return true;
    }
    mrpt::poses::CPose3D pose_laser;
    geometry_msgs::Pose msg_pose_laser;
    tf::Transform transform;
    mrpt::slam::CObservation2DRangeScanPtr laser = observations->getObservationByClass<mrpt::slam::CObservation2DRangeScan>();
    mrpt_bridge::laser_scan::mrpt2ros(*laser, msg_laser_, msg_pose_laser);
    laser->getSensorPose(pose_laser);
    mrpt_bridge::poses::mrpt2ros(pose_laser, transform);
    tf_broadcaster_.sendTransform(tf::StampedTransform(transform, msg_laser_.header.stamp, param()->base_link, msg_laser_.header.frame_id));
    pub_laser_.publish(msg_laser_);
    return false;

}

void RawlogPlayNode::loop() {
    bool end = false;
    for (ros::Rate rate(param()->rate); ros::ok() && !end; loop_count_++) {
        param()->update(loop_count_);
        end = nextEntry();
        ros::spinOnce();
        rate.sleep();
    }
}
