#!/usr/bin/env python3
"""
CuteCaspar Raspberry Pi Startup Script with MQTT Support
Enhanced version with both UDP and MQTT communication
"""

import RPi.GPIO as GPIO
from subprocess import call
from threading import Thread
import socket
import time
import threading
import sys
import json

# MQTT Support
try:
    import paho.mqtt.client as mqtt
    MQTT_AVAILABLE = True
    print("✅ MQTT support available (paho-mqtt)")
except ImportError:
    MQTT_AVAILABLE = False
    print("⚠️ MQTT support not available (install: pip install paho-mqtt)")

GPIO.setwarnings(False)
GPIO.setmode(GPIO.BOARD)

# GPIO Setup
GPIO.setup(18, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)  # Push Button Input
GPIO.setup(12, GPIO.OUT)  # Push Button LED Output
GPIO.output(12, 0)
pulsating_LED = GPIO.PWM(12, 50)

GPIO.setup(22, GPIO.IN, pull_up_down=GPIO.PUD_UP)  # Push Doorbell Input
GPIO.setup(8, GPIO.OUT)   # Magnet Output
GPIO.output(8, 1)
GPIO.setup(10, GPIO.OUT)  # Smoke Output
GPIO.output(10, 1)
GPIO.setup(11, GPIO.OUT)  # Light Output
GPIO.output(11, 1)
GPIO.setup(16, GPIO.OUT)  # Motion Output
GPIO.output(16, 1)

# Configuration
UDP_ENABLED = False  # Set to True to enable UDP backup
UDP_IP_IN = "127.0.0.1"
UDP_IP_OUT = "127.0.0.1"
UDP_PORT_IN = 1235
UDP_PORT_OUT = 1234

# MQTT Configuration
MQTT_ENABLED = True  # Set to False to disable MQTT
MQTT_BROKER_HOST = "192.168.0.184"  # Change this to your CuteCaspar machine IP or external broker
MQTT_BROKER_PORT = 1883
MQTT_CLIENT_ID = "RaspberryPi"
MQTT_TOPIC_PREFIX = "cutecaspar/raspi"
MQTT_COMMAND_TOPIC = f"{MQTT_TOPIC_PREFIX}/command"
MQTT_STATUS_TOPIC = f"{MQTT_TOPIC_PREFIX}/status"

# Global variables
thread_running = True
flashing = False
disableButton = False
mqtt_client = None
mqtt_connected = False

# UDP Sockets
sock_out = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock_in = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

def get_ip_address():
    """Retrieve the real IP address"""
    ip_address = ''
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    connected = False
    while not connected:
        try:
            s.connect(("8.8.8.8", 80))
            connected = True
        except Exception as e:
            pass  # Do nothing, just try again
    ip_address = s.getsockname()[0]
    s.close()
    print(f"IP: {ip_address}")
    return ip_address

def send_status_message(message):
    """Send status message via UDP and/or MQTT based on configuration"""
    print(f"📤 Sending status: {message}")
    
    # Send via UDP if enabled
    if UDP_ENABLED:
        try:
            sock_out.sendto(bytes(f"raspi\\{message}", "utf-8"), (UDP_IP_OUT, UDP_PORT_OUT))
            print(f"📤 UDP sent: {message}")
        except Exception as e:
            print(f"❌ UDP send error: {e}")
    
    # Send via MQTT if available and connected
    if MQTT_ENABLED and MQTT_AVAILABLE and mqtt_connected and mqtt_client:
        try:
            mqtt_client.publish(MQTT_STATUS_TOPIC, message, qos=1)
            print(f"📡 MQTT published: {message} to {MQTT_STATUS_TOPIC}")
        except Exception as e:
            print(f"❌ MQTT publish error: {e}")
    
    # If neither protocol is enabled/available, warn user
    if not UDP_ENABLED and not (MQTT_ENABLED and MQTT_AVAILABLE and mqtt_connected):
        print(f"⚠️ Warning: No communication protocols enabled/available!")

