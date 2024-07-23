from flask import Flask, jsonify, request
from datetime import datetime, timedelta
import random

app = Flask(__name__)

# Set watering times
afternoon_watering_time = datetime.now().replace(
    hour=14, minute=30, second=0, microsecond=0
)
night_watering_time = datetime.now().replace(hour=2, minute=30, second=0, microsecond=0)

# If current time is past 14:30, set next afternoon watering to tomorrow
if datetime.now().time() >= afternoon_watering_time.time():
    afternoon_watering_time += timedelta(days=1)

# If current time is past 2:30 AM but before 14:30, set next night watering to tomorrow
if (
    datetime.now().time() >= night_watering_time.time()
    and datetime.now().time() < afternoon_watering_time.time()
):
    night_watering_time += timedelta(days=1)

# Set watering_time
watering_time = 1000 * 60 * 3  # 3 minutes

# Constants
constants = {
    "pinLED": 5,
    "pinInput": 14,
    "waitThreshold": 8000,
    "maxOnDuration": 10000,
    "reconnectInterval": 5000,
    "reconnectTimeout": 60000,
    "standbyDuration": 7200000,
}


@app.route("/constants", methods=["GET"])
def get_constants():
    return jsonify(constants)


@app.route("/data", methods=["GET"])
def get_data():
    global afternoon_watering_time, night_watering_time, watering_time
    now = datetime.now()

    # Determine next watering time
    if now < night_watering_time:
        next_watering_time = night_watering_time
    elif now < afternoon_watering_time:
        next_watering_time = afternoon_watering_time
    else:
        next_watering_time = night_watering_time + timedelta(days=1)

    remaining_time = (next_watering_time - now).total_seconds()
    print("Time until watering = ", remaining_time)

    if remaining_time <= 0:
        # Update watering times for the next day
        if next_watering_time.hour == 2:  # If it was night watering
            night_watering_time += timedelta(days=1)
            next_watering_time = afternoon_watering_time
        else:  # If it was afternoon watering
            afternoon_watering_time += timedelta(days=1)
            next_watering_time = night_watering_time + timedelta(days=1)
        remaining_time = (next_watering_time - now).total_seconds()

    print(f"New LED duration: {watering_time} ms")
    return jsonify(
        time_until_watering=int(remaining_time),
        watering_time=watering_time,
        current_time=now.strftime("%Y-%m-%d %H:%M:%S"),
    )


@app.route("/no_button_signal", methods=["GET"])
def no_signal():
    no_signal_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    print(f"No button signal received at {no_signal_time}")
    return "OK"


@app.route("/log_light_status", methods=["POST"])
def log_light_status():
    data = request.json
    status = data.get("status")
    timestamp = data.get("timestamp")
    log_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    print(f"Light status: {status}, Timestamp: {timestamp}, Logged at: {log_time}")
    return "Logged"


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5001)
