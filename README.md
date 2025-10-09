# CuteCaspar
An experiment with CasparCG including MIDI DMX control and Raspberry PI connection.

The basic idea behind this tool is to provide a CasparCG based playlist that has some synchronised interfaces. This should enable to program a light show that plays in sync with the clips. This is achieved by sending out predefined MIDI notes that can be picked up by a DMX interface. As an extra a connection with a Raspberry PI such that the automation can be triggered by external GPIO.

## Features
* **Playlist Management**
  * Basic playlist functionality with main playlist, scares, and extras (favorites)
  * Random clip insertion from predefined sets
  * SoundScape background clips for continuous audio and MIDI actions

* **MIDI & DMX Control**
  * Lighting scenarios triggered through MIDI notes
  * Each clip can have a sidecar with MIDI sequence
  * Note/MIDI assignments imported through .csv files
  * Synchronized light shows with video clips

* **Raspberry Pi Integration**
  * **MQTT Communication** (Primary) - Modern, reliable messaging protocol
  * **UDP Communication** (Legacy) - Backward compatibility support
  * GPIO control for external hardware (lights, motion sensors, latch/magnet, smoke)
  * External scare button triggers random clip insertion
  * Button press detection and status feedback

* **CasparCG Integration**
  * Communication through configurable ports
  * Tested with CasparCG 2.3.x releases
  * Professional broadcast video server integration

## Communication Protocols

### MQTT (Recommended)
* **Broker**: Configurable MQTT broker (default: localhost:1883)
* **Topics**: 
  * Commands: `cutecaspar/raspi/command`
  * Status: `cutecaspar/raspi/status`
* **Benefits**: Reliable delivery, automatic reconnection, industry standard
* **Configuration**: Stored in Windows registry (`VRT\CasparCGClient\Configuration`)

### UDP (Legacy)
* **Ports**: Configurable (default: 1234/1235)
* **Status**: Maintained for backward compatibility
* **Migration**: Can be disabled in favor of MQTT

## Raspberry Pi Commands

### GPIO Controls
* **`light_on`** / **`light_off`** - Control lighting (GPIO pin 11)
* **`motion_on`** / **`motion_off`** - Motion sensor control (GPIO pin 16)
* **`smoke_on`** / **`smoke_off`** - Smoke machine control (GPIO pin 10)
* **`latch_open`** / **`latch_close`** - Latch/magnet control (GPIO pin 8)
  * `latch_open`: Temporarily opens latch (0.25s) then auto-closes
  * `latch_close`: Manually close/lock latch
* **`on`** / **`off`** - LED flashing control (GPIO pin 12)

### System Commands
* **`alive`** - Connection status check
* **`quit`** - Shutdown communication
* **`reboot`** - System reboot
* **`shutdown`** - System shutdown

### Status Messages
* **`ok`** - Command acknowledgment
* **`high`** / **`low`** - Button state changes
* **`high2`** / **`low2`** - Doorbell state changes
* **`latch1_closed`** / **`latch1_opened`** - Latch status
* **`latch2_closed`** - Motion sensor status

## Technical Requirements

### Software
* **Qt 6.8.3** (updated from 6.4.1)
* **Windows 10/11** - Primary development platform
* **Python 3** - Raspberry Pi scripts
* **SQLite** - Database storage
* **MQTT Library**: 
  * CuteCaspar: Qt MQTT module
  * Raspberry Pi: paho-mqtt (`pip install paho-mqtt`)

### Hardware
* **Raspberry Pi** (tested with v2, compatible with newer versions)
* **CasparCG Server** 2.3.x
* **MIDI Interface** (for DMX control)
* **GPIO Components**: LEDs, buttons, relays, sensors

## Installation & Setup

