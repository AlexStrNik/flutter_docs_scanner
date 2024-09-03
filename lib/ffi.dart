import 'dart:ffi';
import 'dart:io';

import '../models/image.dart';

final DynamicLibrary sdkNative = () {
  final DynamicLibrary nativeEdgeDetection = Platform.isAndroid
      ? DynamicLibrary.open("libflutter_docs_scanner.so")
      : DynamicLibrary.process();
  return nativeEdgeDetection;
}();

final createImageFrame =
    sdkNative.lookupFunction<_CreateImageFrameNative, _CreateImageFrame>(
        'createImageFrame');

final createImagePlane = sdkNative
    .lookupFunction<_CreateImagePlaneNative, _CreateImagePlane>('createPlane');

final createImage =
    sdkNative.lookupFunction<_CreateImageNative, _CreateImage>('createImage');

typedef _CreateImageFrameNative = Pointer<SdkFrame> Function();
typedef _CreateImageFrame = Pointer<SdkFrame> Function();

typedef _CreateImagePlaneNative = Pointer<SdkImagePlane> Function();
typedef _CreateImagePlane = Pointer<SdkImagePlane> Function();

typedef _CreateImageNative = Pointer<SdkImage> Function();
typedef _CreateImage = Pointer<SdkImage> Function();
