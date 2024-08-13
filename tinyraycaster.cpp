#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <cassert>

uint32_t pack_color(const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a=255) {
    return (a<<24) + (b<<16) + (g<<8) + r;
}

void unpack_color(const uint32_t &color, uint8_t &r, uint8_t &g, uint8_t &b, uint8_t &a) {
    r = (color >>  0) & 255;
    g = (color >>  8) & 255;
    b = (color >> 16) & 255;
    a = (color >> 24) & 255;
}

// save the framebuffer into a file.
void drop_ppm_image(const std::string filename, const std::vector<uint32_t> &image, const size_t w, const size_t h) {
    // assert the format of the file.
    assert(image.size() == w*h);
    // ofstream write files. 
    std::ofstream ofs(filename);
    // for png format, the header goes:
    // First line: P3(plain text data)/P6 (binary data)
    // Second line: width " " height
    // Third line: Maximum color value. In this case 255, 
    // when we use 8 bit to store color per channel. 
    ofs << "P6\n" << w << " " << h << "\n255\n";
    // After the header, each pixel is stored. 
    // although each color use 4 byte = 32 bits to store r,g,b,a,
    // only r,g,b are written in the file. 
    // in each cycle, unpack color converts each pixel into r,g,b,a.
    // note that image is the framebuffer in main, storing colors in format of colors.
    // the actual image stores colors in r,g,b.
    for (size_t i = 0; i < h*w; ++i) {
        uint8_t r, g, b, a;
        unpack_color(image[i], r, g, b, a);
        ofs << static_cast<char>(r) << static_cast<char>(g) << static_cast<char>(b);
    }
    ofs.close();
}

void draw_rectangle(std::vector<uint32_t> &img, const size_t img_w, const size_t img_h, const size_t x, const size_t y, const size_t w, const size_t h, const uint32_t color) {
    assert(img.size()==img_w*img_h);
    for (size_t i=0; i<w; i++) {
        for (size_t j=0; j<h; j++) {
            size_t cx = x+i;
            size_t cy = y+j;
            assert(cx<img_w && cy<img_h);
            img[cx + cy*img_w] = color;
        }
    }
}

int main() {
    const size_t win_w = 512; // image width
    const size_t win_h = 512; // image height
    // the constructor format is std::vector(size_t count, const T& value);
    // first argument specifies the number of pixels. 
    // second argument uint32_t, set to 255, which means 3 bytes (RGB) are 0, and A is 255. 
    std::vector<uint32_t> framebuffer(win_w*win_h, 255); // the image itself, initialized to white

    const size_t map_w = 16; // map width
    const size_t map_h = 16; // map height
    const char map[] = "0000222222220000"\
                       "1              0"\
                       "1      11111   0"\
                       "1     0        0"\
                       "0     0  1110000"\
                       "0     3        0"\
                       "0   10000      0"\
                       "0   0   11100  0"\
                       "0   0   0      0"\
                       "0   0   1  00000"\
                       "0       1      0"\
                       "2       1      0"\
                       "0       0      0"\
                       "0 0000000      0"\
                       "0              0"\
                       "0002222222200000"; // our game map
    assert(sizeof(map) == map_w*map_h+1); // +1 for the null terminated string, 
    // because strings end with '\0'.Each row has map_h+1 columns.

    for (size_t j = 0; j<win_h; j++) { // fill the screen with color gradients
        for (size_t i = 0; i<win_w; i++) {
            uint8_t r = 255*j/float(win_h); // varies between 0 and 255 as j sweeps the vertical
            uint8_t g = 255*i/float(win_w); // varies between 0 and 255 as i sweeps the horizontal
            uint8_t b = 0;
            framebuffer[i+j*win_w] = pack_color(r, g, b);
        }
    }

    const size_t rect_w = win_w/map_w; // the width of each map block
    const size_t rect_h = win_h/map_h; // the height of each map block
    for (size_t j=0; j<map_h; j++) { // draw the map
        for (size_t i=0; i<map_w; i++) {
            if (map[i+j*map_w]==' ') continue; // skip empty spaces
            size_t rect_x = i*rect_w;
            size_t rect_y = j*rect_h;
            draw_rectangle(framebuffer, win_w, win_h, rect_x, rect_y, rect_w, rect_h, pack_color(0, 255, 255));
        }
    }

    std::cout << "started drop_ppm" << std::endl;
    drop_ppm_image("./out93.ppm", framebuffer, win_w, win_h);
    std::cout << "completed drop_ppm" << std::endl;
    // std::cout << "Saving file to: " << std::filesystem::current_path() << "/out93.ppm" << std::endl;
    return 0;
}

