A 2D ray caster that shows the vision of a player. 
![image](https://github.com/user-attachments/assets/419cc22b-facc-48ae-af04-99d25663e923)

This is a spin of the original repo.
The orginal repo uses polar coordinates to find points to check on the ray. However, because the picture is in Cartesian coordinate system, the coordinates need to be converted. Since the digital system have limited accuracy, some number could be truncated. This approach does not guarantee that every pixel on the ray is covered, leading to holes.

![image](https://github.com/user-attachments/assets/ad466ac2-0e34-4dbe-ac56-4fa154ab90c4)

My version uses Bresenham’s Line Drawing Algorithm to cast the ray, which sweeps every grid on the main axis and finds the secondary coordinate. 
A important trick with this algorithm is that steep lines need to be handled differently. Steep lines have an increment on x axis is more than the y axis. This is the result if we renderlines without doing so.

![image](https://github.com/user-attachments/assets/c37a41e3-ae02-43d0-8887-b20c900ffa37)

When the line is steep, a step of x leads to more than one step on y. As a result, the line leaves holes on the y axis. 

To avoid this issue, we need to switch the x, y coordinates.  When the ray is "steep", we need to swap x with y. This way y becomes the main axis, and very y is covered. However, when saving the file and checking the hit map, the main axis used to locate the target pixel is still x axis. Therefore, when performing these tasks, x and y should be swapped back. 

A naive way is to swapped y and x literally. However, if we swap y and x, the primary axis is switched back to x. The main axis is wrong when entering the next loop. To fix the issue, another swap is required. 

``` cpp

    if(player_a >= (float) M_PI/4 && player_a <= (float) M_PI/2) // in octan 2, the line is steep
    {
        c_pri = py;  // primary axis is y, becasue each step of y incurrs delta x <1.
        o_pri = py;
        o_sec = px;  // specify the origin of the secondary axis
        lim_pri = win_h;
        steep = true;
    }

    for (; c_pri <= lim_pri; c_pri++) 
    {   
        double factor;
        steep?  factor = cos(player_a) / sin(player_a) : factor = tan(player_a);
        size_t c_sec = (c_pri - o_pri)  * factor + o_sec;

        if(steep) // first swap
        {
            int temp = c_pri;
            c_pri = c_sec;
            c_sec = temp;
        }

        if(hit_map[c_pri + c_sec * lim_pri] != ' ') break;
        framebuffer[c_pri + c_sec * lim_pri] = pack_color(255, 255, 255); // segfalut

        if(steep) // second swap
        {
            int temp = c_pri;
            c_pri = c_sec;
            c_sec = temp;
        }
    }
```

A smarter solution is when referencing the target pxiel, instead of referencing by (x,y), reference by (y,x). The values are swapped logically instead of physically.

``` cpp
    if(steep) // transpose back by switching x, y coordinates in the coordinate reference.
    {
        if(hit_map[c_sec + c_pri * lim_pri] != ' ') break;
        framebuffer[c_sec + c_pri * lim_pri] = pack_color(255, 255, 255);
    }
    else
    {
        if(hit_map[c_pri + c_sec * lim_pri] != ' ') break;
        framebuffer[c_pri + c_sec * lim_pri] = pack_color(255, 255, 255); 
    }   
```

Another challenge is that the code needs to be separate into 8 cases, corresponding to 8 octans:
![image](https://github.com/user-attachments/assets/04540181-d889-4851-9548-3222b37bb5e3)

This is because when the coordinates are negative, the increment needs to be negated. Since there are 2 axis in the Cartesian system, there are 4 cases. Considering we handle lines differently when it is steep, the loop would have 8 cases. 
|Octan | x | y | Steep |
|---|---|---|---|
|1 | + | + | false|
|2 | + | + | true|
|3 | - | + | true|
|4 | - | + | false|
|5 | - | - | false|
|6 | - | - | true|
|7 | + | - | true|
|8 | + | - | false|



But we don't want to write the code 8 times... What can we do to make the code more efficient?
The sign of x and y increment can be handled by sin(theta) and cos(theta). 
If the line is steep can be checked by comparing the absolute value of sin(theta) and cos(theta). 
Since when we transpose the coordinates, we swap x, y and their byproduct altogether. 
``` cpp

// step 1: calculate all the byproducts
    float dx = cos(player_a);
    float dy = sin(player_a);

    int stepx = (dx == 0) ? 0 : (dx < 0 ? -1 : 1);
    int stepy = (dy == 0) ? 0 : (dy < 0 ? -1 : 1);

    bool steep = false; // flag that idicates the data needs to be transposed.
    int lim_pri = win_w; // limit of the primary axis
    int c_x = px; // cast coordinate on primary axis;
    int c_y = py; // cast coordinate on secondary axis;
    float delta = abs(dy/dx);
// step 2: swap x, y when it's steep.
    if (abs(dy) > abs(dx)) 
    {
        steep = true;
        lim_pri = win_h;
        std::swap(c_x, c_y);
        std::swap(dx, dy);
        std::swap(stepx, stepy);
    }
// Step 3: sweep each pixel in the main axis.
    float sum_delta = 0;
    while (c_x <= lim_pri && c_x >= 0)
    {   
        // render first;
        if(steep) // transpose back by switching x, y coordinates in the coordinate reference.
        {
            if(hit_map[c_y + c_x * lim_pri] != ' ') break;
            framebuffer[c_y + c_x * lim_pri] = pack_color(255, 255, 255); // segfalut
        }
        else
        {
            if(hit_map[c_x + c_y * lim_pri] != ' ') break;
            framebuffer[c_x + c_y * lim_pri] = pack_color(255, 255, 255); // segfalut
        }

        // then figure out the next pixel.
        c_x += stepx;
        sum_delta += delta;
        if (sum_delta >= 0.5)
        {
            c_y++;
            sum_delta -= 1;
        }
    }
```
An issue I have is that when the angle = pi/2, the ray is casted upwards instead of downwards. After hard thought, I realized that the issue is with the precision. When theta = pi/2, cos(theta) is a very small negative number. We can't really manipulate the input to get it to zero, but have to ignore the error and set it to zero.

``` cpp
    float dx = cosf(player_a);
    if (fabs(dx) < 1e-7) {  // 1e-7 is an example threshold
        dx = 0.0f;
    }
```
At this point, we have a somewhat working code for ray casting. There are still a lot of issues with the code. At the moment, the players view is sweeped by casting a ray at each 1 degree. However, when the distance is big, the ray will leave gaps in between. The reason is again that the code is casting rays in polar coordinates.

![image](https://github.com/user-attachments/assets/2f3fd492-f05b-4f9a-a03c-81fd6bab2142)

I will refactor the code, so the polar system is converted to cartisian as soon as possible, to reduce the need to do float point calculations, which is computationally expensive. I will also chance the way sweep the view by incrementing angles. Instend, I will assume the ray intercets a point on the edge of the map, and cast a ray that connects the 2 points. 
