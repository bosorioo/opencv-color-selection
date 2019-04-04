## Simple image segmentation based on color

# Dependencies

- OpenCV 3.2 (or any compatible version)
- G++ (must be able to compile c++11)
- pkg-config

# Compiling

To compile this software, use the Makefile provided:

```make```

The Makefile assumes the device has all required dependencies.
A binary will be generated with the name ```project1```.

# Usage

- ```./project1 <input image>``` to open an image file
- ```./project1 <input video>``` to open a video file
- ```./project1 cam``` to use the camera of the device
- ```./project1 --help``` to display the usage

When running, a window with the contents of the input (either video, image or camera) will open.
The user must use the mouse and left-click any point in this window.
After left clicking somewhere, the pixel value at the moment of the click will be captured and displayed in the console.
Another window will open containing every pixel within a color difference threshold painted as red.

Press ESC to close the output display window (if open) or the input display window.

[Github page](https://github.com/bosorioo/opencv-color-selection)
