


## Install GStreamer on Ubuntu or Debian

Run the following command:

apt-get install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libgstreamer-plugins-bad1.0-dev gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-libav gstreamer1.0-tools

## Building applications using GStreamer:

The only other “development environment” that is required is the gcc compiler and a text editor. In order to compile code that requires GStreamer and uses the GStreamer core library, remember to add this string to your gcc command:

pkg-config --cflags --libs gstreamer-1.0

## Combine 'C' and 'C++':
https://isocpp.org/wiki/faq/mixing-c-and-cpp
https://stackoverflow.com/questions/13694605/how-to-use-c-source-files-in-a-c-project

## How to build test application on raspberry:
$ cd  project
$ ./build.sh

