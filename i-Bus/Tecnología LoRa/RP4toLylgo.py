import tkinter as tk
from tkinter import ttk
from PIL import Image, ImageTk
import serial
import threading
import time
from datetime import datetime
import io
import pynmea2
import RPi.GPIO as GPIO

latest_gps_str = "N/A"
latest_gps_values = "N/A"
latest_speed = "0.00 km/h"
person_count = 0
last_trigger = None

# PIR setup
PIR_A = 17
PIR_B = 27
GPIO.setmode(GPIO.BCM)
GPIO.setup(PIR_A, GPIO.IN)
GPIO.setup(PIR_B, GPIO.IN)

# Serial ports
ser_lora = serial.Serial('/dev/ttyAMA0', 115200, timeout=1)
ser_arduino = serial.Serial('/dev/ttyAMA5', 9600, timeout=1)

time.sleep(2)
print("Configurando RYLR998...")

def send_cmd(ser, cmd):
    ser.write((cmd + '\r\n').encode())
    time.sleep(0.5)
    while ser.in_waiting:
        print(ser.readline().decode().strip())

send_cmd(ser_lora, "AT+ADDRESS=5")
send_cmd(ser_lora, "AT+NETWORKID=5")
send_cmd(ser_lora, "AT+BAND=915000000")
send_cmd(ser_lora, "AT+PARAMETER=9,7,1,12")
send_cmd(ser_lora, "AT+MODE=0")

print("Sistema listo para recibir datos...")

# GUI Setup
tk_root = tk.Tk()
tk_root.title("Monitor LoRa y GPS")
tk_root.attributes('-zoomed', True)  # Maximizar ventana (Linux/RPi)

main_frame = tk.Frame(tk_root)
main_frame.pack(fill=tk.BOTH, expand=True)

frame_data = tk.Frame(main_frame)
frame_data.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=10, pady=10)

frame_table = tk.Frame(frame_data)
frame_table.pack(fill=tk.BOTH, expand=True)
columns = ("Timestamp", "Source", "Data")
table = ttk.Treeview(frame_table, columns=columns, show="headings")
for col in columns:
    table.heading(col, text=col)
    table.column(col, width=150)
table.pack(pady=10, padx=10, fill=tk.BOTH, expand=True)

frame_legend = tk.Frame(frame_data)
frame_legend.pack(fill=tk.X, padx=10, pady=10)
legend_label = tk.Label(frame_legend, text="Rojo = Ocupado, Verde = Disponible", font=("Arial", 12))
legend_label.pack()

frame_image = tk.Frame(main_frame)
frame_image.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True, padx=10, pady=10)

bus_image = Image.open("/home/pi/Downloads/j.png")
bus_photo = ImageTk.PhotoImage(bus_image)
bus_label = tk.Label(frame_image, image=bus_photo)
bus_label.pack()

# Cuadro para mostrar la velocidad y conteo de personas
info_frame = tk.Frame(frame_image)
info_frame.pack(pady=10)

speed_var = tk.StringVar()
speed_var.set("Aprox: 0.00 km/h")
speed_frame = tk.LabelFrame(info_frame, text="Velocidad", font=("Arial", 10, "bold"), padx=10, pady=5)
speed_frame.pack(side=tk.LEFT, padx=5)
speed_label = tk.Label(speed_frame, textvariable=speed_var, font=("Arial", 12))
speed_label.pack()

person_var = tk.StringVar()
person_var.set("Pasajeros a bordo: 0")
person_frame = tk.LabelFrame(info_frame, text="Conteo de Pasajeros", font=("Arial", 10, "bold"), padx=10, pady=5)
person_frame.pack(side=tk.LEFT, padx=5)
person_label = tk.Label(person_frame, textvariable=person_var, font=("Arial", 12))
person_label.pack()

chair_green = ImageTk.PhotoImage(Image.open("/home/pi/Downloads/rojo.png"))
chair_red = ImageTk.PhotoImage(Image.open("/home/pi/Downloads/verde.png"))

chair_labels = {}
chair_positions = {"C": (146, 235), "1": (218, 225), "3": (218, 335), "2": (146, 335)}
for source_id, (x, y) in chair_positions.items():
    label = tk.Label(frame_image, image=chair_red)
    label.place(x=x, y=y)
    chair_labels[source_id] = label

def update_table(data):
    table.insert("", "end", values=data)
    table.yview_moveto(1)
    source_id = data[1]
    if source_id in chair_labels:
        try:
            status_value = float(data[2].split(',')[1])
            if status_value == 1:
                chair_labels[source_id].config(image=chair_green)
            else:
                chair_labels[source_id].config(image=chair_red)
        except Exception:
            pass

