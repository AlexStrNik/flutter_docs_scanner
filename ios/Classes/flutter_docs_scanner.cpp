#include "flutter_docs_scanner.hpp"
#include "utils.hpp"

using namespace cv;
using namespace std;

#define EXPORT extern "C" __attribute__((visibility("default"))) __attribute__((used))


EXPORT Scanner::Rectangle *processFrame(Scanner *scanner, flutter::ImageForDetect *frame) {
    auto img = flutter::prepareMat(frame);
    auto rect = scanner->detectDoc(img);
    
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

void improve_image(Mat srcArry, Mat &dstArry) {
//    cv::cvtColor(srcArry, dstArry, cv::COLOR_BGR2YCrCb);
//
//    std::vector<cv::Mat> channels;
//    cv::split(dstArry, channels);
//
//    cv::equalizeHist(channels[0], channels[0]);
//
//    cv::merge(channels, dstArry);
//
//    cv::Mat result;
//    cv::cvtColor(dstArry, result, cv::COLOR_YCrCb2BGR);
    dstArry = srcArry;
}

cv::Mat Scanner::processDoc(const cv::Mat &img) {
    Mat gray;
    cvtColor(img, gray, COLOR_BGR2GRAY);

    Mat thresh;
    adaptiveThreshold(gray, thresh, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, 11, 2);

    GaussianBlur(thresh, thresh, Size(5, 5), 0);

    Mat edges;
    Canny(thresh, edges, 50, 150);

    vector<vector<cv::Point>> contours;
    findContours(edges, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

    if (contours.empty()) {
        return img;
    }

    vector<cv::Point> largest_contour;
    double max_area = 0;
    for (const auto& contour : contours) {
        double area = contourArea(contour);
        if (area > max_area) {
            max_area = area;
            largest_contour = contour;
        }
    }

    vector<cv::Point> approx;
    double peri = arcLength(largest_contour, true);
    approxPolyDP(largest_contour, approx, 0.02 * peri, true);

    if (approx.size() != 4) {
        return img;
    }

    if (max_area < 1000) {
        return img;
    }

    Rect bounding_box = boundingRect(approx);
    float aspect_ratio = static_cast<float>(bounding_box.width) / bounding_box.height;
    if (aspect_ratio < 0.5 || aspect_ratio > 2.0) {
        return img;
    }
    
    // Sorting the corners and converting them to desired shape.
    auto corners = order_points(approx);
    
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

Scanner::Rectangle* Scanner::detectDoc(const Mat& frame) {
    Mat gray;
    cvtColor(frame, gray, COLOR_BGR2GRAY);

    Mat thresh;
    adaptiveThreshold(gray, thresh, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, 11, 2);

    GaussianBlur(thresh, thresh, Size(5, 5), 0);

    Mat edges;
    Canny(thresh, edges, 50, 150);

    vector<vector<cv::Point>> contours;
    findContours(edges, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

    if (contours.empty()) {
        return nullptr;
    }

    vector<cv::Point> largest_contour;
    double max_area = 0;
    for (const auto& contour : contours) {
        double area = contourArea(contour);
        if (area > max_area) {
            max_area = area;
            largest_contour = contour;
        }
    }
    
    if (largest_contour.empty()) {
        return nullptr;
    }

    vector<cv::Point> approx;
    double peri = arcLength(largest_contour, true);
    approxPolyDP(largest_contour, approx, 0.05 * peri, true);

    if (approx.size() != 4) {
        return nullptr;
    }

    if (max_area < 1000) {
        return nullptr;
    }

    Rect bounding_box = boundingRect(approx);
    float aspect_ratio = static_cast<float>(bounding_box.width) / bounding_box.height;
    if (aspect_ratio < 0.5 || aspect_ratio > 2.0) {
        return nullptr;
    }

    auto corners = order_points(approx);

    vector<Point> normalized_points;
    normalized_points.reserve(4);
    for (const auto& point : corners) {
        normalized_points.emplace_back(
            Point(
                float(point.x) / float(frame.cols),
                float(point.y) / float(frame.rows)
            )
        );
    }

    if (normalized_points.size() != 4) {
        return nullptr;
    }

    return new Rectangle(normalized_points[0], normalized_points[1], 
                         normalized_points[2], normalized_points[3]);
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
