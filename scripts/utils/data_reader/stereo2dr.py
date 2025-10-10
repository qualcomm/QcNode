# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear-Clear

import os
import cv2
import numpy as np
import argparse

CWD = os.path.dirname(__file__)
if CWD == '':
    CWD = '.'

parser = argparse.ArgumentParser(
    description='convert Stereo Evaluation 2015 dataset to inputs of the Sample DataReader')

parser.add_argument('-i', '--input', type=str,
                    help='the input path to Stereo Evaluation 2015 dataset',
                    required=True)
parser.add_argument('-o', '--output', type=str, default="DFS0",
                    help='The output path to store the generated inputs',
                    required=False)
parser.add_argument('-s', '--offset', type=int, default=0,
                    help='the offset of the dataset images',
                    required=False)
parser.add_argument('-m', '--maximum', type=int, default=200,
                    help='The maximum number of the generated inputs',
                    required=False)
parser.add_argument('-r', '--resolution', type=str, default='1280 416',
                    help='The resolution of camera',
                    required=False)
parser.add_argument('-f', '--format', type=str, default='nv12',
                    help='The color format of camera, [uyvy, nv12, nv21, rgb, p010]',
                    required=False)

args = parser.parse_args()

w,h = args.resolution.split(' ')
W,H = int(w), int(h)

if not os.path.exists('/tmp/rgb2yuv'):
    if 0 != os.system('gcc %s/rgb2yuv.c -o /tmp/rgb2yuv' %(CWD)):
        print("failed to compile the simple host color convert tool /tmp/rgb2yuv")
        exit()

LEFT_DIR = args.output+'/left'
RIGHT_DIR = args.output+'/right'
os.makedirs(LEFT_DIR, exist_ok=True)
os.makedirs(RIGHT_DIR, exist_ok=True)

def convert(imgPath, output, idx):
    image_bgr = cv2.imread(imgPath, cv2.IMREAD_COLOR)
    image_rgb = cv2.cvtColor(image_bgr, cv2.COLOR_BGR2RGB)
    resized = cv2.resize(image_rgb, (W, H), interpolation = cv2.INTER_AREA)
    tgt = '%s/%s.%s'%(output, idx, args.format)
    if args.format == 'rgb':
        resized.astype(np.uint8).tofile(tgt)
    else:
        resized.astype(np.uint8).tofile('/tmp/rgb.raw')
    cmd = '/tmp/rgb2yuv %s /tmp/rgb.raw %s/%s.%s %s %s'%(args.format, output, idx, args.format, W, H)
    if 0 != os.system(cmd):
        print('failed: %s' %(cmd))
        exit()

for i in range(args.maximum):
    idx = i + args.offset
    left = args.input + '/image_2/%06d_10.png'%(idx)
    right = args.input + '/image_3/%06d_10.png'%(idx)
    convert(left, LEFT_DIR, i*2)
    convert(right, RIGHT_DIR, i*2)
    left = args.input + '/image_2/%06d_11.png'%(idx)
    right = args.input + '/image_3/%06d_11.png'%(idx)
    convert(left, LEFT_DIR, i*2+1)
    convert(right, RIGHT_DIR, i*2+1)
