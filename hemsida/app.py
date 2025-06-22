from flask import Flask, jsonify, request, render_template_string
from datetime import datetime, timedelta
import pytz
import json
import os
from typing import Dict, List

app = Flask(__name__)

# Set your timezone here
timezone = pytz.timezone("Europe/Stockholm")

# Constants (matching ESP8266 expectations)
constants = {
    "pinLED": 16,
    "pinInput": 2,
    "waitThreshold": 8000,
    "maxOnDuration": 10000,
    "reconnectInterval": 5000,
    "reconnectTimeout": 60000,
    "standbyDuration": 7200000,
}

# System state storage
system_state = {
    "is_watering": False,
    "system_status": "idle",  # idle, watering, error, offline
    "last_watering": None,
    "last_button_signal": None,
    "last_device_contact": None,
    "manual_watering_requested": False,
    "error_message": "",
}

# Add custom watering times storage (separate from system_state)
custom_watering_times = ["00:09", "14:00"]  # Default times

# Event logging
events_log = []
MAX_LOG_ENTRIES = 1000  # Increased from 100 to 1000 for better history


def log_event(event_type: str, message: str, timestamp: datetime = None):
    """Log system events"""
    if timestamp is None:
        timestamp = datetime.now(timezone)

    event = {
        "timestamp": timestamp.strftime("%Y-%m-%d %H:%M:%S %Z"),
        "type": event_type,
        "message": message,
    }

    events_log.insert(0, event)  # Insert at beginning for newest first
    if len(events_log) > MAX_LOG_ENTRIES:
        events_log.pop()  # Remove oldest entry

    # Fix the f-string syntax issue
    print(f"[{event['timestamp']}] {event_type.upper()}: {message}")


def get_next_watering_time():
    """Calculate next scheduled watering time"""
    now = datetime.now(timezone)
    watering_times = custom_watering_times

    # Convert time strings to datetime objects for today
    today_times = []
    for time_str in watering_times:
        hour, minute = map(int, time_str.split(":"))
        watering_time = now.replace(hour=hour, minute=minute, second=0, microsecond=0)
        today_times.append(watering_time)

    # Sort times
    today_times.sort()

    # Find next watering time
    for watering_time in today_times:
        if now < watering_time:
            return watering_time

    # If all times have passed today, return the first time tomorrow
    if today_times:
        tomorrow = today_times[0] + timedelta(days=1)
        return tomorrow

    # Fallback to default if no times are set
    return now.replace(hour=0, minute=9, second=0, microsecond=0) + timedelta(days=1)


def check_system_status():
    """Determine current system status based on various factors"""
    now = datetime.now(timezone)

    # Check if device is offline (no contact for more than 10 minutes)
    if system_state["last_device_contact"]:
        last_contact = datetime.fromisoformat(
            system_state["last_device_contact"].replace(" CET", "").replace(" CEST", "")
        )
        if (now - last_contact).total_seconds() > 600:  # 10 minutes
            return "offline"

    if system_state["is_watering"]:
        return "watering"
    elif system_state["error_message"]:
        return "error"
    else:
        return "idle"


# Set watering_time to 5 minutes (in milliseconds for ESP8266)
valve_on_duration_minutes = 5
valve_on_duration = 1000 * 60 * valve_on_duration_minutes
sleep_time = 14400  # 4 hours in seconds

# Add system configuration storage
system_config = {
    "valve_on_duration_minutes": valve_on_duration_minutes,
    "sleep_time": sleep_time,  # in seconds
}


@app.route("/api/system_config", methods=["GET"])
def get_system_config():
    """Get current system configuration"""
    return jsonify(
        {
            "valve_on_duration_minutes": system_config["valve_on_duration_minutes"],
            "sleep_time": system_config["sleep_time"],
            "sleep_time_hours": round(system_config["sleep_time"] / 3600, 1),
        }
    )


