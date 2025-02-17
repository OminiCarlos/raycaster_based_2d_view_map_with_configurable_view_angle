

#include <player.h>
#include <global_variables.h>

uint32_t pack_color(const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a = 255)
{
    return (a << 24) + (b << 16) + (g << 8) + r;
}

void unpack_color(const uint32_t &color, uint8_t &r, uint8_t &g, uint8_t &b, uint8_t &a)
{
    r = (color >> 0) & 255;
    g = (color >> 8) & 255;
    b = (color >> 16) & 255;
    a = (color >> 24) & 255;
}

// save the framebuffer into a file.
void drop_ppm_image(const std::string filename, const std::vector<uint32_t> &image, const size_t w, const size_t h)
{
    assert(image.size() == w * h);
    // ofstream write files.
    std::ofstream ofs(filename);
    // for ppm format, the header goes:
    // First line: P3(plain text data)/P6 (binary data)
    // Second line: width " " height
    // Third line: Maximum color value. In this case 255,
    // when we use 8 bit to store color per channel.
    ofs << "P6\n"
        << w << " " << h << "\n255\n";
    // After the header, each pixel is stored.
    // although each color use 4 byte = 32 bits to store r,g,b,a,
    // only r,g,b are written in the file.
    // in each cycle, unpack color converts each pixel into r,g,b,a.
    // note that image is the framebuffer in main, storing colors in format of colors.
    // the actual image stores colors in r,g,b.
    for (size_t i = 0; i < h * w; ++i)
    {
        uint8_t r, g, b, a;
        unpack_color(image[i], r, g, b, a);
        ofs << static_cast<char>(r) << static_cast<char>(g) << static_cast<char>(b);
    }
    ofs.close();
}

void draw_rectangle(std::vector<uint32_t> &img, const size_t img_w, const size_t img_h,
                    const size_t x, const size_t y, const size_t w, const size_t h, const uint32_t color)
{
    assert(img.size() == img_w * img_h);
    for (size_t i = 0; i < w; i++)
    {
        for (size_t j = 0; j < h; j++)
        {
            size_t cx = x + i;
            size_t cy = y + j;
            assert(cx < img_w && cy < img_h);
            img[cx + cy * img_w] = color;
        }
    }
}

void generate_hit_map(std::vector<char> *hit_map, size_t hit_w, size_t hit_h, size_t x, size_t y, size_t w, size_t h, char c)
{
    for (size_t i = 0; i < w; i++)
    {
        for (size_t j = 0; j < h; j++)
        {
            size_t cx = x + i;
            size_t cy = y + j;
            assert(cx < hit_w && cy < hit_h);
            (*hit_map)[cx + cy * hit_w] = c;
        }
    }
}
// cast_ray() draws a line to connect the begin and end points.
// refactor to take cartesian coordinates as input. Leave the coordinate conversion to the main function.
void cast_ray(int px, int py, int end_x, int end_y, const size_t win_w, const size_t win_h, std::vector<char> &hit_map, std::vector<unsigned int> &framebuffer)
{
    float dx = end_x - px;
    float dy = end_y - py;

    int stepx = (dx == 0) ? 0 : (dx < 0 ? -1 : 1);
    int stepy = (dy == 0) ? 0 : (dy < 0 ? -1 : 1);

    bool steep = false;  // flag that idicates the data needs to be transposed.
    int lim_pri = win_w; // limit of the primary axis
    int c_x = px;        // cast's coordinate at primary axis;
    int c_y = py;        // cast's coordinate at secondary axis;

    int abs_dy = abs(dy);
    int abs_dx = abs(dx);
    if (abs_dy > abs_dx) // projects everything onto the transposed space.
    {
        steep = true;
        lim_pri = win_h;
        std::swap(c_x, c_y);
        std::swap(abs_dx, abs_dy);
        std::swap(stepx, stepy);
    }

    assert(abs_dx != 0);
    int sum_y = 0;
    while (c_x >= 0 && c_x <= lim_pri)
    {
        // render first;
        if (steep) // transpose back by switching x, y coordinates in the coordinate reference.
        {
            if (hit_map[c_y + c_x * win_w] != ' ')
                break;
            framebuffer[c_y + c_x * win_w] = pack_color(255, 255, 255); // segfalut
        }
        else
        {
            if (hit_map[c_x + c_y * win_w] != ' ')
                break;
            framebuffer[c_x + c_y * win_w] = pack_color(255, 255, 255); // segfalut
        }

        // then figure out the next pixel.
        c_x += stepx;
        sum_y += abs_dy;
        /*
            sum_delta - 1 converts in relation to dx, dy:
            sum_delta - sum_y/ sum_x;
            -> let sum_delta = k;
            sum_y / sum_x  = k
            sum_y = sum_x * k;
            changes to:
            sum_y / sum_x  = k -1;
            sum_y = sum_x * k - sum_x;
            sum_y needs to substract sum_x.
        */
        if (2 * sum_y > abs_dx)
        {
            c_y += stepy;
            sum_y -= abs_dx;
        }
    }
}

