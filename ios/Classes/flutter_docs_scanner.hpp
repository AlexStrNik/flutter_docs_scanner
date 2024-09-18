#include <opencv2/opencv.hpp>

class Scanner {
public:
    struct Point {
        float x;
        float y;

        Point(float x, float y) {
            this->x = x;
            this->y = y;
        }
    };

    struct Rectangle {
        Point a;
        Point b;
        Point c;
        Point d;
        
        Rectangle(Point a, Point b, Point c, Point d): a(a), b(b), c(c), d(d) {}
    };

    Scanner();

    std::vector<cv::Point> detectDoc(const cv::Mat &frame);

    cv::Mat processDoc(const cv::Mat &image);
    
    Rectangle* filterResults(Rectangle* new_result);
    
private:
    std::deque<Rectangle> history;
    static const int HISTORY_SIZE = 5;
    
    cv::KalmanFilter kf_a, kf_b, kf_c, kf_d;

    void initKalmanFilters() {
        initKalmanFilter(kf_a);
        initKalmanFilter(kf_b);
        initKalmanFilter(kf_c);
        initKalmanFilter(kf_d);
    }

    void initKalmanFilter(cv::KalmanFilter &kf) {
        kf.init(4, 2, 0);
        kf.transitionMatrix = (cv::Mat_<float>(4, 4) << 1, 0, 1, 0,
                                                    0, 1, 0, 1,
                                                    0, 0, 1, 0,
                                                    0, 0, 0, 1);

        cv::setIdentity(kf.measurementMatrix);
        kf.processNoiseCov = (cv::Mat_<float>(4, 4) << 1e-2, 0,    0,    0,
                                                    0,    1e-2, 0,    0,
                                                    0,    0,    1e-2, 0,
                                                    0,    0,    0,    1e-2);

        kf.measurementNoiseCov = (cv::Mat_<float>(2, 2) << 1e-1, 0,
                                                       0,    1e-1);
        cv::setIdentity(kf.errorCovPost, cv::Scalar::all(1));
    }

    Point updateKalmanFilter(cv::KalmanFilter &kf, const Point &measurement) {
        cv::Mat prediction = kf.predict();
        cv::Mat estimated = kf.correct((cv::Mat_<float>(2, 1) << measurement.x, measurement.y));
        return Point(estimated.at<float>(0), estimated.at<float>(1));
    }
};
