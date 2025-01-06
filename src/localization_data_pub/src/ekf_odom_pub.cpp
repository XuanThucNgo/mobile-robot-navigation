#include "ros/ros.h"
#include "std_msgs/Int16.h"
#include "std_msgs/String.h"
#include <nav_msgs/Odometry.h>
#include <geometry_msgs/PoseStamped.h>
#include <tf2/LinearMath/Quaternion.h>
#include <tf2_ros/transform_broadcaster.h>
#include "geometry_msgs/Twist.h"
#include <actionlib_msgs/GoalStatusArray.h>
#include <serial/serial.h>

#include <cmath>
#include <iostream>
#include <string>

#define RUN_WITH_UART

#ifdef RUN_WITH_UART
serial::Serial ser("/dev/ttyAMA0", 115200, serial::Timeout::simpleTimeout(100));
#endif

// Create odometry data publishers
ros::Publisher odom_data_pub;
ros::Publisher odom_data_pub_quat;
nav_msgs::Odometry odomNew;
nav_msgs::Odometry odomOld;
 
// Initial pose
const double initialX = 0.0;
const double initialY = 0.0;
const double initialTheta = 0.00000000001;
const double PI = 3.141592;
 
// Robot physical constants
const double WHEEL_RADIUS = 0.0325; // Wheel radius in meters
const double WHEEL_BASE = 0.3; // Center of left tire to center of right tire

const double TICKS_PER_REV_LEFT = 495; 
const double TICKS_PER_REV_RIGHT = 495; 
 
bool initialPoseRecieved = false;

std_msgs::Int16 right_ticks_msg;
std_msgs::Int16 left_ticks_msg;

using namespace std;

// Distance both wheels have traveled
double distanceLeft = 0.0;
double distanceRight = 0.0;
static int lastCountL = 0;
static int lastCountR = 0;
const float WHEEL_CIRCUMFERENCE_LEFT = 0.2041; // R = 0.05
const float WHEEL_CIRCUMFERENCE_RIGHT = 0.2041; // R = 0.05
float checkDistanceR, checkDistanceL;
void Calc_Left(const std_msgs::Int16& leftCount) { 
  if(leftCount.data) {
    int leftTicks = leftCount.data - lastCountL;
    lastCountL = leftCount.data;

    if (leftTicks > 10000) {
      leftTicks = 0 - (65535 - leftTicks);
    }
    else if (leftTicks < -10000) {
      leftTicks = 65535 - leftTicks;
    }
    distanceLeft = ((leftTicks * WHEEL_CIRCUMFERENCE_LEFT) / TICKS_PER_REV_LEFT);
    checkDistanceL += distanceLeft;
  }

  // ROS_INFO("Distance traveled by the left wheel: %f meters", checkDistanceL);
}

void Calc_Right(const std_msgs::Int16& rightCount) {
  if(rightCount.data) {
    int rightTicks = rightCount.data - lastCountR;
    lastCountR = rightCount.data;
    
    if (rightTicks > 10000) {
      rightTicks = 0 - (65535 - rightTicks);
    }
    else if (rightTicks < -10000) {
      rightTicks = 65535 - rightTicks;
    }
    distanceRight = ((rightTicks * WHEEL_CIRCUMFERENCE_RIGHT) / TICKS_PER_REV_RIGHT);
    checkDistanceR += distanceRight;
  }
    
  // ROS_INFO("Distance traveled by the right wheel: %f meters", checkDistanceR);
} 

#ifdef RUN_WITH_UART
void receiveDataFromSerial() {
  if (ser.isOpen()) {
    string serialData = ser.read(ser.available());
    if (!serialData.empty()) {
      ROS_INFO("Character read: %s", serialData.c_str());
      
      istringstream ss(serialData);
      string line;

      std_msgs::Int16 right_ticks_msg;
      std_msgs::Int16 left_ticks_msg;

      while (getline(ss, line)) {
        int leftTicks = 0;
        int rightTicks = 0;
        if (sscanf(line.c_str(), "%dr%dl", &rightTicks, &leftTicks) == 2) {
          right_ticks_msg.data = rightTicks;
          Calc_Right(right_ticks_msg);

          left_ticks_msg.data = leftTicks;
          Calc_Left(left_ticks_msg);
        } else {
          ROS_WARN("Invalid data format: %s", line.c_str());
        }
      }
    } else {
      distanceRight = 0;
      distanceLeft = 0;
    }
  } 
}

void sendUart(float velR, float velL) {
  char message[50];
  snprintf(message, sizeof(message), "%.2f r %.2f l", velR, velL);
  ROS_INFO("Velocity send to STM32: %.02f %.02f", velR, velL);
  ser.write(message);
  ser.flush();
}
#endif

