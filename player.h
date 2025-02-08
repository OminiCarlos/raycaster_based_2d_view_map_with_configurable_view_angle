#ifndef PLAYER_H
#define PLAYER_H
#include <global_variables.h>

class Player
{
public:
    Point map_position;
    float view_width;
    float gaze_angle;

    Player(int map_pos_x, int map_pos_y, float view_width, float gaze_angle);
    ~Player();

    bool is_in_view(Pixel obj_pos);
    Pixel get_pixel_position(Map map);
    std::pair<Pixel, Pixel> find_view_ranges(Map map);


private:
    float normalize_angle(float angle);
    Point Player::find_intersection(float theta, size_t px, size_t py, int max_w, int max_h);
};

#endif // PLAYER_H