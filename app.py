import base64
import cv2
import numpy as np
from flask import Flask, request, jsonify, Response

app = Flask(__name__)

# Variável global para armazenar temporariamente o último frame processado
ultimo_frame = None

@app.route('/a/', methods=['POST'])
def receber_dados():
    global ultimo_frame
    try:
        dados = request.get_json()
        if not dados or 'screenshot' not in dados:
            return jsonify({"status": "erro", "mensagem": "JSON inválido ou sem imagem"}), 400

        # Extrai os dados enviados pelo C++
        mouse_x = dados.get('mouse_x', '0')
        mouse_y = dados.get('mouse_y', '0')
        screenshot_b64 = dados.get('screenshot')

        # Decodifica a string Base64 de volta para bytes binários
        img_bytes = base64.b64decode(screenshot_b64)
        
        # Converte os bytes em um array que o OpenCV entende (Formato BMP do Windows)
        nparr = np.frombuffer(img_bytes, np.uint8)
        frame = cv2.imdecode(nparr, cv2.IMREAD_COLOR)

        # CORREÇÃO CRÍTICA: Valida se o frame não veio vazio ou corrompido
        if frame is not None and frame.size > 0:
            ultimo_frame = frame.copy() # Atualiza o frame global com segurança
            print(f"Frame recebido com sucesso. Mouse: X={mouse_x}, Y={mouse_y}")
            return jsonify({"status": "sucesso"}), 200
        else:
            print("Falha ao decodificar a imagem recebida.")
            return jsonify({"status": "erro", "mensagem": "Imagem corrompida"}), 400

    except Exception as e:
        print(f"Erro no processamento do POST: {e}")
        return jsonify({"status": "erro", "mensagem": str(e)}), 500


def gen_frames():
    global ultimo_frame
    while True:
        if ultimo_frame is None:
            # Se ainda não recebeu nenhum frame, envia uma tela preta temporária ou aguarda
            blank_image = np.zeros((480, 640, 3), np.uint8)
            ret, buffer = cv2.imencode('.jpg', blank_image)
            yield (b'--frame\r\n'
                   b'Content-Type: image/jpeg\r\n\r\n' + buffer.tobytes() + b'\r\n')
            cv2.waitKey(100) # Evita loop infinito travando a CPU
            continue

        try:
            # Captura o estado atual do frame global
            frame = ultimo_frame

            # CORREÇÃO DO ERRO DO LOG: Garante que o frame possui dimensões válidas antes do resize
            if frame is None or frame.shape[0] == 0 or frame.shape[1] == 0:
                continue

            # Redimensiona a imagem para diminuir o uso de banda do streaming web
            small_frame = cv2.resize(frame, (0, 0), fx=0.5, fy=0.5)
            
            # Codifica em JPEG para enviar via streaming HTTP
            ret, buffer = cv2.imencode('.jpg', small_frame)
            if not ret:
                continue
                
            frame_bytes = buffer.tobytes()
            yield (b'--frame\r\n'
                   b'Content-Type: image/jpeg\r\n\r\n' + frame_bytes + b'\r\n')
                   
        except Exception as e:
            print(f"Erro no streaming de frames: {e}")
            continue


@app.route('/video_feed')
def video_feed():
    # Retorna o streaming multipart HTTP correto para exibir no navegador
    return Response(gen_frames(), mimetype='multipart/x-mixed-replace; boundary=frame')


if __name__ == '__main__':
    # Roda localmente na porta 5000 (O Render gerencia a porta dinamicamente em produção)
    app.run(host='0.0.0.0', port=5000)