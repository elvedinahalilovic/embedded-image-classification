# Embedded Vehicle Image Classification

Bachelor's thesis project focused on the development and deployment of an image classification model on the Seeed Studio XIAO ESP32S3 Sense.

The system performs vehicle image classification locally on the microcontroller using four classes:

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
- C/C++
- Arduino

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
| INT8 model testing | 93.64% |
| Practical on-device testing | 88.35% |

The practical test included 103 samples, of which 91 were correctly classified.

The main classification challenge was distinguishing between the visually similar Car and Van classes.

## Deployment

The trained model was deployed to the XIAO ESP32S3 Sense and
tested using both SenseCraft AI and an Arduino-based implementation. The Arduino implementation performs image acquisition, preprocessing, model inference and result output directly on the microcontroller.

The Arduino implementation was developed based on the image classification exercise for the XIAO ESP32S3 Sense provided as part of the *Machine Learning Systems* materials.
The reference implementation was adapted to integrate the custom vehicle classification model developed in this project.

Future work could include collecting a larger and more diverse dataset and testing the system with real vehicles under different lighting and environmental conditions.

Reference:
[Machine Learning Systems – XIAO ESP32S3 Sense Image Classification](https://mlsysbook.ai/kits/contents/seeed/xiao_esp32s3/image_classification/image_classification.html)
