# Axis-Aligned Soft Shadow project milestone 1

## 168 path tracer
I have not taken 168 before so I have been doing some catch up.

I have been building a OptiX ray tracer since the beginning of the quarter and following the MOOC version of 168.\
I have successfully built a indirect path tracer (up to hw3 in 168), but did not finish the importance sampling assignment.\
I plan to stop building ray tracers here and begin with working on sampling and filtering.

These are some images I got from the optix tracer.\
![cornellRR](https://user-images.githubusercontent.com/49463679/198512358-575a18b7-b2ff-4d99-8fac-929d50bc6157.png)
![scene6](https://user-images.githubusercontent.com/49463679/198512431-dfbfea6b-c7a3-4dfc-b03d-8f8df4634209.png)

## Interactive Optix7.5 tracer
The Optix tracer used in 168 was based on OptiX 6, however I wanted a look into Optix7.5, since I heard the performance is better.\
I began porting my code into OptiX 7.5, however this took longer than I expected.\
The main reason was I found these two versions have surprisingly different APIs.

After porting I started working on interativity.\
Since I want to do a real time soft shadow project, I need the program to respond to mouse input and be interactive.\
So I incorporated glfw into Optix to get this interactive view:

https://user-images.githubusercontent.com/49463679/198511488-7a802490-0ed8-4c35-ab98-82c77fa7a41a.mp4

(The noise and fps is worse because of video compresson on github)

I also implemented sampling of d1(distance from geometry to light) and d2(occluder to the light).
but have not visualized it or used it to compute number of samples.

## Future plan

In the next few weeks I plan to:
1. work on calculating the number of samples using d1 d2.
2. work on calculating the beta (filter size), and work on the image blur.
3. Then I will build a scene loader, which will take obj files and render test objects interactively.
4. Finally I will work on animating the light and the objects in the scene to make the demo more intersting.
