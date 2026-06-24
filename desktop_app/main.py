import customtkinter as ctk
import requests
import threading
import time
import re
from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from plyer import notification

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
        ctk.set_appearance_mode("System")
        ctk.set_default_color_theme("green")

        self.is_running = True
        self.fetch_thread = None

        self.show_login()

    def show_login(self):
        self.login_frame = ctk.CTkFrame(self)
        self.login_frame.place(relx=0.5, rely=0.5, anchor="center")

        self.login_title = ctk.CTkLabel(self.login_frame, text="Login", font=ctk.CTkFont(size=24, weight="bold"))
        self.login_title.pack(pady=(20, 10))

        self.password_entry = ctk.CTkEntry(self.login_frame, placeholder_text="Senha", show="*")
        self.password_entry.pack(pady=10, padx=20)
        self.password_entry.bind("<Return>", lambda e: self.check_login())

        self.login_button = ctk.CTkButton(self.login_frame, text="Entrar", command=self.check_login)
        self.login_button.pack(pady=(10, 20), padx=20)

        self.login_error_label = ctk.CTkLabel(self.login_frame, text="", text_color="red")
        self.login_error_label.pack()

    def check_login(self):
        password = self.password_entry.get()
        if password == "admin":  # Default password
            self.login_frame.destroy()
            self.build_main_ui()
        else:
            self.login_error_label.configure(text="Senha incorreta!")

    def build_main_ui(self):
        # Tabs layout
        self.tabview = ctk.CTkTabview(self)
        self.tabview.pack(fill="both", expand=True, padx=20, pady=20)

        self.tab_dashboard = self.tabview.add("Dashboard")
        self.tab_settings = self.tabview.add("Configurações")

        self.build_dashboard_tab()
        self.build_settings_tab()

        # Last notification timestamps to prevent spam
        self.last_temp_notif_time = 0
        self.last_hum_notif_time = 0

        # Start data fetching thread
        self.fetch_thread = threading.Thread(target=self.update_data_loop, daemon=True)
        self.fetch_thread.start()

    def build_dashboard_tab(self):
        # Dashboard Split
        self.left_frame = ctk.CTkFrame(self.tab_dashboard, fg_color="transparent")
        self.left_frame.pack(side="left", fill="y", padx=10, pady=10)

        self.right_frame = ctk.CTkFrame(self.tab_dashboard, fg_color="transparent")
        self.right_frame.pack(side="right", fill="both", expand=True, padx=10, pady=10)

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

    def build_settings_tab(self):
        # Settings Variables
        self.notifications_enabled = ctk.BooleanVar(value=True)
        self.max_temp_var = ctk.StringVar(value="30")
        self.min_hum_var = ctk.StringVar(value="30")

        self.settings_title = ctk.CTkLabel(self.tab_settings, text="Configurações de Notificação", font=ctk.CTkFont(size=18, weight="bold"))
        self.settings_title.pack(pady=20)

        self.enable_notif_switch = ctk.CTkSwitch(self.tab_settings, text="Ativar Notificações", variable=self.notifications_enabled)
        self.enable_notif_switch.pack(pady=10)

        self.limits_frame = ctk.CTkFrame(self.tab_settings, fg_color="transparent")
        self.limits_frame.pack(pady=20)

        ctk.CTkLabel(self.limits_frame, text="Temperatura Máxima (°C):").grid(row=0, column=0, padx=10, pady=10)
        self.max_temp_entry = ctk.CTkEntry(self.limits_frame, textvariable=self.max_temp_var, width=60)
        self.max_temp_entry.grid(row=0, column=1, padx=10, pady=10)

        ctk.CTkLabel(self.limits_frame, text="Humidade Mínima (%):").grid(row=1, column=0, padx=10, pady=10)
        self.min_hum_entry = ctk.CTkEntry(self.limits_frame, textvariable=self.min_hum_var, width=60)
        self.min_hum_entry.grid(row=1, column=1, padx=10, pady=10)

        self.save_button = ctk.CTkButton(self.tab_settings, text="Guardar Configurações", command=self.save_settings)
        self.save_button.pack(pady=20)

        self.save_label = ctk.CTkLabel(self.tab_settings, text="", text_color="green")
        self.save_label.pack()

    def save_settings(self):
        try:
            float(self.max_temp_var.get())
            float(self.min_hum_var.get())
            self.save_label.configure(text="Configurações guardadas com sucesso!", text_color="green")
        except ValueError:
            self.save_label.configure(text="Erro: Por favor, insira valores numéricos válidos.", text_color="red")

        # Ocultar a mensagem após 3 segundos
        self.after(3000, lambda: self.save_label.configure(text=""))

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
            # Em caso de erro e se a UI do Dashboard ainda não foi criada, ignoramos
            if hasattr(self, 'status_label'):
                self.status_label.configure(text=data["error"], text_color="red")
        elif "live" in data:
            live = data["live"]
            if hasattr(self, 'temp_label'):
                self.temp_label.configure(text=f"Temperatura: {live['temp']} °C")
                self.light_label.configure(text=f"Luminosidade: {live['light']} lx")
                self.hum_label.configure(text=f"Humidade Solo: {live['hum']} %")
                self.status_label.configure(text=f"Status: {live['status']}", text_color="green")

            self.check_alarms(live)

        if "history" in data and data["history"]:
            hist = data["history"]
            temps = [d["temp"] for d in hist]
            hums = [d["hum"] for d in hist]

            if hasattr(self, 'ax'):
                self.ax.clear()
                self.ax.set_title("Histórico")
                self.ax.plot(temps, label="Temp (°C)", color="red")
                self.ax.plot(hums, label="Humidade (%)", color="blue")
                self.ax.legend()
                self.canvas.draw()

    def check_alarms(self, live_data):
        if not hasattr(self, 'notifications_enabled'):
            return

        if not self.notifications_enabled.get():
            return

        try:
            current_temp = float(live_data['temp'])
            current_hum = float(live_data['hum'])
            max_temp = float(self.max_temp_var.get())
            min_hum = float(self.min_hum_var.get())

            current_time = time.time()
            alarm_messages = []

            # Repetir alerta no máximo a cada 5 minutos (300 segundos)
            COOLDOWN = 300

            if current_temp > max_temp:
                if current_time - self.last_temp_notif_time > COOLDOWN:
                    alarm_messages.append(f"Temperatura alta: {current_temp}°C (Máx: {max_temp}°C)")
                    self.last_temp_notif_time = current_time
            else:
                self.last_temp_notif_time = 0 # reset se voltar ao normal

            if current_hum < min_hum:
                if current_time - self.last_hum_notif_time > COOLDOWN:
                    alarm_messages.append(f"Humidade baixa: {current_hum}% (Mín: {min_hum}%)")
                    self.last_hum_notif_time = current_time
            else:
                self.last_hum_notif_time = 0

            if alarm_messages:
                # Disparar notificação (plyer pode bloquear, então corremos numa thread separada se necessário, mas costuma ser rápido no SO)
                notification.notify(
                    title="Alerta Monitor de Plantas",
                    message="\n".join(alarm_messages),
                    app_name="Monitor de Plantas",
                    timeout=5
                )

        except ValueError:
            pass # Ignorar se as conversões falharem (ex: valores inválidos nos settings)

    def update_data_loop(self):
        while self.is_running:
            data = self.fetch_data()
            # Schedule the UI update in the main thread
            self.after(0, self.update_ui, data)
            time.sleep(5) # Wait 5 seconds before fetching again

if __name__ == "__main__":
    app = PlantMonitorApp()
    app.mainloop()