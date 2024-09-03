import 'dart:async';
import 'dart:ffi';

import 'package:camera/camera.dart';
import 'package:flutter/material.dart';
import 'package:flutter_docs_scanner/scanner.dart';

final class ScannerController {
  late CameraController _cameraController;
  late FlutterDocsScanner _scanner;

  bool isAttached = false;

  Future<MemoryImage> takeAndProcess() async {
    _cameraController.pausePreview();
    assert(isAttached, 'ScannerController not attached to any preview views.');
    final image = await _cameraController.takePicture();
    final processedImage = await _scanner.processImage(image);
    final flutterImage = MemoryImage(
      processedImage.bytes.asTypedList(processedImage.length),
    );
    _cameraController.resumePreview();

    return flutterImage;
  }

  void onAttached(FlutterDocsScanner scanner, CameraController controller) {
    _cameraController = controller;
    _scanner = scanner;
    isAttached = true;
  }
}
