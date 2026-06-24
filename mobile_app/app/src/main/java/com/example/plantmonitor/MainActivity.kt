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
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.delay
import kotlinx.coroutines.withContext
import okhttp3.OkHttpClient
import okhttp3.Request
import java.util.regex.Pattern
import java.util.concurrent.TimeUnit

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
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
                    DashboardScreen()
                }
            }
        }
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun DashboardScreen() {
    var temperatura by remember { mutableStateOf("--") }
    var luminosidade by remember { mutableStateOf("--") }
    var humidade by remember { mutableStateOf("--") }
    var status by remember { mutableStateOf("A aguardar dados...") }

    val client = remember {
        OkHttpClient.Builder()
            .connectTimeout(3, TimeUnit.SECONDS)
            .readTimeout(3, TimeUnit.SECONDS)
            .build()
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

                        val hasTemp = tempMatcher.find()
                        val hasLight = lightMatcher.find()
                        val hasHum = humMatcher.find()

                        withContext(Dispatchers.Main) {
                            if (hasTemp && hasLight && hasHum) {
                                temperatura = tempMatcher.group(1) ?: "--"
                                luminosidade = lightMatcher.group(1) ?: "--"
                                humidade = humMatcher.group(1) ?: "--"
                                status = "Conectado"
                            } else {
                                status = "Não foi possível extrair dados do HTML."
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

            Spacer(modifier = Modifier.height(40.dp))
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
fun DataCard(title: String, value: String, iconColor: Color) {
    Card(
        modifier = Modifier.fillMaxWidth(),
        elevation = CardDefaults.cardElevation(defaultElevation = 4.dp),
        shape = RoundedCornerShape(15.dp),
        colors = CardDefaults.cardColors(containerColor = Color.White)
    ) {
        Row(
            modifier = Modifier
                .padding(vertical = 20.dp, horizontal = 30.dp)
                .fillMaxWidth(),
            horizontalArrangement = Arrangement.Center,
            verticalAlignment = Alignment.CenterVertically
        ) {
            // Simplified icon placeholder using a colored box or text
            Box(
                modifier = Modifier
                    .size(40.dp),
                contentAlignment = Alignment.Center
            ) {
                Text(
                    text = when(title) {
                        "Temperatura" -> "🌡️"
                        "Luminosidade" -> "☀️"
                        else -> "💧"
                    },
                    fontSize = 24.sp
                )
            }
            Spacer(modifier = Modifier.width(15.dp))
            Column(horizontalAlignment = Alignment.CenterHorizontally) {
                Text(text = title, fontSize = 16.sp, color = Color.Gray)
                Text(text = value, fontSize = 32.sp, fontWeight = FontWeight.Bold)
            }
        }
    }
}
