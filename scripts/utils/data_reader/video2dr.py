# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# All rights reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.


import os
import argparse
import cv2
import numpy as np

CWD = os.path.dirname(__file__)
if CWD == '':
    CWD = '.'

parser = argparse.ArgumentParser(
    description='convert video to inputs of the Sample DataReader')
parser.add_argument('-i', '--input', type=str,
                    help='The input video file(*.mp4)', required=True)
parser.add_argument('-o', '--output', type=str, default="CAM0",
                    help='The output path to store the generated inputs',
                    required=False)
parser.add_argument('-s', '--offset', type=int, default=0,
                    help='the offset of the frame in the video',
                    required=False)
parser.add_argument('-m', '--maximum', type=int, default=1000,
                    help='The maximum number of the generated inputs',
                    required=False)
parser.add_argument('-r', '--resolution', type=str, default='1920 1024',
                    help='The resolution of camera',
                    required=False)
parser.add_argument('-f', '--format', type=str, default='uyvy',
                    help='The color format of camera, [uyvy, nv12, nv21, rgb, p010]',
                    required=False)
args = parser.parse_args()


if not os.path.exists('/tmp/rgb2yuv'):
    if 0 != os.system('gcc %s/rgb2yuv.c -o /tmp/rgb2yuv' %(CWD)):
        print("failed to compile the simple host color convert tool /tmp/rgb2yuv")
        exit()

w,h = args.resolution.split(' ')
W,H = int(w), int(h)

os.makedirs(args.output, exist_ok=True)

vid = cv2.VideoCapture(args.input)
isRGB = vid.get(cv2.CAP_PROP_CONVERT_RGB)
if type(isRGB) is float:
    isRGB = (isRGB == 1.0)

ret = True
idx = 0
if args.offset > 0:
    vid.set(cv2.CAP_PROP_POS_FRAMES, args.offset)

idx = 0
while(ret and idx < args.maximum):
    ret, frame = vid.read()
    if ret:
        resized = cv2.resize(frame, (W, H), interpolation = cv2.INTER_AREA)
        if args.format == 'rgb':
            tgt = '%s/%s.%s'%(args.output, idx, args.format)
            if False == isRGB:
                rgb = cv2.cvtColor(resized, cv2.COLOR_BGR2RGB)
                rgb.astype(np.uint8).tofile(tgt)
            else:
                resized.astype(np.uint8).tofile(tgt)
            print('gen %s'%(tgt))
        else:
            if False == isRGB:
                rgb = cv2.cvtColor(resized, cv2.COLOR_BGR2RGB)
                rgb.astype(np.uint8).tofile('/tmp/rgb.raw')
            else:
                resized.astype(np.uint8).tofile('/tmp/rgb.raw')
            cmd = '/tmp/rgb2yuv %s /tmp/rgb.raw %s/%s.%s %s %s'%(args.format, args.output, idx, args.format, W, H)
            if 0 != os.system(cmd):
                print('failed: %s' %(cmd))
                exit()
        idx += 1

vid.release()