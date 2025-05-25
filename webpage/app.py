from flask import Flask, request
import paho.mqtt.publish as publish

app = Flask(__name__)

MQTT_BROKER = "test.mosquitto.org"
MQTT_TOPIC = "esp32/test"

@app.route('/send', methods=['POST'])
def send_message():
    msg = request.form.get("msg")
    if not msg:
        return "No message sent", 400

    publish.single(MQTT_TOPIC, msg, hostname=MQTT_BROKER)

    return f'''
        <p>✅ Message sent to MQTT: {msg}</p>
        <a href="/"><button>Back to form</button></a>
    '''

@app.route('/')
def index():
    return '''
        <form method="POST" action="/send">
            <input name="msg" placeholder="Enter message">
            <button type="submit">Send to ESP32</button>
        </form>
    '''

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)
