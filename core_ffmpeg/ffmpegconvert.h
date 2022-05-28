#ifndef FFMPEGCONVERT_H
#define FFMPEGCONVERT_H

#include "ffmpeghead.h"

//下面的函数在整个系统中都没有用到

//NV12      yyyyyyyy uv uv
//YUV420P   yyyyyyyy uu vv
//注意这里开辟了一帧内存,需要绘制完以后释放
//NV12 与 YUV420P 格式互换
//type = 0 NV12转YUV420P
//type = 1 YUV420P转NV12
static AVFrame *formatConvert(AVFrame *frame, int type)
{
    //实例化一帧
    AVFrame *dst_frame = av_frame_alloc();
    if (!dst_frame) {
        return 0;
    }

    //设置格式和宽高
    if (type == 0) {
        dst_frame->format = AV_PIX_FMT_YUV420P;
    } else if (type == 1) {
        dst_frame->format = AV_PIX_FMT_NV12;
    }

    int width = frame->width;
    int height = frame->height;
    dst_frame->width = width;
    dst_frame->height = height;

    //内存对齐 一般为32
    if (av_frame_get_buffer(dst_frame, 32) < 0) {
        return 0;
    }

    //设置帧可写
    if (av_frame_make_writable(dst_frame) < 0) {
        return 0;
    }

    //复制数据
    int x, y;
    int linesize = frame->linesize[0];
    if (linesize == width) {
        memcpy(dst_frame->data[0], frame->data[0], height * linesize);
    } else {
        //下面这部分没有遇到过执行
        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x++) {
                dst_frame->data[0][y * dst_frame->linesize[0] + x] = frame->data[0][y * frame->linesize[0] + x];
            }
        }
    }

    for (y = 0; y < height / 2; y++) {
        for (x = 0; x < width / 2; x++) {
            if (type == 0) {
                dst_frame->data[1][y * dst_frame->linesize[1] + x] = frame->data[1][y * frame->linesize[1] + x * 2];
                dst_frame->data[2][y * dst_frame->linesize[2] + x] = frame->data[1][y * frame->linesize[1] + x * 2 + 1];
            } else if (type == 1) {
                dst_frame->data[1][y * dst_frame->linesize[1] + x * 2] = frame->data[1][y * frame->linesize[1] + x];
                dst_frame->data[1][y * dst_frame->linesize[1] + x * 2 + 1] = frame->data[2][y * frame->linesize[2] + x];
            }
        }
    }

    return dst_frame;
}

static bool YUV420ToRGB32(const unsigned char *yuv420, unsigned char *&rgb32, int width, int height)
{
    if (width < 1 || height < 1 || yuv420 == NULL) {
        return false;
    }

    const long len = width * height;
    unsigned char *yData = (unsigned char *)yuv420;
    unsigned char *uData = &yData[len];
    unsigned char *vData = &uData[len >> 2];

    const int nRgbLen = width * height * 4;
    uchar *rgb = new uchar[nRgbLen];
    memset(rgb, 0, nRgbLen);

    int bgr[3];
    int yIdx, uIdx, vIdx, idx;
    for (int i = 0 ; i < height ; i++) {
        for (int j = 0; j < width; j++) {
            //!YUV420格式 每4个Y 共用一个UV。
            yIdx = i * width + j;
            uIdx = (i / 2) * (width / 2) + (j / 2);
            vIdx = uIdx;

            bgr[0] = (int)(yData[yIdx] + 1.779 * (uData[vIdx] - 128));		//b分量
            bgr[1] = (int)(yData[yIdx] - 0.3455 * (uData[uIdx] - 128) - 0.7169 * (vData[vIdx] - 128)); //g分量
            bgr[2] = (int)(yData[yIdx] + 1.4075 * (vData[uIdx] - 128));		//!r分量

            for (int k = 0; k < 3; k++) {
                idx = (i * width + j) * 4 + k ;
                if (bgr[k] >= 0 && bgr[k] <= 255) {
                    rgb[idx] = bgr[k];
                } else {
                    rgb[idx] = (bgr[k] < 0) ? 0 : 255;
                }
            }
        }
    }
    rgb32 = rgb;
    return true;
}

static bool YV12ToRGB888(const unsigned char *yv12, unsigned char *rgb888, int width, int height)
{
    if ((width < 1) || (height < 1) || (yv12 == NULL) || (rgb888 == NULL)) {
        return false;
    }

    int len = width * height;
    unsigned char const *yData = yv12;
    unsigned char const *vData = &yData[len];
    unsigned char const *uData = &vData[len >> 2];

    int rgb[3];
    int yIdx, uIdx, vIdx, idx;

    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            yIdx = i * width + j;
            vIdx = (i / 2) * (width / 2) + (j / 2);
            uIdx = vIdx;

            rgb[0] = static_cast<int>(yData[yIdx] + 1.370705 * (vData[uIdx] - 128));
            rgb[1] = static_cast<int>(yData[yIdx] - 0.698001 * (uData[uIdx] - 128) - 0.703125 * (vData[vIdx] - 128));
            rgb[2] = static_cast<int>(yData[yIdx] + 1.732446 * (uData[vIdx] - 128));

            for (int k = 0; k < 3; ++k) {
                idx = (i * width + j) * 3 + k;
                if ((rgb[k] >= 0) && (rgb[k] <= 255)) {
                    rgb888[idx] = static_cast<unsigned char>(rgb[k]);
                } else {
                    rgb888[idx] = (rgb[k] < 0) ? (0) : (255);
                }
            }
        }
    }
    return true;
}

static int YUV422ToYUV420(const unsigned char *yuv422, unsigned char *yuv420, int width, int height)
{
    int ynum = width * height;
    int i, j, k = 0;

    //得到Y分量
    for (i = 0; i < ynum; i++) {
        yuv420[i] = yuv422[i * 2];
    }

    //得到U分量
    for (i = 0; i < height; i++) {
        if ((i % 2) != 0) {
            continue;
        }
        for (j = 0; j < (width / 2); j++) {
            if ((4 * j + 1) > (2 * width)) {
                break;
            }
            yuv420[ynum + k * 2 * width / 4 + j] = yuv422[i * 2 * width + 4 * j + 1];
        }
        k++;
    }

    k = 0;
    //得到V分量
    for (i = 0; i < height; i++) {
        if ((i % 2) == 0) {
            continue;
        }
        for (j = 0; j < (width / 2); j++) {
            if ((4 * j + 3) > (2 * width)) {
                break;
            }
            yuv420[ynum + ynum / 4 + k * 2 * width / 4 + j] = yuv422[i * 2 * width + 4 * j + 3];

        }
        k++;
    }
    return 1;
}

#endif // FFMPEGCONVERT_H
