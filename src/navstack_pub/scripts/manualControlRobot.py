#!/usr/bin/env python3

from __future__ import print_function
import serial
import time
from pynput import keyboard
import sys, termios, tty

# Initialize serial communication
ser = serial.Serial('/dev/ttyAMA0', 115200, timeout = 100)  # Adjust the port and baud rate as needed

# Initial maximum speed
max_speed = 1.0 # m/s

msg = """
    >( ͡° ͜ʖ ͡°)> Hieuneee
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

# def clear_screen():
#     """Clear the terminal screen."""
#     # ANSI escape sequence to clear screen
#     sys.stdout.write("\033[H\033[J")
#     sys.stdout.flush()

def clear_last_line():
    """Clear the last line in the terminal."""
    # Move the cursor to the beginning of the current line
    sys.stdout.write("\033[F")
    # Clear the current line
    sys.stdout.write("\033[K")
    sys.stdout.flush()

def send_velocity(v_r, v_l):
    message = f"{v_r:.2f}r{v_l:.2f}l"
    ser.write(message.encode())
    ser.flush()
    # print(f"Velocity sent to STM32: {v_r:.2f}, {v_l:.2f}")

def vel_info(speed):
    return "currently:\tspeed %s " % (speed)

def on_press(key):
    tty.setraw(sys.stdin.fileno())

    global max_speed
    
    """Handle key press events."""
    try:
        if key.char in velocity:
            v_r, v_l = velocity[key.char]
            send_velocity(v_r * max_speed, v_l * max_speed)
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

def on_release(key):
    """Handle key release events."""
    # Stop the robot when a key is released
    if hasattr(key, 'char'):  # Check if the key object has the 'char' attribute
        if key.char in velocity:
            send_velocity(0.0, 0.0)
    else:
        if key == keyboard.Key.esc:
            # Stop listener
            return False



# Save the original terminal settings
original_settings = termios.tcgetattr(sys.stdin)

try:    
    # Start the keyboard listener
    with keyboard.Listener(on_press=on_press, on_release=on_release) as listener:
        listener.join()

except KeyboardInterrupt:
    # Exit the program gracefully when Ctrl+C is pressed
    print("Exiting program...")
    ser.close()  # Close the serial port

finally:
    # Restore the original terminal settings
    termios.tcsetattr(sys.stdin, termios.TCSADRAIN, original_settings)

# Close the serial port
ser.close()
