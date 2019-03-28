#include <opencv2/opencv.hpp>
#include <fstream>
#include <cstdio>
#include <chrono>
#include <thread>
#include "Clock.h"

#define WINDOW_TITLE_INPUT "Segmentation by color - original image"
#define WINDOW_TITLE_OUTPUT "Result"
#define WINDOW_POSITION_X 80
#define WINDOW_POSITION_Y 20
#define WINDOW_POSITION_Y_OUTPUT (WINDOW_POSITION_Y + 250)
#define COLOR_THRESHOLD 13

void showUsage();
void openInput(cv::String filePath);
void onMouseEvent(int event, int x, int y, int flags, void* userdata);
void segmentImageByPixelColor();
bool isWindowOpen(const char* windowName);

cv::VideoCapture g_videoCapture;
cv::Mat g_imageInput;
cv::Mat g_imageOutput;

cv::Vec3b g_clickedPixelColorRGB;
uint8_t g_clickedPixelColorGray;
int32_t g_videoFrameCount;
int32_t g_windowWidth;
int32_t g_windowHeight;

bool g_isInputImage = false;
bool g_isInputVideo = false;
bool g_isVideoPaused = false;
bool g_isImageGrayscale = false;
bool g_hasClicked = false;
bool g_renderOutput = false;

cv::Point g_mouseClickPoint;

int main(int argc, char** argv)
{
    setvbuf(stdout, nullptr, _IONBF, 0);

    std::vector<cv::String> args(argv, argv + argc);

    if (argc <= 1)
    {
        showUsage();
        return 0;
    }

    if (args[1] == "-h" || args[1] == "--help")
    {
        showUsage();
        return 0;
    }
    else if (args[1] == "cam" || args[1] == "webcam")
    {
        g_videoCapture = cv::VideoCapture(0);
        std::this_thread::sleep_for(std::chrono::seconds(1));

        if (g_videoCapture.isOpened())
        {
            printf("Failed to open webcam for reading.\n");
            return 0;
        }

        g_isInputVideo = true;
        g_videoCapture.grab();
        g_videoCapture.retrieve(g_imageInput);

        int32_t grabAttempts = 0;
        bool grabbed = true;

        while (grabAttempts-- > 0)
        {
            grabbed = g_videoCapture.grab();

            if (grabbed)
                break;

            printf("Failed to grab webcam frame.\n");

            if (grabAttempts > 0)
            {
                printf("Trying again in 1.5 seconds...\n");
                std::this_thread::sleep_for(std::chrono::milliseconds(1500));
            }
        }

        if (!grabbed)
        {
            g_videoCapture.release();
            printf("Failed to read data from the camera.\n");
            return 0;
        }
    }
    else
    {
        std::ifstream inputFile(args[1].c_str());

        if (!inputFile.is_open())
        {
            printf("Couldn't open input file \"%s\".\n", args[1].c_str());
            return 0;
        }

        inputFile.close();

        openInput(args[1]);

        if (!g_isInputImage && !g_isInputVideo)
        {
            printf("Input file \"%s\" wasn't recognized as image or video.\n", args[1].c_str());
            return 0;
        }
    }

    double fps = 0.;
    double fpsInv = 0.;
    double secondsSinceLastDraw = 0.;
    int32_t windowFlags = cv::WINDOW_KEEPRATIO;

    if (g_isInputImage)
    {
        g_windowWidth = g_imageInput.cols;
        g_windowHeight = g_imageInput.rows;
    }
    else if (g_isInputVideo)
    {
        g_windowWidth = g_videoCapture.get(cv::CAP_PROP_FRAME_WIDTH);
        g_windowHeight = g_videoCapture.get(cv::CAP_PROP_FRAME_HEIGHT);
    }

    int32_t maxWindowSize = 1440;

    if (g_windowWidth > maxWindowSize || g_windowHeight > maxWindowSize)
    {
        float ratio = static_cast<float>(std::max(g_windowWidth, g_windowHeight)) / maxWindowSize;
        g_windowWidth = static_cast<int32_t>(g_windowWidth / ratio);
        g_windowHeight = static_cast<int32_t>(g_windowHeight / ratio);
    }

    g_windowWidth = std::max(80, g_windowWidth);
    g_windowHeight = std::max(80, g_windowHeight);

    cv::namedWindow(WINDOW_TITLE_INPUT, windowFlags);
    cv::resizeWindow(WINDOW_TITLE_INPUT, g_windowWidth, g_windowHeight);
    cv::moveWindow(WINDOW_TITLE_INPUT, WINDOW_POSITION_X, WINDOW_POSITION_Y);

    if (g_isInputVideo)
    {
        fps = g_videoCapture.get(cv::CAP_PROP_FPS);
        fpsInv = 1. / 20.;

        if (fps > 0.)
        {
            fpsInv = 1. / fps;
            printf("Video fps: %.3f\n", fps);
        }
    }

    if (g_imageInput.rows > 0 && g_imageInput.cols > 0)
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
                {
                    g_hasClicked = false;
                    cv::destroyWindow(WINDOW_TITLE_OUTPUT);
                }
                else
                    break;
            }

            if (key == ' ')
                g_isVideoPaused = !g_isVideoPaused;
        }

        if (g_isInputVideo)
        {
            if (secondsSinceLastDraw >= fpsInv)
            {
                secondsSinceLastDraw -= fpsInv;
                g_videoCapture.read(g_imageInput);

                if (g_imageInput.rows > 0 && g_imageInput.cols > 0)
                    cv::imshow(WINDOW_TITLE_INPUT, g_imageInput);

                if (g_hasClicked)
                    g_renderOutput = true;
            }
        }

        if (g_renderOutput)
        {
            g_renderOutput = false;

            segmentImageByPixelColor();

            if (!isWindowOpen(WINDOW_TITLE_OUTPUT))
            {
                cv::namedWindow(WINDOW_TITLE_OUTPUT, windowFlags);
                cv::resizeWindow(WINDOW_TITLE_OUTPUT, g_windowWidth, g_windowHeight);
                cv::moveWindow(WINDOW_TITLE_OUTPUT, WINDOW_POSITION_X, WINDOW_POSITION_Y_OUTPUT);
            }

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
    printf("\n");
    printf("Use the mouse to click on any pixel. Every pixel\n");
    printf("that is within a certain threshold will be paint\n");
    printf("with the color red in a new screen that will open.\n");
}