@app.route("/api/system_config", methods=["POST"])
def set_system_config():
    """Set system configuration"""
    global valve_on_duration_minutes, sleep_time

    data = request.json
    if not data:
        return jsonify({"error": "No configuration data provided"}), 400

    # Validate valve duration (1-60 minutes)
    if "valve_on_duration_minutes" in data:
        duration = data["valve_on_duration_minutes"]
        if not isinstance(duration, (int, float)) or not (1 <= duration <= 60):
            return (
                jsonify({"error": "Valve duration must be between 1 and 60 minutes"}),
                400,
            )
        system_config["valve_on_duration_minutes"] = int(duration)
        valve_on_duration_minutes = int(duration)

    # Validate sleep time (0.5-24 hours)
    if "sleep_time_hours" in data:
        hours = data["sleep_time_hours"]
        if not isinstance(hours, (int, float)) or not (0.5 <= hours <= 24):
            return (
                jsonify({"error": "Sleep time must be between 0.5 and 24 hours"}),
                400,
            )
        system_config["sleep_time"] = int(hours * 3600)  # Convert to seconds
        sleep_time = int(hours * 3600)

    log_event(
        "config",
        f"System configuration updated: valve={system_config['valve_on_duration_minutes']}min, sleep={round(system_config['sleep_time']/3600, 1)}h",
    )

    return jsonify(
        {
            "status": "success",
            "message": "System configuration updated",
            "valve_on_duration_minutes": system_config["valve_on_duration_minutes"],
            "sleep_time": system_config["sleep_time"],
            "sleep_time_hours": round(system_config["sleep_time"] / 3600, 1),
        }
    )


@app.route("/get_device_variables", methods=["GET"])
def get_device_variables():
    """Main endpoint for ESP8266 to fetch watering schedule and timing"""
    now = datetime.now(timezone)
    next_watering_time = get_next_watering_time()
    remaining_time = (next_watering_time - now).total_seconds()

    # Update last contact time
    system_state["last_device_contact"] = now.strftime("%Y-%m-%d %H:%M:%S %Z")

    # If manual watering is requested, override the timing
    if system_state["manual_watering_requested"]:
        remaining_time = 0  # Trigger immediate watering
        system_state["manual_watering_requested"] = False
        log_event("manual", "Manual watering triggered from web interface")

    response_data = {
        "time_until_watering": int(remaining_time),
        "valve_on_duration_minutes": system_config[
            "valve_on_duration_minutes"
        ],  # Use config value
        "sleep_time": system_config["sleep_time"],  # Use config value
        "current_time": now.strftime("%Y-%m-%d %H:%M:%S %Z"),
        "next_watering_time": next_watering_time.strftime("%Y-%m-%d %H:%M:%S %Z"),
    }

    print(f"Device request - Time until watering: {remaining_time} seconds")
    return jsonify(response_data)


@app.route("/set_is_watering", methods=["POST"])
def set_is_watering():
    """Endpoint for ESP8266 to report watering status"""
    data = request.json
    if not data:
        return jsonify({"error": "No JSON data provided"}), 400

    is_watering = data.get("isWatering", False)
    timestamp = datetime.now(timezone)

    system_state["is_watering"] = is_watering
    system_state["last_device_contact"] = timestamp.strftime("%Y-%m-%d %H:%M:%S %Z")

    if is_watering:
        system_state["error_message"] = ""  # Clear any previous errors
        log_event("watering", "Watering started")
    else:
        system_state["last_watering"] = timestamp.strftime("%Y-%m-%d %H:%M:%S %Z")
        log_event("watering", "Watering completed")

    system_state["system_status"] = check_system_status()

    return jsonify({"status": "success", "isWatering": is_watering})


@app.route("/no_button_signal", methods=["GET"])
def no_signal():
    """Endpoint to handle when no button signal is received"""
    timestamp = datetime.now(timezone)
    system_state["last_device_contact"] = timestamp.strftime("%Y-%m-%d %H:%M:%S %Z")
    system_state["error_message"] = "No button signal received"
    system_state["system_status"] = "error"

    log_event("error", "No button signal received - device may be shutting down")
    return "OK"


@app.route("/log_light_status", methods=["POST"])
def log_light_status():
    """Endpoint to log LED/light status changes"""
    data = request.json
    if not data:
        return jsonify({"error": "No JSON data provided"}), 400

    status = data.get("status")
    timestamp = data.get("timestamp") or datetime.now(timezone).strftime(
        "%Y-%m-%d %H:%M:%S %Z"
    )

    system_state["last_device_contact"] = datetime.now(timezone).strftime(
        "%Y-%m-%d %H:%M:%S %Z"
    )
    log_event("light", f"LED status: {status}")

    return jsonify({"status": "logged"})


# Web Interface Endpoints


