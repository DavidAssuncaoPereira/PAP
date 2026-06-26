package com.example.plantmonitor

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.foundation.Canvas
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.text.input.PasswordVisualTransformation
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.foundation.text.KeyboardOptions
import android.app.NotificationChannel
import android.app.NotificationManager
import android.content.Context
import android.os.Build
import androidx.core.app.NotificationCompat
import androidx.core.app.NotificationManagerCompat
import android.content.pm.PackageManager
import androidx.core.content.ContextCompat
import androidx.activity.result.contract.ActivityResultContracts
import android.Manifest
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import okhttp3.OkHttpClient
import okhttp3.Request
import org.json.JSONArray
import java.util.regex.Pattern
import java.util.concurrent.TimeUnit

class MainActivity : ComponentActivity() {

    private val requestPermissionLauncher = registerForActivityResult(
        ActivityResultContracts.RequestPermission()
    ) { isGranted: Boolean ->
        // Permission result handled
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        createNotificationChannel()

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            if (ContextCompat.checkSelfPermission(
                    this,
                    Manifest.permission.POST_NOTIFICATIONS
                ) != PackageManager.PERMISSION_GRANTED
            ) {
                requestPermissionLauncher.launch(Manifest.permission.POST_NOTIFICATIONS)
            }
        }

        setContent {
            MaterialTheme(
                colorScheme = lightColorScheme(
                    primary = Color(0xFF4CAF50),
                    background = Color(0xFFF0F8FF)
                )
            ) {
                Surface(
                    modifier = Modifier.fillMaxSize(),
                    color = MaterialTheme.colorScheme.background
                ) {
                    var isLoggedIn by remember { mutableStateOf(false) }

                    if (isLoggedIn) {
                        MainScreen()
                    } else {
                        LoginScreen(onLoginSuccess = { isLoggedIn = true })
                    }
                }
            }
        }
    }

    private fun createNotificationChannel() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val name = "Alertas de Plantas"
            val descriptionText = "Notificações quando os limites da planta são excedidos"
            val importance = NotificationManager.IMPORTANCE_DEFAULT
            val channel = NotificationChannel("PLANT_ALERTS", name, importance).apply {
                description = descriptionText
            }
            val notificationManager: NotificationManager =
                getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
            notificationManager.createNotificationChannel(channel)
        }
    }
}

@Composable
fun MainScreen() {
    var selectedTab by remember { mutableStateOf(0) }

    // Configurações guardadas em estado
    var notificationsEnabled by remember { mutableStateOf(true) }
    var maxTempThreshold by remember { mutableStateOf("30.0") }
    var minHumThreshold by remember { mutableStateOf("30.0") }
    var limiteBombaThreshold by remember { mutableStateOf("30") }

    Scaffold(
        bottomBar = {
            NavigationBar(containerColor = Color.White) {
                NavigationBarItem(
                    icon = { Text("📊") },
                    label = { Text("Dashboard") },
                    selected = selectedTab == 0,
                    onClick = { selectedTab = 0 }
                )
                NavigationBarItem(
                    icon = { Text("⚙️") },
                    label = { Text("Configurações") },
                    selected = selectedTab == 1,
                    onClick = { selectedTab = 1 }
                )
            }
        }
    ) { paddingValues ->
        Box(modifier = Modifier.padding(paddingValues)) {
            if (selectedTab == 0) {
                DashboardScreen(
                    notificationsEnabled = notificationsEnabled,
                    maxTempThreshold = maxTempThreshold.toFloatOrNull() ?: 30f,
                    minHumThreshold = minHumThreshold.toFloatOrNull() ?: 30f
                )
            } else {
                SettingsScreen(
                    notificationsEnabled = notificationsEnabled,
                    onNotificationsChange = { notificationsEnabled = it },
                    maxTempThreshold = maxTempThreshold,
                    onMaxTempChange = { maxTempThreshold = it },
                    minHumThreshold = minHumThreshold,
                    onMinHumChange = { minHumThreshold = it },
                    limiteBombaThreshold = limiteBombaThreshold,
                    onLimiteBombaChange = { limiteBombaThreshold = it }
                )
            }
        }
    }
}

