from flask import Flask, jsonify

app = Flask(__name__)
counter = 0


# Endpoint that returns a value
@app.route("/data", methods=["GET"])
def get_data():
    global counter  # Declare counter as global to modify it
    counter += 1  # Increment the counter

    return jsonify(value=counter)


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5001)
