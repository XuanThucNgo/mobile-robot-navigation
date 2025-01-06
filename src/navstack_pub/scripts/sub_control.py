#!/usr/bin/env python3

from __future__ import print_function

import rospy
from std_msgs.msg import String
import serial

# Initialize serial communication
ser = serial.Serial('/dev/ttyAMA0', 115200, timeout=100)  # Adjust the port and baud rate as needed

def send_velocity(v_r, v_l):
    message = f"{v_r:.2f}r{v_l:.2f}l"
    ser.write(message.encode())
    ser.flush()
    print(f"Velocity sent to STM32: {v_r:.2f}, {v_l:.2f}")

def callback(data):
    rospy.loginfo(f"I heard {data.data}")
    try:
        commands = data.data.split(',')
        if len(commands) == 2:
            v_r = float(commands[0])
            v_l = float(commands[1])
            send_velocity(v_r, v_l)
        else:
            rospy.logwarn("Received invalid command format")
    except ValueError as e:
        rospy.logwarn(f"ValueError encountered: {e}")

def listener():
    rospy.init_node('manual_control_listener', anonymous=True)
    rospy.Subscriber('manual_control', String, callback)
    rospy.spin()

if __name__ == '__main__':
    try:
        listener()
    except rospy.ROSInterruptException:
        pass
    finally:
        ser.close()  # Close the serial port when the node is shut down