void openInput(cv::String filePath)
{
    g_imageInput = cv::imread(filePath, cv::IMREAD_ANYCOLOR);

    if (g_imageInput.rows > 0 && g_imageInput.cols > 0)
    {
        int32_t channels = g_imageInput.channels();
        printf("Input image has %d channel(s).\n", channels);
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
        if (x < 0 || x >= g_imageInput.cols || y < 0 || y >= g_imageInput.rows)
            return;

        g_mouseClickPoint = cv::Point(x, y);
        g_hasClicked = true;
        g_renderOutput = true;

        if (g_isImageGrayscale)
        {
            g_clickedPixelColorGray = g_imageInput.at<uint8_t>(g_mouseClickPoint);
            printf(
               "Clicked pixel: x/y = %d/%d; Gray Intensity = %d\n",
               g_mouseClickPoint.x, g_mouseClickPoint.y,
               g_clickedPixelColorGray
            );
        }
        else
        {
            g_clickedPixelColorRGB = g_imageInput.at<cv::Vec3b>(g_mouseClickPoint);
            printf(
               "Clicked pixel: x/y = %d/%d; R/G/B = %d/%d/%d\n",
               g_mouseClickPoint.x, g_mouseClickPoint.y,
               g_clickedPixelColorRGB.val[2], g_clickedPixelColorRGB.val[1], g_clickedPixelColorRGB.val[0]
            );
        }
    }
}

void segmentImageByPixelColor()
{
    constexpr int32_t threshold = COLOR_THRESHOLD * COLOR_THRESHOLD;

    g_imageOutput = g_imageInput.clone();

    if (g_isImageGrayscale)
    {
        g_imageOutput.forEach<uint8_t>([&](uint8_t& p, const int position[]) -> void {
            int32_t diff = static_cast<int32_t>(p) - static_cast<int32_t>(g_clickedPixelColorGray);
            diff = diff * diff;
            p = (diff < threshold) * 255;
        });

        cv::Mat channels[3];
        cv::cvtColor(g_imageOutput, g_imageOutput, cv::COLOR_GRAY2BGR, 3);
        cv::split(g_imageOutput, channels);
        channels[0] *= 0;
        channels[1] *= 0;
        cv::merge(channels, 3, g_imageOutput);
    }
    else
    {
        g_imageOutput.forEach<cv::Vec3b>([&](cv::Vec3b& p, const int position[]) -> void {
            int32_t diff0 = static_cast<int32_t>(p.val[0]) - static_cast<int32_t>(g_clickedPixelColorRGB.val[0]);
            int32_t diff1 = static_cast<int32_t>(p.val[1]) - static_cast<int32_t>(g_clickedPixelColorRGB.val[1]);
            int32_t diff2 = static_cast<int32_t>(p.val[2]) - static_cast<int32_t>(g_clickedPixelColorRGB.val[2]);
            diff0 = diff0 * diff0 + diff1 * diff1 + diff2 * diff2;
            p.val[0] = 0;
            p.val[1] = 0;
            p.val[2] = (diff0 < threshold) * 255;
        });
    }
}

bool isWindowOpen(const char* windowName)
{
    return cv::getWindowProperty(windowName, cv::WND_PROP_VISIBLE) != 0.;
}