@app.route("/", methods=["GET"])
def dashboard():
    """Main web dashboard"""
    html_template = """
    <!DOCTYPE html>
    <html>
    <head>
        <title>Gurk Watering System</title>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <style>
            body { font-family: Arial, sans-serif; margin: 20px; background-color: #f5f5f5; }
            .container { max-width: 800px; margin: 0 auto; background-color: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
            .status-card { background-color: #e8f5e8; padding: 15px; border-radius: 5px; margin: 10px 0; border-left: 4px solid #4CAF50; }
            .error-card { background-color: #ffe8e8; border-left-color: #f44336; }
            .watering-card { background-color: #e8f4fd; border-left-color: #2196F3; }
            .offline-card { background-color: #f0f0f0; border-left-color: #9E9E9E; }
            .schedule-card { background-color: #fff3e0; border-left-color: #ff9800; }
            .config-card { background-color: #f3e5f5; border-left-color: #9c27b0; }
            .btn { background-color: #4CAF50; color: white; padding: 10px 20px; border: none; border-radius: 4px; cursor: pointer; margin: 5px; }
            .btn:hover { background-color: #45a049; }
            .btn:disabled { background-color: #cccccc; cursor: not-allowed; }
            .btn-danger { background-color: #f44336; }
            .btn-danger:hover { background-color: #da190b; }
            .btn-secondary { background-color: #ff9800; }
            .btn-secondary:hover { background-color: #e68900; }
            .btn-purple { background-color: #9c27b0; }
            .btn-purple:hover { background-color: #7b1fa2; }
            .events-log { max-height: 400px; overflow-y: auto; border: 1px solid #ddd; padding: 10px; background-color: #f9f9f9; }
            .event { padding: 5px; border-bottom: 1px solid #eee; }
            .event:last-child { border-bottom: none; }
            .refresh-note { color: #666; font-size: 0.9em; margin-top: 10px; }
            .schedule-section { margin: 20px 0; }
            .time-input { padding: 5px; margin: 2px; border: 1px solid #ddd; border-radius: 3px; width: 70px; }
            .config-input { padding: 5px; margin: 2px; border: 1px solid #ddd; border-radius: 3px; width: 80px; }
            .time-list { margin: 10px 0; }
            .time-item { display: inline-block; margin: 5px; padding: 5px 10px; background-color: #e8f5e8; border-radius: 15px; border: 1px solid #4CAF50; }
            .remove-time { margin-left: 5px; color: #f44336; cursor: pointer; font-weight: bold; }
            .modal { display: none; position: fixed; z-index: 1000; left: 0; top: 0; width: 100%; height: 100%; background-color: rgba(0,0,0,0.5); }
            .modal-content { background-color: white; margin: 15% auto; padding: 20px; border-radius: 10px; width: 400px; max-width: 90%; }
            .close { color: #aaa; float: right; font-size: 28px; font-weight: bold; cursor: pointer; }
            .close:hover { color: black; }
            .config-row { margin: 10px 0; display: flex; align-items: center; justify-content: space-between; }
            .config-label { font-weight: bold; flex: 1; }
            .config-value { flex: 1; text-align: right; }
        </style>
        <script>
            let currentWateringTimes = [];
            let currentConfig = {};
            
            function manualWater() {
                if (confirm('Start manual watering cycle?')) {
                    fetch('/manual_water', { method: 'POST' })
                        .then(response => response.json())
                        .then(data => {
                            alert(data.message);
                            setTimeout(() => location.reload(), 1000);
                        });
                }
            }
            
            function clearError() {
                fetch('/clear_error', { method: 'POST' })
                    .then(response => response.json())
                    .then(data => {
                        alert(data.message);
                        location.reload();
                    });
            }
            
            function openScheduleModal() {
                document.getElementById('scheduleModal').style.display = 'block';
                loadCurrentSchedule();
            }
            
            function closeScheduleModal() {
                document.getElementById('scheduleModal').style.display = 'none';
            }
            
            function openConfigModal() {
                document.getElementById('configModal').style.display = 'block';
                loadCurrentConfig();
            }
            
            function closeConfigModal() {
                document.getElementById('configModal').style.display = 'none';
            }
            
            function loadCurrentSchedule() {
                fetch('/api/watering_schedule')
                    .then(response => response.json())
                    .then(data => {
                        currentWateringTimes = data.watering_times || [];
                        updateTimesList();
                    });
            }
            
            function loadCurrentConfig() {
                fetch('/api/system_config')
                    .then(response => response.json())
                    .then(data => {
                        currentConfig = data;
                        document.getElementById('valveDurationInput').value = data.valve_on_duration_minutes;
                        document.getElementById('sleepTimeInput').value = data.sleep_time_hours;
                    });
            }
            
            function updateTimesList() {
                const timesList = document.getElementById('timesList');
                timesList.innerHTML = currentWateringTimes.map((time, index) => 
                    `<div class="time-item">
                        ${time} 
                        <span class="remove-time" onclick="removeTime(${index})">×</span>
                    </div>`
                ).join('');
            }
            
            function addTime() {
                const timeInput = document.getElementById('newTimeInput');
                const timeValue = timeInput.value;
                
                if (!timeValue) {
                    alert('Please enter a time');
                    return;
                }
                
                if (currentWateringTimes.includes(timeValue)) {
                    alert('This time is already in the schedule');
                    return;
                }
                
                currentWateringTimes.push(timeValue);
                currentWateringTimes.sort();
                updateTimesList();
                timeInput.value = '';
            }
            
            function removeTime(index) {
                currentWateringTimes.splice(index, 1);
                updateTimesList();
            }
            
            function saveSchedule() {
                if (currentWateringTimes.length === 0) {
                    alert('Please add at least one watering time');
                    return;
                }
                
                fetch('/api/watering_schedule', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json',
                    },
                    body: JSON.stringify({
                        watering_times: currentWateringTimes
                    })
                })
                .then(response => response.json())
                .then(data => {
                    if (data.status === 'success') {
                        alert(data.message);
                        closeScheduleModal();
                        location.reload();
                    } else {
                        alert('Error: ' + data.error);
                    }
                });
            }
            
            function saveConfig() {
                const valveDuration = parseFloat(document.getElementById('valveDurationInput').value);
                const sleepTime = parseFloat(document.getElementById('sleepTimeInput').value);
                
                if (!valveDuration || valveDuration < 1 || valveDuration > 60) {
                    alert('Valve duration must be between 1 and 60 minutes');
                    return;
                }
                
                if (!sleepTime || sleepTime < 0.5 || sleepTime > 24) {
                    alert('Sleep time must be between 0.5 and 24 hours');
                    return;
                }
                
                fetch('/api/system_config', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json',
                    },
                    body: JSON.stringify({
                        valve_on_duration_minutes: valveDuration,
                        sleep_time_hours: sleepTime
                    })
                })
                .then(response => response.json())
                .then(data => {
                    if (data.status === 'success') {
                        alert(data.message);
                        closeConfigModal();
                        location.reload();
                    } else {
                        alert('Error: ' + data.error);
                    }
                });
            }
            
            // Auto-refresh every 30 seconds
            setTimeout(() => location.reload(), 30000);
        </script>
    </head>
    <body>
        <div class="container">
            <h1>🥒 Gurk Watering System</h1>
            
            <div id="status-section">
                <!-- Status will be inserted here -->
            </div>
            
            <div class="schedule-section">
                <div class="schedule-card status-card">
                    <h3>⏰ Watering Schedule</h3>
                    <div id="current-schedule">
                        <!-- Schedule will be loaded here -->
                    </div>
                    <button class="btn btn-secondary" onclick="openScheduleModal()">🛠️ Configure Schedule</button>
                </div>
            </div>
            
            <div class="schedule-section">
                <div class="config-card status-card">
                    <h3>⚙️ System Configuration</h3>
                    <div id="current-config">
                        <!-- Config will be loaded here -->
                    </div>
                    <button class="btn btn-purple" onclick="openConfigModal()">🔧 Configure System</button>
                </div>
            </div>
            
            <div id="controls-section">
                <h3>Controls</h3>
                <button class="btn" onclick="manualWater()" id="manual-btn">💧 Manual Water</button>
                <button class="btn btn-danger" onclick="clearError()" id="clear-error-btn" style="display: none;">❌ Clear Error</button>
                <button class="btn" onclick="location.reload()">🔄 Refresh</button>
            </div>
            
            <div id="events-section">
                <h3>Recent Events</h3>
                <div class="events-log" id="events-log">
                    <!-- Events will be inserted here -->
                </div>
            </div>
            
            <div class="refresh-note">
                Page auto-refreshes every 30 seconds
            </div>
        </div>
        
        <!-- Schedule Modal -->
        <div id="scheduleModal" class="modal">
            <div class="modal-content">
                <span class="close" onclick="closeScheduleModal()">&times;</span>
                <h2>Configure Watering Schedule</h2>
                <p>Add watering times in 24-hour format (HH:MM):</p>
                
                <div>
                    <input type="time" id="newTimeInput" class="time-input">
                    <button class="btn" onclick="addTime()">Add Time</button>
                </div>
                
                <div class="time-list">
                    <h4>Current Schedule:</h4>
                    <div id="timesList">
                        <!-- Times will be loaded here -->
                    </div>
                </div>
                
                <div style="margin-top: 20px;">
                    <button class="btn" onclick="saveSchedule()">💾 Save Schedule</button>
                    <button class="btn btn-danger" onclick="closeScheduleModal()">Cancel</button>
                </div>
            </div>
        </div>
        
        <!-- System Config Modal -->
        <div id="configModal" class="modal">
            <div class="modal-content">
                <span class="close" onclick="closeConfigModal()">&times;</span>
                <h2>System Configuration</h2>
                
                <div class="config-row">
                    <label class="config-label">Valve Duration (minutes):</label>
                    <input type="number" id="valveDurationInput" class="config-input" min="1" max="60" step="1">
                </div>
                <p style="font-size: 0.9em; color: #666; margin: 5px 0 15px 0;">How long the valve stays open for watering (1-60 minutes)</p>
                
                <div class="config-row">
                    <label class="config-label">Sleep Time (hours):</label>
                    <input type="number" id="sleepTimeInput" class="config-input" min="0.5" max="24" step="0.5">
                </div>
                <p style="font-size: 0.9em; color: #666; margin: 5px 0 15px 0;">How long the device sleeps between checks (0.5-24 hours)</p>
                
                <div style="margin-top: 20px;">
                    <button class="btn btn-purple" onclick="saveConfig()">💾 Save Configuration</button>
                    <button class="btn btn-danger" onclick="closeConfigModal()">Cancel</button>
                </div>
            </div>
        </div>
        
        <script>
            // Load current status and update UI
            fetch('/api/status')
                .then(response => response.json())
                .then(data => {
                    updateStatusDisplay(data);
                    updateEventsList(data.events);
                });
                
            // Load current schedule
            fetch('/api/watering_schedule')
                .then(response => response.json())
                .then(data => {
                    updateScheduleDisplay(data);
                });
                
            // Load current config
            fetch('/api/system_config')
                .then(response => response.json())
                .then(data => {
                    updateConfigDisplay(data);
                });
                
            function updateStatusDisplay(data) {
                const statusSection = document.getElementById('status-section');
                const status = data.system_status;
                let cardClass = 'status-card';
                let statusIcon = '✅';
                
                if (status === 'error') {
                    cardClass += ' error-card';
                    statusIcon = '❌';
                    document.getElementById('clear-error-btn').style.display = 'inline-block';
                } else if (status === 'watering') {
                    cardClass += ' watering-card';
                    statusIcon = '💧';
                } else if (status === 'offline') {
                    cardClass += ' offline-card';
                    statusIcon = '📴';
                }
                
                statusSection.innerHTML = `
                    <div class="${cardClass}">
                        <h3>${statusIcon} System Status: ${status.toUpperCase()}</h3>
                        <p><strong>Current Time:</strong> ${data.current_time}</p>
                        <p><strong>Next Watering:</strong> ${data.next_watering_time}</p>
                        <p><strong>Time Until Watering:</strong> ${data.time_until_watering_formatted}</p>
                        <p><strong>Last Watering:</strong> ${data.last_watering || 'Never'}</p>
                        <p><strong>Last Device Contact:</strong> ${data.last_device_contact || 'Never'}</p>
                        ${data.error_message ? `<p><strong>Error:</strong> ${data.error_message}</p>` : ''}
                    </div>
                `;
                
                // Disable manual water button if already watering
                document.getElementById('manual-btn').disabled = (status === 'watering');
            }
            
            function updateScheduleDisplay(data) {
                const scheduleDiv = document.getElementById('current-schedule');
                const times = data.watering_times || [];
                
                scheduleDiv.innerHTML = `
                    <p><strong>Scheduled Times:</strong> ${times.join(', ') || 'None set'}</p>
                    <p><strong>Next Watering:</strong> ${data.next_watering || 'Not scheduled'}</p>
                `;
            }
            
            function updateConfigDisplay(data) {
                const configDiv = document.getElementById('current-config');
                
                configDiv.innerHTML = `
                    <div class="config-row">
                        <span class="config-label">Valve Duration:</span>
                        <span class="config-value">${data.valve_on_duration_minutes} minutes</span>
                    </div>
                    <div class="config-row">
                        <span class="config-label">Device Sleep Time:</span>
                        <span class="config-value">${data.sleep_time_hours} hours</span>
                    </div>
                `;
            }
            
            function updateEventsList(events) {
                const eventsLog = document.getElementById('events-log');
                eventsLog.innerHTML = events.map(event => 
                    `<div class="event"><strong>${event.timestamp}</strong> [${event.type.toUpperCase()}] ${event.message}</div>`
                ).join('');
            }
        </script>
    </body>
    </html>
    """
    return render_template_string(html_template)