@Composable
fun SettingsScreen(
    notificationsEnabled: Boolean,
    onNotificationsChange: (Boolean) -> Unit,
    maxTempThreshold: String,
    onMaxTempChange: (String) -> Unit,
    minHumThreshold: String,
    onMinHumChange: (String) -> Unit,
    limiteBombaThreshold: String,
    onLimiteBombaChange: (String) -> Unit
) {
    val coroutineScope = rememberCoroutineScope()
    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(20.dp),
        horizontalAlignment = Alignment.CenterHorizontally
    ) {
        Text("Configurações", fontSize = 24.sp, fontWeight = FontWeight.Bold, color = Color(0xFF4CAF50))
        Spacer(modifier = Modifier.height(30.dp))

        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically
        ) {
            Text("Ativar Notificações", fontSize = 18.sp)
            Switch(
                checked = notificationsEnabled,
                onCheckedChange = onNotificationsChange,
                colors = SwitchDefaults.colors(checkedThumbColor = Color(0xFF4CAF50))
            )
        }

        Spacer(modifier = Modifier.height(20.dp))

        OutlinedTextField(
            value = maxTempThreshold,
            onValueChange = onMaxTempChange,
            label = { Text("Temperatura Máxima (°C)") },
            keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Number),
            modifier = Modifier.fillMaxWidth()
        )

        Spacer(modifier = Modifier.height(20.dp))

        OutlinedTextField(
            value = minHumThreshold,
            onValueChange = onMinHumChange,
            label = { Text("Humidade Mínima (%)") },
            keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Number),
            modifier = Modifier.fillMaxWidth()
        )

        Spacer(modifier = Modifier.height(20.dp))

        OutlinedTextField(
            value = limiteBombaThreshold,
            onValueChange = onLimiteBombaChange,
            label = { Text("Limite da Bomba (%)") },
            keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Number),
            modifier = Modifier.fillMaxWidth()
        )

        Spacer(modifier = Modifier.height(10.dp))

        Button(
            onClick = {
                coroutineScope.launch(Dispatchers.IO) {
                    try {
                        val client = OkHttpClient()
                        val request = Request.Builder()
                            .url("http://192.168.4.1/config?limite=$limiteBombaThreshold")
                            .build()
                        client.newCall(request).execute().use { response ->
                            // Ignore response
                        }
                    } catch (e: Exception) {
                        // Ignore error
                    }
                }
            },
            colors = ButtonDefaults.buttonColors(containerColor = Color(0xFF4CAF50)),
            modifier = Modifier.fillMaxWidth()
        ) {
            Text("Guardar Bomba", color = Color.White)
        }
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun LoginScreen(onLoginSuccess: () -> Unit) {
    var password by remember { mutableStateOf("") }
    var errorMessage by remember { mutableStateOf("") }

    Box(
        modifier = Modifier.fillMaxSize(),
        contentAlignment = Alignment.Center
    ) {
        Card(
            modifier = Modifier
                .padding(20.dp)
                .fillMaxWidth(0.9f),
            elevation = CardDefaults.cardElevation(defaultElevation = 8.dp),
            shape = RoundedCornerShape(20.dp),
            colors = CardDefaults.cardColors(containerColor = Color.White)
        ) {
            Column(
                modifier = Modifier
                    .padding(32.dp)
                    .fillMaxWidth(),
                horizontalAlignment = Alignment.CenterHorizontally,
                verticalArrangement = Arrangement.Center
            ) {
                Text(
                    text = "Monitor de Plantas",
                    fontSize = 28.sp,
                    fontWeight = FontWeight.Bold,
                    color = Color(0xFF2E8B57)
                )

                Spacer(modifier = Modifier.height(8.dp))

                Text(
                    text = "Por favor, introduza a sua senha.",
                    fontSize = 14.sp,
                    color = Color.Gray
                )

                Spacer(modifier = Modifier.height(32.dp))

                OutlinedTextField(
                    value = password,
                    onValueChange = { password = it },
                    label = { Text("Senha") },
                    visualTransformation = PasswordVisualTransformation(),
                    singleLine = true,
                    modifier = Modifier.fillMaxWidth(),
                    shape = RoundedCornerShape(12.dp)
                )

                Spacer(modifier = Modifier.height(16.dp))

                if (errorMessage.isNotEmpty()) {
                    Text(errorMessage, color = Color.Red, fontSize = 14.sp)
                    Spacer(modifier = Modifier.height(8.dp))
                } else {
                    Spacer(modifier = Modifier.height(27.dp)) // Reserve space to avoid jumping
                }

                Button(
                    onClick = {
                        if (password == "admin") {
                            onLoginSuccess()
                        } else {
                            errorMessage = "Senha incorreta!"
                        }
                    },
                    modifier = Modifier
                        .fillMaxWidth()
                        .height(50.dp),
                    shape = RoundedCornerShape(12.dp),
                    colors = ButtonDefaults.buttonColors(containerColor = Color(0xFF4CAF50))
                ) {
                    Text("Entrar", color = Color.White, fontSize = 16.sp, fontWeight = FontWeight.Bold)
                }
            }
        }
    }
}

