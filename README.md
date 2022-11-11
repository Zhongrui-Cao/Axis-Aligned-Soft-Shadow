# Axis-Aligned Soft Shadow project milestone 2

## Current results
Here are some of the results of my denoiser right now
Noisy image is on the left and filtered is on the right

Noisy             |  Filtered
:-------------------------:|:-------------------------:
![cornell_noisy](https://user-images.githubusercontent.com/49463679/201291163-2e68f65f-436f-44fe-8214-d2dfe6cdef7b.PNG)  |  ![cornell_denoised](https://user-images.githubusercontent.com/49463679/201291173-b468229c-cdbf-40f4-98fa-ffbfbee0e5d5.PNG)
![detail_noisy](https://user-images.githubusercontent.com/49463679/201291656-2638a8f6-6d3d-42ce-9a8b-b62761ec20b7.PNG)  |  ![detail_denoised](https://user-images.githubusercontent.com/49463679/201291663-a31006f3-e3c3-417a-a065-da435d5c469f.PNG)
![detail2_noisy](https://user-images.githubusercontent.com/49463679/201291689-1082a9d9-b9c2-4e1d-9f8d-d7809413e5ac.PNG) | ![detail2_denoised](https://user-images.githubusercontent.com/49463679/201291693-eb102f47-5604-4b84-b488-de4c436933a3.PNG)
![detail3_noisy](https://user-images.githubusercontent.com/49463679/201291703-c9f6c292-e3f6-4775-aa7f-da470e87f61e.PNG) | ![detail3_denoised](https://user-images.githubusercontent.com/49463679/201291705-acb700c3-be65-4132-8779-aa64f427506e.PNG)

The image below show some sort of "firefly"-like artifacts that I been trying to get rid off, but have not succeeded yet.\
Also the filter will blur different objects together, so I need to implement methods to mitigate this.
![artifact](https://user-images.githubusercontent.com/49463679/201292408-8b29142f-0876-4329-831c-165ed9fd91c9.PNG)

Also I would try to find thing to optimize in order to get the best framerate.\
I currently use a GTX980Ti, which came out years ago and not as powerful now. I am getting 15~30fps on the cornell box scene.\

## Process

Here are the different variables I got visualized
spp | beta
:-------------------------:|:-------------------------:
![spp](https://user-images.githubusercontent.com/49463679/201293838-d48699a6-5348-42db-b2c1-575410dc37b7.PNG) | ![beta](https://user-images.githubusercontent.com/49463679/201293885-0313b83c-aa93-41b7-8bc6-dd5a764dd7a4.PNG)

d1 | d2max
:-------------------------:|:-------------------------:
![d1](https://user-images.githubusercontent.com/49463679/201294042-077a29dd-e4f0-4037-ac3e-8b03dd78117a.PNG) | ![d2max](https://user-images.githubusercontent.com/49463679/201294052-1c26296b-8c9f-49e8-8fbf-e7a90219aa82.PNG)

## future plan

1. debug the artifacts on the filter
2. test out different parameters for best results
3. implement scene loader to get more complex scene and objects
4. implement animation to demonstrate real time shadow changes.
5. Is there anything else I could explore?
