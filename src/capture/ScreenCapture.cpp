#include "ScreenCapture.h"
#include "../utils/Logger.h"
#include <cstring>

#ifdef __linux__
namespace {
    struct X11ErrorState {
        bool triggered = false;
        int code = 0;
        char message[256] = {0};
    };

    thread_local X11ErrorState g_x11_error_state;

    int ScreenCaptureXErrorHandler(Display* display, XErrorEvent* error_event) {
        g_x11_error_state.triggered = true;
        g_x11_error_state.code = error_event->error_code;
        if (display) {
            XGetErrorText(display, error_event->error_code, g_x11_error_state.message, sizeof(g_x11_error_state.message));
        }
        return 0;
    }
}
#elif defined(_WIN32)
    #include <windows.h>
#endif

ScreenCapture::ScreenCapture() 
    : initialized_(false),
    last_error_("Not initialized")
#ifdef __linux__
    , display_(nullptr)
    , root_window_(0)
    , screen_number_(0)
#elif defined(_WIN32)
    , hdc_screen_(nullptr)
    , hdc_mem_(nullptr)
    , hbitmap_(nullptr)
#endif
{
}

ScreenCapture::~ScreenCapture() {
#ifdef __linux__
    if (display_) {
        XCloseDisplay(display_);
        display_ = nullptr;
        root_window_ = 0;
    }
#elif defined(_WIN32)
    if (hbitmap_) {
        DeleteObject(hbitmap_);
        hbitmap_ = nullptr;
    }
    if (hdc_mem_) {
        DeleteDC(static_cast<HDC>(hdc_mem_));
        hdc_mem_ = nullptr;
    }
    if (hdc_screen_) {
        ReleaseDC(nullptr, static_cast<HDC>(hdc_screen_));
        hdc_screen_ = nullptr;
    }
#endif
}

bool ScreenCapture::init() {
#ifdef __linux__
    // Open connection to X server
    display_ = XOpenDisplay(nullptr);
    if (!display_) {
        last_error_ = "Failed to open X display";
        Logger::log(Logger::LogLevel::ERROR_LEVEL, last_error_);
        return false;
    }

    screen_number_ = DefaultScreen(display_);
    root_window_ = RootWindow(display_, screen_number_);

    Logger::log(Logger::LogLevel::INFO, "X11 screen capture initialized");
    initialized_ = true;
    return true;

#elif defined(_WIN32)
    // Get screen DC
    hdc_screen_ = GetDC(nullptr);
    if (!hdc_screen_) {
        last_error_ = "Failed to get screen DC";
        Logger::log(Logger::LogLevel::ERROR_LEVEL, last_error_);
        return false;
    }

    // Create compatible DC
    hdc_mem_ = CreateCompatibleDC(static_cast<HDC>(hdc_screen_));
    if (!hdc_mem_) {
        last_error_ = "Failed to create compatible DC";
        Logger::log(Logger::LogLevel::ERROR_LEVEL, last_error_);
        ReleaseDC(nullptr, static_cast<HDC>(hdc_screen_));
        hdc_screen_ = nullptr;
        return false;
    }

    Logger::log(Logger::LogLevel::INFO, "Windows GDI screen capture initialized");
    initialized_ = true;
    return true;

#else
    last_error_ = "Screen capture not implemented for this platform";
    Logger::log(Logger::LogLevel::ERROR_LEVEL, last_error_);
    return false;
#endif
}

bool ScreenCapture::getScreenDimensions(int& width, int& height) {
    if (!initialized_) {
        last_error_ = "Screen capture not initialized";
        return false;
    }

#ifdef __linux__
    Screen* screen = ScreenOfDisplay(display_, screen_number_);
    width = WidthOfScreen(screen);
    height = HeightOfScreen(screen);
    return true;

#elif defined(_WIN32)
    width = GetSystemMetrics(SM_CXSCREEN);
    height = GetSystemMetrics(SM_CYSCREEN);
    return true;

#else
    return false;
#endif
}

std::vector<uint8_t> ScreenCapture::captureScreen(int& width, int& height) {
    if (!getScreenDimensions(width, height)) {
        return std::vector<uint8_t>();
    }

    return captureRegion(0, 0, width, height);
}

