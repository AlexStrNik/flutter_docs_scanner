import 'dart:ffi';

final class SdkFrame extends Struct {
  external Pointer<SdkImagePlane> plane;

  @Int32()
  external int platform; // 0 ios, 1 android

  @Int32()
  external int width;

  @Int32()
  external int height;

  @Int32()
  external int rotation;
}

final class SdkImage extends Struct {
  external Pointer<Uint8> bytes;

  @LongLong()
  external int length;
}

final class SdkImagePlane extends Struct {
  external Pointer<Uint8> planeData;

  @Int32()
  external int length;

  @Int32()
  external int bytesPerRow;

  external Pointer<SdkImagePlane> nextPlane;
}
