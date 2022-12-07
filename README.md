# Axis-Aligned Soft Shadow project report

## Overview

![overview](https://user-images.githubusercontent.com/49463679/206032840-b5613582-a180-40e0-88e6-feea1a05fad3.png)

My project is an implementation of the paper [*Axis-Aligned Filtering for Interactive Sampled Soft Shadows*](http://graphics.berkeley.edu/papers/UdayMehta-AAF-2012-12/) by Mehta Et al. This paper focuses on rendering real-time Monte Carlo ray traced soft shadows by using Axis-Aligned Filtering. More specifically, it is acheived by doing frequency analysis on the Fourier spectrum of the occlusion function. Using this analysis, we can apply adaptive sampling and finally filtering to acheive real time soft shadows that converge to the ground truth. 

My project is written using OptiX and C++. It can achieve ~30fps on the scene you see above, while having no visible difference to the ground truth. 

## Theory

![Occlusion_Spectrum_Figure](https://user-images.githubusercontent.com/49463679/206034604-61f6bef5-11cf-4183-a62c-607e2761f5bc.png)

The occulsion function and its spectrum is given in this graph. Given a planer area light source, we sample the distance from the receiver to the light source d1. Then we sample the min and max distances from the occluder to the light source, d2min and d2max. Using this information we are given the 2-d occlusion function shown in (b) and the Fourier transform of this funcion shown in (c). 

![aa-vs-shear](https://user-images.githubusercontent.com/49463679/206036060-212fb8af-1d76-428a-98ae-42515e214b48.PNG)

The occlusion function is confined in a double-wedge with slopes s1 and s2. By applying the axis-aligned filter shown in (a) above, we can substantially reduce the number of samples needed for a bias free reconstruction. We can sample more but filter less in high-frequency regions of the shadow, and sample less and filter more in low-frequency regions. This approch is much faster than the sheared filter shown in (b), which requires irregular search over the light field and introduces minutes of overhead. Axis-aligned filtering adds minimal overhead (<5ms).

## Implementation

I implemented the paper using NVIDIA's OptiX 6.5.

### Step 1: Stratified Sampling

First, I trace the primary ray to find any hit. From the geometry hit location I shoot 9 stratified rays to the light source. This 9 rays will give me the values d1, d2min, and d2max. As shown here, these values are spacially varying.

d1 | d2max
:-------------------------:|:-------------------------:
![d1](https://user-images.githubusercontent.com/49463679/201294042-077a29dd-e4f0-4037-ac3e-8b03dd78117a.PNG) | ![d2max](https://user-images.githubusercontent.com/49463679/201294052-1c26296b-8c9f-49e8-8fbf-e7a90219aa82.PNG)

### Step 2: Calculate number of samples

After getting d1, d2min, and d2max, we can calculate the slope of the double wedge.![捕获](https://user-images.githubusercontent.com/49463679/206046566-42e330ee-c65c-465d-922b-1947183e8467.PNG).\
Then, we can calculate the number of samples, as given in the paper: ![spp](https://user-images.githubusercontent.com/49463679/206046727-1f4c358d-e6fa-4e2a-be50-fe297ee53f4d.PNG)\
This will tell us how much extra sampling we need to do for each pixel in the scene.\
For my pratical implementation, I limited the maximum spp to be 80, which yields good results without hampering performance.

<img src="https://user-images.githubusercontent.com/49463679/201293838-d48699a6-5348-42db-b2c1-575410dc37b7.PNG" width=40% height=40%>

As visualized here, the spp for the higher frequency part of the shadow is significantly more than the lower frequency part.

### Step 3: Calculate size of filter

Now, I calcuate the size of the gaussian blur filter. Given by: ![beta_formula](https://user-images.githubusercontent.com/49463679/206048654-de0bb62b-b0c9-4971-9dcf-9c667e3e8487.PNG)

Here is the visualized size on the cornell box scene.\
<img src="https://user-images.githubusercontent.com/49463679/201293885-0313b83c-aa93-41b7-8bc6-dd5a764dd7a4.PNG" width=40% height=40%>

#### spp and beta comparison
As shown in this comparison: for lower frequency areas, the samples per pixel is lower but the size of the filter will be very large.
Whereas for higher frequency details(e.g. the region where occluder is close to geometry), we sample more aggressively but uses a very small filter size.
This behavior contributes to the fast performance of AAF.

result | spp | beta
:-------------------------:|:-------------------------:|:-------------------------:
![grid](https://user-images.githubusercontent.com/49463679/206049143-88d4b8cc-6c55-40ea-beaa-1257ae83825a.png) | ![grid_spp](https://user-images.githubusercontent.com/49463679/206049154-c86db952-0deb-46cf-bd3d-85f3014ebc63.png) | ![grid_beta](https://user-images.githubusercontent.com/49463679/206049180-0d5dc40c-397f-4cd7-8ee6-8a37bed275b5.png)
![cow](https://user-images.githubusercontent.com/49463679/206049209-64753911-221b-46a8-a260-652ba7797db4.png) | ![cow_spp](https://user-images.githubusercontent.com/49463679/206049239-916fe36c-afde-49fd-b28d-3ea5988ab6e4.png) | ![cow_beta](https://user-images.githubusercontent.com/49463679/206049306-da6a2ad6-3c1e-48ad-993e-c77992218e1c.png)

Notice for the feet of the cow, we shoot more samples and filter less. But for the shadow of the head, we sample much less but filter more aggresively.

### Step 4: Image space blur

Finally, I apply the gaussian blur on the noisy output. Given by this formula: ![gaussian](https://user-images.githubusercontent.com/49463679/206050238-7e7f7bb8-6c35-4e7a-901d-aec7be537529.PNG).\
I utilize the world-space distances between objects to compute the filter weights in this equation using a depth buffer.\
Also to get greater efficiency, I use two 1D separable filters along the image dimension. We can rewrite the equation as: w(xij − xkl) = w(xij − xkj )w(xkj − xkl).\
Since beta varies slowly in practice, there is no observable difference between 2d filter and two 1d filters.\

noisy | filtered
:-------------------------:|:-------------------------:
![noisy](https://user-images.githubusercontent.com/49463679/206051055-892519d4-7ae6-470d-b3df-6f36549d7d43.png) | ![filtered](https://user-images.githubusercontent.com/49463679/206051061-54a1d96f-7683-4b78-a59f-e107091579f4.png)

Furthermore, I used a per-pixel object ID check to avoid filtering different objects or regions.\
This image shows different objects with different color corresponding to their ID.

<img src="https://user-images.githubusercontent.com/49463679/206051330-129ce7c1-18a8-4f2c-b1c6-40e08f5c078a.png" width=40% height=40%>

## Results

I am running my implementation on a GTX980Ti graphics card. I am getting 15-30 fps on simple scenes, and 10-15 fps on more complex geometries.\
My results are virtually noise free, and much faster than the ground truth, which is 1-5 fps on most scenes.\
Here are some results on different scenes.

### Grids Scene
noisy | denoised | ground truth
:-------------------------:|:-------------------------:|:-------------------------:
![noisy_grid](https://user-images.githubusercontent.com/49463679/206053665-827b9399-9885-48db-828f-cf42f5e90d98.png) | ![denoised_grid](https://user-images.githubusercontent.com/49463679/206053691-bc43cea0-6df7-4ced-bce2-d08fe9094332.png) | ![gt_grid](https://user-images.githubusercontent.com/49463679/206053735-d676ba84-1c1d-4c14-81d9-345ed0d23bc4.png)

#### Noisy visibility VS. Denoised result
![noisy_vs_denoised](https://user-images.githubusercontent.com/49463679/206054187-697bf921-78f9-488b-87ad-812d4e6f8896.png)\
The sampling and filtering provides so much better result while staying around the same fps as noisy.

#### Ground truth VS. Denoised result
![gt_vs_denoised](https://user-images.githubusercontent.com/49463679/206054029-144b358e-87a5-4c03-981a-eb49d7fd10b8.png)\
Ground truth is on left, denoised on the right. Virtually indistinguishable, while ground truth is not interactive (5 fps).

#### Diff images
Here's noisy and denoised difference from the ground truth. The diffent pixels are highlighted in purple.
noisy | denoised
:-------------------------:|:-------------------------:
![noisy_diff](https://user-images.githubusercontent.com/49463679/206055051-81f5af90-bc5a-465d-9d00-eaf3fbb148f0.png) | ![denoised_diff](https://user-images.githubusercontent.com/49463679/206055072-562298ba-3520-4bc0-be5b-4e03c3c757b6.png)

The denoised result is virtually no difference from ground truth.

### Cow
noisy | denoised | ground truth
:-------------------------:|:-------------------------:|:-------------------------:
![cow_noisy](https://user-images.githubusercontent.com/49463679/206056254-24c02794-53e9-441b-aefc-0f33fa51dc9b.png) | ![cow_denoised](https://user-images.githubusercontent.com/49463679/206056265-34e1fa45-474f-40fd-a029-d3f3b1043ce7.png) | ![cow_gt](https://user-images.githubusercontent.com/49463679/206056275-0e9ad1fb-a196-4fd7-afe2-29bdb129b538.png)

#### Noisy visibility VS. Denoised result
![cow_noisy_vs_denoised](https://user-images.githubusercontent.com/49463679/206056342-7b5f9e00-14da-4781-b880-98142a9a32e9.png)\
For the cow object, the denoised fps is at ~15fps.

#### Ground truth VS. Denoised result
![cow_gt_vs_denoised](https://user-images.githubusercontent.com/49463679/206056423-d2ed6c5d-24c4-4f38-9e8a-c73b1e001516.png)\
Ground truth is on the left and denoised result on the right.\
The ground truth for the cow object is bearly interative at 1-2 fps. The denoised result provides interative fps while having no visible difference to ground truth.

#### Diff images
Here's noisy and denoised difference from the ground truth, this is the enlarged detail of the cow's head. The diffent pixels are highlighted in purple.
noisy | denoised
:-------------------------:|:-------------------------:
![cow_noisy_diff](https://user-images.githubusercontent.com/49463679/206056800-2d072f93-5737-46f3-9d53-64d7b6272b02.png) | ![cow_denoised_diff](https://user-images.githubusercontent.com/49463679/206056812-4359da3e-f95c-419a-a979-2670117e7480.png)

The denoised result is virtually no difference from ground truth.

## Conclusion

From implementing this project, I have learned a lot about frequency analysis and denoising, and also about real-time ray tracing using OptiX.\
This a-priori paper from a decade ago shows the prospect that Monte Carlo rendering can become real time by using denoising techniques.\
As we see in class, this prophecy have been fulfilled by novel methods to denoise using AI in recent years.\
It has been a fun journey implementing this old paper, and seeing the improvement in framerate through better hardware and APIs.
