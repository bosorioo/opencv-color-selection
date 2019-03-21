#include <opencv2/opencv.hpp>
#include <fstream>
#include <cstdio>
#include "Clock.h"

#define WINDOW_TITLE_INPUT "Segmentation by color - original image"
#define WINDOW_TITLE_OUTPUT "Result"
#define COLOR_THRESHOLD 13

void showUsage();
void openInput(cv::String filePath);
void onMouseEvent(int event, int x, int y, int flags, void* userdata);
void makePixelSelection();
bool isWindowOpen(const char* windowName);

cv::VideoCapture g_videoCapture;
cv::Mat g_imageInput;
cv::Mat g_imageOutput;

int32_t g_videoFrameCount;

bool g_isInputImage = false;
bool g_isInputVideo = false;
bool g_isVideoPaused = false;
bool g_isImageGrayscale = false;
bool g_hasClicked = false;

cv::Point g_mouseClickPoint;

int main(int argc, char** argv)
{
    setvbuf(stdout, nullptr, _IONBF, 0);
    cv::String firstArg;

    if (argc <= 1)
    {
        showUsage();
        return 0;
    }

    firstArg = argv[1];

    if (firstArg == "-h" || firstArg == "--help")
    {
        showUsage();
        return 0;
    }
    else if (firstArg == "cam" || firstArg == "webcam")
    {
        g_videoCapture.open(0);

        if (!g_videoCapture.isOpened())
        {
            printf("Failed to open webcam for reading.\n");
            return 0;
        }
    }
    else
    {
        std::ifstream inputFile(firstArg.c_str());

        if (!inputFile.is_open())
        {
            printf("Couldn't open input file \"%s\".\n", firstArg.c_str());
            return 0;
        }

        inputFile.close();

        openInput(firstArg);

        if (!g_isInputImage && !g_isInputVideo)
        {
            printf("Input file \"%s\" wasn't recognized as image or video.\n", firstArg.c_str());
            return 0;
        }
    }

    double fps = 0.;
    double fpsInv = 0.;
    double secondsSinceLastDraw = 0.;
    int32_t windowFlags = cv::WINDOW_FULLSCREEN;

    cv::namedWindow(WINDOW_TITLE_INPUT, windowFlags);

    if (g_isInputVideo)
    {
        fps = g_videoCapture.get(cv::CAP_PROP_FPS);
        fpsInv = 1. / 20.;

        if (fps >= 0.)
            fpsInv = 1. / fps;

        printf("Video fps: %.3f\n", fps);
    }

    cv::imshow(WINDOW_TITLE_INPUT, g_imageInput);
    cv::setMouseCallback(WINDOW_TITLE_INPUT, onMouseEvent);

    Clock clock;

    while (true)
    {
        int32_t delay = 33;

        if (g_isInputVideo)
        {
            float delta = clock.restart();
            if (!g_isVideoPaused)
                secondsSinceLastDraw += delta;
            delay = static_cast<int32_t>((fpsInv - secondsSinceLastDraw) * 1000.f) >> 1;
        }

        if (delay > 0)
        {
            int32_t key = cv::waitKey(std::max(1, delay));

            if (key == 'q' || key == 27)
            {
                if (isWindowOpen(WINDOW_TITLE_OUTPUT))
                    cv::destroyWindow(WINDOW_TITLE_OUTPUT);
                else
                    break;
            }

            if (key == ' ')
                g_isVideoPaused = !g_isVideoPaused;
        }

        if (g_isInputVideo && secondsSinceLastDraw >= fpsInv)
        {
            secondsSinceLastDraw -= fpsInv;
            g_videoCapture.read(g_imageInput);
            cv::imshow(WINDOW_TITLE_INPUT, g_imageInput);
        }

        if (g_hasClicked)
        {
            g_hasClicked = false;
            makePixelSelection();

            if (!isWindowOpen(WINDOW_TITLE_OUTPUT))
                cv::namedWindow(WINDOW_TITLE_OUTPUT, windowFlags);

            cv::imshow(WINDOW_TITLE_OUTPUT, g_imageOutput);
        }
    }

    cv::destroyAllWindows();

    if (g_videoCapture.isOpened())
        g_videoCapture.release();

    return 0;
}

void showUsage()
{
    printf("Usage:\n");
    printf(" ./project1 <image or video file path>\n");
    printf(" ./project1 webcam\n");
}

void openInput(cv::String filePath)
{
    g_imageInput = cv::imread(filePath, cv::IMREAD_ANYCOLOR);

    if (g_imageInput.rows > 0 && g_imageInput.cols > 0)
    {
        int32_t channels = g_imageInput.channels();
        printf("Input image has %d channels.\n", channels);
        g_isInputImage = true;
        g_isImageGrayscale = channels <= 2;
        return;
    }

    g_videoCapture.open(filePath);

    if (!g_videoCapture.isOpened())
        return;

    g_videoCapture.read(g_imageInput);

    g_videoFrameCount = g_videoCapture.get(cv::CAP_PROP_FRAME_COUNT);

    if (g_videoFrameCount > 0)
    {
        g_isInputVideo = true;
    }
    else
    {
        g_videoCapture.release();
    }
}

void onMouseEvent(int32_t event, int32_t x, int32_t y, int32_t flags, void* userdata)
{
    if (event == cv::EVENT_LBUTTONDOWN)
    {
        g_mouseClickPoint = cv::Point(x, y);
        g_hasClicked = true;
    }
}

void makePixelSelection()
{
    constexpr int32_t threshold = COLOR_THRESHOLD * COLOR_THRESHOLD;
    cv::Vec3b pixelColorRGB;

    g_imageOutput = g_imageInput.clone();

    if (g_isImageGrayscale)
    {
        uint8_t pixelColorGray = g_imageInput.at<uint8_t>(g_mouseClickPoint);
        printf(
               "Clicked pixel: x/y = %d/%d; Gray Intensity = %d\n",
               g_mouseClickPoint.x, g_mouseClickPoint.y,
               pixelColorGray
        );
        cv::cvtColor(g_imageOutput, g_imageOutput, cv::COLOR_GRAY2RGB);
        pixelColorRGB = cv::Vec3b(pixelColorGray, pixelColorGray, pixelColorGray);
    }
    else
    {
        pixelColorRGB = g_imageInput.at<cv::Vec3b>(g_mouseClickPoint);
        printf(
               "Clicked pixel: x/y = %d/%d; R/G/B = %d/%d/%d\n",
               g_mouseClickPoint.x, g_mouseClickPoint.y,
               pixelColorRGB.val[2], pixelColorRGB.val[1], pixelColorRGB.val[0]
        );
    }

    g_imageOutput.forEach<cv::Vec3b>([&](cv::Vec3b& p, const int position[]) -> void {
        int32_t diff0 = static_cast<int32_t>(p.val[0]) - static_cast<int32_t>(pixelColorRGB.val[0]);
        int32_t diff1 = static_cast<int32_t>(p.val[1]) - static_cast<int32_t>(pixelColorRGB.val[1]);
        int32_t diff2 = static_cast<int32_t>(p.val[2]) - static_cast<int32_t>(pixelColorRGB.val[2]);
        diff0 = diff0 * diff0 + diff1 * diff1 + diff2 * diff2;
        p.val[0] = 0;
        p.val[1] = 0;
        p.val[2] = (diff0 < threshold) * 255;
    });
}

bool isWindowOpen(const char* windowName)
{
    return cv::getWindowProperty(windowName, cv::WND_PROP_VISIBLE) != 0.;
}
