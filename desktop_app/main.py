import customtkinter as ctk
import requests
import threading
import time
import re
from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg

# ESP32 configuration
ESP32_IP = "192.168.4.1"
URL = f"http://{ESP32_IP}/"

class PlantMonitorApp(ctk.CTk):
    def __init__(self):
        super().__init__()

        self.title("Monitor de Plantas")
        self.geometry("800x450")
        self.resizable(False, False)

        # Configure theme
        ctk.set_appearance_mode("System")  # Modes: "System" (standard), "Dark", "Light"
        ctk.set_default_color_theme("green")  # Themes: "blue" (standard), "green", "dark-blue"

        # Layout Frames
        self.left_frame = ctk.CTkFrame(self, fg_color="transparent")
        self.left_frame.pack(side="left", fill="y", padx=20, pady=20)

        self.right_frame = ctk.CTkFrame(self, fg_color="transparent")
        self.right_frame.pack(side="right", fill="both", expand=True, padx=20, pady=20)

        # UI Elements - Left
        self.title_label = ctk.CTkLabel(self.left_frame, text="Estação de Monitorização", font=ctk.CTkFont(size=20, weight="bold"))
        self.title_label.pack(pady=(0, 20))

        self.temp_frame = ctk.CTkFrame(self.left_frame)
        self.temp_frame.pack(pady=10, fill="x")
        self.temp_label = ctk.CTkLabel(self.temp_frame, text="Temperatura: -- °C", font=ctk.CTkFont(size=16))
        self.temp_label.pack(pady=10)

        self.light_frame = ctk.CTkFrame(self.left_frame)
        self.light_frame.pack(pady=10, fill="x")
        self.light_label = ctk.CTkLabel(self.light_frame, text="Luminosidade: -- lx", font=ctk.CTkFont(size=16))
        self.light_label.pack(pady=10)

        self.hum_frame = ctk.CTkFrame(self.left_frame)
        self.hum_frame.pack(pady=10, fill="x")
        self.hum_label = ctk.CTkLabel(self.hum_frame, text="Humidade Solo: -- %", font=ctk.CTkFont(size=16))
        self.hum_label.pack(pady=10)

        self.status_label = ctk.CTkLabel(self.left_frame, text="A aguardar dados...", text_color="gray", font=ctk.CTkFont(size=12))
        self.status_label.pack(side="bottom", pady=10)

        # UI Elements - Right (Graph)
        self.fig = Figure(figsize=(5, 4), dpi=100)
        self.ax = self.fig.add_subplot(111)
        self.ax.set_title("Histórico")
        self.canvas = FigureCanvasTkAgg(self.fig, master=self.right_frame)
        self.canvas.get_tk_widget().pack(fill="both", expand=True)

        self.is_running = True

        # Start data fetching thread
        self.fetch_thread = threading.Thread(target=self.update_data_loop, daemon=True)
        self.fetch_thread.start()

    def fetch_data(self):
        result = {}
        try:
            # Fetch live data
            response = requests.get(URL, timeout=3)
            response.raise_for_status()
            response.encoding = 'utf-8'
            html = response.text

            temp_match = re.search(r"Temperatura: <strong>([-0-9.]+) °C</strong>", html)
            light_match = re.search(r"Luminosidade: <strong>([0-9.]+) lx</strong>", html)
            hum_match = re.search(r"Humidade: <strong>([0-9.]+) %</strong>", html)

            if temp_match and light_match and hum_match:
                result["live"] = {
                    "temp": temp_match.group(1),
                    "light": light_match.group(1),
                    "hum": hum_match.group(1),
                    "status": "Conectado"
                }
            else:
                result["error"] = "Não foi possível extrair dados do HTML."
        except requests.exceptions.RequestException as e:
            result["error"] = f"Erro de ligação: {e}"

        try:
            # Fetch history
            hist_response = requests.get(f"{URL}history", timeout=3)
            if hist_response.status_code == 200:
                result["history"] = hist_response.json()
        except Exception:
            pass # Ignore history fetch errors so it doesn't block live data

        return result

    def update_ui(self, data):
        if "error" in data:
            self.status_label.configure(text=data["error"], text_color="red")
        elif "live" in data:
            live = data["live"]
            self.temp_label.configure(text=f"Temperatura: {live['temp']} °C")
            self.light_label.configure(text=f"Luminosidade: {live['light']} lx")
            self.hum_label.configure(text=f"Humidade Solo: {live['hum']} %")
            self.status_label.configure(text=f"Status: {live['status']}", text_color="green")

        if "history" in data and data["history"]:
            hist = data["history"]
            temps = [d["temp"] for d in hist]
            hums = [d["hum"] for d in hist]

            self.ax.clear()
            self.ax.set_title("Histórico")
            self.ax.plot(temps, label="Temp (°C)", color="red")
            self.ax.plot(hums, label="Humidade (%)", color="blue")
            self.ax.legend()
            self.canvas.draw()

    def update_data_loop(self):
        while self.is_running:
            data = self.fetch_data()
            # Schedule the UI update in the main thread
            self.after(0, self.update_ui, data)
            time.sleep(5) # Wait 5 seconds before fetching again

if __name__ == "__main__":
    app = PlantMonitorApp()
    app.mainloop()