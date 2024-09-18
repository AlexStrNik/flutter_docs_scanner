#include "flutter_docs_scanner.hpp"
#include "utils.hpp"

using namespace cv;
using namespace std;

#define EXPORT extern "C" __attribute__((visibility("default"))) __attribute__((used))


EXPORT Scanner::Rectangle *processFrame(Scanner *scanner, flutter::ImageForDetect *frame) {
    auto img = flutter::prepareMat(frame);
    auto corners = scanner->detectDoc(img);
    
    Scanner::Rectangle* rect = nullptr;
    
    if (!corners.empty()) {
        std::vector<Scanner::Point> normalized_points;
        for (const auto& point : corners) {
            normalized_points.emplace_back(
                Scanner::Point(
                    float(point.x) / float(img.cols),
                    float(point.y) / float(img.rows)
                )
            );
        }
        
        rect = new Scanner::Rectangle(normalized_points[0], normalized_points[1],
                                           normalized_points[2], normalized_points[3]);
    }
    
    return scanner->filterResults(rect);
}

EXPORT flutter::Image *processImage(Scanner *scanner, flutter::Image *image) {
    auto img = flutter::decodeImage(image);
    auto processedImg = scanner->processDoc(img);
    
    return flutter::encodeImage(processedImg);
}

EXPORT Scanner *initScanner() {
    auto *scanner = new Scanner();
    return scanner;
}

EXPORT void deinitScanner(void *scanner) {
    delete (Scanner *) scanner;
}

EXPORT flutter::Plane *createPlane() {
    return (struct flutter::Plane *) malloc(sizeof(struct flutter::Plane));
}

EXPORT flutter::Image *createImage() {
    return (struct flutter::Image *) malloc(sizeof(struct flutter::Image));
}

EXPORT flutter::ImageForDetect *createImageFrame() {
    return (struct flutter::ImageForDetect *) malloc(sizeof(struct flutter::ImageForDetect));
}

Scanner::Scanner() {
    initKalmanFilters();
}

vector<cv::Point> order_points(vector<cv::Point>& pts) {
    vector<cv::Point> rect(4);

    vector<float> s(pts.size());
    for (size_t i = 0; i < pts.size(); i++) {
        s[i] = pts[i].x + pts[i].y;
    }

    rect[0] = pts[min_element(s.begin(), s.end()) - s.begin()];

    rect[2] = pts[max_element(s.begin(), s.end()) - s.begin()];

    vector<float> diff(pts.size());
    for (size_t i = 0; i < pts.size(); i++) {
        diff[i] = pts[i].x - pts[i].y;
    }

    rect[1] = pts[min_element(diff.begin(), diff.end()) - diff.begin()];

    // Bottom-left will have the largest difference
    rect[3] = pts[max_element(diff.begin(), diff.end()) - diff.begin()];

    return rect;
}

vector<cv::Point> find_dest(const vector<cv::Point>& pts) {
    // Extract the points
    cv::Point tl = pts[0];
    cv::Point tr = pts[1];
    cv::Point br = pts[2];
    cv::Point bl = pts[3];

    // Finding the maximum width
    float widthA = sqrt(pow(br.x - bl.x, 2) + pow(br.y - bl.y, 2));
    float widthB = sqrt(pow(tr.x - tl.x, 2) + pow(tr.y - tl.y, 2));
    int maxWidth = max(static_cast<int>(widthA), static_cast<int>(widthB));

    // Finding the maximum height
    float heightA = sqrt(pow(tr.x - br.x, 2) + pow(tr.y - br.y, 2));
    float heightB = sqrt(pow(tl.x - bl.x, 2) + pow(tl.y - bl.y, 2));
    int maxHeight = max(static_cast<int>(heightA), static_cast<int>(heightB));

    // Final destination coordinates
    vector<cv::Point> destination_corners = {
        cv::Point(0, 0),
        cv::Point(maxWidth - 1, 0),
        cv::Point(maxWidth - 1, maxHeight - 1),
        cv::Point(0, maxHeight - 1)
    };

    return destination_corners; // No need to order since they are already in order
}

vector<cv::Point2f> convert(vector<cv::Point> int_points) {
    vector<cv::Point2f> float_points;
    for (auto point : int_points) {
        float_points.push_back(cv::Point2f(float(point.x), float(point.y)));
    }
    
    return float_points;
}

