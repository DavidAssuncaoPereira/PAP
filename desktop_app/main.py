import customtkinter as ctk
import requests
import threading
import time
import re
from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from plyer import notification
import tkinter as tk

class Tooltip:
    """
    Classe para criar uma Tooltip moderna numa aplicação customtkinter (baseada em Tkinter).
    Quando o rato passa por cima de um widget, uma pequena janela com texto explicativo é mostrada.
    """
    def __init__(self, widget, text):
        self.widget = widget
        self.text = text
        self.tooltip_window = None
        self.widget.bind("<Enter>", self.show_tooltip)
        self.widget.bind("<Leave>", self.hide_tooltip)

    def show_tooltip(self, event=None):
        x = self.widget.winfo_rootx() + 25
        y = self.widget.winfo_rooty() + 25

        self.tooltip_window = tw = tk.Toplevel(self.widget)
        tw.wm_overrideredirect(True)
        tw.wm_geometry(f"+{x}+{y}")

        # Estilo moderno para a tooltip
        label = tk.Label(tw, text=self.text, justify='left',
                         background="#2b2b2b", foreground="#ffffff",
                         relief='solid', borderwidth=1,
                         font=("Arial", 10, "normal"), padx=8, pady=4)
        label.pack(ipadx=1)

    def hide_tooltip(self, event=None):
        if self.tooltip_window:
            self.tooltip_window.destroy()
        self.tooltip_window = None

# ESP32 configuration
ESP32_IP = "192.168.4.1"
URL = f"http://{ESP32_IP}/"