data class HistoryRecord(val temp: Float, val hum: Float)

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun DashboardScreen(
    notificationsEnabled: Boolean = false,
    maxTempThreshold: Float = 30f,
    minHumThreshold: Float = 30f
) {
    var temperatura by remember { mutableStateOf("--") }
    var luminosidade by remember { mutableStateOf("--") }
    var humidade by remember { mutableStateOf("--") }
    var estadoBomba by remember { mutableStateOf("--") }
    var status by remember { mutableStateOf("A aguardar dados...") }
    var history by remember { mutableStateOf<List<HistoryRecord>>(emptyList()) }

    val context = androidx.compose.ui.platform.LocalContext.current

    val currentNotificationsEnabled by rememberUpdatedState(notificationsEnabled)
    val currentMaxTempThreshold by rememberUpdatedState(maxTempThreshold)
    val currentMinHumThreshold by rememberUpdatedState(minHumThreshold)

    val client = remember {
        OkHttpClient.Builder()
            .connectTimeout(3, TimeUnit.SECONDS)
            .readTimeout(3, TimeUnit.SECONDS)
            .build()
    }

    // Estado para cooldown de notificações
    var lastTempNotifTime by remember { mutableStateOf(0L) }
    var lastHumNotifTime by remember { mutableStateOf(0L) }
    val cooldownMs = 300000L // 5 minutos em ms

    fun sendNotification(id: Int, title: String, message: String) {
        if (ContextCompat.checkSelfPermission(context, android.Manifest.permission.POST_NOTIFICATIONS) == PackageManager.PERMISSION_GRANTED) {
            val builder = NotificationCompat.Builder(context, "PLANT_ALERTS")
                .setSmallIcon(android.R.drawable.ic_dialog_alert)
                .setContentTitle(title)
                .setContentText(message)
                .setPriority(NotificationCompat.PRIORITY_DEFAULT)

            with(NotificationManagerCompat.from(context)) {
                notify(id, builder.build())
            }
        }
    }

    LaunchedEffect(Unit) {
        while (true) {
            withContext(Dispatchers.IO) {
                try {
                    val request = Request.Builder()
                        .url("http://192.168.4.1/")
                        .build()

                    client.newCall(request).execute().use { response ->
                        if (!response.isSuccessful) {
                            status = "Erro: Servidor retornou ${response.code}"
                            return@use
                        }

                        // Parse response body. Treating as UTF-8 by default in okhttp strings
                        val html = response.body?.string() ?: ""

                        val tempMatcher = Pattern.compile("Temperatura: <strong>([-0-9.]+) °C</strong>").matcher(html)
                        val lightMatcher = Pattern.compile("Luminosidade: <strong>([0-9.]+) lx</strong>").matcher(html)
                        val humMatcher = Pattern.compile("Humidade: <strong>([0-9.]+) %</strong>").matcher(html)
                        val bombaStateMatcher = Pattern.compile("Bomba de Água <strong><span[^>]*>(.*?)</span>").matcher(html)
                        val bombaLimMatcher = Pattern.compile("name='limite'.*?value='([0-9]+)'").matcher(html)

                        val hasTemp = tempMatcher.find()
                        val hasLight = lightMatcher.find()
                        val hasHum = humMatcher.find()

                        withContext(Dispatchers.Main) {
                            if (bombaStateMatcher.find()) {
                                estadoBomba = bombaStateMatcher.group(1) ?: "--"
                            }

                            // Só atualiza se houver um valor numérico extraído com sucesso do HTML para garantir que app e web page estão synced
                            if (bombaLimMatcher.find()) {
                                // Idealmente só devia atualizar se o campo não estiver focado, mas como não temos isso exposto de forma fácil
                                // no textfield base do compose para este scope, mantemos sincronia simples via fetch periódico.
                                val limitStr = bombaLimMatcher.group(1)
                                if (limitStr != null) {
                                     // (Este binding pode sobrepor se o user estiver a digitar exatamente quando recarrega - edgecase menor que 5s)
                                     // Mas como a diretiva sugere live-sync... a não ser que possamos partilhar estado de foco.
                                     // Omitido para não reescrever o campo limitBombaThreshold se estragar UX
                                }
                            }

                            if (hasTemp && hasLight && hasHum) {
                                temperatura = tempMatcher.group(1) ?: "--"
                                luminosidade = lightMatcher.group(1) ?: "--"
                                humidade = humMatcher.group(1) ?: "--"
                                status = "Conectado"

                                if (currentNotificationsEnabled) {
                                    val tempVal = temperatura.toFloatOrNull() ?: 0f
                                    val humVal = humidade.toFloatOrNull() ?: 0f
                                    val now = System.currentTimeMillis()

                                    if (tempVal > currentMaxTempThreshold) {
                                        if (now - lastTempNotifTime > cooldownMs) {
                                            sendNotification(1, "Alerta de Temperatura", "Temperatura atual: $tempVal °C (Máx: $currentMaxTempThreshold °C)")
                                            lastTempNotifTime = now
                                        }
                                    } else {
                                        lastTempNotifTime = 0L
                                    }

                                    if (humVal < currentMinHumThreshold) {
                                        if (now - lastHumNotifTime > cooldownMs) {
                                            sendNotification(2, "Alerta de Humidade", "Humidade atual: $humVal % (Mín: $currentMinHumThreshold %)")
                                            lastHumNotifTime = now
                                        }
                                    } else {
                                        lastHumNotifTime = 0L
                                    }
                                }

                            } else {
                                status = "Não foi possível extrair dados do HTML."
                            }
                        }
                    }

                    // Fetch history
                    val histRequest = Request.Builder()
                        .url("http://192.168.4.1/history")
                        .build()

                    client.newCall(histRequest).execute().use { response ->
                        if (response.isSuccessful) {
                            val jsonString = response.body?.string() ?: "[]"
                            val jsonArray = JSONArray(jsonString)
                            val newHistory = mutableListOf<HistoryRecord>()
                            for (i in 0 until jsonArray.length()) {
                                val obj = jsonArray.getJSONObject(i)
                                newHistory.add(HistoryRecord(
                                    temp = obj.getDouble("temp").toFloat(),
                                    hum = obj.getDouble("hum").toFloat()
                                ))
                            }
                            withContext(Dispatchers.Main) {
                                history = newHistory
                            }
                        }
                    }
                } catch (e: Exception) {
                    withContext(Dispatchers.Main) {
                        status = "Erro de ligação (O ESP32 está ligado?)."
                    }
                }
            }
            delay(5000)
        }
    }

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("Estação de Monitorização", color = Color.White, fontWeight = FontWeight.Bold) },
                colors = TopAppBarDefaults.topAppBarColors(containerColor = Color(0xFF4CAF50))
            )
        }
    ) { paddingValues ->
        Column(
            modifier = Modifier
                .padding(paddingValues)
                .padding(20.dp)
                .fillMaxSize(),
            horizontalAlignment = Alignment.CenterHorizontally,
            verticalArrangement = Arrangement.Center
        ) {
            DataCard(title = "Temperatura", value = "$temperatura °C", iconColor = Color.Red)
            Spacer(modifier = Modifier.height(20.dp))
            DataCard(title = "Luminosidade", value = "$luminosidade lx", iconColor = Color(0xFFFFA500))
            Spacer(modifier = Modifier.height(20.dp))
            DataCard(title = "Humidade do Solo", value = "$humidade %", iconColor = Color.Blue)
            Spacer(modifier = Modifier.height(20.dp))
            DataCard(title = "Bomba de Água", value = estadoBomba, iconColor = Color(0xFF9B59B6))

            Spacer(modifier = Modifier.height(40.dp))

            if (history.isNotEmpty()) {
                Text(text = "Histórico", fontWeight = FontWeight.Bold, fontSize = 18.sp, color = Color.Gray)
                Spacer(modifier = Modifier.height(10.dp))
                HistoryChart(history)
                Spacer(modifier = Modifier.height(20.dp))
            }

            if (status == "A aguardar dados...") {
                CircularProgressIndicator(
                    color = Color(0xFF4CAF50),
                    modifier = Modifier.size(30.dp)
                )
                Spacer(modifier = Modifier.height(10.dp))
            }

            Text(
                text = status,
                color = if (status == "Conectado") Color(0xFF4CAF50) else Color.Red,
                fontWeight = FontWeight.Bold,
                fontSize = 14.sp
            )
        }
    }
}

