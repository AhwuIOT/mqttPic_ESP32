from flask import Flask, request, render_template
import paho.mqtt.publish as publish
import base64

app = Flask(__name__)

MQTT_BROKER = "test.mosquitto.org"
TOPIC_TEXT = "esp32/test"
TOPIC_IMAGE = "esp32/display/image"

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/send', methods=['POST'])
def send_message():
    msg = request.form.get("msg")
    file = request.files.get("image")
    
    if msg:
        publish.single(TOPIC_TEXT, msg, hostname=MQTT_BROKER)

    if file and file.filename:
        image_data = file.read()
        b64 = base64.b64encode(image_data).decode('ascii')  # or 'utf-8'

        # ✅ 去除換行與空白
        b64_cleaned = b64.replace('\n', '').replace('\r', '').replace(' ', '')

        publish.single(TOPIC_IMAGE, b64_cleaned, hostname=MQTT_BROKER)
    return '''
        <p>✅ 訊息與圖片已送出（若有）</p>
        <a href="/"><button>回到表單</button></a>
    '''

# @app.route('/')
# def index():
#     return f'''
#         <form method="POST" action="/send">
#             <input name="msg" placeholder="Enter message">
#             <button type="submit">Send to ESP32</button>
#         </form>
#     '''

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)
