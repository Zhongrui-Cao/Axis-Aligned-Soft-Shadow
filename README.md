# Axis-Aligned Soft Shadow project report

## Overview

![overview](https://user-images.githubusercontent.com/49463679/206032840-b5613582-a180-40e0-88e6-feea1a05fad3.png)

My project is an implementation of the paper [*Axis-Aligned Filtering for Interactive Sampled Soft Shadows*](http://graphics.berkeley.edu/papers/UdayMehta-AAF-2012-12/) by Mehta Et al. This paper focuses on rendering real-time Monte Carlo ray traced soft shadows by using Axis-Aligned Filtering. More specifically, it is acheived by doing frequency analysis on the Fourier spectrum of the occlusion function. Using this analysis, we can apply adaptive sampling and finally filtering to acheive real time soft shadows that converge to the ground truth. 

## Theory

![Occlusion_Spectrum_Figure](https://user-images.githubusercontent.com/49463679/206034604-61f6bef5-11cf-4183-a62c-607e2761f5bc.png)

The occulsion function and its spectrum is given in this graph. Given a planer area light source, we sample the distance from the receiver to the light source d1. Then we sample the min and max distances from the occluder to the light source, d2min and d2max. Using this information we are given the 2-d occlusion function shown in (b) and the Fourier transform of this funcion shown in (c). 

![aa-vs-shear](https://user-images.githubusercontent.com/49463679/206036060-212fb8af-1d76-428a-98ae-42515e214b48.PNG)

The occlusion function is confined in a double-wedge with slopes s1 and s2. By applying the axis-aligned filter shown in (a) above, we can substantially reduce the number of samples needed for a bias free reconstruction. We can sample more but filter less in high-frequency regions of the shadow, and sample less and filter more in low-frequency regions. This approch is much faster than the sheared filter shown in (b), which requires irregular search over the light field and introduces minutes of overhead. Axis-aligned filtering adds minimal overhead (<5ms).

## Interactive video


## Process

Here are the different variables I got visualized

d1 | d2max
:-------------------------:|:-------------------------:
![d1](https://user-images.githubusercontent.com/49463679/201294042-077a29dd-e4f0-4037-ac3e-8b03dd78117a.PNG) | ![d2max](https://user-images.githubusercontent.com/49463679/201294052-1c26296b-8c9f-49e8-8fbf-e7a90219aa82.PNG)

spp | beta
:-------------------------:|:-------------------------:
![spp](https://user-images.githubusercontent.com/49463679/201293838-d48699a6-5348-42db-b2c1-575410dc37b7.PNG) | ![beta](https://user-images.githubusercontent.com/49463679/201293885-0313b83c-aa93-41b7-8bc6-dd5a764dd7a4.PNG)