### CuteCaspar Configuration
MQTT settings are automatically configured in Windows registry:
```
HKEY_CURRENT_USER\SOFTWARE\VRT\CasparCGClient\Configuration
- mqtt_enabled: true
- mqtt_broker_host: 192.168.x.x
- mqtt_broker_port: 1883
- mqtt_client_id: CuteCaspar
- mqtt_topic_prefix: cutecaspar/raspi
```

### Raspberry Pi Setup
1. **Install dependencies**:
   ```bash
   pip install paho-mqtt
   sudo apt install mosquitto mosquitto-clients  # Optional: local broker
   ```

2. **Configure script**:
   ```python
   # In raspberrypi_startup_mqtt.py
   MQTT_ENABLED = True
   MQTT_BROKER_HOST = "192.168.x.x"  # CuteCaspar machine IP
   UDP_ENABLED = False  # Disable for MQTT-only mode
   ```

3. **Run script**:
   ```bash
   python3 raspberrypi_startup_mqtt.py
   ```

### Auto-Start at Boot (Systemd Service)

For production use, configure the script to start automatically at boot using systemd:

#### **1. Create Service File**
```bash
sudo nano /etc/systemd/system/cutecaspar-raspi.service
```

Add this content:
```ini
[Unit]
Description=CuteCaspar Raspberry Pi MQTT Controller
Documentation=https://github.com/geertdaelemans/CuteCaspar
After=network-online.target
Wants=network-online.target
StartLimitIntervalSec=0

[Service]
Type=simple
User=pi
Group=pi
WorkingDirectory=/home/pi

# Main command
ExecStart=/usr/bin/python3 /home/pi/raspberrypi_startup_mqtt.py

# Restart policy
Restart=always
RestartSec=10
StartLimitBurst=5

# Logging
StandardOutput=journal
StandardError=journal
SyslogIdentifier=cutecaspar-raspi

# Environment
Environment=PYTHONUNBUFFERED=1
Environment=PYTHONIOENCODING=utf-8

# Security (optional hardening)
NoNewPrivileges=true
PrivateTmp=true

# Process management
KillMode=mixed
KillSignal=SIGTERM
TimeoutStopSec=30

[Install]
WantedBy=multi-user.target
```

#### **2. Install and Enable Service**
```bash
# Reload systemd to read the new service
sudo systemctl daemon-reload

# Enable the service to start at boot
sudo systemctl enable cutecaspar-raspi.service

# Start the service now
sudo systemctl start cutecaspar-raspi.service

# Check status
sudo systemctl status cutecaspar-raspi.service
```

#### **3. Service Management Commands**
```bash
# Start service
sudo systemctl start cutecaspar-raspi.service

# Stop service  
sudo systemctl stop cutecaspar-raspi.service

# Restart service
sudo systemctl restart cutecaspar-raspi.service

# Disable auto-start
sudo systemctl disable cutecaspar-raspi.service

# Enable auto-start
sudo systemctl enable cutecaspar-raspi.service

# View live logs
sudo journalctl -u cutecaspar-raspi.service -f

# View recent logs
sudo journalctl -u cutecaspar-raspi.service -n 50
```

#### **4. Verify Auto-Start**
```bash
# Test reboot
sudo reboot

# After reboot, check if service started automatically
sudo systemctl status cutecaspar-raspi.service
```

#### **Alternative Methods**

**Crontab (Simple alternative):**
```bash
# Edit user crontab
crontab -e

# Add this line:
@reboot sleep 30 && cd /home/pi && python3 raspberrypi_startup_mqtt.py
```

**RC.local (Legacy method):**
```bash
# Edit rc.local
sudo nano /etc/rc.local

# Add before "exit 0":
su pi -c 'cd /home/pi && python3 raspberrypi_startup_mqtt.py' &
```

#### **Benefits of Systemd Service**
- ✅ **Automatic restart** if script crashes
- ✅ **Proper logging** through systemd journal  
- ✅ **Network dependency** waits for network to be ready
- ✅ **Standard Linux service** management
- ✅ **Security features** (process isolation)
- ✅ **Easy troubleshooting** with `systemctl status`