Point find_intersection(float theta, size_t px, size_t py, int max_w, int max_h)
{
    // initialize parameters
    Point intersection;
    const float epsilon = 1e-6;
    // if steep, transpose to avoid tan(theta) being too big.
    float dx = cos(theta);
    if (abs(dx) < epsilon)
        dx = 0.0f;
    float dy = sin(theta);
    if (abs(dy) < epsilon)
        dy = 0.0f;
    if (dx == 0 && dy == 0)
    {
        intersection.x = px;
        intersection.y = py;
        std::cout << "The intersection for angle: " << theta << " is ("
                  << intersection.x << "," << intersection.y << ")" << std::endl;
        return intersection;
    }
    if (dx == 0)
    {
        intersection.x = px;
        intersection.y = dy > 0 ? max_h : 0;
        std::cout << "The intersection for angle: " << theta << " is ("
                  << intersection.x << "," << intersection.y << ")" << std::endl;
        return intersection;
    }
    if (dy == 0)
    {
        intersection.y = py;
        intersection.x = dx > 0 ? max_w : 0;
        std::cout << "The intersection for angle: " << theta << " is ("
                  << intersection.x << "," << intersection.y << ")" << std::endl;
        return intersection;
    }
    bool steep = false;
    if (dy > dx)
    {
        std::swap(px, py);
        std::swap(max_h, max_w);
        std::swap(dx, dy);
        steep = true;
    }
    float slope = dy / dx;
    int intercept = py - slope * px;

    // find intersection with the limits. (make it a separate function)
    // find the quadrant the intersection is in.
    // if angle more than angle to the corner, change the known factor for intersection.
    // plot the tangent function, you will find that in any quadrant, tan(theta) increases with theta.

    if (dx > 0 && dy > 0)
    {
        int x_2_a = max_w - px; // horizontal distance to a, a is the corner in the first quadrant;
        int y_2_a = max_h - py; // vertical distance to a, a a is the corner in the first quadrant;
        // fisrt quadrant; compare with a
        if (slope > ((float)y_2_a / x_2_a))
        {
            // tan(theta) > tan(a), know y, find x
            intersection.y = max_h;
            intersection.x = (intersection.y - intercept) / slope;
        }
        else
        {
            // tan(theta) < tan(a), know x, find y
            intersection.x = max_w;
            intersection.y = intersection.x * slope + intercept;
        }
    }

    if (dx <= 0 && dy >= 0)
    {
        // second quadrant; compare with b
        int x_2_b = 0 - px;     // horizontal distance to b, b is the corner in the second quadrant;
        int y_2_b = max_h - py; // vertical distance to b, b is the corner in the second quadrant;
        if (slope < ((float)y_2_b / x_2_b))
        {
            // tan(theta) < tan(a), know y, find x
            intersection.y = max_h;
            intersection.x = (intersection.y - intercept) / slope;
        }
        else
        {
            // tan(theta) > tan(a), know x, find y
            intersection.x = 0;
            intersection.y = intersection.x * slope + intercept;
        }
    }

    if (dx <= 0 && dy <= 0)
    {
        // third quadrant; compare with c
        int x_2_c = 0 - px; // horizontal distance to d, d is the corner in the first quadrant;
        int y_2_c = 0 - py; // vertical distance to d, d is the corner in the first quadrant;
        if (slope > ((float)y_2_c / x_2_c))
        {
            // tan(theta) > tan(a), know y, find x
            intersection.y = 0;
            intersection.x = (intersection.y - intercept) / slope;
        }
        else
        {
            // tan(theta) < tan(a), know x, find y
            intersection.x = 0;
            intersection.y = intersection.x * slope + intercept;
        }
    }
    if (dx >= 0 && dy <= 0)
    {
        // forth quadrant; compare with d
        int x_2_d = max_w - px; // horizontal distance to d, d is the corner in the forth quadrant;
        int y_2_d = 0 - py;     // vertical distance to d, d is the corner in the forth quadrant;
        if (slope < ((float)y_2_d / x_2_d))
        {
            // tan(theta) < tan(a), know y, find x
            intersection.y = 0;
            intersection.x = (intersection.y - intercept) / slope;
        }
        else
        {
            // tan(theta) < tan(a), know x, find y
            intersection.x = max_w;
            intersection.y = intersection.x * slope + intercept;
        }
    }

    if (steep)
    {
        std::swap(intersection.x, intersection.y);
    }
    std::cout << "The intersection for angle: " << theta << " is ("
              << intersection.x << "," << intersection.y << ")" << std::endl;
    return intersection;
}

