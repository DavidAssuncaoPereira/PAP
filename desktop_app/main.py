import customtkinter as ctk
import requests
import threading
import time
import re

# ESP32 configuration
ESP32_IP = "192.168.4.1"
URL = f"http://{ESP32_IP}/"

class PlantMonitorApp(ctk.CTk):
    def __init__(self):
        super().__init__()

        self.title("Monitor de Plantas")
        self.geometry("400x380")
        self.resizable(False, False)

        # Configure theme
        ctk.set_appearance_mode("System")  # Modes: "System" (standard), "Dark", "Light"
        ctk.set_default_color_theme("green")  # Themes: "blue" (standard), "green", "dark-blue"

        # UI Elements
        self.title_label = ctk.CTkLabel(self, text="Estação de Monitorização", font=ctk.CTkFont(size=20, weight="bold"))
        self.title_label.pack(pady=20)

        self.temp_frame = ctk.CTkFrame(self)
        self.temp_frame.pack(pady=10, padx=20, fill="x")
        self.temp_label = ctk.CTkLabel(self.temp_frame, text="Temperatura: -- °C", font=ctk.CTkFont(size=16))
        self.temp_label.pack(pady=10)

        self.light_frame = ctk.CTkFrame(self)
        self.light_frame.pack(pady=10, padx=20, fill="x")
        self.light_label = ctk.CTkLabel(self.light_frame, text="Luminosidade: -- lx", font=ctk.CTkFont(size=16))
        self.light_label.pack(pady=10)

        self.hum_frame = ctk.CTkFrame(self)
        self.hum_frame.pack(pady=10, padx=20, fill="x")
        self.hum_label = ctk.CTkLabel(self.hum_frame, text="Humidade Solo: -- %", font=ctk.CTkFont(size=16))
        self.hum_label.pack(pady=10)

        self.status_label = ctk.CTkLabel(self, text="A aguardar dados...", text_color="gray", font=ctk.CTkFont(size=12))
        self.status_label.pack(side="bottom", pady=10)

        self.is_running = True

        # Start data fetching thread
        self.fetch_thread = threading.Thread(target=self.update_data_loop, daemon=True)
        self.fetch_thread.start()

    def fetch_data(self):
        try:
            # We add a short timeout so the UI doesn't hang forever if the ESP32 is offline
            response = requests.get(URL, timeout=3)
            response.raise_for_status()
            response.encoding = 'utf-8'
            html = response.text

            # The ESP32 returns HTML, we need to extract the values using regex
            temp_match = re.search(r"Temperatura: <strong>([-0-9.]+) °C</strong>", html)
            light_match = re.search(r"Luminosidade: <strong>([0-9.]+) lx</strong>", html)
            hum_match = re.search(r"Humidade: <strong>([0-9.]+) %</strong>", html)

            if temp_match and light_match and hum_match:
                return {
                    "temp": temp_match.group(1),
                    "light": light_match.group(1),
                    "hum": hum_match.group(1),
                    "status": "Conectado"
                }
            else:
                return {"error": "Não foi possível extrair dados do HTML."}

        except requests.exceptions.RequestException as e:
            return {"error": f"Erro de ligação: {e}"}

    def update_ui(self, data):
        if "error" in data:
            self.status_label.configure(text=data["error"], text_color="red")
        else:
            self.temp_label.configure(text=f"Temperatura: {data['temp']} °C")
            self.light_label.configure(text=f"Luminosidade: {data['light']} lx")
            self.hum_label.configure(text=f"Humidade Solo: {data['hum']} %")
            self.status_label.configure(text=f"Status: {data['status']}", text_color="green")

    def update_data_loop(self):
        while self.is_running:
            data = self.fetch_data()
            # Schedule the UI update in the main thread
            self.after(0, self.update_ui, data)
            time.sleep(5) # Wait 5 seconds before fetching again

if __name__ == "__main__":
    app = PlantMonitorApp()
    app.mainloop()