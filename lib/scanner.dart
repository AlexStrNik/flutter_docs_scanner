import 'dart:ffi';
import 'dart:math';

import 'package:camera/camera.dart';
import 'package:ffi/ffi.dart';
import 'package:flutter/foundation.dart';
import 'package:flutter_docs_scanner/extensions.dart';
import 'package:flutter_docs_scanner/ffi.dart';
import 'package:flutter_docs_scanner/models/image.dart';

final _init =
    sdkNative.lookupFunction<_InitDetectorNative, _InitDetector>('initScanner');

typedef _InitDetectorNative = Pointer<NativeType> Function();
typedef _InitDetector = Pointer<NativeType> Function();

final _deinit = sdkNative
    .lookupFunction<_DeinitDetectorNative, _DeinitDetector>('deinitScanner');

typedef _DeinitDetectorNative = Void Function(Pointer<NativeType>);
typedef _DeinitDetector = void Function(Pointer<NativeType>);

final class _PointNative extends Struct {
  @Float()
  external double x;
  @Float()
  external double y;
}

final class _RectangleNative extends Struct {
  external _PointNative a;
  external _PointNative b;
  external _PointNative c;
  external _PointNative d;
}

/// Area recognized as document.
final class Rectangle {
  /// Four frame relative points in order TL, TR, BR, BL.
  List<Point<double>> points;

  /// Constructs rectangle.
  Rectangle(this.points);
}

/// Wrapper around native Scanner.
class FlutterDocsScanner {
  Pointer<NativeType> _scanner = nullptr;

  /// Does nothing.
  FlutterDocsScanner();

  /// Initializing native scanner.
  Future<void> init() async {
    dispose(); //dispose if there was any native scanner inited
    _scanner = _init();
    return;
  }

  /// Disposing native scanner.
  dispose() {
    if (_scanner != nullptr) {
      _deinit(_scanner);
      _scanner = nullptr;
    }
  }

  /// Should called each frame, returns rectangle of recognized doc if any.
  Future<Rectangle?> processFrame(CameraImage image, int rotation) async {
    //some checks to ignore problems with flutter camera plugin
    if (!image.isEmpty() && _scanner != nullptr) {
      return compute(
          _processFrameAsync, _FrameData(_scanner.address, image, rotation));
    } else {
      return null;
    }
  }

  /// Processes taken image and returns new one – cropped and with corrected perspective.
  Future<SdkImage> processImage(XFile image) async {
    return compute(_processImageAsync, _ImageData(_scanner.address, image));
  }
}

class _FrameData {
  CameraImage image;
  int rotation;
  int scanner;

  _FrameData(this.scanner, this.image, this.rotation);
}

class _ImageData {
  XFile image;
  int scanner;

  _ImageData(this.scanner, this.image);
}

final _processFrame = sdkNative
    .lookupFunction<_ProcessFrameNative, _ProcessFrame>('processFrame');

typedef _ProcessFrameNative = Pointer<_RectangleNative> Function(
    Pointer<NativeType>, Pointer<SdkFrame>);
typedef _ProcessFrame = Pointer<_RectangleNative> Function(
    Pointer<NativeType>, Pointer<SdkFrame>);

Future<Rectangle?> _processFrameAsync(_FrameData detect) async {
  try {
    Pointer<SdkFrame> image = detect.image.toSdkImagePointer(detect.rotation);
    final scanner = Pointer.fromAddress(detect.scanner);
    Pointer<_RectangleNative> result;

    result = _processFrame(scanner, image);
    image.release();

    if (result == nullptr) {
      return null;
    }
    final rectangle = Rectangle([
      _mapNativePoint(result.ref.a),
      _mapNativePoint(result.ref.b),
      _mapNativePoint(result.ref.c),
      _mapNativePoint(result.ref.d),
    ]);

    malloc.free(result);

    return rectangle;
  } catch (e) {
    print(e);
  }

  return null;
}

final _processImage = sdkNative
    .lookupFunction<_ProcessImageNative, _ProcessImage>('processImage');

typedef _ProcessImageNative = Pointer<SdkImage> Function(
    Pointer<NativeType>, Pointer<SdkImage>);
typedef _ProcessImage = Pointer<SdkImage> Function(
    Pointer<NativeType>, Pointer<SdkImage>);

Future<SdkImage> _processImageAsync(_ImageData detect) async {
  final scanner = Pointer.fromAddress(detect.scanner);

  final sdkImage = await detect.image.toSdkImagePointer();
  final processedImage = _processImage(scanner, sdkImage);

  malloc.free(sdkImage.ref.bytes);
  malloc.free(sdkImage);

  return processedImage.ref;
}

Point<double> _mapNativePoint(_PointNative native) {
  return Point(
    native.x,
    native.y,
  );
}
