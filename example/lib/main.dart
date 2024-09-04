import 'package:flutter/material.dart';
import 'package:flutter_docs_scanner/flutter_docs_scanner.dart';

void main() {
  runApp(const MyApp());
}

class MyApp extends StatefulWidget {
  const MyApp({super.key});

  @override
  State<MyApp> createState() => _MyAppState();
}

class _MyAppState extends State<MyApp> {
  @override
  void initState() {
    super.initState();
  }

  @override
  Widget build(BuildContext context) {
    return const MaterialApp(
      home: Scaffold(
        body: ScannerPage(),
      ),
    );
  }
}

class ScannerPage extends StatefulWidget {
  const ScannerPage({
    super.key,
  });

  @override
  State<ScannerPage> createState() => _ScannerPageState();
}

class _ScannerPageState extends State<ScannerPage> {
  final _scannerController = ScannerController();

  @override
  Widget build(BuildContext context) {
    return Stack(
      children: [
        ScannerPreview(
          controller: _scannerController,
        ),
        Align(
          alignment: Alignment.bottomCenter,
          child: SafeArea(
            child: FloatingActionButton(
              child: const Icon(Icons.camera),
              onPressed: () async {
                final image = await _scannerController.takeAndProcess();
                if (!context.mounted) return;

                Scaffold.of(context).showBottomSheet(
                  (context) {
                    return SafeArea(
                      child: Padding(
                        padding: const EdgeInsets.all(20),
                        child: Image(
                          image: image,
                          fit: BoxFit.fitWidth,
                        ),
                      ),
                    );
                  },
                );
              },
            ),
          ),
        )
      ],
    );
  }
}
