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
    assert(image.size() == w*h);
    // ofstream write files. 
    std::ofstream ofs(filename);
    // for ppm format, the header goes:
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

void draw_rectangle(std::vector<uint32_t> &img, const size_t img_w, const size_t img_h, 
    const size_t x, const size_t y, const size_t w, const size_t h, const uint32_t color) {
    assert(img.size()==img_w * img_h);
    for (size_t i=0; i<w; i++) {
        for (size_t j=0; j<h; j++) {
            size_t cx = x+i;
            size_t cy = y+j;
            assert(cx<img_w && cy<img_h);
            img[cx + cy*img_w] = color;
        }
    }
}

void generate_hit_map(std::vector<char>* hit_map, size_t hit_w, size_t hit_h, size_t x, size_t y, size_t w, size_t h, char c)
{
    for (size_t i = 0; i<w; i++) {
        for (size_t j = 0; j<h; j++) {
            size_t cx = x+i;
            size_t cy = y+j;
            assert(cx<hit_w && cy<hit_h);
            (*hit_map)[cx + cy * hit_w] = c;
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
    std::vector<char> hit_map(win_w*win_h, ' ');
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


    // the player state.
    float player_x = 13.456; // player x position
    float player_y = 2.345; // player y position
    float degree = 75;
    float player_a = (float) (degree/ 180) * M_PI; // player view direction


    // map rendering: background. 
    for (size_t j = 0; j<win_h; j++) { // fill the screen with color gradients
        for (size_t i = 0; i<win_w; i++) {
            uint8_t r = 255*j/float(win_h); // varies between 0 and 255 as j sweeps the vertical
            uint8_t g = 255*i/float(win_w); // varies between 0 and 255 as i sweeps the horizontal
            uint8_t b = 0;
            framebuffer[i+j*win_w] = pack_color(r, g, b);
        }
    }

    // map rendering: wall initialization. (render the wall and generate hit map)
    const size_t rect_w = win_w/map_w; // the width of each map block
    const size_t rect_h = win_h/map_h; // the height of each map block
    for (size_t j=0; j<map_h; j++) { // draw the map
        for (size_t i=0; i<map_w; i++) {
            if (map[i+j*map_w]==' ') continue; // skip empty spaces
            size_t rect_x = i*rect_w;
            size_t rect_y = j*rect_h;
            draw_rectangle(framebuffer, win_w, win_h, rect_x, rect_y, rect_w, rect_h, pack_color(0, 255, 255));
            generate_hit_map(&hit_map, win_w, win_h, rect_x, rect_y, rect_w, rect_h,map[i+j*map_w]);
        }
    }


    // player initialization. 
    // player rendering: draw the player on the map
    draw_rectangle(framebuffer, win_w, win_h, player_x*rect_w, player_y*rect_h, 5, 5, pack_color(255, 255, 255));

    // raycasting: render a ray that represents the gaze of the player.
    // assume theta is within [0,45]. if theta > 45, an increment of x can represent more than 1 increment of y. 
        // leading to holes.
    // It's a bit hard to do it pixel by pixel. 
    size_t px = player_x*rect_w; // player x in pixel
    size_t py = player_y*rect_h; // player y in pixel
    int c_pri; // cast coordinate at primary axis;
    int c_sec; // cast coordinate at secondary axis;
    int o_pri; // origin at primary axis
    int o_sec; // origin at secondary axis
    int lim_pri; // limit of the original axis
    bool steep = false; // flag that idicates the data needs to be transposed.
    if(player_a >= (float) M_PI/4 && player_a <= (float) M_PI/2)
    {
        c_pri = py;  // primary axis is y, becasue each step of y incurrs delta x <1.
        o_pri = py;
        o_sec = px;  // specify the origin of the secondary axis
        lim_pri = win_h;
        steep = true;
    }

    if(player_a >= (float) 0 && player_a <= (float)M_PI_4)
    {
        c_pri = px;  // primary axis is x, becasue each step of x leads to delta y <1.
        o_pri = px;
        o_sec = py;  // specify the origin of the secondary axis
        lim_pri = win_w;
    }

    for (; c_pri <= lim_pri; c_pri++) 
    {   
        double factor;
        steep?  factor = cos(player_a) / sin(player_a) : factor = tan(player_a);
        size_t c_sec = (c_pri - o_pri)  * factor + o_sec;

        if(steep) 
        {
            int temp = c_pri;
            c_pri = c_sec;
            c_sec = temp;
        }

        if(hit_map[c_pri + c_sec * lim_pri] != ' ') break;
        framebuffer[c_pri + c_sec * lim_pri] = pack_color(255, 255, 255); // segfalut

        if(steep) 
        {
            int temp = c_pri;
            c_pri = c_sec;
            c_sec = temp;
        }
    }

    drop_ppm_image("./out.ppm", framebuffer, win_w, win_h);
    std::cout << "yo" << std::endl;

    return 0;
}