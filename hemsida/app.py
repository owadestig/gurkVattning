from flask import Flask, jsonify, request
from datetime import datetime, timedelta
import pytz

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


# Set watering times to be every other minute
def get_next_watering_time():
    now = datetime.now(timezone)
    next_minute = (now + timedelta(minutes=1)).replace(second=0, microsecond=0)
    next_watering_time = (
        next_minute if now.minute % 2 == 0 else next_minute + timedelta(minutes=1)
    )
    return next_watering_time


# Set watering_time to 15 seconds
watering_time = 1000 * 15  # 15 seconds


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


# Other routes remain unchanged

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5001)
