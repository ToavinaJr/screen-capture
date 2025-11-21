#ifndef SCREENCAPTURE_H
#define SCREENCAPTURE_H

#include <vector>
#include <cstdint>
#include <string>

#ifdef __linux__
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

/**
 * Cross-platform screen capture utility
 * Captures the entire primary display or a specific region
 */
class ScreenCapture {
public:
    ScreenCapture();
    ~ScreenCapture();

    /**
     * Initialize the screen capture system
     * @return true if successful, false otherwise
     */
    bool init();

    /**
     * Capture the entire primary screen
     * @param width Output parameter for screen width
     * @param height Output parameter for screen height
     * @return Vector containing ARGB8888 pixel data (4 bytes per pixel)
     *         Empty vector on failure
     */
    std::vector<uint8_t> captureScreen(int& width, int& height);

    /**
     * Capture a specific region of the screen
     * @param x X coordinate of top-left corner
     * @param y Y coordinate of top-left corner
     * @param width Width of region to capture
     * @param height Height of region to capture
     * @return Vector containing ARGB8888 pixel data (4 bytes per pixel)
     *         Empty vector on failure
     */
    std::vector<uint8_t> captureRegion(int x, int y, int width, int height);

    /**
     * Get the primary screen dimensions
     * @param width Output parameter for screen width
     * @param height Output parameter for screen height
     * @return true if successful, false otherwise
     */
    bool getScreenDimensions(int& width, int& height);

    /**
     * Check if the capture system is initialized
     * @return true if initialized, false otherwise
     */
    bool isInitialized() const { return initialized_; }

    /**
     * Get the last error message
     * @return Error message string
     */
    std::string getLastError() const { return last_error_; }

private:
    bool initialized_;
    std::string last_error_;

#ifdef __linux__
    Display* display_;
    Window root_window_;
    int screen_number_;
#elif defined(_WIN32)
    void* hdc_screen_;  // HDC for screen
    void* hdc_mem_;     // HDC for memory
    void* hbitmap_;     // HBITMAP for capture
#endif
};

#endif // SCREENCAPTURE_H
