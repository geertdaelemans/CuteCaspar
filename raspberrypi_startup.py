import RPi.GPIO as GPIO
from subprocess import call
from threading import Thread

GPIO.setwarnings(False)
GPIO.setmode(GPIO.BOARD)

# Push Button Input
GPIO.setup(18, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)

# Push Button LED Output
GPIO.setup(12, GPIO.OUT)
GPIO.output(12, 0)
pulsating_LED = GPIO.PWM(12, 50)  

# Push Doorbell Input
GPIO.setup(22, GPIO.IN, pull_up_down=GPIO.PUD_UP)                 

# Magnet Output
GPIO.setup(8, GPIO.OUT)
GPIO.output(8, 1)

# Smoke Output
GPIO.setup(10, GPIO.OUT)
GPIO.output(10, 1)

# Light Output
GPIO.setup(11, GPIO.OUT)
GPIO.output(11, 1)

# Motion Output
GPIO.setup(16, GPIO.OUT)
GPIO.output(16, 1)

import socket
import time
import threading

UDP_IP_IN = "127.0.0.1"        # Local Host
UDP_IP_OUT = "127.0.0.1"    # Address of Client
UDP_PORT_IN = 1235    
UDP_PORT_OUT = 1234

thread_running = True
flashing = False

sock_out = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock_in = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

disableButton = False

# Function to retrieve the real IP address
def get_ip_address():
    ip_address = '';
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    connected = False
    while not connected:
        try:
            s.connect(("8.8.8.8",80))
            connected = True
        except Exception as e:
            pass #Do nothing, just try again
    ip_address = s.getsockname()[0]
    s.close()
    print("IP : ", ip_address)
    return ip_address    

def pushButton():
    global disableButton
    button_state = 0
    doorbell_state = 0
    try:
        while True:
            if (button_state == 0 and GPIO.input(18) == 1 and not disableButton):
                button_state = 1
                sock_out.sendto(bytes("raspi\high", "utf-8"), (UDP_IP_OUT, UDP_PORT_OUT))
                print('Button: high')
            elif (doorbell_state == 0 and GPIO.input(22) == 0):
                doorbell_state = 1
                sock_out.sendto(bytes("raspi\high2", "utf-8"), (UDP_IP_OUT, UDP_PORT_OUT))
                print('Doorbell: high')    
            elif (button_state == 1 and GPIO.input(18) == 0 and not disableButton):
                button_state = 0
                sock_out.sendto(bytes("raspi\low", "utf-8"), (UDP_IP_OUT, UDP_PORT_OUT))
                print('Button: low')
            elif (doorbell_state == 1 and GPIO.input(22) == 1):
                doorbell_state = 0    
                sock_out.sendto(bytes("raspi\low2", "utf-8"), (UDP_IP_OUT, UDP_PORT_OUT))
                print('Doorbell: low')
            time.sleep(0.1)
    except KeyboardInterrupt:
        GPIO.cleanup()
        sys.exit()

def flashingProcess():
    global flashing
    global thread_running
    global pulsating_LED
    global disableButton
    ledCycle = 0
    ledBrighter = True
    ledActive = False
    while thread_running:
        if flashing and not disableButton:
            # Initiate sequence
            if not ledActive:
                ledCycle = 0
                ledBrighter = True
                ledActive = True
                pulsating_LED.start(0)
            # Calculate dimming LED status
            if ledBrighter:
                ledCycle = ledCycle + 1
            else:
                ledCycle = ledCycle - 1
            if ledCycle > 100:
                ledCycle = 99
                ledBrighter = False
            if ledCycle < 0:
                ledCycle = 1
                ledBrighter = True
            # Set LED
            pulsating_LED.ChangeDutyCycle(ledCycle)
        else:
            # Stop sequence
            if ledActive:
                pulsating_LED.stop()
                ledActive = False
        time.sleep(0.01)
    print("Closing flashing thread")
    
def Main():
    global UDP_IP_IN
    global UDP_IP_OUT
    global sock_in
    global sock_out
    global disableButton
    global flashing

    # Get real local host IP and put it in UDP_PORT_IN
    UDP_IP_IN = get_ip_address()

    # Bind incoming UDP port
    sock_in.bind((UDP_IP_IN, UDP_PORT_IN))
    
    # Start threads
    x = Thread(target=pushButton)
    x.daemon = True
    x.start()
    t1 = Thread(target=flashingProcess) 
    t1.start()
    
    while True:
        # Wait for wake-up call
        print('Started listening...')
        while True:
            data, addr = sock_in.recvfrom(1024) # buffer size is 1024 bytes
            UDP_IP_OUT = addr[0]
            message = str(data.decode('ascii'))[6:]
            
            # message sent to server
            sock_out.sendto(bytes("raspi\ok", "utf-8"), (UDP_IP_OUT, UDP_PORT_OUT))

            if message == 'quit':
                GPIO.output(12, 0)    # Turn button LED off
                print("Shutting down...")
                sys.exit()
            elif message == 'alive':
                break
        
        print('Type high or low to simulate pushbutton')
        
        while True:

            data, addr = sock_in.recvfrom(1024) # buffer size is 1024 bytes
            message = str(data.decode('ascii'))[6:]

            # message sent to server
            sock_out.sendto(bytes("raspi\ok", "utf-8"), (UDP_IP_OUT, UDP_PORT_OUT))
            
            if message == 'quit':
                flashing = False
                print("Shutting down...")
                break
            elif message == 'off':
                flashing = False
                print("LED switched off")
            elif message == 'on':
                flashing = True
                print("LED switched on")
            elif message == 'magnet_on':
                print("Magnet on")
                GPIO.output(8, 1)
                sock_out.sendto(bytes("raspi\latch1_closed", "utf-8"), (UDP_IP_OUT, UDP_PORT_OUT))
            elif message == 'magnet_off':
                disableButton = True;
                print("Magnet off")
                GPIO.output(8, 0)
                time.sleep(0.25)
                GPIO.output(8, 1)
                sock_out.sendto(bytes("raspi\latch1_closed", "utf-8"), (UDP_IP_OUT, UDP_PORT_OUT))
                time.sleep(1.0)
                disableButton = False;
            elif message == 'smoke_off':
                print("Smoke switched off")
                GPIO.output(10, 1)
            elif message == 'smoke_on':
                print("Smoke switched on")
                GPIO.output(10, 0)
            elif message == 'light_off':
                print("Light switched off")
                GPIO.output(11, 1)
            elif message == 'light_on':
                print("Light switched on")
                GPIO.output(11, 0)
            elif message == 'motion_off':
                print("Motion switched off")
                GPIO.output(16, 1)
                sock_out.sendto(bytes("raspi\latch2_closed", "utf-8"), (UDP_IP_OUT, UDP_PORT_OUT))
            elif message == 'motion_on':
                print("Motion switched on")
                GPIO.output(16, 0)
                time.sleep(0.25)
                GPIO.output(16, 1)
                sock_out.sendto(bytes("raspi\latch2_closed", "utf-8"), (UDP_IP_OUT, UDP_PORT_OUT))
            elif message == 'reboot':
                print("Rebooting")
                call("sudo reboot", shell=True)
            elif message == 'shutdown':
                print("Shutting down")
                call("sudo shutdown -h now", shell=True)
            else:
                print("received message:", message, "from", UDP_IP_OUT)

    sock_in.close()
    sock_out.close()

if __name__ == '__main__':
    Main()
