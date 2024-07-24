from flask import Flask, jsonify, request
from datetime import datetime, timedelta
import pytz
from datetime import datetime, timedelta, time

app = Flask(__name__)

# Set your timezone here
timezone = pytz.timezone("Europe/Stockholm")  # Replace with your actual timezone

# Constants (unchanged)
constants = {
    "pinLED": 5,
    "pinInput": 14,
    "waitThreshold": 8000,
    "maxOnDuration": 10000,
    "reconnectInterval": 5000,
    "reconnectTimeout": 60000,
    "standbyDuration": 7200000,
}

def get_next_watering_time():
    now = datetime.now(timezone)
    # Define the two fixed watering times: 00:10 AM and 12:10 PM
    first_watering_time = now.replace(hour=0, minute=20, second=0, microsecond=0)
    second_watering_time = now.replace(hour=12, minute=20, second=0, microsecond=0)
    
    if now < first_watering_time:
        next_watering_time = first_watering_time
    elif now < second_watering_time:
        next_watering_time = second_watering_time
    else:
        next_watering_time = first_watering_time + timedelta(days=1)
    
    return next_watering_time



# Set watering_time to 15 seconds
watering_time = 1000 * 60 * 5  # 5 minuter


@app.route("/data", methods=["GET"])
def get_data():
    now = datetime.now(timezone)
    next_watering_time = get_next_watering_time()
    remaining_time = (next_watering_time - now).total_seconds()

    print(f"Current time: {now}")
    print(f"Next watering time: {next_watering_time}")
    print(f"Time until watering = {remaining_time} seconds")

    return jsonify(
        time_until_watering=int(remaining_time),
        watering_time=watering_time,
        current_time=now.strftime("%Y-%m-%d %H:%M:%S %Z"),
        next_watering_time=next_watering_time.strftime("%Y-%m-%d %H:%M:%S %Z"),
    )

# Endpoint to handle the request when no HIGH signal is received
@app.route("/no_button_signal", methods=["GET"])
def no_signal():
    no_signal_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    print(f"No button signal received at {no_signal_time}")
    # Perform any desired actions here (e.g., logging, sending notifications, etc.)
    return "OK"


# Endpoint to log light status
@app.route("/log_light_status", methods=["POST"])
def log_light_status():
    data = request.json
    status = data.get("status")
    timestamp = data.get("timestamp")
    log_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    
    print(f"Light status: {status}, Timestamp: {timestamp}, Logged at: {log_time}")
    # Perform any desired actions here (e.g., logging, sending notifications, etc.)
    return "Logged"


# Other routes remain unchanged

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5001)
