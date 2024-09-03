import Flutter
import UIKit

public class FlutterDocsScannerPlugin: NSObject, FlutterPlugin {
  public static func register(with registrar: FlutterPluginRegistrar) {
    let channel = FlutterMethodChannel(name: "flutter_docs_scanner", binaryMessenger: registrar.messenger())
    let instance = FlutterDocsScannerPlugin()
    registrar.addMethodCallDelegate(instance, channel: channel)
  }

  public func handle(_ call: FlutterMethodCall, result: @escaping FlutterResult) {}
}
