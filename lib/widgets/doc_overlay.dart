import 'package:flutter/material.dart';
import 'package:flutter_docs_scanner/scanner.dart';

/// Helper widget for displaying rectangle over recognized document.
class DocOverlayWidget extends StatefulWidget {
  final Stream<Rectangle?> stream;

  const DocOverlayWidget({
    super.key,
    required this.stream,
  });

  @override
  State<DocOverlayWidget> createState() => _DocOverlayWidgetState();
}

class _DocOverlayWidgetState extends State<DocOverlayWidget> {
  ValueNotifier<Rectangle?> notifier = ValueNotifier(null);

  @override
  void initState() {
    startListenStream(widget.stream);
    super.initState();
  }

  void startListenStream(Stream<Rectangle?> stream) async {
    await for (final result in stream) {
      notifier.value = result;
    }
  }

  @override
  Widget build(BuildContext context) {
    var shapesPainter = ShapesPainter(notifier);
    final size = MediaQuery.of(context).size;

    return CustomPaint(
      size: size,
      painter: shapesPainter,
    );
  }
}

/// Helper widget for displaying rectangle over recognized document.
class ShapesPainter extends CustomPainter {
  ValueNotifier<Rectangle?> notifier;
  Paint circlePaint = Paint()
    ..strokeWidth = 3
    ..color = Colors.lightBlue.withAlpha(155);

  ShapesPainter(this.notifier) : super(repaint: notifier);

  @override
  void paint(Canvas canvas, Size size) {
    var rectangle = notifier.value;
    if (rectangle != null) {
      final path = Path();
      path.moveTo(
        rectangle.points[0].x * size.width,
        rectangle.points[0].y * size.height,
      );
      for (final point in rectangle.points.skip(1)) {
        path.lineTo(point.x * size.width, point.y * size.height);
      }
      path.close();
      canvas.drawPath(path, circlePaint);
    }
  }

  @override
  bool shouldRepaint(covariant CustomPainter oldDelegate) {
    return true;
  }
}
