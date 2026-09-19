# Embedded Vehicle Image Classification

Bachelor's thesis project focused on the development and deployment of an image classification model on the Seeed Studio XIAO ESP32S3 Sense.

## Dataset

A custom dataset was collected using a smartphone. Images were captured from a fixed frontal-side perspective and prepared for image classification.
The dataset consists of four classes:

- Car
- Van
- Truck
- No Vehicle


## Overview

The project explores the use of TinyML for running an image classification model on a resource-constrained embedded device.
A MobileNetV2-based model was trained using transfer learning in Edge Impulse and optimized using INT8 quantization before deployment to the XIAO ESP32S3 Sense. 
The complete system performs image acquisition, preprocessing and classification locally on the device.

## Technologies

- Edge Impulse
- TinyML
- MobileNetV2
- TensorFlow Lite Micro
- Arduino / C++
- SenseCraft AI

## Model

- Architecture: MobileNetV2
- Input resolution: 160 × 160 RGB
- Width multiplier: 0.35
- Transfer learning with ImageNet pretrained weights
- INT8 quantization
- 4 output classes

## Results

| Evaluation | Accuracy |
|---|---:|
| Float32 model testing | 95.45% |
| INT8 model testing | 94.55% |
| Practical on-device testing | 88.35% |

The practical test included 103 samples, of which 91 were correctly classified.

The main classification challenge was distinguishing between the visually similar Car and Van classes.

## Deployment

The trained INT8 model was deployed to the XIAO ESP32S3 Sense using two approaches:

- **SenseCraft AI** – used for live on-device classification and practical testing.
- **Arduino** – used to run image acquisition, preprocessing and model inference directly on the microcontroller using the Edge Impulse inference library.

The Edge Impulse Arduino library used by the project is provided in
`edge-impulse/VlastitiDatasetCrop_inferencing.zip` and can be installed in the Arduino IDE using `Sketch > Include Library > Add .ZIP Library`.

The Arduino implementation was developed based on the XIAO ESP32S3 Sense image classification exercise from the *Machine Learning Systems* materials and adapted to integrate the custom vehicle classification model developed in this project.

Future work could include collecting a larger and more diverse dataset and testing the system with real vehicles under different lighting and environmental conditions.

Reference:
[Machine Learning Systems – XIAO ESP32S3 Sense Image Classification](https://mlsysbook.ai/kits/contents/seeed/xiao_esp32s3/image_classification/image_classification.html)