// Get initial_2d message from either Rviz clicks or a manual pose publisher
void set_initial_2d(const geometry_msgs::PoseStamped &rvizClick) {
  odomOld.pose.pose.position.x = rvizClick.pose.position.x;
  odomOld.pose.pose.position.y = rvizClick.pose.position.y;
  odomOld.pose.pose.orientation.z = rvizClick.pose.orientation.z;
  initialPoseRecieved = true;

  ROS_INFO("initialPoseRecieved = %s", initialPoseRecieved ? "true" : "false");
}

// Publish a nav_msgs::Odometry message in quaternion format
void publish_quat() {
 
  tf2::Quaternion q;
         
  q.setRPY(0, 0, odomNew.pose.pose.orientation.z);
 
  nav_msgs::Odometry quatOdom;
  quatOdom.header.stamp = odomNew.header.stamp;
  quatOdom.header.frame_id = "odom";
  quatOdom.child_frame_id = "base_link";
  quatOdom.pose.pose.position.x = odomNew.pose.pose.position.x;
  quatOdom.pose.pose.position.y = odomNew.pose.pose.position.y;
  quatOdom.pose.pose.position.z = odomNew.pose.pose.position.z;
  quatOdom.pose.pose.orientation.x = q.x();
  quatOdom.pose.pose.orientation.y = q.y();
  quatOdom.pose.pose.orientation.z = q.z();
  quatOdom.pose.pose.orientation.w = q.w();
  quatOdom.twist.twist.linear.x = odomNew.twist.twist.linear.x;
  quatOdom.twist.twist.linear.y = odomNew.twist.twist.linear.y;
  quatOdom.twist.twist.linear.z = odomNew.twist.twist.linear.z;
  quatOdom.twist.twist.angular.x = odomNew.twist.twist.angular.x;
  quatOdom.twist.twist.angular.y = odomNew.twist.twist.angular.y;
  quatOdom.twist.twist.angular.z = odomNew.twist.twist.angular.z;
 
  for(int i = 0; i<36; i++) {
    if(i == 0 || i == 7 || i == 14) {
      quatOdom.pose.covariance[i] = 0.01;
     }
     else if (i == 21 || i == 28 || i== 35) {
       quatOdom.pose.covariance[i] = 0.1;
     }
     else {
       quatOdom.pose.covariance[i] = 0;
     }
  }
 
  odom_data_pub_quat.publish(quatOdom);
}
 
// Update odometry information
void update_odom() {
  // Calculate the average distance
  double cycleDistance = (distanceRight + distanceLeft) / 2;

  // Calculate the number of radians the robot has turned since the last cycle
  double cycleAngle = -asin((distanceRight - distanceLeft) / WHEEL_BASE);
  // double cycleAngle = atan2(distanceRight - distanceLeft, WHEEL_BASE);

  // Average angle during the last cycle
  double avgAngle = (cycleAngle / 2) + odomOld.pose.pose.orientation.z;
  if (avgAngle > PI) {
    avgAngle -= 2*PI;
  }
  else if (avgAngle < -PI) {
    avgAngle += 2*PI;
  }

  // Calculate the new pose (x, y, and theta)
  odomNew.pose.pose.position.x = odomOld.pose.pose.position.x + cos(avgAngle)*cycleDistance;
  odomNew.pose.pose.position.y = odomOld.pose.pose.position.y + sin(avgAngle)*cycleDistance;
  odomNew.pose.pose.orientation.z = cycleAngle + odomOld.pose.pose.orientation.z;

  // Prevent lockup from a single bad cycle
  if (isnan(odomNew.pose.pose.position.x) || isnan(odomNew.pose.pose.position.y)
    || isnan(odomNew.pose.pose.position.z)) {
    odomNew.pose.pose.position.x = odomOld.pose.pose.position.x;
    odomNew.pose.pose.position.y = odomOld.pose.pose.position.y;
    odomNew.pose.pose.position.z = odomOld.pose.pose.position.z;
    odomNew.pose.pose.orientation.z = odomOld.pose.pose.orientation.z;
  }

  // Make sure theta stays in the correct range
  if (odomNew.pose.pose.orientation.z > PI) {
    odomNew.pose.pose.orientation.z -= 2 * PI;
  }
  else if (odomNew.pose.pose.orientation.z < -PI) {
    odomNew.pose.pose.orientation.z += 2 * PI;
  }

  odomNew.header.stamp = ros::Time::now();
  double dt = odomNew.header.stamp.toSec() - odomOld.header.stamp.toSec();
  if (dt > 0) {
    odomNew.twist.twist.linear.x = cycleDistance / dt;
    odomNew.twist.twist.angular.z = cycleAngle / dt;
  }

  // Save the pose data for the next cycle
  odomOld.pose.pose.position.x = odomNew.pose.pose.position.x;
  odomOld.pose.pose.position.y = odomNew.pose.pose.position.y;
  odomOld.pose.pose.position.z = odomNew.pose.pose.position.z;
  odomOld.pose.pose.orientation.z = odomNew.pose.pose.orientation.z;
  odomOld.header.stamp = odomNew.header.stamp;

  // Publish the odometry message
  odom_data_pub.publish(odomNew);
}

