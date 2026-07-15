import serial
import time
import base64
import sys

port = 'COM7'
baud = 115200
file_path = 'build_arm/metalslug'

print(f"Lendo {file_path}...")
with open(file_path, 'rb') as f:
    data = f.read()

b64_data = base64.b64encode(data)
print(f"Tamanho em Base64: {len(b64_data)} bytes.")

try:
    ser = serial.Serial(port, baud, timeout=1)
except Exception as e:
    print(f"ERRO: Nao foi possivel abrir {port}. O Putty esta aberto? Feche primeiro! Erro: {e}")
    sys.exit(1)

ser.reset_input_buffer()
ser.reset_output_buffer()

print("Desativando echo do terminal Linux...")
ser.write(b"\n")
time.sleep(0.5)
ser.write(b"stty -echo\n")
time.sleep(1)

print("Iniciando transferencia...")
ser.write(b"base64 -d > /root/metalslug\n")
time.sleep(1)

chunk_size = 1024
total = len(b64_data)

for i in range(0, total, chunk_size):
    chunk = b64_data[i:i+chunk_size]
    ser.write(chunk)
    ser.flush()
    time.sleep(0.012)
    if (i % (chunk_size * 100)) == 0:
        pct = (i / total) * 100
        print(f"{pct:.1f}% enviado...")
        # Lendo para esvaziar o RX buffer caso tenha sobrado algum lixo
        if ser.in_waiting > 0:
            ser.read(ser.in_waiting)

print("100.0% Finalizando arquivo...")
time.sleep(1)
ser.write(b"\n")
time.sleep(1)
ser.write(b"\x04")
time.sleep(1)

print("Dando permissao de execucao...")
ser.write(b"chmod +x /root/metalslug\n")
time.sleep(1)
ser.write(b"stty echo\n")
time.sleep(1)

ser.close()
print("Sucesso! Pode abrir o Putty de novo!")