def process_command(message):
    """Process command received from either UDP or MQTT"""
    global flashing, disableButton
    
    print(f"🔄 Processing command: {message}")
    
    if message == 'quit':
        flashing = False
        GPIO.output(12, 0)  # Turn button LED off
        print("Shutting down...")
        return 'quit'
    elif message == 'alive':
        # Don't return here, continue to send "ok" response
        print("Alive check received")
    elif message == 'off':
        flashing = False
        print("LED switched off")
    elif message == 'on':
        flashing = True
        print("LED switched on")
    elif message == 'latch_close' or message == 'magnet_on':  # Support both new and old names
        print("Latch close (manual)")
        GPIO.output(8, 1)
        send_status_message("latch1_closed")
        return None  # Don't send additional "ok"
    elif message == 'latch_open' or message == 'magnet_off':  # Support both new and old names
        disableButton = True
        print("Latch open (temporary)")
        GPIO.output(8, 0)  # Open latch
        time.sleep(0.25)   # Brief open time
        GPIO.output(8, 1)  # Auto-close latch
        send_status_message("latch1_closed")  # Notify CuteCaspar latch is closed again
        time.sleep(1.0)    # Safety delay
        disableButton = False
        return None  # Don't send additional "ok"
    elif message == 'light_on':
        print("Light on")
        GPIO.output(11, 0)  # Assuming 0 = on for relay
    elif message == 'light_off':
        print("Light off")
        GPIO.output(11, 1)  # Assuming 1 = off for relay
    elif message == 'smoke_on':
        print("Smoke on")
        GPIO.output(10, 0)
    elif message == 'smoke_off':
        print("Smoke off")
        GPIO.output(10, 1)
    elif message == 'motion_on':
        print("Motion sensor on")
        GPIO.output(16, 0)
    elif message == 'motion_off':
        print("Motion sensor off")
        GPIO.output(16, 1)
        send_status_message("latch2_closed")
        return None  # Don't send additional "ok"
    elif message == 'reboot':
        print("Rebooting system...")
        call("sudo reboot", shell=True)
    elif message == 'shutdown':
        print("Shutting down system...")
        call("sudo shutdown -h now", shell=True)
    else:
        print(f"Unknown command: {message}")
    
    # Send OK acknowledgment
    send_status_message("ok")
    return None

def pushButton():
    """Handle button press detection"""
    global disableButton
    button_state = 0
    doorbell_state = 0
    
    try:
        while True:
            if (button_state == 0 and GPIO.input(18) == 1 and not disableButton):
                button_state = 1
                send_status_message("high")
                print('Button: high')
            elif (doorbell_state == 0 and GPIO.input(22) == 0):
                doorbell_state = 1
                send_status_message("high2")
                print('Doorbell: high')
            elif (button_state == 1 and GPIO.input(18) == 0 and not disableButton):
                button_state = 0
                send_status_message("low")
                print('Button: low')
            elif (doorbell_state == 1 and GPIO.input(22) == 1):
                doorbell_state = 0
                send_status_message("low2")
                print('Doorbell: low')
            time.sleep(0.1)
    except KeyboardInterrupt:
        GPIO.cleanup()
        sys.exit()

def flashingProcess():
    """Handle LED flashing"""
    global flashing, thread_running, pulsating_LED, disableButton
    ledCycle = 0
    
    while thread_running:
        try:
            if flashing and not disableButton:
                if ledCycle == 0:
                    pulsating_LED.start(2)
                    ledCycle = 1
                elif ledCycle == 1:
                    pulsating_LED.ChangeDutyCycle(100)
                    ledCycle = 2
                elif ledCycle == 2:
                    for dc in range(0, 101, 5):
                        pulsating_LED.ChangeDutyCycle(dc)
                        time.sleep(0.05)
                    ledCycle = 3
                elif ledCycle == 3:
                    for dc in range(100, -1, -5):
                        pulsating_LED.ChangeDutyCycle(dc)
                        time.sleep(0.05)
                    ledCycle = 2
            else:
                pulsating_LED.stop()
                GPIO.output(12, 0)
                ledCycle = 0
                time.sleep(0.2)
        except Exception as e:
            print(f"LED process error: {e}")
            time.sleep(0.1)

# MQTT Event Handlers
def on_mqtt_connect(client, userdata, flags, rc):
    """MQTT connection callback"""
    global mqtt_connected
    if rc == 0:
        mqtt_connected = True
        print(f"✅ MQTT connected to {MQTT_BROKER_HOST}:{MQTT_BROKER_PORT}")
        print(f"📥 Subscribing to: {MQTT_COMMAND_TOPIC}")
        client.subscribe(MQTT_COMMAND_TOPIC, qos=1)
    else:
        mqtt_connected = False
        print(f"❌ MQTT connection failed with code {rc}")

