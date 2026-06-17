import 'package:flutter_test/flutter_test.dart';
import 'package:mobile_app/main.dart';

void main() {
  testWidgets('App smoke test', (WidgetTester tester) async {
    // Build our app and trigger a frame.
    await tester.pumpWidget(const PlantMonitorApp());

    // Verify that our app renders the correct title.
    expect(find.text('Estação de Monitorização'), findsOneWidget);
  });
}