// Utility function to normalize an angle to the range [0, 2*PI)
float normalize_angle(float angle)
{
    while (angle < 0)
        angle += 2 * M_PI;
    while (angle >= 2 * M_PI)
        angle -= 2 * M_PI;
    return angle;
}

bool is_in_view_range(float corner_x, float corner_y, float player_x, float player_y, float lower_bound_of_view, float upper_bound_of_view)
{
    // Calculate the angle of the vertex with respect to the player's position
    float vertex_angle = atan2(corner_y - player_y, corner_x - player_x);

    // Normalize the angles to be within [0, 2*PI)
    lower_bound_of_view = normalize_angle(lower_bound_of_view);
    upper_bound_of_view = normalize_angle(upper_bound_of_view);
    vertex_angle = normalize_angle(vertex_angle);

    // Check if the vertex_angle lies between theta1 and theta2
    if (lower_bound_of_view <= upper_bound_of_view)
    {
        return vertex_angle >= lower_bound_of_view && vertex_angle <= upper_bound_of_view;
    }
    else
    {
        // Handle the case where the range crosses the 0-degree (or 2 * PI) line
        return vertex_angle >= lower_bound_of_view || vertex_angle <= upper_bound_of_view;
    }
}

int main()
{
    const int win_w = 512; // image width
    const int win_h = 512; // image height
    // the constructor format is std::vector(size_t count, const T& value);
    // first argument specifies the number of pixels.
    // second argument uint32_t, set to 255, which means 3 bytes (RGB) are 0, and A is 255.
    std::vector<uint32_t> framebuffer(win_w * win_h, 255); // the image itself, initialized to white
    std::vector<char> hit_map(win_w * win_h, ' ');
    const char map_char[] =
        // "0000222222220000"
        // "1              0"
        // "1              0"
        // "1              0"
        // "0              0"
        // "0              0"
        // "0              0"
        // "0              0"
        // "0              0"
        // "0              0"
        // "0              0"
        // "2              0"
        // "0              0"
        // "0              0"
        // "0              0"
        // "0002222222200000"; // empty game map
        "0000222222220000"
        "1              0"
        "1      11111   0"
        "1     0        0"
        "0     0  1110000"
        "0     3        0"
        "0   10000      0"
        "0   0   11100  0"
        "0   0   0      0"
        "0   0   1  00000"
        "0       1      0"
        "2       1      0"
        "0       0      0"
        "0 0000000      0"
        "0              0"
        "0002222222200000"; // our game map
    Map map =
        {
            16,
            16,
            map_char};
    assert(sizeof(map_char) == map.w * map.h + 1); // +1 for the null terminated string,
    // because strings end with '\0'.Each row has map_h+1 columns.

    // the player state.
    float player_x = 13.456; // player x position
    float player_y = 5.345;  // player y position
    float degree = 155.8;
    float player_a = (degree / 180) * M_PI;   // player view direction
    float view_width = (270.0f / 180 * M_PI); // parameter; how wide the player can see.

    Player player(player_x, player_y, player_a, view_width);

    for (int k = 0; k <= 24; k++) // test, render the player's view for every 15 degrees, and save an output. k is used to calculate the current player's angle. line 411.
    {
        // map rendering: background.
        for (size_t j = 0; j < win_h; j++)
        { // fill the screen with color gradients
            for (size_t i = 0; i < win_w; i++)
            {
                uint8_t r = 255 * j / float(win_h); // varies between 0 and 255 as j sweeps the vertical
                uint8_t g = 255 * i / float(win_w); // varies between 0 and 255 as i sweeps the horizontal
                uint8_t b = 0;
                framebuffer[i + j * win_w] = pack_color(r, g, b);
            }
        }

        // map rendering: wall initialization. (render the wall and generate hit map)
        const size_t rect_w = win_w / map.w; // the width of each map block
        const size_t rect_h = win_h / map.h; // the height of each map block
        for (size_t j = 0; j < map.h; j++)
        { // draw the map
            for (size_t i = 0; i < map.w; i++)
            {
                if (map.map[i + j * map.w] == ' ')
                    continue; // skip empty spaces
                size_t rect_x = i * rect_w;
                size_t rect_y = j * rect_h;
                draw_rectangle(framebuffer, win_w, win_h, rect_x, rect_y, rect_w, rect_h, pack_color(0, 255, 255));
                generate_hit_map(&hit_map, win_w, win_h, rect_x, rect_y, rect_w, rect_h, map.map[i + j * map.w]);
            }
        }


        //TODO: change this to a method in player
    const size_t rect_w = win_w / map.w; // the width of each map block
    const size_t rect_h = win_h / map.h; // the height of each map block

        player.gaze_angle = player.gaze_angle + M_PI / 180 * 15 * k; // increment the player angle by 15 degree for this round.
        // first find the end points of players view, which are the rays' intersection with the border.
        float lower_bound_of_view = player.gaze_angle - player.view_width / 2; // lower bound of player view;
        float upper_bound_of_view = player.gaze_angle + player.view_width / 2; // upper bound of player view;
        // pre-treatment: find the map_corners to iterate;
        // calculate the intersection.
        size_t px = player.get_pixel_position(map).x;
        size_t py = player.get_pixel_position(map).y;
        Point intersection1 = find_intersection(lower_bound_of_view, px, py, win_w, win_h);
        Point intersection2 = find_intersection(upper_bound_of_view, px, py, win_w, win_h);
        // End TODO.

        // transfer intersection (pixel) to ints to reduce calculation.
        // TODO: round down only? Need to consider >0.5 case.
        std::pair<Pixel, Pixel> end_points = player.find_view_ranges(map);
        Pixel start = end_points.first;
        Pixel end = end_points.second;

        // determine if a corner is in the range.
        // initialize map_corners;
        // C===============D
        // ||              ||
        // B===============A

        Pixel map_corners[4] = {
            {win_w, win_h},
            {0, win_h},
            {0, 0},
            {win_w, 0} };

        int i;                          // index of the map_corners to start scanning
        int diff_x = start.x - (int)px; // intersection's horizontal distance to the player, used to determine quadrant.
        int diff_y = start.y - (int)py; // intersection's vertical distance to the player, used to determine quadrant.
        if (diff_x == 0 && diff_y == 0)
        {
            // on one of the corners
            // use px py to determine corner
            if (px == win_w && px == win_h)
                i = 0; // on A, start with A
            if (px == 0 && px == win_h)
                i = 1; // on B, start with B
            if (px == win_w && px == win_h)
                i = 2; // on C, start with C
            if (px == win_w && px == win_h)
                i = 3; // on D, start with D
        }

        if (diff_x == 0)
        {
            if (px == 0)
                i = 1; // on left edge, start with B or C. Let say B.
            if (px == win_w)
                i = 0; // on right edge, start with A or D. Let say A.
        }

        if (diff_y == 0)
        {
            // on top or bottom edge
            if (py == 0)
                i = 2; // on top edge, start with C or D. Let say C.
            if (py == win_h)
                i = 1; // on top edge, start with A or B. Let say A.
        }
        if (diff_x > 0 && diff_y > 0)
        {
            i = 0;
        }
        else if (diff_x < 0 && diff_y > 0)
        {
            i = 1;
        }
        else if (diff_x < 0 && diff_y < 0)
        {
            i = 2;
        }
        else if (diff_x > 0 && diff_y < 0)
        {
            i = 3;
        }

        std::vector<Pixel> points_to_cast;
        // add all the map_corners to render;
        points_to_cast.push_back(start);
        for (int m = 0; m < 4; m++)
        {
            if (is_in_view_range(map_corners[i].x, map_corners[i].y, px, py, lower_bound_of_view, upper_bound_of_view))
            {
                Pixel p = {(int)map_corners[i].x, (int)map_corners[i].y};
                points_to_cast.push_back(p);
            }
            i++;
            i = i % 4;
        }
        points_to_cast.push_back(end);

        // clock-wise, iterate each pixel on the edge between each two visible map_corners in view.
        // start -> a -> b -> c -> d -> end, the corners are optional.
        for (int i = 1; i < (int)points_to_cast.size(); i++)
        {
            // on the same vertical line
            if (points_to_cast[i - 1].x == points_to_cast[i].x)
            {
                int start_y = points_to_cast[i].y;
                int end_y = points_to_cast[i - 1].y;
                if (start_y > end_y)
                {
                    std::swap(start_y, end_y);
                }
                for (int j = start_y; j < end_y; j++)
                {
                    cast_ray(px, py, points_to_cast[i].x, j, win_w, win_h, hit_map, framebuffer);
                }
            }
            // on the same horizontal line
            if (points_to_cast[i - 1].y == points_to_cast[i].y)
            {
                int start_x = points_to_cast[i].x;
                int end_x = points_to_cast[i - 1].x;
                if (start_x > end_x)
                {
                    std::swap(start_x, end_x);
                }
                for (int j = start_x; j < end_x; j++)
                {
                    cast_ray(px, py, j, points_to_cast[i].y, win_w, win_h, hit_map, framebuffer);
                }
            }
        }
        // raycasting: render a ray that represents the gaze of the player.
        // cast_ray() draws a line to connect the begin and end map_corners.
        cast_ray(px, py, (int)intersection1.x, (int)intersection1.y, win_w, win_h, hit_map, framebuffer);
        cast_ray(px, py, (int)intersection2.x, (int)intersection2.y, win_w, win_h, hit_map, framebuffer);
        // draw player's position
        draw_rectangle(framebuffer, win_w, win_h, px - 2, py - 2, 5, 5, pack_color(255, 0, 0));
        std::string build_folder = "output/";              // Define a relative path inside the build folder
        std::filesystem::create_directories(build_folder); // Ensure the folder exists

        std::string file_path = build_folder + "out_" + std::to_string(k) + ".ppm";
        std::cout << "Saving to: " << file_path << std::endl;

        drop_ppm_image(file_path, framebuffer, win_w, win_h);
        std::cout << "wrote file." << std::endl;
    }
    std::cout << "Done." << std::endl;
    return 0;
}