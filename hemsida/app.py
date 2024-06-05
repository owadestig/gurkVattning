from flask import Flask, jsonify, request
from datetime import datetime, timedelta
import random

app = Flask(__name__)

# Set the initial timer activation time (e.g., at the start of the next minute)
next_timer_activation = datetime.now().replace(second=0, microsecond=0) + timedelta(
    minutes=1
)

# Set the initial LED duration
led_duration = random.randint(5, 20)


# Endpoint that returns the time until the next timer activation and the LED duration
@app.route("/data", methods=["GET"])
def get_data():
    global next_timer_activation, led_duration
    now = datetime.now()
    remaining_time = (next_timer_activation - now).total_seconds()

    if remaining_time <= 0:
        # Timer has activated, set the next activation time and generate a new LED duration
        next_timer_activation += timedelta(minutes=1)
        led_duration = random.randint(5, 13)
        print(f"New LED duration: {led_duration} seconds")
        return jsonify(
            value=int(remaining_time + timedelta(minutes=1).total_seconds()),
            led_duration=led_duration,
        )

    return jsonify(
        value=int(remaining_time),
        led_duration=led_duration,
    )


# Endpoint to handle the request when the LED turns on
@app.route("/led_on", methods=["GET"])
def led_on():
    led_on_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    print(f"LED turned on at {led_on_time}")
    # Perform any desired actions here (e.g., logging, sending notifications, etc.)
    return "OK"


# Endpoint to handle the request when the LED turns off
@app.route("/led_off", methods=["GET"])
def led_off():
    led_off_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    print(f"LED turned off at {led_off_time}")
    # Perform any desired actions here (e.g., logging, sending notifications, etc.)
    return "OK"


# Endpoint to handle the request when no HIGH signal is received
@app.route("/no_signal", methods=["GET"])
def no_signal():
    no_signal_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    print(f"No HIGH signal received at {no_signal_time}")
    # Perform any desired actions here (e.g., logging, sending notifications, etc.)
    return "OK"


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5001)