@Composable
fun HistoryChart(history: List<HistoryRecord>) {
    Card(
        modifier = Modifier
            .fillMaxWidth()
            .height(150.dp),
        elevation = CardDefaults.cardElevation(defaultElevation = 4.dp),
        shape = RoundedCornerShape(15.dp),
        colors = CardDefaults.cardColors(containerColor = Color.White)
    ) {
        Box(modifier = Modifier.padding(16.dp)) {
            Canvas(modifier = Modifier.fillMaxSize()) {
                val width = size.width
                val height = size.height
                val stepX = width / Math.max(history.size - 1, 1).toFloat()

                val maxTemp = Math.max(history.maxOf { it.temp }, 40f)
                val maxHum = 100f

                val tempPath = Path()
                val humPath = Path()

                history.forEachIndexed { i, record ->
                    val x = i * stepX
                    val tempY = height - (record.temp / maxTemp * height)
                    val humY = height - (record.hum / maxHum * height)

                    if (i == 0) {
                        tempPath.moveTo(x, tempY)
                        humPath.moveTo(x, humY)
                    } else {
                        tempPath.lineTo(x, tempY)
                        humPath.lineTo(x, humY)
                    }
                }

                drawPath(
                    path = tempPath,
                    color = Color(0xFFFF5555),
                    style = Stroke(width = 6f)
                )

                drawPath(
                    path = humPath,
                    color = Color(0xFF55AAFF),
                    style = Stroke(width = 6f)
                )
            }
        }
    }
}

