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
    "pinLED": 5,
    "pinInput": 14,
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

# Event logging
events_log = []
MAX_LOG_ENTRIES = 100

def log_event(event_type: str, message: str, timestamp: datetime = None):
    """Log system events"""
    if timestamp is None:
        timestamp = datetime.now(timezone)
    
    event = {
        "timestamp": timestamp.strftime("%Y-%m-%d %H:%M:%S %Z"),
        "type": event_type,
        "message": message
    }
    
    events_log.insert(0, event)  # Insert at beginning for newest first
    if len(events_log) > MAX_LOG_ENTRIES:
        events_log.pop()  # Remove oldest entry
    
    # Fix the f-string syntax issue
    print(f"[{event['timestamp']}] {event_type.upper()}: {message}")

def get_next_watering_time():
    """Calculate next scheduled watering time"""
    now = datetime.now(timezone)
    # Define the two fixed watering times: 00:09 AM and 14:00 PM
    first_watering_time = now.replace(hour=0, minute=9, second=0, microsecond=0)
    second_watering_time = now.replace(hour=14, minute=0, second=0, microsecond=0)

    if now < first_watering_time:
        next_watering_time = first_watering_time
    elif now < second_watering_time:
        next_watering_time = second_watering_time
    else:
        next_watering_time = first_watering_time + timedelta(days=1)

    return next_watering_time

def check_system_status():
    """Determine current system status based on various factors"""
    now = datetime.now(timezone)
    
    # Check if device is offline (no contact for more than 10 minutes)
    if system_state["last_device_contact"]:
        last_contact = datetime.fromisoformat(system_state["last_device_contact"].replace(" CET", "").replace(" CEST", ""))
        if (now - last_contact).total_seconds() > 600:  # 10 minutes
            return "offline"
    
    if system_state["is_watering"]:
        return "watering"
    elif system_state["error_message"]:
        return "error"
    else:
        return "idle"

# Set watering_time to 5 minutes (in milliseconds for ESP8266)
watering_time_minutes = 5
watering_time = 1000 * 60 * watering_time_minutes
sleep_time = 14400  # 4 hours in seconds

# ESP8266 Endpoints (matching your existing code)

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
        "watering_time": watering_time_minutes,  # ESP8266 expects this in minutes
        "sleep_time": sleep_time,
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
    timestamp = data.get("timestamp") or datetime.now(timezone).strftime("%Y-%m-%d %H:%M:%S %Z")
    
    system_state["last_device_contact"] = datetime.now(timezone).strftime("%Y-%m-%d %H:%M:%S %Z")
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
            .btn { background-color: #4CAF50; color: white; padding: 10px 20px; border: none; border-radius: 4px; cursor: pointer; margin: 5px; }
            .btn:hover { background-color: #45a049; }
            .btn:disabled { background-color: #cccccc; cursor: not-allowed; }
            .btn-danger { background-color: #f44336; }
            .btn-danger:hover { background-color: #da190b; }
            .events-log { max-height: 300px; overflow-y: auto; border: 1px solid #ddd; padding: 10px; background-color: #f9f9f9; }
            .event { padding: 5px; border-bottom: 1px solid #eee; }
            .event:last-child { border-bottom: none; }
            .refresh-note { color: #666; font-size: 0.9em; margin-top: 10px; }
        </style>
        <script>
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
        
        <script>
            // Load current status and update UI
            fetch('/api/status')
                .then(response => response.json())
                .then(data => {
                    updateStatusDisplay(data);
                    updateEventsList(data.events);
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
        hours = int(remaining_time/3600)
        minutes = int((remaining_time % 3600)/60)
        time_formatted = f"{hours}h {minutes}m"
    
    system_state["system_status"] = check_system_status()
    
    return jsonify({
        "system_status": system_state["system_status"],
        "is_watering": system_state["is_watering"],
        "current_time": now.strftime("%Y-%m-%d %H:%M:%S %Z"),
        "next_watering_time": next_watering_time.strftime("%Y-%m-%d %H:%M:%S %Z"),
        "time_until_watering": int(remaining_time),
        "time_until_watering_formatted": time_formatted,
        "last_watering": system_state["last_watering"],
        "last_device_contact": system_state["last_device_contact"],
        "error_message": system_state["error_message"],
        "events": events_log[:10]  # Return last 10 events
    })

@app.route("/manual_water", methods=["POST"])
def manual_water():
    """Trigger manual watering cycle"""
    if system_state["is_watering"]:
        return jsonify({"status": "error", "message": "System is already watering"}), 400
    
    system_state["manual_watering_requested"] = True
    system_state["error_message"] = ""  # Clear any errors
    log_event("manual", "Manual watering requested from web interface")
    
    return jsonify({"status": "success", "message": "Manual watering initiated. Device will start watering on next check-in."})

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
    limit = request.args.get("limit", 50, type=int)
    return jsonify({"events": events_log[:limit]})

# Legacy endpoint for backward compatibility
@app.route("/data", methods=["GET"])
def get_data():
    """Legacy endpoint - redirects to get_device_variables"""
    return get_device_variables()

# Initialize system
log_event("system", "Watering system started")

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5001, debug=True)