def read_serial():
    global latest_gps_str, latest_gps_values
    while True:
        if ser_lora.in_waiting:
            msg = ser_lora.readline().decode().strip()
            if msg.startswith("+RCV="):
                parts = msg.split(',')
                if len(parts) >= 4:
                    rcv_id = parts[0][5:].strip()
                    msg_length = parts[1]
                    data = ','.join(parts[2:])
                    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                    print(f"LoRa Data: {rcv_id}, {msg_length}, {data}, {timestamp}")

                    with open("registrosillas.json", "a") as json_file:
                        json_file.write(f'{{"Timestamp": "{timestamp}", "Source": "{rcv_id}", "Length": "{msg_length}", "Data": "{data}"}}\n')

                    if rcv_id in chair_labels:
                        mensaje_uart = f"{rcv_id},{msg_length},{data},{latest_gps_values},{person_count}\n"
                    else:
                        mensaje_uart = f"{rcv_id},{msg_length},{data},{timestamp},{person_count}\n"
                    ser_arduino.write(mensaje_uart.encode())

                    tk_root.after(0, update_table, (timestamp, rcv_id, data))
                    if rcv_id in chair_labels:
                        tk_root.after(0, update_table, (timestamp, "GPS", latest_gps_str))
        time.sleep(0.5)

def read_gps():
    global latest_gps_str, latest_gps_values, latest_speed
    ser_gps = serial.Serial('/dev/ttyAMA4', 9600, timeout=1)
    sio = io.TextIOWrapper(io.BufferedRWPair(ser_gps, ser_gps))
    lat = lon = alt = speed = course = None
    last_update = time.time()

    while True:
        try:
            line = sio.readline()
            if not line.startswith("$"):
                continue
            try:
                msg = pynmea2.parse(line)
            except pynmea2.ParseError:
                continue
            if msg.sentence_type == 'RMC' and msg.status == 'A':
                lat = msg.latitude
                lon = msg.longitude
                speed = float(msg.spd_over_grnd) * 1.852 if msg.spd_over_grnd else None
            elif msg.sentence_type == 'GGA':
                alt = float(msg.altitude) if msg.altitude else None
            elif msg.sentence_type == 'VTG':
                course = float(msg.true_track) if msg.true_track else None
            if time.time() - last_update >= 1:
                dirs = ["N", "NE", "E", "SE", "S", "SW", "W", "NW"]
                cardinal = dirs[int(round((course or 0) / 45)) % 8] if course is not None else "N/A"
                lat_str = f"{lat:.3f}" if lat else "N/A"
                lon_str = f"{lon:.3f}" if lon else "N/A"
                alt_str = f"{alt:.2f}" if alt else "N/A"
                speed_str = f"{speed:.2f}" if speed else "0.00"
                latest_gps_str = f"Lat: {lat_str}, Lon: {lon_str}, Alt: {alt_str} km, Speed: {speed_str} km/h, Card: {cardinal}"
                latest_gps_values = f"{lat_str},{lon_str},{alt_str},{speed_str}"
                latest_speed = f"{speed_str} km/h"
                print("GPS Updated:", latest_gps_str)
                tk_root.after(0, update_speed_display)
                last_update = time.time()
        except Exception as e:
            print("GPS Error:", e)
            continue

def read_pir():
    global person_count, last_trigger
    while True:
        sensor_a = GPIO.input(PIR_A)
        sensor_b = GPIO.input(PIR_B)

        if sensor_a and last_trigger is None:
            last_trigger = 'A'
            time.sleep(0.5)
        elif sensor_b and last_trigger is None:
            last_trigger = 'B'
            time.sleep(0.5)
        elif sensor_b and last_trigger == 'A':
            person_count += 1
            last_trigger = None
            tk_root.after(0, update_person_count)
            time.sleep(1)
        elif sensor_a and last_trigger == 'B':
            person_count = max(0, person_count - 1)
            last_trigger = None
            tk_root.after(0, update_person_count)
            time.sleep(1)
        time.sleep(0.1)

def update_speed_display():
    speed_var.set(f"Aprox: {latest_speed}")

def update_person_count():
    person_var.set(f"Pasajeros a bordo: {person_count}")

def start_serial_thread():
    threading.Thread(target=read_serial, daemon=True).start()

def start_gps_thread():
    threading.Thread(target=read_gps, daemon=True).start()

def start_pir_thread():
    threading.Thread(target=read_pir, daemon=True).start()

start_serial_thread()
start_gps_thread()
start_pir_thread()

tk_root.mainloop()
GPIO.cleanup()