std::vector<uint8_t> ScreenCapture::captureRegion(int x, int y, int width, int height) {
    if (!initialized_) {
        last_error_ = "Screen capture not initialized";
        return std::vector<uint8_t>();
    }

#ifdef __linux__
    Display* display = display_;
    Window root = root_window_;

    // Validate and clamp capture region to screen bounds
    int screen_width, screen_height;
    if (!getScreenDimensions(screen_width, screen_height)) {
        last_error_ = "Failed to get screen dimensions";
        return std::vector<uint8_t>();
    }

    // Debug log
    static bool first_capture = true;
    if (first_capture) {
        std::string msg = "Screen dimensions: " + std::to_string(screen_width) + "x" + std::to_string(screen_height);
        Logger::log(Logger::LogLevel::INFO, msg);
        msg = "Capture request: x=" + std::to_string(x) + " y=" + std::to_string(y) + 
              " w=" + std::to_string(width) + " h=" + std::to_string(height);
        Logger::log(Logger::LogLevel::INFO, msg);
        first_capture = false;
    }

    // Clamp coordinates and dimensions
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= screen_width) x = screen_width - 1;
    if (y >= screen_height) y = screen_height - 1;
    if (x + width > screen_width) width = screen_width - x;
    if (y + height > screen_height) height = screen_height - y;
    
    if (width <= 0 || height <= 0) {
        last_error_ = "Invalid capture dimensions after clamping: " + std::to_string(width) + "x" + std::to_string(height);
        Logger::log(Logger::LogLevel::ERROR_LEVEL, last_error_);
        return std::vector<uint8_t>();
    }

    // Get root window attributes to verify
    XWindowAttributes attrs;
    if (!XGetWindowAttributes(display, root, &attrs)) {
        last_error_ = "Failed to get window attributes";
        Logger::log(Logger::LogLevel::ERROR_LEVEL, last_error_);
        return std::vector<uint8_t>();
    }

    // Debug: Log window attributes
    static bool logged_attrs = false;
    if (!logged_attrs) {
        std::string msg = "Window attributes - width: " + std::to_string(attrs.width) + 
                         " height: " + std::to_string(attrs.height) +
                         " depth: " + std::to_string(attrs.depth) +
                         " visual: " + std::to_string(reinterpret_cast<unsigned long>(attrs.visual));
        Logger::log(Logger::LogLevel::INFO, msg);
        logged_attrs = true;
    }

    // Make sure we don't exceed the actual window size
    if (x + width > attrs.width) width = attrs.width - x;
    if (y + height > attrs.height) height = attrs.height - y;

    // Synchronize with X server to ensure all pending operations are complete
    XSync(display, False);

    // Capture the screen using XGetImage with error trapping to avoid fatal X errors
    g_x11_error_state = X11ErrorState{};
    auto previous_handler = XSetErrorHandler(ScreenCaptureXErrorHandler);

    unsigned long plane_mask = AllPlanes;
    XImage* image = XGetImage(display, root, x, y, static_cast<unsigned int>(width), static_cast<unsigned int>(height), plane_mask, ZPixmap);

    // Force synchronization so that any X errors are delivered now
    XSync(display, False);

    // Restore original handler
    XSetErrorHandler(previous_handler);

    if (!image || g_x11_error_state.triggered) {
        if (image) {
            XDestroyImage(image);
        }

        if (g_x11_error_state.triggered) {
            last_error_ = std::string("XGetImage failed: ") + g_x11_error_state.message;
            if (g_x11_error_state.code == BadMatch) {
                last_error_ += " (BadMatch indicates the compositor denied direct screen capture. On Wayland sessions, X11 capture is not permitted.)";
            }
        } else {
            last_error_ = "XGetImage returned null image";
        }

        Logger::log(Logger::LogLevel::WARN, last_error_);
        return std::vector<uint8_t>();
    }

    // Allocate buffer for ARGB8888 format
    std::vector<uint8_t> pixels(width * height * 4);

    // Convert XImage data to ARGB8888
    for (int row = 0; row < height; ++row) {
        for (int col = 0; col < width; ++col) {
            unsigned long pixel = XGetPixel(image, col, row);
            
            // Extract RGB components (X11 typically uses BGR or RGB depending on display)
            // We'll assume 24-bit or 32-bit color depth
            uint8_t r = (pixel >> 16) & 0xFF;
            uint8_t g = (pixel >> 8) & 0xFF;
            uint8_t b = pixel & 0xFF;
            uint8_t a = 0xFF; // Fully opaque

            // Store as ARGB8888
            int idx = (row * width + col) * 4;
            pixels[idx + 0] = a;  // A
            pixels[idx + 1] = r;  // R
            pixels[idx + 2] = g;  // G
            pixels[idx + 3] = b;  // B
        }
    }

    XDestroyImage(image);
    return pixels;

#elif defined(_WIN32)
    HDC hdc_screen = static_cast<HDC>(hdc_screen_);
    HDC hdc_mem = static_cast<HDC>(hdc_mem_);

    // Create bitmap
    HBITMAP hbitmap = CreateCompatibleBitmap(hdc_screen, width, height);
    if (!hbitmap) {
        last_error_ = "Failed to create compatible bitmap";
        Logger::log(Logger::LogLevel::ERROR_LEVEL, last_error_);
        return std::vector<uint8_t>();
    }

    // Select bitmap into memory DC
    HGDIOBJ old_bitmap = SelectObject(hdc_mem, hbitmap);

    // Copy screen to memory DC
    if (!BitBlt(hdc_mem, 0, 0, width, height, hdc_screen, x, y, SRCCOPY)) {
        last_error_ = "BitBlt failed";
        Logger::log(Logger::LogLevel::ERROR_LEVEL, last_error_);
        SelectObject(hdc_mem, old_bitmap);
        DeleteObject(hbitmap);
        return std::vector<uint8_t>();
    }

    // Get bitmap bits
    BITMAPINFOHEADER bi;
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = width;
    bi.biHeight = -height; // Top-down
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

    std::vector<uint8_t> pixels(width * height * 4);
    
    if (!GetDIBits(hdc_mem, hbitmap, 0, height, pixels.data(), 
                   reinterpret_cast<BITMAPINFO*>(&bi), DIB_RGB_COLORS)) {
        last_error_ = "GetDIBits failed";
        Logger::log(Logger::LogLevel::ERROR_LEVEL, last_error_);
        SelectObject(hdc_mem, old_bitmap);
        DeleteObject(hbitmap);
        return std::vector<uint8_t>();
    }

    // Windows gives us BGRA, convert to ARGB
    for (int i = 0; i < width * height; ++i) {
        int idx = i * 4;
        uint8_t b = pixels[idx + 0];
        uint8_t g = pixels[idx + 1];
        uint8_t r = pixels[idx + 2];
        uint8_t a = pixels[idx + 3];
        
        pixels[idx + 0] = a;
        pixels[idx + 1] = r;
        pixels[idx + 2] = g;
        pixels[idx + 3] = b;
    }

    SelectObject(hdc_mem, old_bitmap);
    DeleteObject(hbitmap);
    return pixels;

#else
    last_error_ = "Screen capture not implemented for this platform";
    return std::vector<uint8_t>();
#endif
}
