from flask import Flask, jsonify, request
from datetime import datetime, timedelta
import pytz

app = Flask(__name__)

# Set your timezone here
timezone = pytz.timezone("Europe/Stockholm")  # Replace with your actual timezone


# Set watering times
def get_next_watering_times():
    now = datetime.now(timezone)
    today = now.date()
    tomorrow = today + timedelta(days=1)

    afternoon = timezone.localize(
        datetime.combine(today, datetime.min.time().replace(hour=14, minute=30))
    )
    night = timezone.localize(
        datetime.combine(tomorrow, datetime.min.time().replace(hour=2, minute=30))
    )

    if now > afternoon:
        afternoon = timezone.localize(datetime.combine(tomorrow, afternoon.time()))

    return afternoon, night


# Set watering_time
watering_time = 1000 * 60 * 3  # 3 minutes

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


@app.route("/data", methods=["GET"])
def get_data():
    now = datetime.now(timezone)
    afternoon, night = get_next_watering_times()

    if now < afternoon:
        next_watering_time = afternoon
    elif now < night:
        next_watering_time = night
    else:
        # This case should never happen due to how we set up the times, but just in case:
        next_watering_time = afternoon + timedelta(days=1)

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
