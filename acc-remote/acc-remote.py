#!/usr/bin/env python3

import sys
import math
import tkinter as tk
from tkinter import ttk

try:
    from accvis import AccPanelFan, AccPanelMode
    from accvis import AccParsedPanel
except ImportError:
    print("Install accvis by changing to the acc-machvis directory and\
           typing `python3 setup.py install`")
    sys.exit(1)

try:
    import paho.mqtt.publish as publish
except ImportError:
    print("Install paho-mqtt by typing `pip install paho-mqtt`")
    sys.exit(1)

tempdefault = 75
tempmin = 64
tempmax = 86
mqtthost = "mosquitto.int.ivanveloz.com"
mqtttopic = "ac-cloudifier-cmd"

# Create the main application window
root = tk.Tk()
root.title("acc-remote")
root.geometry("300x400")

def temp_to_digits(temp: int):
    if(0 > temp > 99):
        print("Temperature out of range, skipping")
        return (0,0)
    msd = math.floor(temp/10)
    lsd = temp % 10
    return (msd, lsd)

# Function to show selected values
def publish_selected():
    fan_value = AccPanelFan(int(fan_var.get()))
    mode_value = AccPanelMode(int(mode_var.get()))
    try:
        temperature_value = int(temperature_entry.get())
    except ValueError:
        temperature_entry.set(tempdefault)
        return
    if(temperature_value < tempmin):
        temperature_value = tempmin
        temperature_entry.set(tempmin)
    elif(temperature_value > tempmax):
        temperature_value = tempmax
        temperature_entry.set(tempmax)
    (temperature_msd, temperature_lsd) = temp_to_digits(temperature_value)
    
    panel = AccParsedPanel(
        fan=fan_value, 
        mode=mode_value, 
        msdigit=temperature_msd,
        lsdigit=temperature_lsd,
        filterbad=False
    )
    command = str(panel)
    result_label.config(text=command)
    publish.single(hostname=mqtthost, topic=mqtttopic, payload=command)

# Create radio buttons for AccPanelFan
fan_var = tk.StringVar(value=AccPanelFan.FAN_NONE.value)
fan_label = ttk.Label(root, text="Fan Setting")
fan_label.pack(pady=5)

for fan in AccPanelFan:
    tk.Radiobutton(root, text=fan.name, variable=fan_var, value=fan.value).pack(anchor=tk.W)

# Create radio buttons for AccPanelMode
mode_var = tk.StringVar(value=AccPanelMode.MODE_NONE.value)
mode_label = ttk.Label(root, text="Mode Setting")
mode_label.pack(pady=5)

for mode in AccPanelMode:
    tk.Radiobutton(root, text=mode.name, variable=mode_var, value=mode.value).pack(anchor=tk.W)

# Temperature entry field
temperature_label = ttk.Label(root, text="Enter Temperature")
temperature_label.pack(pady=5)
temperature_entry = ttk.Spinbox(root, from_=tempmin, to=tempmax, increment=1)
temperature_entry.set(tempdefault)
temperature_entry.pack(pady=5)

# Button to show selected values
publish_button = ttk.Button(root, text="Send command", command=publish_selected)
publish_button.pack(pady=10)

# Label to display the selection
result_label = ttk.Label(root, text="")
result_label.pack(pady=5)

# Start the GUI event loop
root.mainloop()