cv::Mat Scanner::processDoc(const cv::Mat &img) {
    auto corners = detectDoc(img);
    
    if (corners.size() != 4) {
        return img;
    }
    
    vector<cv::Point> destination_corners = find_dest(corners);
    
//    int h = img.rows;
//    int w = img.cols;
//    
    // Getting the homography.
    Mat M = getPerspectiveTransform(convert(corners), convert(destination_corners));
    
    // Perspective transform using homography.
    Mat f_img;
    warpPerspective(
        img,
        f_img, M, Size(destination_corners[2].x, destination_corners[2].y),
        INTER_LINEAR
    );
    
    cv::transpose(f_img, f_img);
    
//    Mat enchanced;
//    improve_image(f_img, enchanced);
    
    return f_img;
}

vector<cv::Point> Scanner::detectDoc(const cv::Mat& frame) {
    cv::Mat gray1, contrast_enhanced_image, edges;

    // Enhance contrast using CLAHE
    cv::cvtColor(frame, gray1, cv::COLOR_BGR2GRAY);
    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE();
    clahe->setClipLimit(4);
    clahe->apply(gray1, gray1);

    // Apply edge-preserving filter
    Mat gray;
    cv::bilateralFilter(gray1, gray, 9, 75, 75);

    // Edge detection with dynamic thresholds
    cv::Scalar mean, stddev;
    cv::meanStdDev(gray, mean, stddev);
    double lower_thresh = std::max(0.0, (1.0 - 0.33) * mean[0]);
    double upper_thresh = std::min(255.0, (1.0 + 0.33) * mean[0]);
    cv::Canny(gray, edges, lower_thresh, upper_thresh);

    // Morphological operations
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5));
    cv::morphologyEx(edges, edges, cv::MORPH_CLOSE, kernel);

    // Find contours
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(edges, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    if (contours.empty()) {
        return vector<cv::Point>();
    }

    // Sort contours by area
    std::sort(contours.begin(), contours.end(), [](const std::vector<cv::Point>& c1, const std::vector<cv::Point>& c2) {
        return cv::contourArea(c1, false) > cv::contourArea(c2, false);
    });

    // Iterate over top contours
    for (size_t i = 0; i < std::min(contours.size(), size_t(5)); ++i) {
        std::vector<cv::Point> approx;
        double peri = cv::arcLength(contours[i], true);
        cv::approxPolyDP(contours[i], approx, 0.02 * peri, true);

        if (approx.size() == 4 && cv::isContourConvex(approx)) {
            double area = cv::contourArea(approx);
            if (area < 1000) continue;

            // Check aspect ratio
            cv::Rect bounding_box = cv::boundingRect(approx);
            double current_ratio = static_cast<double>(bounding_box.width) / bounding_box.height;
            double expected_ratio = 0.7071;
            if (current_ratio < expected_ratio * 0.8 || current_ratio > expected_ratio * 1.2) {
                continue;
            }

//            // Check angles
//            bool angles_close_to_90 = true;
//            for (int j = 0; j < 4; j++) {
//                cv::Point2f p1 = approx[j];
//                cv::Point2f p2 = approx[(j + 1) % 4];
//                cv::Point2f p3 = approx[(j + 2) % 4];
//
//                cv::Point2f v1 = p1 - p2;
//                cv::Point2f v2 = p3 - p2;
//
//                double angle = std::fabs(atan2(v2.y, v2.x) - atan2(v1.y, v1.x)) * 180.0 / CV_PI;
//                if (angle < 80 || angle > 100) {
//                    angles_close_to_90 = false;
//                    break;
//                }
//            }
//            if (!angles_close_to_90) {
//                continue;
//            }

            // Order points and normalize
            auto corners = order_points(approx);
            return corners;
        }
    }
    return vector<cv::Point>();
}


Scanner::Rectangle* Scanner::filterResults(Rectangle* new_result) {
    if (new_result == nullptr) {
        if (history.empty()) {
            return nullptr;
        } else {
            auto rectangle = history.back();
            history.pop_back();
            return new Rectangle(rectangle);
        }
    }

    history.push_back(*new_result);
    if (history.size() > HISTORY_SIZE) {
        history.pop_front();
    }

    Point filtered_a = updateKalmanFilter(kf_a, new_result->a);
    Point filtered_b = updateKalmanFilter(kf_b, new_result->b);
    Point filtered_c = updateKalmanFilter(kf_c, new_result->c);
    Point filtered_d = updateKalmanFilter(kf_d, new_result->d);

    return new Rectangle(filtered_a, filtered_b, filtered_c, filtered_d);
}