class PlantMonitorApp(ctk.CTk):
    def __init__(self):
        super().__init__()

        self.title("Monitor de Plantas")
        self.geometry("800x450")
        self.resizable(False, False)

        try:
            from PIL import Image, ImageTk
            import os
            # Configurar o ícone da janela
            icon_path = os.path.join(os.path.dirname(__file__), "assets", "app_icon.png")
            if os.path.exists(icon_path):
                img = Image.open(icon_path)
                photo = ImageTk.PhotoImage(img)
                self.wm_iconphoto(True, photo)
        except Exception as e:
            print(f"Aviso: Não foi possível carregar o ícone: {e}")

        # Configure theme
        ctk.set_appearance_mode("System")
        ctk.set_default_color_theme("green")

        self.is_running = True
        self.fetch_thread = None

        self.show_login()

    def show_login(self):
        # Card moderno com padding e cantos arredondados
        self.login_frame = ctk.CTkFrame(self, corner_radius=15, fg_color=("gray85", "gray20"))
        self.login_frame.place(relx=0.5, rely=0.5, anchor="center")

        # Título
        self.login_title = ctk.CTkLabel(self.login_frame, text="Monitor de Plantas", font=ctk.CTkFont(size=24, weight="bold"))
        self.login_title.pack(pady=(30, 10), padx=40)

        # Subtítulo explicativo
        self.login_subtitle = ctk.CTkLabel(self.login_frame, text="Por favor, introduza a sua senha.", font=ctk.CTkFont(size=12), text_color="gray")
        self.login_subtitle.pack(pady=(0, 20), padx=40)

        # Entrada de senha
        self.password_entry = ctk.CTkEntry(self.login_frame, placeholder_text="Senha", show="*", width=200, height=35)
        self.password_entry.pack(pady=10, padx=40)
        self.password_entry.bind("<Return>", lambda e: self.check_login())

        # Botão de Login (estilo premium)
        self.login_button = ctk.CTkButton(
            self.login_frame,
            text="Entrar",
            command=self.check_login,
            width=200, height=35,
            corner_radius=8,
            fg_color="#1f6aa5",
            hover_color="#144870"
        )
        self.login_button.pack(pady=(10, 10), padx=40)

        self.login_error_label = ctk.CTkLabel(self.login_frame, text="", text_color="red", font=ctk.CTkFont(size=12))
        self.login_error_label.pack(pady=(0, 20))

    def check_login(self):
        password = self.password_entry.get()
        if password == "admin":  # Default password
            self.login_error_label.configure(text="")
            # Limpar UI de login
            self.login_frame.destroy()
            self.build_main_ui()
        else:
            self.login_error_label.configure(text="Senha incorreta! Tente novamente.")
            # Efeito visual de tremor (pequena UX melhoria) poderia ser feito aqui, mas fiquemos simples por agora.

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

        # Sensor Cards (Modern design)
        card_fg_color = ("gray85", "gray25")

        self.temp_frame = ctk.CTkFrame(self.left_frame, corner_radius=10, fg_color=card_fg_color)
        self.temp_frame.pack(pady=10, fill="x", ipadx=10, ipady=5)
        self.temp_label = ctk.CTkLabel(self.temp_frame, text="Temperatura\n-- °C", font=ctk.CTkFont(size=16, weight="bold"))
        self.temp_label.pack(pady=10, padx=20)

        self.light_frame = ctk.CTkFrame(self.left_frame, corner_radius=10, fg_color=card_fg_color)
        self.light_frame.pack(pady=10, fill="x", ipadx=10, ipady=5)
        self.light_label = ctk.CTkLabel(self.light_frame, text="Luminosidade\n-- lx", font=ctk.CTkFont(size=16, weight="bold"))
        self.light_label.pack(pady=10, padx=20)

        self.hum_frame = ctk.CTkFrame(self.left_frame, corner_radius=10, fg_color=card_fg_color)
        self.hum_frame.pack(pady=10, fill="x", ipadx=10, ipady=5)
        self.hum_label = ctk.CTkLabel(self.hum_frame, text="Humidade Solo\n-- %", font=ctk.CTkFont(size=16, weight="bold"))
        self.hum_label.pack(pady=10, padx=20)

        self.bomba_frame = ctk.CTkFrame(self.left_frame, corner_radius=10, fg_color=card_fg_color)
        self.bomba_frame.pack(pady=10, fill="x", ipadx=10, ipady=5)
        self.bomba_label = ctk.CTkLabel(self.bomba_frame, text="Bomba de Água\n--", font=ctk.CTkFont(size=16, weight="bold"))
        self.bomba_label.pack(pady=10, padx=20)

        # Status and Loading Indicator
        self.status_frame = ctk.CTkFrame(self.left_frame, fg_color="transparent")
        self.status_frame.pack(side="bottom", pady=10, fill="x")

        self.status_label = ctk.CTkLabel(self.status_frame, text="A aguardar dados...", text_color="gray", font=ctk.CTkFont(size=12))
        self.status_label.pack()

        self.progress_bar = ctk.CTkProgressBar(self.status_frame, width=150, mode="indeterminate")
        self.progress_bar.pack(pady=5)
        self.progress_bar.start()

        # UI Elements - Right (Graph)
        # Configurar estilo moderno para o Matplotlib
        self.fig = Figure(figsize=(5, 4), dpi=100, facecolor="#2b2b2b")
        self.ax = self.fig.add_subplot(111)
        self.ax.set_facecolor("#2b2b2b")
        self.ax.tick_params(colors="white")
        for spine in self.ax.spines.values():
            spine.set_edgecolor('gray')

        self.ax.set_title("Histórico de Monitorização", color="white", pad=15)
        self.canvas = FigureCanvasTkAgg(self.fig, master=self.right_frame)
        self.canvas.get_tk_widget().pack(fill="both", expand=True)

    def build_settings_tab(self):
        # Settings Variables
        self.notifications_enabled = ctk.BooleanVar(value=True)
        self.max_temp_var = ctk.StringVar(value="30")
        self.min_hum_var = ctk.StringVar(value="30")
        self.limite_bomba_var = ctk.StringVar(value="30")
        self.limite_bomba_temp_var = ctk.StringVar(value="30")

        self.settings_title = ctk.CTkLabel(self.tab_settings, text="Configurações de Notificação", font=ctk.CTkFont(size=18, weight="bold"))
        self.settings_title.pack(pady=20)

        self.enable_notif_switch = ctk.CTkSwitch(self.tab_settings, text="Ativar Notificações", variable=self.notifications_enabled)
        self.enable_notif_switch.pack(pady=10)

        self.limits_frame = ctk.CTkFrame(self.tab_settings, fg_color="transparent")
        self.limits_frame.pack(pady=20)

        lbl_max_temp = ctk.CTkLabel(self.limits_frame, text="Temperatura Máxima (°C):")
        lbl_max_temp.grid(row=0, column=0, padx=10, pady=10)
        Tooltip(lbl_max_temp, "Acima deste valor receberá uma notificação de perigo.")

        self.max_temp_entry = ctk.CTkEntry(self.limits_frame, textvariable=self.max_temp_var, width=60)
        self.max_temp_entry.grid(row=0, column=1, padx=10, pady=10)

        lbl_min_hum = ctk.CTkLabel(self.limits_frame, text="Humidade Mínima (%):")
        lbl_min_hum.grid(row=1, column=0, padx=10, pady=10)
        Tooltip(lbl_min_hum, "Abaixo deste valor será alertado para regar a planta.")

        self.min_hum_entry = ctk.CTkEntry(self.limits_frame, textvariable=self.min_hum_var, width=60)
        self.min_hum_entry.grid(row=1, column=1, padx=10, pady=10)

        lbl_limite_bomba = ctk.CTkLabel(self.limits_frame, text="Limite Bomba (%):")
        lbl_limite_bomba.grid(row=2, column=0, padx=10, pady=10)
        Tooltip(lbl_limite_bomba, "Abaixo deste valor a bomba será acionada automaticamente.")

        self.limite_bomba_entry = ctk.CTkEntry(self.limits_frame, textvariable=self.limite_bomba_var, width=60)
        self.limite_bomba_entry.grid(row=2, column=1, padx=10, pady=10)

        lbl_limite_bomba_temp = ctk.CTkLabel(self.limits_frame, text="Lim. Temp. Bomba (°C):")
        lbl_limite_bomba_temp.grid(row=3, column=0, padx=10, pady=10)
        Tooltip(lbl_limite_bomba_temp, "Acima deste valor a bomba será acionada automaticamente.")

        self.limite_bomba_temp_entry = ctk.CTkEntry(self.limits_frame, textvariable=self.limite_bomba_temp_var, width=60)
        self.limite_bomba_temp_entry.grid(row=3, column=1, padx=10, pady=10)

        self.save_button = ctk.CTkButton(self.tab_settings, text="Guardar Configurações", command=self.save_settings)
        self.save_button.pack(pady=20)

        self.save_label = ctk.CTkLabel(self.tab_settings, text="", text_color="green")
        self.save_label.pack()

    def save_settings(self):
        try:
            float(self.max_temp_var.get())
            float(self.min_hum_var.get())

            lim_bomba = int(self.limite_bomba_var.get())
            if lim_bomba < 0 or lim_bomba > 100:
                raise ValueError("Limite da bomba deve ser entre 0 e 100")

            lim_bomba_temp = int(self.limite_bomba_temp_var.get())
            if lim_bomba_temp < 0 or lim_bomba_temp > 100:
                raise ValueError("Limite da bomba temp deve ser entre 0 e 100")

            import threading
            def update_bomba():
                try:
                    requests.get(f"{URL}config?limite={lim_bomba}&limite_temp={lim_bomba_temp}", timeout=3)
                except Exception:
                    pass
            threading.Thread(target=update_bomba, daemon=True).start()

            self.save_label.configure(text="Configurações guardadas com sucesso!", text_color="green")
        except ValueError as e:
            self.save_label.configure(text=f"Erro: Valores inválidos. {str(e)}", text_color="red")

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
            bomba_estado_match = re.search(r"Bomba de Água <strong><span[^>]*>([A-Z]+)", html)
            bomba_lim_match = re.search(r"name='limite'.*?value='([0-9]+)'", html)
            bomba_lim_temp_match = re.search(r"name='limite_temp'.*?value='([0-9]+)'", html)

            if temp_match and light_match and hum_match:
                b_estado = bomba_estado_match.group(1) if bomba_estado_match else "--"
                b_lim = bomba_lim_match.group(1) if bomba_lim_match else None
                b_lim_temp = bomba_lim_temp_match.group(1) if bomba_lim_temp_match else None

                result["live"] = {
                    "temp": temp_match.group(1),
                    "light": light_match.group(1),
                    "hum": hum_match.group(1),
                    "bomba_estado": b_estado,
                    "bomba_lim": b_lim,
                    "bomba_lim_temp": b_lim_temp,
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
                self.status_label.configure(text=f"Erro: {data['error'][:30]}...", text_color="red")
                if hasattr(self, 'progress_bar') and self.progress_bar.winfo_ismapped():
                    self.progress_bar.stop()
                    self.progress_bar.pack_forget()
        elif "live" in data:
            live = data["live"]
            if hasattr(self, 'temp_label'):
                self.temp_label.configure(text=f"Temperatura\n{live['temp']} °C")
                self.light_label.configure(text=f"Luminosidade\n{live['light']} lx")
                self.hum_label.configure(text=f"Humidade Solo\n{live['hum']} %")
                if hasattr(self, 'bomba_label'):
                    self.bomba_label.configure(text=f"Bomba de Água\n{live.get('bomba_estado', '--')}")

                if live.get("bomba_lim") is not None:
                    # Só atualizamos se a caixa de texto não tiver foco para não incomodar a edição do utilizador
                    if str(self.focus_get()) != str(self.limite_bomba_entry):
                        self.limite_bomba_var.set(live["bomba_lim"])

                if live.get("bomba_lim_temp") is not None:
                    if str(self.focus_get()) != str(self.limite_bomba_temp_entry):
                        self.limite_bomba_temp_var.set(live["bomba_lim_temp"])

                self.status_label.configure(text=f"Status: {live['status']}", text_color="green")
                if hasattr(self, 'progress_bar') and self.progress_bar.winfo_ismapped():
                    self.progress_bar.stop()
                    self.progress_bar.pack_forget()

            self.check_alarms(live)

        if "history" in data and data["history"]:
            hist = data["history"]
            temps = [d["temp"] for d in hist]
            hums = [d["hum"] for d in hist]

            if hasattr(self, 'ax'):
                self.ax.clear()
                self.ax.set_title("Histórico de Monitorização", color="white", pad=15)
                # Estilo das linhas mais apelativo e grelha para facilitar leitura
                self.ax.plot(temps, label="Temp (°C)", color="#ff5555", linewidth=2, marker='o', markersize=4)
                self.ax.plot(hums, label="Humidade (%)", color="#55aaff", linewidth=2, marker='o', markersize=4)
                self.ax.grid(True, linestyle='--', alpha=0.5, color='gray')

                # Configurar leganda com fundo escuro e texto branco para não chocar com o dark theme
                legend = self.ax.legend(facecolor="#2b2b2b", edgecolor="gray")
                for text in legend.get_texts():
                    text.set_color("white")

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
                def send_notif():
                    notification.notify(
                        title="Alerta Monitor de Plantas",
                        message="\n".join(alarm_messages),
                        app_name="Monitor de Plantas",
                        timeout=5
                    )
                threading.Thread(target=send_notif, daemon=True).start()

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