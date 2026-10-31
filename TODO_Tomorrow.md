# CuteCaspar TODO List - October 10, 2025

## Priority Tasks for Tomorrow

### 1. MQTT Configuration GUI Implementation
- [ ] **Add MQTT settings tab/section** to CuteCaspar settings dialog
- [ ] **Create UI controls** for:
  - [ ] Enable/Disable MQTT checkbox
  - [ ] MQTT Broker Host (IP address input)
  - [ ] MQTT Broker Port (default: 1883)
  - [ ] MQTT Client ID (default: "CuteCaspar")
  - [ ] MQTT Topic Prefix (default: "cutecaspar/raspi")
- [ ] **Connect UI to registry settings** (VRT\CasparCGClient\Configuration)
- [ ] **Add validation** for IP addresses and port numbers
- [ ] **Test GUI changes** with live MQTT connection
- [ ] **Add connection status indicator** (connected/disconnected)

**Files to modify:**
- `CuteCaspar/SettingsDialog.ui` - Add MQTT controls
- `CuteCaspar/SettingsDialog.cpp` - Wire up the controls
- `CuteCaspar/SettingsDialog.h` - Add member variables
- `CuteCaspar/RaspberryPI.cpp` - Reload settings when changed

### 2. Verify Raspberry Pi System Commands
- [ ] **Test `reboot` command** via MQTT:
  ```bash
  mosquitto_pub -h 192.168.0.184 -t "cutecaspar/raspi/command" -m "reboot"
  ```
- [ ] **Test `shutdown` command** via MQTT:
  ```bash
  mosquitto_pub -h 192.168.0.184 -t "cutecaspar/raspi/command" -m "shutdown"
  ```
- [ ] **Test `quit` command** via MQTT:
  ```bash
  mosquitto_pub -h 192.168.0.184 -t "cutecaspar/raspi/command" -m "quit"
  ```
- [ ] **Verify commands work from CuteCaspar GUI** (if reboot/shutdown buttons exist)
- [ ] **Check systemd service** restarts properly after reboot
- [ ] **Test graceful shutdown** vs force shutdown

**Expected behaviors:**
- `reboot`: System restarts, service auto-starts
- `shutdown`: System powers off cleanly  
- `quit`: Script stops, systemd restarts it (unless disabled)

### 3. Additional Testing
- [ ] **MQTT reconnection** after network interruption
- [ ] **Registry settings persistence** after CuteCaspar restart
- [ ] **Error handling** for invalid MQTT broker addresses
- [ ] **Fallback behavior** when MQTT is unavailable

## Current Status (October 9, 2025)
✅ **COMPLETED:**
- MQTT integration with Qt 6.8.3
- Bidirectional MQTT communication
- Button debouncing (300ms)
- Motion button logic fix
- Latch command renaming (magnet_* → latch_*)
- Systemd service setup
- Complete README.md documentation

✅ **MQTT Registry Settings (Already Configured):**
- mqtt_enabled = true
- mqtt_broker_host = 192.168.0.184
- mqtt_broker_port = 1883
- mqtt_client_id = CuteCaspar
- mqtt_topic_prefix = cutecaspar/raspi

## Notes for Implementation

### MQTT GUI Integration Tips:
1. **Look at existing settings tabs** for UI patterns
2. **Use QSettings** to read/write registry values
3. **Add validation signals** for real-time feedback
4. **Consider adding "Test Connection" button**
5. **Emit signals** to RaspberryPI class when settings change

### Testing Commands:
```bash
# Test from command line
mosquitto_pub -h 192.168.0.184 -t "cutecaspar/raspi/command" -m "alive"

# Monitor responses
mosquitto_sub -h 192.168.0.184 -t "cutecaspar/raspi/status"

# Check systemd service
sudo systemctl status cutecaspar-raspi.service
sudo journalctl -u cutecaspar-raspi.service -f
```

### Potential Issues to Watch:
- **Reboot/shutdown timing** - ensure clean MQTT disconnection
- **Service restart loops** - if quit command causes systemd restart
- **Network timing** - MQTT reconnection after reboot
- **Registry permissions** - GUI writing to correct registry location

## Future Enhancements (Lower Priority)
- [ ] MQTT broker auto-discovery
- [ ] SSL/TLS MQTT connections
- [ ] MQTT authentication (username/password)
- [ ] Multiple Raspberry Pi support
- [ ] MQTT command queuing/retry logic

---
**Remember:** Test each change incrementally to avoid breaking the working MQTT setup!