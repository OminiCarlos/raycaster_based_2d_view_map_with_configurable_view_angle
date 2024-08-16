A 2D ray caster that 
This is a spin of the original repo.
The orginal repo uses polar coordinates to locate points on the ray. The issue with this approach is that if the increment is not small enough, it is hard to guarantee that every pixel on the ray is covered, leading to holes.
![image](https://github.com/user-attachments/assets/ad466ac2-0e34-4dbe-ac56-4fa154ab90c4)

This version uses Bresenham’s Line Drawing Algorithm to cast the ray. This algorithm locates the points on the ray by sweeping every pixel on the x or y axis. 
A limitation is that if the increment on the main axis is smaller than the secondary axis, the point on the ray will leave holes.

![image](https://github.com/user-attachments/assets/c37a41e3-ae02-43d0-8887-b20c900ffa37)

In this case, a step of x leads to more than one step on y. Therefore the point gets 1 pixel higher than it is supposed to be, leaving holes. 
The solution is transposing. When y is larger than x, the ray is "steep", swap x with y. This way y becomes the main axis. This way, every y is covered. However, when saving the file and checking the hit map, the main axis used to locate the target pixel is still x axis. Therefore, when performing these tasks, x and y should be swapped back. 
In the beginning, I swapped y and x literally... Then I found that after using the framebuffer and the hit map, the primary axis is switched back to x. When entering the next loop, the main axis is wrong. To fix the issue, another swap is required. A smarter solution is when referencing the target pxiel, instead of referencing by (x,y), reference by (y,x). The reason is after the swap, the y is x, x is y. 
The next challenge is that the code needs to be separate into 8 case, corresponding to 8 octans:
![image](https://github.com/user-attachments/assets/04540181-d889-4851-9548-3222b37bb5e3)

When the gradient of the ray on a axis is opposite to the default direction of that axis, the increment needs to be negated. Considering if the ray is steep, the loop needs to be modified slightly in these 8 cases. 
|Octan | Negate_Main | Negate_Second | Steep |
|---|---|---|---|
|1 | false | false | false|
|2 | false | false | true|
|3 | false | true | true|
|4 | true | false | false|
|5 | true | true | false|
|6 | true | true | true|
|7 | true | false | true|
|8 | false | true | false|

(in progress... I will update how to refector the code to avoid writing similar code 8 times.)