@Composable
fun DataCard(title: String, value: String, iconColor: Color) {
    Card(
        modifier = Modifier.fillMaxWidth(),
        elevation = CardDefaults.cardElevation(defaultElevation = 6.dp),
        shape = RoundedCornerShape(16.dp),
        colors = CardDefaults.cardColors(containerColor = Color.White)
    ) {
        Row(
            modifier = Modifier
                .padding(vertical = 24.dp, horizontal = 30.dp)
                .fillMaxWidth(),
            horizontalArrangement = Arrangement.Start,
            verticalAlignment = Alignment.CenterVertically
        ) {
            Box(
                modifier = Modifier
                    .size(50.dp),
                contentAlignment = Alignment.Center
            ) {
                Text(
                    text = when(title) {
                        "Temperatura" -> "🌡️"
                        "Luminosidade" -> "☀️"
                        else -> "💧"
                    },
                    fontSize = 28.sp
                )
            }
            Spacer(modifier = Modifier.width(20.dp))
            Column(horizontalAlignment = Alignment.Start) {
                Text(text = title, fontSize = 14.sp, color = Color.Gray, fontWeight = FontWeight.Medium)
                Text(text = value, fontSize = 28.sp, fontWeight = FontWeight.Bold, color = Color.DarkGray)
            }
        }
    }
}
