# ac-cloudifier
Making my dumb AC smart. This project controls a GE window air conditioner with MQTT messages.

## Overview

This project demonstrates the use of the following:
* A multi-process application written in C and Python.
* A custom Yocto image with CI infrastructure.
* Yocto recipes written from scratch for internal applications and third-party open source projects.
* Safe, leak-free C programming, targeting embedded Linux.
* The use of Linux drivers, sockets, pthreads.
* Configuration of a board support package.
* OpenCV for basic machine vision and OCR.
* Bash scripts.
* SystemD services.

## More

[The full writeup is here, with pictures and diagrams!](https://github.com/cu-ecen-aeld/final-project-IvanVeloz/wiki/Project-Overview)

### Repositories
* [cu-ecen-aeld/final-project-IvanVeloz](https://github.com/cu-ecen-aeld/final-project-IvanVeloz) contains the Wiki.
* [IvanVeloz/ac-cloudifier-prj](https://github.com/IvanVeloz/ac-cloudifier-prj) contains the project board.
* [IvanVeloz/ac-cloudifier](https://github.com/IvanVeloz/ac-cloudifier) contains the application code. It targets Raspbian and/or the project's Yocto build.
* [IvanVeloz/ac-cloudifier-yocto](https://github.com/IvanVeloz/ac-cloudifier-yocto) contains the Yocto build.
* [IvanVeloz/ac-cloudifier-demo](https://github.com/IvanVeloz/ac-cloudifier-demo) contains code snippets to demonstrate the drivers and hardware work. It targets Raspbian and/or the project's Yocto build.

### System diagram
![Block diagram of the system. A textual description can be found in the writeup.](https://github.com/cu-ecen-aeld/final-project-IvanVeloz/raw/main/docs/System-block-diagram.png?raw=true)


AELD peer reviewers: please visit the [Project Overview](https://github.com/cu-ecen-aeld/final-project-IvanVeloz/wiki/Project-Overview) for more details.
