import 'dart:async';

import 'package:camera/camera.dart';
import 'package:flutter/material.dart';
import 'package:flutter_docs_scanner/scanner.dart';
import 'package:flutter_docs_scanner/scanner_controller.dart';
import 'package:flutter_docs_scanner/widgets/doc_overlay.dart';

/// Ready-to-use widget for displaying camera preview and process images using controller.
class ScannerPreview extends StatefulWidget {
  /// Creates ScannerPreview.
  const ScannerPreview({
    super.key,
    this.controller,
  });

  /// Pass controller to be able take shots.
  final ScannerController? controller;

  @override
  State<ScannerPreview> createState() => _ScannerPreviewState();
}

class _ScannerPreviewState extends State<ScannerPreview> {
  CameraController? _controller;
  FlutterDocsScanner? _scanner;

  late StreamController<Rectangle?> detectionResultStreamController;

  @override
  void initState() {
    super.initState();
    detectionResultStreamController = StreamController();

    Future.wait([
      _initializeCameraController(),
      _initializeScanner(),
    ]).then((_) {
      widget.controller?.onAttached(_scanner!, _controller!);
    });
  }

  Future<void> _initializeCameraController() async {
    final cameras = await availableCameras();
    _controller = CameraController(
      cameras[0],
      ResolutionPreset.high,
      imageFormatGroup: ImageFormatGroup.bgra8888,
      enableAudio: false,
    );
    await _controller!.initialize();
    _controller!.startImageStream(onFrameAvailable);
    setState(() {});
  }

  Future<void> _initializeScanner() async {
    final scanner = FlutterDocsScanner();
    await scanner.init();
    _scanner = scanner;
  }

  Future<void> onFrameAvailable(CameraImage image) async {
    if (!context.mounted || _scanner == null) {
      return;
    }
    final result = await _scanner!.processFrame(image, 0);
    detectionResultStreamController.add(result);
  }

  @override
  void dispose() {
    _controller?.dispose();
    _scanner?.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    if (_controller == null || !_controller!.value.isInitialized) {
      return Container();
    }

    return Center(
      child: CameraPreview(
        _controller!,
        child: DocOverlayWidget(
          stream: detectionResultStreamController.stream,
        ),
      ),
    );
  }
}
