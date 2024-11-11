import tkinter as tk
from tkinter import messagebox, simpledialog
import serial
import time
import threading

# Global variable for the serial connection
ser = None

def initialize_serial(port):
    global ser
    try:
        ser = serial.Serial(port, 115200)  # Change 'COM5' to your port
        time.sleep(2)  # Wait for the serial connection to initialize
        initialize_sensor_states()
    except serial.SerialException as e:
        messagebox.showerror("Serial Error", f"Could not open serial port: {e}")



def read_from_eeprom():
    if ser is None:
        messagebox.showerror("Error", "Serial connection not established.")
        return

    text_area.delete('1.0', tk.END)

    ser.write(b'R')
    time.sleep(0.2)  # Allow Arduino time to respond
    eeprom_data = []

    try:
        while True:
            if ser.in_waiting:
                line = ser.readline().decode('utf-8').strip()
                # Debug print removed
                if line == "END":  # Check for end marker
                    break
                eeprom_data.append(line)

                if line == "No valid data in EEPROM to read.":
                    messagebox.showinfo("EEPROM Data", "No valid data in EEPROM to read.")
                    break

        if not eeprom_data:
            messagebox.showinfo("EEPROM Data", "No data received.")
            return

        display_data(eeprom_data)

    except Exception as e:
        messagebox.showerror("Error", f"An error occurred: {e}")

    finally:
        # Close the loading window once data reading is complete
        if loading_window:
            loading_window.destroy()



def display_data(data):
    for line in data:
        try:
            address, value = line.split(': ')
            formatted_data = f"{address.strip().split()[-1]},{value.strip()}\n"
            text_area.insert(tk.END, formatted_data)
        except ValueError:
            messagebox.showwarning("Data Format Error", f"Received invalid data: {line}")



def toggle_sensor(sensor):
    if ser is None:
        return

    ser.write(sensor.encode())
    time.sleep(0.1)  # Allow time for Arduino to process



def initialize_sensor_states():
    if ser is None:
        return

    ser.write(b'S')  # Command to read current sensor states
    time.sleep(0.1)
    states = ser.readline().decode('utf-8').strip()

    if states:
        try:
            sensor_states = [int(bit) for bit in states.split()]  # Split by space
            sensor1_var.set(sensor_states[0])
            sensor2_var.set(sensor_states[1])
            sensor3_var.set(sensor_states[2])
            sensor4_var.set(sensor_states[3])
        except ValueError:
            messagebox.showwarning("Sensor State Error", f"Received invalid sensor states: {states}")



def on_serial_select():
    port = serial_port_entry.get()
    if not port:
        messagebox.showwarning("Input Error", "Please enter a serial port.")
        return
    initialize_serial(port)



def read_from_eeprom_thread():
    global loading_window
    loading_window = tk.Toplevel(root)
    loading_window.title("Loading")
    loading_window.geometry("200x100")

    loading_label = tk.Label(loading_window, text="Loading...", font=("Arial", 14))
    loading_label.pack(pady=20)

    threading.Thread(target=read_from_eeprom).start()



def clear_text_area():
    text_area.delete('1.0', tk.END)
    

# Create the main window
root = tk.Tk()
root.title("Data Logger")
root.geometry("400x500")  # Set window size

# Frame for serial port selection
port_frame = tk.Frame(root)
port_frame.pack(pady=10)

serial_port_label = tk.Label(port_frame, text="Serial Port:")
serial_port_label.pack(side=tk.LEFT, padx=5)

serial_port_entry = tk.Entry(port_frame)
serial_port_entry.pack(side=tk.LEFT, padx=5)

connect_button = tk.Button(port_frame, text="Connect", command=on_serial_select)
connect_button.pack(side=tk.LEFT, padx=5)

# Frame for buttons and checkboxes
frame = tk.Frame(root)
frame.pack(pady=10)

# Create a button to read data from EEPROM
read_button = tk.Button(frame, text="Read Data", command=read_from_eeprom_thread)
read_button.grid(row=0, column=0, padx=10, pady=5)

# Create a button to clear the data display
clear_button = tk.Button(frame, text="Clear Data", command=clear_text_area)
clear_button.grid(row=2, column=0, padx=10, pady=5)

# Create checkboxes for selecting sensors
sensor1_var = tk.IntVar()
sensor2_var = tk.IntVar()
sensor3_var = tk.IntVar()
sensor4_var = tk.IntVar()

sensor1_checkbox = tk.Checkbutton(frame, text="Sensor 1", variable=sensor1_var, command=lambda: toggle_sensor('1'))
sensor1_checkbox.grid(row=1, column=0, padx=10, pady=5)

sensor2_checkbox = tk.Checkbutton(frame, text="Sensor 2", variable=sensor2_var, command=lambda: toggle_sensor('2'))
sensor2_checkbox.grid(row=1, column=1, padx=10, pady=5)

sensor3_checkbox = tk.Checkbutton(frame, text="Sensor 3", variable=sensor3_var, command=lambda: toggle_sensor('3'))
sensor3_checkbox.grid(row=1, column=2, padx=10, pady=5)

sensor4_checkbox = tk.Checkbutton(frame, text="Sensor 4", variable=sensor4_var, command=lambda: toggle_sensor('4'))
sensor4_checkbox.grid(row=1, column=3, padx=10, pady=5)

# Create a text area to display the EEPROM data
text_area = tk.Text(root, height=20, width=50)
text_area.pack(pady=10)

# Initialize loading window variable
loading_window = None

# Start the Tkinter main loop
root.mainloop()