@app.route("/api/status", methods=["GET"])
def api_status():
    """API endpoint to get current system status"""
    now = datetime.now(timezone)
    next_watering_time = get_next_watering_time()
    remaining_time = (next_watering_time - now).total_seconds()

    # Format remaining time
    if remaining_time < 60:
        time_formatted = f"{int(remaining_time)} seconds"
    elif remaining_time < 3600:
        time_formatted = f"{int(remaining_time/60)} minutes"
    else:
        hours = int(remaining_time / 3600)
        minutes = int((remaining_time % 3600) / 60)
        time_formatted = f"{hours}h {minutes}m"

    system_state["system_status"] = check_system_status()

    return jsonify(
        {
            "system_status": system_state["system_status"],
            "is_watering": system_state["is_watering"],
            "current_time": now.strftime("%Y-%m-%d %H:%M:%S %Z"),
            "next_watering_time": next_watering_time.strftime("%Y-%m-%d %H:%M:%S %Z"),
            "time_until_watering": int(remaining_time),
            "time_until_watering_formatted": time_formatted,
            "last_watering": system_state["last_watering"],
            "last_device_contact": system_state["last_device_contact"],
            "error_message": system_state["error_message"],
            "events": events_log[:100],  # Return last 100 events instead of 10
        }
    )


