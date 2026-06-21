import cv2
import base64
import numpy as np

from flask import Flask, request, Response

app = Flask(__name__)

frame = None

def base64_to_frame(string):
    img_bytes = base64.b64decode(string)
    img_arr = np.frombuffer(img_bytes, dtype=np.uint8)
    frame = cv2.imdecode(img_arr, flags=cv2.IMREAD_COLOR)

    return frame

@app.route('/a/', methods=['POST'])
def receiveData():
    global frame
    
    data = request.get_json()
    
    if data:
        screenshot_base64 = data.get('screenshot')

        if screenshot_base64:
            frame = base64_to_frame(screenshot_base64)

        return "ok"
    
    return "Nenhum dado JSON recebido", 400

def gen_frames():
    global frame
    while True:
        if frame is not None:
            small_frame = cv2.resize(frame, (0, 0), fx=0.5, fy=0.5)

            succeed, buffer = cv2.imencode(".jpg", small_frame)

            if succeed:
                bytes_jpeg=buffer.tobytes()

                yield(
                    b"--frame\r\n"
                    b"Content-Type: image/jpeg\r\n\r\n" + bytes_jpeg + b"\r\n"
                )
            
@app.route('/video_feed')
def video_feed():
    return Response(
        gen_frames(), mimetype="multipart/x-mixed-replace; boundary=frame"
    )
if __name__ == "__main__":
    import os

    # O Render injeta automaticamente a porta correta nesta variável de ambiente
    porta = int(os.environ.get("PORT", 3000))

    # IMPORTANTE: Mudar host para '0.0.0.0' para o Render conseguir mapear sua API
    app.run(host="0.0.0.0", port=porta, debug=False)