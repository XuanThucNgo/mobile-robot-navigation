#!/usr/bin/env python3

import rospy
from geometry_msgs.msg import PoseWithCovarianceStamped
from tf.transformations import quaternion_from_euler

def setInitPose(x, y, yaw):
    initpose_msg = PoseWithCovarianceStamped()
    initpose_msg.header.frame_id = "map"
    initpose_msg.pose.pose.position.x = x
    initpose_msg.pose.pose.position.y = y

    # Convert yaw to quaternion
    q = quaternion_from_euler(0, 0, yaw)
    initpose_msg.pose.pose.orientation.x = q[0]
    initpose_msg.pose.pose.orientation.y = q[1]
    initpose_msg.pose.pose.orientation.z = q[2]
    initpose_msg.pose.pose.orientation.w = q[3]
    
    pub.publish(initpose_msg)

# SIMPLY RUN THIS .py FILE
# if __name__ == '__main__':
#     rospy.init_node('pub_initpose_node', anonymous=True)
#     pub = rospy.Publisher('/initialpose', PoseWithCovarianceStamped, queue_size=10)

#     rospy.sleep(1)  # Give some time for connections to establish
    
#     setInitPose(5, 2, 1)

#     # Keep the node running for a short time to ensure the message is sent
#     rospy.sleep(1)

# USE IN ROS
if __name__ == '__main__':
    rospy.init_node('pub_initpose_node', anonymous=True)
    pub = rospy.Publisher('/initialpose', PoseWithCovarianceStamped, queue_size=10)

    rospy.sleep(1)  # Give some time for connections to establish
    
    # Read parameters from the parameter server
    x = rospy.get_param('~x', 0.0)
    y = rospy.get_param('~y', 0.0)
    yaw = rospy.get_param('~yaw', 0.0)

    setInitPose(x, y, yaw)

    # Keep the node running for a short time to ensure the message is sent
    rospy.sleep(1)

'''
header:
  seq: 0
  stamp:
    secs: 0
    nsecs: 0
  frame_id: 'map'
pose:
  pose:
    position: {x: 1.0, y: 1.0, z: 0.0}
    orientation: {x: 0.0, y: 0.0, z: 0.0, w: 1.0} # Assuming no rotation, which corresponds to a quaternion [0, 0, 0, 1]
  covariance: [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]

'''