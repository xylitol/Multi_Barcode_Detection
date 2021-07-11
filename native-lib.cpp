
#include <string>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <iostream>
#include <jni.h>

using namespace std;
using namespace cv;
class BarcodeDetector {

private:

    Mat _getGrayImageFromRGB(Mat rgbImg);
    Mat _sobelFromGrayImg(Mat grayImg);
    Mat _blurWithMorphologyFromSobel(Mat sobelImg);
    vector<vector<Point>> _getContoursFromPreprocessedImg(Mat processedImg);

public:
    BarcodeDetector() {}

    vector<vector<Point>> detection(jlong matAddrInput);

};

vector<vector<Point>> BarcodeDetector::detection(jlong matAddrInput) {
    Mat &matInput = *(Mat *) matAddrInput; // getMatFromJLong
    Mat result = matInput.clone();

    resize(matInput,result,Size(160,120),INTER_NEAREST);
    result = _getGrayImageFromRGB(result);
    result = _sobelFromGrayImg(result);
    result = _blurWithMorphologyFromSobel(result);
    vector<vector<Point>> contours = _getContoursFromPreprocessedImg(result);

    return contours;
}


Mat BarcodeDetector::_getGrayImageFromRGB(Mat rgbImg) {
    Mat result = rgbImg;

    cvtColor(rgbImg, result, COLOR_RGBA2GRAY);
    Laplacian(result, result, CV_8U, 3);
    threshold(result, result, 100, 255, THRESH_BINARY + THRESH_OTSU);

    return result;
}

Mat BarcodeDetector::_sobelFromGrayImg(Mat grayImg) {
    Mat sobelX;
    Mat sobelY;
    Sobel(grayImg, sobelX, CV_32F, 1, 0);
    Sobel(grayImg, sobelY, CV_32F, 0, 1);

    return abs(sobelX - sobelY);
}

Mat BarcodeDetector::_blurWithMorphologyFromSobel(Mat sobelImg) {

    Mat result = sobelImg;

    int mask1[3][3] = {
            {-2,4,-2},
            {-2,4,-2},
            {-2,4,-2}
    };

    int mask2[3][3] = {
            {-2,-2,-2},
            {4,4,4},
            {-2,-2,-2}

    };

    int mask3[6][2];
    fill(&mask3[0][0], &mask3[5][2], 1.);

    Mat kernel1 = Mat(3, 3, CV_8UC1, mask1);
    Mat kernel2 = Mat(3, 3, CV_8UC1, mask2);
    Mat kernel3 = Mat(6, 2, CV_32SC1, mask3);

    medianBlur(result, result, 3);
    medianBlur(result, result, 3);

    erode(result, result, kernel3, Point(-1, -1), 1);
    dilate(result, result, kernel2, Point(-1, -1), 2);
    dilate(result, result, kernel1, Point(-1, -1), 1);

    return result;
}

vector<vector<Point>> BarcodeDetector::_getContoursFromPreprocessedImg(Mat processedImg) {

    processedImg.convertTo(processedImg, CV_8UC1);

    vector<vector<Point>> contours;
    vector<Vec4i> hierarchy;
    findContours(processedImg, contours, hierarchy, RETR_LIST, CHAIN_APPROX_SIMPLE, Point(0, 0));

    return contours;

}


extern "C"
JNIEXPORT jintArray JNICALL
Java_com_sps_zbar_ZBarScannerActivity_BarcodeDetector(JNIEnv *env, jobject thiz,
                                                      jlong mat_addr_input) {
    Mat &matInput = *(Mat *) mat_addr_input;
    BarcodeDetector detector_instance = *new BarcodeDetector();
    vector<vector<Point>> contours = detector_instance.detection(mat_addr_input);

    vector<int> vec;
    Rect r;
    int padding = 5;
    for (int cun = 0; cun<contours.size(); cun++) {
        r = boundingRect(contours[cun]);
        if (r.height < 5) continue;
        if (r.width < 25) continue;

        if(r.x - padding > 0)
            r.x -= padding;
        else
            r.x = 0;

        if(r.x + r.width + 2 * padding < 160)
            r.width += 2 * padding;
        else
            r.width = 160 - r.x;


        vec.push_back(r.x*5);
        vec.push_back(r.y*5);
        vec.push_back(r.width*5);
        vec.push_back(r.height*5);
//        rectangle(matInput, Point(r.x*5, r.y*5),
//                Point(r.x*5 + r.width*5, r.y*5 + r.height*5),
//                Scalar(0, 255, 0),5);

    }


    int size = vec.size();
    jintArray result = env->NewIntArray(size);
    if (result == NULL) {
        return NULL; // out of memory error thrown /
    }

    jint fill[size];
    for (int i = 0; i < size; i++) {
        fill[i] = vec[i]; // put whatever logic you want to populate the values here.
    }
    env->SetIntArrayRegion( result, 0, size, fill);

    return result;
}


