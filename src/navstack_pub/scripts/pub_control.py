#!/usr/bin/env python3

from __future__ import print_function

import threading

import roslib; roslib.load_manifest('teleop_twist_keyboard')
import rospy

from geometry_msgs.msg import Twist

import sys, select, termios, tty

#-------------------------------------------------------

import rospy
from std_msgs.msg import String
from pynput import keyboard
import sys, termios, tty

msg = """
                    >( ͡° ͜ʖ ͡°)> Thucneee
------------------------------------------------------------
>> Moving around:
        w    
    a   s   d
>> release the key: stop
>> e/r: increase/decrease max speeds by 10%
>> Esc to quit                                     
                      #      #                       
                   #*          *#                    
                  #-             =#                  
                 #:               ##                 
                ##                 #.                
                ##                ###                
                ###               ##.                
                 ###  ########:  ###                 
                  ####        .####                  
            ############## ##############            
         .##+      ######   #####=      ###          
        ##        ##  ##    ###  #        *#+        
        #         *#.  .#####   ##          #        
       #           ##:  #####  ##           ##       
       #             ## ##### ##             #       
                        #####                        
                       ######                        
                      ###- ####                      
           ++      +####     ####       ++           
              #######           #######              
------------------------------------------------------------                                           
"""

print(msg)

# Define velocities for each command
velocity = {
    'a': (-1.0, 1.0),  # Turn left 
    'w': (1.0, 1.0),   # Move forward 
    's': (-1.0, -1.0), # Move backward 
    'd': (1.0, -1.0)   # Turn right 
}

max_speed = 0.2  # Initial maximum speed

def clear_last_line():
    """Clear the last line in the terminal."""
    sys.stdout.write("\033[F")
    sys.stdout.write("\033[K")
    sys.stdout.flush()

def vel_info(speed):
    return "currently:\tspeed %s " % (speed)

def on_press(key, pub):
    tty.setraw(sys.stdin.fileno())
    global max_speed

    try:

        if key.char in velocity:
            v_r, v_l = velocity[key.char]
            message = f"{v_r * max_speed},{v_l * max_speed}"
            pub.publish(message)
            print(f"Key pressed: {key.char} - Publishing: {message}")
        elif key.char == 'e':
            max_speed *= 1.1
            clear_last_line()  
            print(f"Max speed increased to {max_speed:.2f}")
        elif key.char == 'r':
            max_speed *= 0.9
            clear_last_line() 
            print(f"Max speed decreased to {max_speed:.2f}")
    except AttributeError:
        pass

def on_release(key, pub):
    if hasattr(key, 'char') and key.char in velocity:
        pub.publish("0.0, 0.0")
    elif key == keyboard.Key.esc:
        return False

def talker():
    pub = rospy.Publisher('manual_control', String, queue_size=10)
    rospy.init_node('manual_control_talker', anonymous=True)

    original_settings = termios.tcgetattr(sys.stdin)

    try:
        with keyboard.Listener(on_press=lambda key: on_press(key, pub), 
                               on_release=lambda key: on_release(key, pub)) as listener:
            listener.join()
    except KeyboardInterrupt:
        print("Exiting program...")
    finally:
        termios.tcsetattr(sys.stdin, termios.TCSADRAIN, original_settings)

if __name__ == '__main__':
    try:
        talker()
    except rospy.ROSInterruptException:
        pass
