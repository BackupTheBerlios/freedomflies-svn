# Import Libraries
import pygame
import sys,time

# Get X, Y and Yaw Positions
def getPos():
    raw_x = stick.get_axis(0)
    raw_y = stick.get_axis(1)
    raw_yaw = stick.get_axis(2)
    x = int(round(raw_x * 100))
    y = int(round(raw_y * -100))
    yaw = int(round(raw_yaw * -100))
    return x,y,yaw
    
# Get Throttle Position
def getThrottle():
    raw_throt = stick.get_axis(3)
    throttle = int(round(raw_throt*-50+50))
    return throttle

# Get State of Hat
def getHat():
    raw_hat_x,raw_hat_y = stick.get_hat(0)
    #raw_hat_x = stick.get_axis(4)
    #raw_hat_y = stick.get_axis(5)
    hat_x = int(round(raw_hat_x))
    hat_y = int(round(raw_hat_y))
    if (hat_x == 0) and (hat_y == 0):
        hat = "Center"
    if (hat_x == 1) and (hat_y == 0):
	hat = "Right"
    if (hat_x == -1) and (hat_y == 0):
	hat = "Left"
    if (hat_x == 0) and (hat_y == -1):
	hat = "Down"
    if (hat_x == 0) and (hat_y == 1):
	hat = "Up"
    if (hat_x == 1) and (hat_y == -1):
	hat = "DownRight"
    if (hat_x == 1) and (hat_y == 1):
	hat = "UpRight"
    if (hat_x == -1) and (hat_y == -1):
	hat = "DownLeft"
    if (hat_x == -1) and (hat_y == 1):
	hat = "UpLeft"
    return hat


# Get State of All 10 Buttons
def getButtons():
    b1 = stick.get_button(0)
    b2 = stick.get_button(1)
    b3 = stick.get_button(2)
    b4 = stick.get_button(3)
    b5 = stick.get_button(4)
    b6 = stick.get_button(5)
    b7 = stick.get_button(6)
    b8 = stick.get_button(7)
    b9 = stick.get_button(8)
    b10 = stick.get_button(9)
    return b1,b2,b3,b4,b5,b6,b7,b8,b9,b10

# Main Fuction
if __name__ == "__main__":
    # Initialize Pygame and Joystick
    pygame.init()
    stick = pygame.joystick.Joystick(0)
    stick.init()
    
    # Make Sure Joystick Initilized
    status = stick.get_init()
    if(status== 1):
        print "Joystick Successfully Initialized with %s axes, %s buttons, %s hats" % (stick.get_numaxes(),stick.get_numbuttons(),stick.get_numhats())
    	print "Press Trigger or Move Hat Up to Capture State"
    else:
        print "Joystick Failure"
        sys.exit()

    # Loop Forever
    while(True):
        # Call Pygame Event Queue
	pygame.event.pump()
	# Get All Joystick Characteristics
        x,y,yaw = getPos()
        throt = getThrottle()
	hat = getHat()
	b1,b2,b3,b4,b5,b6,b7,b8,b9,b10 = getButtons()
	# Print Joystick Position if Trigger is Pushed or Hat is Up
	if (b1 == 1) or (hat == "Up"):
            print x,y,yaw,throt,hat
            
        # Pause
	time.sleep(.15)
        # Exit Loop If Buttons 9 and 10 Are Pushed
        if (b9 == 1) and (b10 == 1):
	    stick.quit()
     	    print "Goodbye"
	    sys.exit()