@app.route("/manual_water", methods=["POST"])
def manual_water():
    """Trigger manual watering cycle"""
    if system_state["is_watering"]:
        return (
            jsonify({"status": "error", "message": "System is already watering"}),
            400,
        )

    system_state["manual_watering_requested"] = True
    system_state["error_message"] = ""  # Clear any errors
    log_event("manual", "Manual watering requested from web interface")

    return jsonify(
        {
            "status": "success",
            "message": "Manual watering initiated. Device will start watering on next check-in.",
        }
    )


@app.route("/clear_error", methods=["POST"])
def clear_error():
    """Clear system error state"""
    system_state["error_message"] = ""
    system_state["system_status"] = "idle"
    log_event("system", "Error state cleared from web interface")

    return jsonify({"status": "success", "message": "Error cleared"})


@app.route("/api/events", methods=["GET"])
def api_events():
    """Get system events log"""
    limit = request.args.get("limit", 100, type=int)  # Default to 100 instead of 50
    return jsonify({"events": events_log[:limit]})


@app.route("/api/watering_schedule", methods=["GET"])
def get_watering_schedule():
    """Get current watering schedule"""
    return jsonify(
        {
            "watering_times": custom_watering_times,
            "next_watering": get_next_watering_time().strftime("%Y-%m-%d %H:%M:%S %Z"),
        }
    )


