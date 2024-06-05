from flask import Flask, jsonify, request
from datetime import datetime, timedelta
import random

app = Flask(__name__)

# Set the initial timer activation time (e.g., at the start of the next minute)
time_interval = timedelta(minutes=1)
next_timer_activation = datetime.now() + time_interval

# Set the initial LED duration
led_duration = 8


# Endpoint that returns the time until the next timer activation and the LED duration
@app.route("/data", methods=["GET"])
def get_data():
    global next_timer_activation, led_duration, time_interval
    now = datetime.now()
    remaining_time = (next_timer_activation - now).total_seconds()

    if remaining_time <= 0:
        # Timer has activated, set the next activation time and generate a new LED duration
        next_timer_activation += time_interval
        # led_duration = random.randint(5, 13)
        led_duration = 8
        print(f"New LED duration: {led_duration} seconds")

    return jsonify(
        value=int(
            remaining_time if remaining_time > 0 else time_interval.total_seconds()
        ),
        led_duration=led_duration,
        time_interval=int(time_interval.total_seconds()),
        current_time=now.strftime("%Y-%m-%d %H:%M:%S"),
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


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5001)
