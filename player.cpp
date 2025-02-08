#include "player.h"
#include <cassert>
#include <math.h>

Player::Player(int map_pos_x, int map_pos_y, float view_width, float gaze_angle)
{
    assert(view_width > 0);
    assert(view_width < 2 * M_PI);

    this->map_position.x = map_pos_x;
    this->map_position.y = map_pos_y;

    this->view_width = view_width;
    this->gaze_angle = gaze_angle;
}

Player::~Player() {}

bool Player::is_in_view(Pixel obj_pos)
{
    // Calculate the angle of the vertex with respect to the player's position
    float relative_angle = atan2((float)obj_pos.y - this->map_position.y, (float)obj_pos.x - this->map_position.x);

    // Normalize the angles to be within [0, 2*PI)
    float lower_bound_of_view = this->gaze_angle - this->view_width / 2;
    lower_bound_of_view = normalize_angle(lower_bound_of_view);
    float upper_bound_of_view = this->gaze_angle + this->view_width / 2;
    upper_bound_of_view = normalize_angle(upper_bound_of_view);
    relative_angle = normalize_angle(relative_angle);

    // Check if the relative_angle lies between the two bounds
    if (lower_bound_of_view <= upper_bound_of_view)
    {
        return relative_angle >= lower_bound_of_view && relative_angle <= upper_bound_of_view;
    }
    else
    {
        // Handle the case where the range crosses the 0-degree (or 2 * PI) line
        return relative_angle >= lower_bound_of_view || relative_angle <= upper_bound_of_view;
    }
}

// Utility function to normalize an angle to the range [0, 2*PI)
float Player::normalize_angle(float angle)
{
    while (angle < 0)
        angle += 2 * M_PI;
    while (angle >= 2 * M_PI)
        angle -= 2 * M_PI;
    return angle;
}

std::pair<Pixel, Pixel> Player::find_view_ranges(Map map)
{

    size_t px = player.get_pix_position(map).x; // player x in pixel
    size_t py = player.get_pix_position(map).y; // player y in pixel

    player.gaze_angle = player.gaze_angle + M_PI / 180 * 15 * k; // increment the player angle by 15 degree for this round.
    // first find the end points of players view, which are the rays' intersection with the border.
    float lower_bound_of_view = player.gaze_angle - player.view_width / 2; // lower bound of player view;
    float upper_bound_of_view = player.gaze_angle + player.view_width / 2; // upper bound of player view;
    // pre-treatment: find the map_corners to iterate;
    // calculate the intersection.
    Point intersection1 = find_intersection(lower_bound_of_view, px, py, win_w, win_h);
    Point intersection2 = find_intersection(upper_bound_of_view, px, py, win_w, win_h);
    Pixel endpoint1 = {0, 0};
    Pixel endpoint2 = {0, 0};

    return std::make_pair(endpoint1, endpoint2);
}

Point Player::find_intersection(float theta, size_t px, size_t py, int max_w, int max_h)
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

Pixel Player::get_pixel_position(Map map)
{
  
};