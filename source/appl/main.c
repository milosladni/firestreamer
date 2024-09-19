
/**
* \file     main.c
* \ingroup  g_applspec
* \brief    Implementation of the FireStreamer application.
* \author   Milos Ladicorbic
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include "libv4l2.h"

#include "firestreamer.h"

#define CLEAR(x) memset(&(x), 0, sizeof(x))

struct buffer {
    void   *start;
    size_t length;
};

static void xioctl(int fh, int request, void *arg)
{
    int r;

    do {
        r = v4l2_ioctl(fh, request, arg);
    } while (r == -1 && ((errno == EINTR) || (errno == EAGAIN)));

    if (r == -1) {
        fprintf(stderr, "error %d, %s\n", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

int main(void) {

    struct v4l2_format              fmt;
    struct v4l2_buffer              buf;
    struct v4l2_requestbuffers      req;
    enum v4l2_buf_type              type;
    fd_set                          fds;
    struct timeval                  tv;
    int                             r, fd = -1;
    unsigned int                    i, n_buffers;
    char                            *dev_name = "/dev/video0";
    char                            out_name[256];
    FILE                            *fout;
    struct buffer                   *buffers;

    fd = v4l2_open(dev_name, O_RDWR | O_NONBLOCK, 0);
    if (fd < 0) {
        perror("Cannot open device");
        exit(EXIT_FAILURE);
    }

    CLEAR(fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
//    fmt.fmt.pix.width       = 384;
//    fmt.fmt.pix.height      = 288;
//    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
//    fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;
    fmt.fmt.pix.width       = 768;
    fmt.fmt.pix.height      = 288;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_SRGGB8;
    fmt.fmt.pix.field       = V4L2_FIELD_NONE;

    fmt.fmt.pix.colorspace  = V4L2_COLORSPACE_RAW;
    xioctl(fd, VIDIOC_S_FMT, &fmt);
    if (fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_SRGGB8) {
        printf("Libv4l didn't accept V4L2_PIX_FMT_SRGGB8 format. Can't proceed.\n");
        exit(EXIT_FAILURE);
    }
    if ((fmt.fmt.pix.width != 384) || (fmt.fmt.pix.height != 288)) {
        printf("Warning: driver is sending image at %dx%d\n", fmt.fmt.pix.width, fmt.fmt.pix.height);
    }

    CLEAR(req);
    req.count = 2;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    xioctl(fd, VIDIOC_REQBUFS, &req);

    buffers = calloc(req.count, sizeof(*buffers));
    for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
        CLEAR(buf);

        buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory      = V4L2_MEMORY_MMAP;
        buf.index       = n_buffers;

        xioctl(fd, VIDIOC_QUERYBUF, &buf);

        buffers[n_buffers].length = buf.length;
        buffers[n_buffers].start = v4l2_mmap(NULL, buf.length,
                      PROT_READ | PROT_WRITE, MAP_SHARED,
                      fd, buf.m.offset);

        if (MAP_FAILED == buffers[n_buffers].start) {
                perror("mmap");
                exit(EXIT_FAILURE);
        }
    }

    for (i = 0; i < n_buffers; ++i) {
        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        xioctl(fd, VIDIOC_QBUF, &buf);
    }
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    xioctl(fd, VIDIOC_STREAMON, &type);

    FireStreamer_initialize("rtsps://185.241.214.38:8322/project001/firestream1", "p001fsw1", "p001fsw1234", 384, 288);

    for (i = 0; i < 5000; i++) {
        do {
                FD_ZERO(&fds);
                FD_SET(fd, &fds);

                /* Timeout. */
                tv.tv_sec = 2;
                tv.tv_usec = 0;

                r = select(fd + 1, &fds, NULL, NULL, &tv);
        } while ((r == -1 && (errno = EINTR)));
        if (r == -1) {
                perror("select");
                return errno;
        }

        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        xioctl(fd, VIDIOC_DQBUF, &buf);

        if (i % 25 == 0) {
            printf("Read Frame %dx%d - id_%d, size_%d bytes!\n", fmt.fmt.pix.width, fmt.fmt.pix.height, i, buf.bytesused);
        }

        /* write ppm image */
//        sprintf(out_name, "out%03d.ppm", i);
//        fout = fopen(out_name, "w");
//        if (!fout) {
//                perror("Cannot open image");
//                exit(EXIT_FAILURE);
//        }
//        fprintf(fout, "P6\n%d %d 255\n",
//                fmt.fmt.pix.width, fmt.fmt.pix.height);
//        fwrite(buffers[buf.index].start, buf.bytesused, 1, fout);
//        fclose(fout);

        /* write RAW image */
//        sprintf(out_name, "out%03d.raw", i);
//        fout = fopen(out_name, "w");
//        if (!fout) {
//                perror("Cannot open image");
//                exit(EXIT_FAILURE);
//        }
//        fwrite(buffers[buf.index].start, buf.bytesused, 1, fout);
//        fclose(fout);

        FireStreamer_pushFrame(buffers[buf.index].start, buf.bytesused);

        xioctl(fd, VIDIOC_QBUF, &buf);
    }

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    xioctl(fd, VIDIOC_STREAMOFF, &type);
    for (i = 0; i < n_buffers; ++i)
            v4l2_munmap(buffers[i].start, buffers[i].length);
    v4l2_close(fd);

    return 0;
}

