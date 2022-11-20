# Axis-Aligned Soft Shadow project milestone 2

## Current results
Here are some of the results of my denoiser right now\
Noisy image is on the left (adaptive sampled but not filtered) and filtered is on the right

Noisy             |  Filtered
:-------------------------:|:-------------------------:
![cornell_noisy](https://user-images.githubusercontent.com/49463679/201491993-5730a5d2-913e-478d-a4f4-d6c3a2362aaa.PNG) | ![cornell_denoised](https://user-images.githubusercontent.com/49463679/201492007-9254258e-63e1-434e-8e5a-6c16d08629bc.PNG)
![detail1_noisy](https://user-images.githubusercontent.com/49463679/201492012-200effd7-43c0-4fa5-a644-42222bdbe52c.PNG) | ![detail1_denoised](https://user-images.githubusercontent.com/49463679/201492015-7f0073ef-2a7b-4507-b6fb-4badd0c0a584.PNG)
![detail3_noisy](https://user-images.githubusercontent.com/49463679/201492021-23651bf5-452f-4b30-a4f1-9ae58dde935c.PNG) | ![detail3_denoised](https://user-images.githubusercontent.com/49463679/201492024-4124f624-1b92-4d51-9b43-d475f28e708f.PNG)
![detail2_noisy](https://user-images.githubusercontent.com/49463679/201492034-9dc25757-70e1-40ce-b4dc-c2049c853fa2.PNG) | ![detail2_denoised](https://user-images.githubusercontent.com/49463679/201492036-4550ff56-a112-4c46-a34f-7b52de445d10.PNG)

The image below show some sort of "firefly"-like artifacts that I been trying to get rid off, but have not succeeded yet.\
Also the filter will blur different objects together, so I need to implement methods to mitigate this.
![artifact](https://user-images.githubusercontent.com/49463679/201292408-8b29142f-0876-4329-831c-165ed9fd91c9.PNG)

Also I would try to find things to optimize in order to get the best framerate.\
I am currently using a GTX980Ti, which came out years ago and is not as powerful now. I am getting 15~30fps on the cornell box scene.

## Interactive video

https://youtu.be/frIpJnoC-yo

## Process

Here are the different variables I got visualized

d1 | d2max
:-------------------------:|:-------------------------:
![d1](https://user-images.githubusercontent.com/49463679/201294042-077a29dd-e4f0-4037-ac3e-8b03dd78117a.PNG) | ![d2max](https://user-images.githubusercontent.com/49463679/201294052-1c26296b-8c9f-49e8-8fbf-e7a90219aa82.PNG)

spp | beta
:-------------------------:|:-------------------------:
![spp](https://user-images.githubusercontent.com/49463679/201293838-d48699a6-5348-42db-b2c1-575410dc37b7.PNG) | ![beta](https://user-images.githubusercontent.com/49463679/201293885-0313b83c-aa93-41b7-8bc6-dd5a764dd7a4.PNG)

## future plan

1. debug the artifacts on the filter
2. test out different parameters for best results
3. implement scene loader to get more complex scene and objects
4. implement animation to demonstrate real-time shadow changes.
5. Is there anything else I could explore?
