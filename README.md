# flutter_docs_scanner

A Flutter plugin for iOS and Android allowing to scan documents using your camera.

## Features

- Live preview of recognized document.
- Perspective correction.
- Flexible API.
- OpenCV based backend.

## Setup

Same as for [`packages/camera`](https://pub.dev/packages/camera)

## Example

```dart
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
```

## ToDo:

- [ ] Lock focus on document.
- [ ] Color/gamma enchantments.
- [ ] Better perspective correction using camera lens info.
