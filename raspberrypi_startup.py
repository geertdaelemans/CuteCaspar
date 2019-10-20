import RPi.GPIO as GPIO

GPIO.setwarnings(False)
GPIO.setmode(GPIO.BOARD)

# Push Button Input
GPIO.setup(18, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)

# Push Button LED Output
GPIO.setup(7, GPIO.OUT)
GPIO.output(7, 0)

# Magnet Output
GPIO.setup(8, GPIO.OUT)
GPIO.output(8, 1)

# Smoke Output
GPIO.setup(10, GPIO.OUT)
GPIO.output(10, 1)

# Light Output
GPIO.setup(12, GPIO.OUT)
GPIO.output(12, 1)

# Motion Output
GPIO.setup(16, GPIO.OUT)
GPIO.output(16, 1)

import socket
import time
import threading

UDP_IP_IN = "127.0.0.1"		# Local Host
UDP_IP_OUT = "127.0.0.1"	# Address of Client
UDP_PORT_IN = 1235    
UDP_PORT_OUT = 1234

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
	state = 0

	try:
		while True:
			if (not disableButton):
				if (GPIO.input(18) == 1):
					if (state == 0):
						print('High')
						sock_out.sendto(bytes("raspi\high", "utf-8"), (UDP_IP_OUT, UDP_PORT_OUT))
						state = 1
					time.sleep(0.1)
				else:
					if (state == 1):
						print('Low')
						sock_out.sendto(bytes("raspi\low", "utf-8"), (UDP_IP_OUT, UDP_PORT_OUT))
						state = 0
	except KeyboardInterrupt:
		GPIO.cleanup()
		sys.exit()

def Main():
	global UDP_IP_IN
	global UDP_IP_OUT
	global sock_in
	global sock_out
	global disableButton

	# Get real local host IP and put it in UDP_PORT_IN
	UDP_IP_IN = get_ip_address()

	# Bind incoming UDP port
	sock_in.bind((UDP_IP_IN, UDP_PORT_IN))
	
	# Wait for wake-up call
	print('Started listening...')
	while True:
		data, addr = sock_in.recvfrom(1024) # buffer size is 1024 bytes
		UDP_IP_OUT = addr[0]
		message = str(data.decode('ascii'))[6:]
		
		# message sent to server
		sock_out.sendto(bytes("raspi\ok", "utf-8"), (UDP_IP_OUT, UDP_PORT_OUT))

		if message == 'quit':
			GPIO.output(7, 0)	# Turn button LED off
			print("Shutting down...")
			sys.exit()
		elif message == 'alive':
		    break
	
	print('Type high or low to simulate pushbutton')
	
	x = threading.Thread(target=pushButton)
	x.daemon = True
	x.start()

	while True:
		data, addr = sock_in.recvfrom(1024) # buffer size is 1024 bytes
		message = str(data.decode('ascii'))[6:]

		# message sent to server
		sock_out.sendto(bytes("raspi\ok", "utf-8"), (UDP_IP_OUT, UDP_PORT_OUT))
		
		if message == 'quit':
			GPIO.output(7, 0)
			print("Shutting down...")
			break
		elif message == 'off':
			print("LED switched off")
			GPIO.output(7, 0)
		elif message == 'on':
			print("LED switched on")
			GPIO.output(7, 1)
		elif message == 'magnet_on':
			print("Magnet on")
			GPIO.output(8, 0)
		elif message == 'magnet_off':
			disableButton = True;
			print("Magnet off")
			GPIO.output(8, 1)
			time.sleep(1)
			disableButton = False;
		elif message == 'smoke_off':
			print("Smoke switched off")
			GPIO.output(10, 1)
		elif message == 'smoke_on':
			print("Smoke switched on")
			GPIO.output(10, 0)
		elif message == 'light_off':
			print("Light switched off")
			GPIO.output(12, 1)
		elif message == 'light_on':
			print("Light switched on")
			GPIO.output(12, 0)
		elif message == 'motion_off':
			print("Motion switched off")
			GPIO.output(16, 1)
		elif message == 'motion_on':
			print("Motion switched on")
			GPIO.output(16, 0)
		else:
			print("received message:", message, "from", UDP_IP_OUT)

	sock_in.close()
	sock_out.close()

if __name__ == '__main__':
	Main()
