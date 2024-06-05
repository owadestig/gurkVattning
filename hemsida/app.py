from flask import Flask, jsonify, request
from datetime import datetime, timedelta
import random

app = Flask(__name__)

# Set the initial timer activation time (e.g., at the start of the next minute)
time_interval = timedelta(minutes=1)
next_timer_activation = datetime.now() + time_interval

# Set watering_time
watering_time = 1000 * 60 * 5  # 3 minuter

# Constants
constants = {
    "pinLED": 5,
    "pinInput": 14,
    "waitThreshold": 1000
    * 60
    * 240,  # Hur länge den ska sova om tid kvar är mer än detta, #! 4 timmar
    "maxOnDuration": 10000,  # tills den ska sluta snurra om knappen är trasig #! 10 sekunder
    "reconnectInterval": 5000,  # hur ofta jag söker efter nät ifall den inte hittar, intervall #! 5 sekunder
    "reconnectTimeout": 60000,  # efter hur mycket tid den ska sluta leta efter nät o somna #! 1 minute
    "standbyDuration": 7200000,  # Om det inte gick att ansluta till internet #! 2 timmar
}


# Endpoint to provide constants
@app.route("/constants", methods=["GET"])
def get_constants():
    return jsonify(constants)


# Endpoint that returns the time until the next timer activation and the LED duration
@app.route("/data", methods=["GET"])
def get_data():
    global next_timer_activation, watering_time, time_interval
    now = datetime.now()
    remaining_time = (next_timer_activation - now).total_seconds()

    if remaining_time <= 0:
        next_timer_activation += time_interval
        print(f"New LED duration: {watering_time} seconds")
        return jsonify(
            time_until_watering=int(
                remaining_time + timedelta(minutes=1).total_seconds()
            ),
            watering_time=watering_time,
            current_time=now.strftime("%Y-%m-%d %H:%M:%S"),
        )

    return jsonify(
        time_until_watering=int(remaining_time),
        watering_time=watering_time,
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