def on_mqtt_disconnect(client, userdata, rc):
    """MQTT disconnection callback"""
    global mqtt_connected
    mqtt_connected = False
    print("❌ MQTT disconnected")

def on_mqtt_message(client, userdata, msg):
    """MQTT message received callback"""
    try:
        command = msg.payload.decode('utf-8')
        topic = msg.topic
        print(f"📨 MQTT received: '{command}' from topic '{topic}'")
        
        # Process the command using the same logic as UDP
        result = process_command(command)
        
        if result == 'quit':
            sys.exit()
            
    except Exception as e:
        print(f"❌ MQTT message processing error: {e}")

def setup_mqtt():
    """Initialize MQTT client"""
    global mqtt_client
    
    if not MQTT_ENABLED:
        print("🔒 MQTT disabled by configuration")
        return False
        
    if not MQTT_AVAILABLE:
        print("❌ MQTT not available (missing paho-mqtt)")
        return False
    
    try:
        # Handle both paho-mqtt v1.x and v2.x compatibility
        try:
            # Try v2.x syntax first (with callback_api_version)
            mqtt_client = mqtt.Client(client_id=MQTT_CLIENT_ID, callback_api_version=mqtt.CallbackAPIVersion.VERSION1)
            print("📋 Using paho-mqtt v2.x (callback API v1 compatibility)")
        except (TypeError, AttributeError):
            # Fallback to v1.x syntax
            mqtt_client = mqtt.Client(MQTT_CLIENT_ID)
            print("📋 Using paho-mqtt v1.x syntax")
        
        mqtt_client.on_connect = on_mqtt_connect
        mqtt_client.on_disconnect = on_mqtt_disconnect
        mqtt_client.on_message = on_mqtt_message
        
        print(f"🔌 Connecting to MQTT broker {MQTT_BROKER_HOST}:{MQTT_BROKER_PORT}...")
        mqtt_client.connect(MQTT_BROKER_HOST, MQTT_BROKER_PORT, 60)
        mqtt_client.loop_start()  # Start background loop
        
        return True
    except Exception as e:
        print(f"❌ MQTT setup error: {e}")
        return False

def listen_udp():
    """Listen for UDP commands (original functionality)"""
    global UDP_IP_OUT
    
    print("👂 Starting UDP listener...")
    sock_in.bind((UDP_IP_IN, UDP_PORT_IN))
    
    while True:
        try:
            data, addr = sock_in.recvfrom(1024)
            UDP_IP_OUT = addr[0]
            
            # Extract message (remove "raspi/" prefix)
            raw_message = data.decode('ascii')
            if raw_message.startswith('raspi/') or raw_message.startswith('raspi\\'):
                message = raw_message[6:]
            else:
                message = raw_message
            
            print(f"📨 UDP received: '{message}' from {addr[0]}")
            
            # Process command
            result = process_command(message)
            
            if result == 'quit':
                break
                
        except Exception as e:
            print(f"❌ UDP listen error: {e}")
            time.sleep(1)

def main():
    """Main function"""
    print("🚀 Starting CuteCaspar Raspberry Pi with MQTT support...")
    print(f"📡 MQTT: {'Enabled' if MQTT_ENABLED else 'Disabled'}")
    print(f"📤 UDP: {'Enabled' if UDP_ENABLED else 'Disabled'} ({UDP_IP_OUT}:{UDP_PORT_OUT})")
    
    # Get IP address
    get_ip_address()
    
    # Setup MQTT if enabled
    setup_mqtt()
    
    # Start button monitoring thread
    print("🔘 Starting button monitoring...")
    button_thread = Thread(target=pushButton)
    button_thread.daemon = True
    button_thread.start()
    
    # Start LED flashing thread
    print("💡 Starting LED controller...")
    led_thread = Thread(target=flashingProcess)
    led_thread.daemon = True
    led_thread.start()
    
    try:
        if UDP_ENABLED:
            # Start UDP listener (main thread)
            listen_udp()
        else:
            # If UDP is disabled, just keep main thread alive
            print("⏳ UDP disabled - running MQTT-only mode...")
            while True:
                time.sleep(1)
    except KeyboardInterrupt:
        print("\n🛑 Interrupted by user")
    finally:
        print("🧹 Cleaning up...")
        GPIO.cleanup()
        if mqtt_client:
            mqtt_client.loop_stop()
            mqtt_client.disconnect()
        sys.exit()

if __name__ == "__main__":
    main()