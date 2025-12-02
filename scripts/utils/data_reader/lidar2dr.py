# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear

import os
import argparse
import numpy as np
from PIL import Image

CWD = os.path.dirname(__file__)
if CWD == '':
    CWD = '.'

parser = argparse.ArgumentParser(
    description='convert lidar pointcloud to inputs of the Sample DataReader')
parser.add_argument('-i', '--input', type=str,
                    help='The input directory that contains the pointclound files', required=True)
parser.add_argument('-o', '--output', type=str, default="LIDAR0",
                    help='The output path to store the generated inputs',
                    required=False)
parser.add_argument('-s', '--offset', type=int, default=0,
                    help='the offset of the lidar frame in the input directory',
                    required=False)
parser.add_argument('-m', '--maximum', type=int, default=500,
                    help='The maximum number of the generated inputs',
                    required=False)
parser.add_argument('-r', '--range', type=str, default="0 69.12 -39.68 39.68 -3 1",
                    help='the range of points: [minX, maxX, minY, maxY, minZ, maxZ]',
                    required=False)
args = parser.parse_args()


if not os.path.exists('/tmp/rgb2yuv'):
    if 0 != os.system('gcc %s/rgb2yuv.c -o /tmp/rgb2yuv' %(CWD)):
        print("failed to compile the simple host color convert tool /tmp/rgb2yuv")
        exit()

# the generate lidar image resolution
W, H = 1920, 1024

minX, maxX, minY, maxY, minZ, maxZ = [float(x) for x in args.range.split(' ')]

'''
                                     Y
                                     ^
  ------+----------------------------|---------------------------------+----> x 
     ^  |          +-----------------|-----------------+ maxY          |
     |  |          |                 |                 |               |
     |  |          |                 |                 |               |
     |  |          |                 |                 |               |
     |  |          |                 |                 |               |
     |  |          |                 +-----------------------------------------> X
     H  |     minX |                                   |maxX           |
     |  |          |                                   |               |
     |  |          |                                   |               |
     |  |          |                                   |               |
     V  |          +-----------------------------------+ minY          |
  ------+----------------------------|---------------------------------+
        |<-------------------------- W ------------------------------->|
        V
        y
'''
ratioI = H / W
ratioP = (maxY - minY) / (maxX - minX)

# offsetX * 2 + ratioW * (maxX - minX) = W
# offsetY * 2 + ratioH * (maxY - minY) = H

if ratioI < ratioP:
    offsetY = 0
    ratioH = H / (maxY - minY)
    ratioW = ratioH
    offsetX = (W - ratioW * (maxX - minX)) / 2
else:
    offsetX = 0
    ratioW = W / (maxW - minW)
    ratioH = ratioW
    offsetY = (H - ratioH * (maxY - minY)) / 2

print('range = [%s %s %s %s %s %s]'%(minX, maxX, minY, maxY, minZ, maxZ))
print('ratioI:', ratioI)
print('ratioP:', ratioP)
print('offsetX:', offsetX)
print('offsetY:', offsetY)
print('ratioW:', ratioW)
print('ratioH:', ratioH)

def HeatColor(z):
    rgb = [ 255, 255, 255 ]
    normalized_z = ( z - minZ ) / ( maxZ - minZ );
    inverted_group = ( 1 - normalized_z ) / 0.25 # invert and group
    decimal_part = int( inverted_group ) # this is the integer part

    # fractional_part part from 0 to 255
    fractional_part = int( 255 * ( inverted_group - decimal_part ) )

    if 0 == decimal_part:
        rgb[0] = 255
        rgb[1] = fractional_part
        rgb[2] = 0
    elif 1 == decimal_part:
        rgb[0] = 255 - fractional_part
        rgb[1] = 255
        rgb[2] = 0
    elif 2 == decimal_part:
        rgb[0] = 0
        rgb[1] = 255
        rgb[2] = fractional_part
    elif 3 == decimal_part:
        rgb[0] = 0
        rgb[1] = 255 - fractional_part
        rgb[2] = 255
    elif 4 == decimal_part:
        rgb[0] = 0
        rgb[1] = 0
        rgb[2] = 255
    return rgb

def Pcd2Image(pcd, idx):
    img = np.zeros([H, W, 3],dtype=np.uint8)
    img[:, :, :] = [255, 255, 255]
    pcdL = [(x,y,z) for x,y,z,r in pcd]
    pcdL.sort(key=lambda x:x[2]) # sort by Z
    for x,y,z in pcdL:
        w = int(offsetX + ratioW * (x - minX))
        h = int(offsetY + ratioH * (maxY - y))
        color = HeatColor(z)
        for dx in [0, 1]:
            for dy in [0, 1]:
                w1 = max(0, min(w+dx, W-1))
                h1 = max(0, min(h+dy, H-1))
                img[h1][w1] = color

    im = Image.fromarray(img)
    tgt = '%s/%s.jpg'%(args.output, idx)
    im.save(tgt)
    img.tofile('/tmp/rgb.raw')
    cmd = '/tmp/rgb2yuv nv12 /tmp/rgb.raw %s/%s.nv12 %s %s'%(args.output, idx, W, H)
    if 0 != os.system(cmd):
        print('failed: %s' %(cmd))
        exit()

os.makedirs(args.output, exist_ok=True)

pcds = []
for i in range(args.maximum):
    p = '%s/%06d.bin'%(args.input, i+args.offset)
    if not os.path.isfile(p):
        p = '%s/%s.raw'%(args.input, i+args.offset)
    if os.path.isfile(p):
        pcds.append(p)

maxP = 0
for i, p in enumerate(pcds):
    num = int(os.path.getsize(p)/16)
    if num > maxP:
        maxP = num
    pcd = np.fromfile(p, np.float32).reshape(-1, 4)
    d = []
    for x,y,z,r in pcd:
        if x >= minX and x <= maxX and y >= minY and y <= maxY and z >= minZ and z <= maxZ:
            d.append([x,y,z,r])
    Pcd2Image(pcd, i)
    pcd = np.asarray(d).astype(np.float32)
    pcd.tofile('%s/%s.raw'%(args.output, i))
    print('process %s:%s OK'%(i, p))

print('max points:', maxP)

with open('%s/info.txt'%(args.output), 'w') as f:
  f.write('max points: %s\n'%(maxP))
  f.write('offsetX: %s\n'%(offsetX))
  f.write('offsetY: %s\n'%(offsetY))
  f.write('ratioW: %s\n'%(ratioW))
  f.write('ratioH: %s\n'%(ratioH))
  for i, p in enumerate(pcds):
      f.write('%s: %s\n'%(i, p))

os.makedirs(args.output, exist_ok=True)