## Files Structure

### Scripts
* **`raspberrypi_startup_mqtt.py`** - Enhanced MQTT + UDP support
* **`raspberrypi_startup.py`** - Original UDP-only version  
* **`test_mqtt_communication.py`** - MQTT testing utility

### Configuration
* **`cutecaspar-raspi.service`** - Systemd service file for auto-start
* **CuteCaspar.pro** - Updated with MQTT module support
* **Registry settings** - MQTT broker configuration
* **CSV files** - MIDI note assignments

## Recent Updates (2025)

### MQTT Migration
* ✅ **Complete MQTT integration** with Qt 6.8.3
* ✅ **Bidirectional communication** (commands + status)
* ✅ **Dual protocol support** during migration
* ✅ **Registry-based configuration** for production use
* ✅ **Backward compatibility** with existing UDP installations

### Bug Fixes & Improvements
* ✅ **Button debouncing** (300ms) prevents accidental double-clicks
* ✅ **Motion button logic** fixed (was inverted)
* ✅ **Command naming** clarified (`magnet_*` → `latch_*`)
* ✅ **Status message cleanup** prevents UI bouncing
* ✅ **Enhanced error handling** and reconnection logic

### UI Enhancements
* ✅ **Responsive controls** with proper state management
* ✅ **Real-time status updates** via MQTT
* ✅ **Visual feedback** for all GPIO operations
* ✅ **Connection indicators** and debugging output

## Development Status

**CURRENTLY STABLE** - MQTT migration completed successfully.

### Tested & Working
* ✅ MQTT communication (primary)
* ✅ UDP communication (legacy)
* ✅ All GPIO controls (light, motion, latch, smoke)
* ✅ Button debouncing and state management
* ✅ CasparCG integration
* ✅ MIDI/DMX functionality
* ✅ Playlist management

### Known Limitations
* SoundScape clipName is hardcoded
* Windows-only development environment
* Requires manual MQTT broker setup for distributed deployments

## Troubleshooting

### MQTT Connection Issues
1. Check broker IP address in registry settings
2. Verify broker is running and accessible
3. Test with `mosquitto_pub`/`mosquitto_sub` tools
4. Check firewall settings on broker machine

### GPIO Not Responding
1. Verify Raspberry Pi script is running
2. Check MQTT connection status in output
3. Test individual commands with MQTT tools
4. Verify GPIO pin assignments match hardware

### Button Bouncing/Rapid Clicks
* Issue resolved with 300ms debouncing
* Recompile CuteCaspar with latest fixes
* Check for multiple status message conflicts

### Auto-Start Service Issues

**Service not starting:**
```bash
# Check service status
sudo systemctl status cutecaspar-raspi.service

# View detailed logs
sudo journalctl -u cutecaspar-raspi.service -n 50

# Check service file syntax
sudo systemctl daemon-reload
```

**Script crashes on boot:**
```bash
# Check for missing dependencies
sudo journalctl -u cutecaspar-raspi.service | grep -i error

# Verify file permissions
ls -la /home/pi/raspberrypi_startup_mqtt.py

# Test script manually
cd /home/pi && python3 raspberrypi_startup_mqtt.py
```

**Network not ready:**
- Service waits for `network-online.target`
- If still failing, increase `RestartSec=10` to `RestartSec=30`
- Check network configuration on Raspberry Pi

**Permission issues:**
```bash
# Ensure pi user owns the script
sudo chown pi:pi /home/pi/raspberrypi_startup_mqtt.py
sudo chmod +x /home/pi/raspberrypi_startup_mqtt.py

# Check service file permissions
sudo chmod 644 /etc/systemd/system/cutecaspar-raspi.service
```

## License & Disclaimer

**DEVELOPMENT SOFTWARE** - No guarantees provided.

This software is experimental and intended for educational/hobbyist use. Use at your own risk in production environments.