@app.route("/api/watering_schedule", methods=["POST"])
def set_watering_schedule():
    """Set custom watering schedule"""
    global custom_watering_times

    data = request.json
    if not data or "watering_times" not in data:
        return jsonify({"error": "No watering times provided"}), 400

    watering_times = data["watering_times"]

    # Validate time format
    for time_str in watering_times:
        try:
            hour, minute = map(int, time_str.split(":"))
            if not (0 <= hour <= 23 and 0 <= minute <= 59):
                raise ValueError("Invalid time")
        except (ValueError, AttributeError):
            return (
                jsonify(
                    {"error": f"Invalid time format: {time_str}. Use HH:MM format"}
                ),
                400,
            )

    # Update schedule
    custom_watering_times = watering_times
    log_event("schedule", f"Watering schedule updated: {', '.join(watering_times)}")

    return jsonify(
        {
            "status": "success",
            "message": "Watering schedule updated",
            "watering_times": watering_times,
            "next_watering": get_next_watering_time().strftime("%Y-%m-%d %H:%M:%S %Z"),
        }
    )


# Legacy endpoint for backward compatibility
@app.route("/data", methods=["GET"])
def get_data():
    """Legacy endpoint - redirects to get_device_variables"""
    return get_device_variables()


# Initialize system
log_event("system", "Watering system started")

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5001, debug=True)
