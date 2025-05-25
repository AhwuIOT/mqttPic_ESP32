from flask import Flask, request, render_template
import paho.mqtt.publish as publish
import base64
from PIL import Image, ImageOps
import io

app = Flask(__name__)

MQTT_BROKER = "test.mosquitto.org"
TOPIC_TEXT = "esp32/test"
TOPIC_IMAGE = "esp32/display/image"
MAX_B64_LEN = 7000  # 根據 ESP32 可承受大小調整（建議 6k~8k）

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
        try:
            # 1. 開啟圖片並轉為 RGB
            img = Image.open(file).convert("RGB")

            # 2. 顏色反轉（根據你 ESP32 顯示邏輯需要）
            img = ImageOps.invert(img)

            # 3. 縮小至 ESP32 支援解析度
            img = img.resize((128, 160))

            # 4. 壓縮儲存為 baseline JPEG（避免解碼失敗）
            buf = io.BytesIO()
            img.save(buf, format='JPEG', quality=60, optimize=True, progressive=False)
            buf.seek(0)

            # 5. base64 編碼並清除換行與空白
            b64 = base64.b64encode(buf.read()).decode('ascii')
            b64_cleaned = b64.replace('\n', '').replace('\r', '').replace(' ', '')

            # 6. 檢查長度，避免傳送超過 ESP32 buffer 限制
            if len(b64_cleaned) > MAX_B64_LEN:
                return f'''
                    <p>❌ 圖片太大！目前大小為 {len(b64_cleaned)} bytes，請改用更小或內容簡單的圖片。</p>
                    <a href="/"><button>回到表單</button></a>
                '''

            # 7. 發送至 MQTT
            publish.single(TOPIC_IMAGE, b64_cleaned, hostname=MQTT_BROKER)
        except Exception as e:
            return f"<p>❌ 圖片處理失敗: {e}</p>"

    return '''
        <p>✅ 訊息與圖片已送出（若有）</p>
        <a href="/"><button>回到表單</button></a>
    '''

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)
