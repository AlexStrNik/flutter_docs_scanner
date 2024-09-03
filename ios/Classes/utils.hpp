#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <iostream>
#include <opencv2/opencv.hpp>

namespace flutter {
    struct Plane {
        uint8_t *planeData;
        int length;
        int bytesPerRow;
        Plane *nextPlane = nullptr;
    };

    struct ImageForDetect {
        Plane *plane;
        int platform; // 0 ios, 1 android*
        int width;
        int height;
        int orientation;
    };

    struct Image {
        uint8_t *bytes;
        long long length;
    };

    static cv::Mat rotateMatrixCCW(cv::Mat const &img, unsigned int times) {
        cv::Mat dst;
        times = times % 4;
        switch (times) {
            case 0:
                return img;
                break;
            case 1:
                cv::transpose(img, dst);
                cv::flip(dst, dst, 0);
                break;
            case 2:
                cv::flip(img, dst, -1);
                break;
            case 3:
                cv::transpose(img, dst);
                cv::flip(dst, dst, 1);
            default:
                break;
        }
        return dst;
    }

    static void fixMatOrientation(int orientation, cv::Mat &img) {
        if (orientation != 0) {
            switch (orientation) {
                case 90:
                    img = rotateMatrixCCW(img, 3);
                    break;
                case 180:
                    img = rotateMatrixCCW(img, 2);
                    break;
                case 270:
                    img = rotateMatrixCCW(img, 1);
                    break;
                default:
                    break;
            }
        }
    }

    static cv::Mat prepareMatAndroid(
            uint8_t *plane0,
            int bytesPerRow0,
            uint8_t *plane1,
            int lenght1,
            int bytesPerRow1,
            uint8_t *plane2,
            int lenght2,
            int bytesPerRow2,
            int width,
            int height,
            int orientation) {
        uint8_t *yPixel = plane0;
        uint8_t *uPixel = plane1;
        uint8_t *vPixel = plane2;

        int32_t uLen = lenght1;
        int32_t vLen = lenght2;

        cv::Mat _yuv_rgb_img;
        assert(bytesPerRow0 == bytesPerRow1 && bytesPerRow1 == bytesPerRow2);
        uint8_t *uv = new uint8_t[uLen + vLen];
        memcpy(uv, uPixel, vLen);
        memcpy(uv + uLen, vPixel, vLen);
        cv::Mat mYUV = cv::Mat(height, width, CV_8UC1, yPixel, bytesPerRow0);
        cv::copyMakeBorder(mYUV, mYUV, 0, height >> 1, 0, 0, cv::BORDER_CONSTANT, 0);

        cv::Mat mUV = cv::Mat((height >> 1), width, CV_8UC1, uv, bytesPerRow0);
        cv::Mat dst_roi = mYUV(cv::Rect(0, height, width, height >> 1));
        mUV.copyTo(dst_roi);

        cv::cvtColor(mYUV, _yuv_rgb_img, cv::COLOR_YUV2RGBA_NV21, 3);
        
        // fixMatOrientation(orientation, _yuv_rgb_img);

        return _yuv_rgb_img;
    }

    static cv::Mat prepareMatIos(uint8_t *plane,
                                 int bytesPerRow,
                                 int width,
                                 int height,
                                 int orientation) {
        uint8_t *yPixel = plane;

        cv::Mat mYUV = cv::Mat(height, width, CV_8UC4, yPixel, bytesPerRow);

        fixMatOrientation(orientation, mYUV);

        return mYUV;

    }

    static cv::Mat decodeImage(flutter::Image *image) {
        std::vector<uint8_t> buffer(image->bytes, image->bytes + image->length);
        cv::Mat img = cv::imdecode(buffer, cv::IMREAD_COLOR);
        
        return img;
    }

    static flutter::Image* encodeImage(cv::Mat mat) {
        std::vector<uint8_t> buffer;
        cv::imencode(".jpg", mat, buffer);
        
        uint8_t *bytes = new uint8_t[buffer.size()];
        std::copy(buffer.begin(), buffer.end(), bytes);
        
        auto image = new Image();
        image->bytes = bytes;
        image->length = buffer.size();
        
        return image;
    }

    static cv::Mat prepareMat(flutter::ImageForDetect *image) {
        if (image->platform == 0) {
            auto *plane = image->plane;
            return flutter::prepareMatIos(plane->planeData,
                                          plane->bytesPerRow,
                                          image->width,
                                          image->height,
                                          image->orientation);
        }
        if (image->platform == 1) {
            auto *plane0 = image->plane;
            auto *plane1 = plane0->nextPlane;
            auto *plane2 = plane1->nextPlane;
            return flutter::prepareMatAndroid(plane0->planeData,
                                              plane0->bytesPerRow,
                                              plane1->planeData,
                                              plane1->length,
                                              plane1->bytesPerRow,
                                              plane2->planeData,
                                              plane2->length,
                                              plane2->bytesPerRow,
                                              image->width,
                                              image->height,
                                              image->orientation);
        }
        throw "Can't parse image data due to the unknown platform";
    }
}
