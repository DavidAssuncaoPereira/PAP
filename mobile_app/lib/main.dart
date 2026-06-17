import 'package:flutter/material.dart';
import 'package:http/http.dart' as http;
import 'package:html/parser.dart' show parse;
import 'dart:async';

void main() {
  runApp(const PlantMonitorApp());
}

class PlantMonitorApp extends StatelessWidget {
  const PlantMonitorApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Monitor de Plantas',
      theme: ThemeData(
        colorScheme: ColorScheme.fromSeed(seedColor: Colors.green),
        useMaterial3: true,
      ),
      home: const DashboardScreen(),
    );
  }
}

class DashboardScreen extends StatefulWidget {
  const DashboardScreen({super.key});

  @override
  State<DashboardScreen> createState() => _DashboardScreenState();
}

class _DashboardScreenState extends State<DashboardScreen> {
  String _temperatura = "--";
  String _luminosidade = "--";
  String _humidade = "--";
  String _status = "A aguardar dados...";
  Timer? _timer;

  // IP do ESP32 (Modo AP)
  final String esp32Url = "http://192.168.4.1/";

  @override
  void initState() {
    super.initState();
    _fetchData();
    // Atualiza os valores automaticamente a cada 5 segundos
    _timer = Timer.periodic(const Duration(seconds: 5), (timer) {
      _fetchData();
    });
  }

  @override
  void dispose() {
    _timer?.cancel();
    super.dispose();
  }

  Future<void> _fetchData() async {
    try {
      final response = await http.get(Uri.parse(esp32Url)).timeout(const Duration(seconds: 3));

      if (response.statusCode == 200) {
        // Fazer parse do HTML retornado pelo ESP32
        var document = parse(response.body);

        var divs = document.getElementsByClassName('dado');

        if (divs.length >= 3) {
          String tempRaw = divs[0].text;
          String lightRaw = divs[1].text;
          String humRaw = divs[2].text;

          // Extrair apenas os valores numéricos
          RegExp tempRegExp = RegExp(r"Temperatura:\s*([-0-9.]+)\s*°C");
          RegExp lightRegExp = RegExp(r"Luminosidade:\s*([0-9.]+)\s*lx");
          RegExp humRegExp = RegExp(r"Humidade:\s*([0-9.]+)\s*%");

          var tempMatch = tempRegExp.firstMatch(tempRaw);
          var lightMatch = lightRegExp.firstMatch(lightRaw);
          var humMatch = humRegExp.firstMatch(humRaw);

          setState(() {
            if (tempMatch != null) _temperatura = tempMatch.group(1)!;
            if (lightMatch != null) _luminosidade = lightMatch.group(1)!;
            if (humMatch != null) _humidade = humMatch.group(1)!;
            _status = "Conectado";
          });
        } else {
           setState(() {
            _status = "Erro: Estrutura HTML inválida.";
          });
        }
      } else {
        setState(() {
          _status = "Erro: Servidor retornou ${response.statusCode}";
        });
      }
    } catch (e) {
      setState(() {
        _status = "Erro de ligação (O ESP32 está ligado?).";
      });
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: const Color(0xFFF0F8FF),
      appBar: AppBar(
        backgroundColor: Colors.green,
        title: const Text('Estação de Monitorização', style: TextStyle(color: Colors.white, fontWeight: FontWeight.bold)),
        centerTitle: true,
      ),
      body: Center(
        child: Padding(
          padding: const EdgeInsets.all(20.0),
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            children: <Widget>[
              // Cartão de Temperatura
              Card(
                elevation: 4,
                shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(15)),
                child: Padding(
                  padding: const EdgeInsets.symmetric(vertical: 20.0, horizontal: 30.0),
                  child: Row(
                    mainAxisAlignment: MainAxisAlignment.center,
                    children: [
                      const Icon(Icons.thermostat, color: Colors.redAccent, size: 40),
                      const SizedBox(width: 15),
                      Column(
                        children: [
                          const Text("Temperatura", style: TextStyle(fontSize: 16, color: Colors.grey)),
                          Text("$_temperatura °C", style: const TextStyle(fontSize: 32, fontWeight: FontWeight.bold)),
                        ],
                      )
                    ],
                  ),
                ),
              ),
              const SizedBox(height: 20),
              // Cartão de Luminosidade
              Card(
                elevation: 4,
                shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(15)),
                child: Padding(
                  padding: const EdgeInsets.symmetric(vertical: 20.0, horizontal: 30.0),
                  child: Row(
                    mainAxisAlignment: MainAxisAlignment.center,
                    children: [
                      const Icon(Icons.wb_sunny, color: Colors.orangeAccent, size: 40),
                      const SizedBox(width: 15),
                      Column(
                        children: [
                          const Text("Luminosidade", style: TextStyle(fontSize: 16, color: Colors.grey)),
                          Text("$_luminosidade lx", style: const TextStyle(fontSize: 32, fontWeight: FontWeight.bold)),
                        ],
                      )
                    ],
                  ),
                ),
              ),
              const SizedBox(height: 20),
              // Cartão de Humidade do Solo
              Card(
                elevation: 4,
                shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(15)),
                child: Padding(
                  padding: const EdgeInsets.symmetric(vertical: 20.0, horizontal: 30.0),
                  child: Row(
                    mainAxisAlignment: MainAxisAlignment.center,
                    children: [
                      const Icon(Icons.water_drop, color: Colors.blueAccent, size: 40),
                      const SizedBox(width: 15),
                      Column(
                        children: [
                          const Text("Humidade do Solo", style: TextStyle(fontSize: 16, color: Colors.grey)),
                          Text("$_humidade %", style: const TextStyle(fontSize: 32, fontWeight: FontWeight.bold)),
                        ],
                      )
                    ],
                  ),
                ),
              ),
              const SizedBox(height: 40),
              // Status da Ligação
              Text(
                _status,
                style: TextStyle(
                  fontSize: 14,
                  color: _status == "Conectado" ? Colors.green : Colors.red,
                  fontWeight: FontWeight.bold
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }
}