// Callback function to handle messages on the cmd_vel topic
void cmdVelCallback(const geometry_msgs::Twist::ConstPtr& msg)
{
  // Extract linear and angular velocities from the received message
  float linear_velocity = msg->linear.x;
  float angular_velocity = msg->angular.z;

  float wheel_distance = 0.2; // Assuming the distance between the two wheels
  float v_l = linear_velocity - (angular_velocity * wheel_distance / 2.0);
  float v_r = linear_velocity + (angular_velocity * wheel_distance / 2.0);

  const float MAX_VELOCITY = 0.3;
  const float MIN_VELOCITY = -0.3;
  if (v_l > MAX_VELOCITY) v_l = MAX_VELOCITY;
  else if (v_l < MIN_VELOCITY) v_l = MIN_VELOCITY;
  if (v_r > MAX_VELOCITY) v_r = MAX_VELOCITY;
  else if (v_r < MIN_VELOCITY) v_r = MIN_VELOCITY;

#ifdef RUN_WITH_UART
  // char message[50];
  // snprintf(message, sizeof(message), "%.2f r %.2f l", v_r, v_l);
  // ROS_INFO("Velocity send to STM32: %.02f %.02f", v_r, v_l);
  // ser.write(message);
  // ser.flush();

  sendUart(v_r, v_l);
#endif
}

void manualControlCallback(const std_msgs::String::ConstPtr& manualVel) {
  string data = manualVel->data;
  istringstream ss(data);
  string token;

  float velR, velL;

  if (getline(ss, token, ',')) {
    velR = stof(token);
  }

  if (getline(ss, token, ',')) {
    velL = stof(token);
  }

  ROS_INFO("velR: %f, velL: %f", velR, velL);
  sendUart(velR, velL);
}

// void goalStatusCallback(const actionlib_msgs::GoalStatusArray::ConstPtr& msg) {
//   for(const auto& status : msg->status_list)
//   {
//     if(status.text == "Goal reached.")
//     {
//       ROS_INFO("Goal reached!");
//     }
//   }
// }

int main(int argc, char **argv) {
  // Set the data fields of the odometry message
  odomNew.header.frame_id = "odom";
  odomNew.pose.pose.position.z = 0;
  odomNew.pose.pose.orientation.x = 0;
  odomNew.pose.pose.orientation.y = 0;
  odomNew.twist.twist.linear.x = 0;
  odomNew.twist.twist.linear.y = 0;
  odomNew.twist.twist.linear.z = 0;
  odomNew.twist.twist.angular.x = 0;
  odomNew.twist.twist.angular.y = 0;
  odomNew.twist.twist.angular.z = 0;
  odomOld.pose.pose.position.x = initialX;
  odomOld.pose.pose.position.y = initialY;
  odomOld.pose.pose.orientation.z = initialTheta;

  ros::init(argc, argv, "ekf_odom_pub");
  ros::NodeHandle node;

  odom_data_pub = node.advertise<nav_msgs::Odometry>("odom_data_euler", 100);
  odom_data_pub_quat = node.advertise<nav_msgs::Odometry>("odom_data_quat", 100);

  ros::Subscriber subInitialPose = node.subscribe("initial_2d", 1, set_initial_2d);
  ros::Subscriber subCmdVel = node.subscribe("cmd_vel", 1000, cmdVelCallback);
  // ros::Subscriber subMoveBaseStatus = node.subscribe("/move_base/status", 10, goalStatusCallback);
  ros::Subscriber subManualControl = node.subscribe("manual_control", 1, manualControlCallback);

  ros::Rate loop_rate(30); 
  while(ros::ok()) {

#ifdef RUN_WITH_UART
    receiveDataFromSerial();
#endif

    if(initialPoseRecieved == true) {
      update_odom();
      publish_quat();
    }
    ros::spinOnce();
    loop_rate.sleep();

  }
 
  return 0;
}
