
*Menu*:
- [Tools that convert video files to the inputs of the Sample DataReader](#tools-that-convert-video-files-to-the-inputs-of-the-sample-datareader)
- [Tools that convert stereo 2015 dataset to the inputs of the Sample DataReader](#tools-that-convert-stereo-2015-dataset-to-the-inputs-of-the-sample-datareader)
- [Tools that convert point clound files to the inputs of the Sample DataReader](#tools-that-convert-point-clound-files-to-the-inputs-of-the-sample-datareader)

# Tools that convert video files to the inputs of the Sample DataReader.


## Usage

Please note to generate images with width and height is aligned according to the hardware zero copy requirement, generally, for the width, it should be 128 aligned, for the height, it should be 32 aligned.

```sh
usage: video2dr.py [-h] -i INPUT [-o OUTPUT] [-s OFFSET] [-m MAXIMUM]
                   [-r RESOLUTION] [-f FORMAT]

convert video to inputs of the Sample DataReader

optional arguments:
  -h, --help            show this help message and exit
  -i INPUT, --input INPUT
                        The input video file(*.mp4)
  -o OUTPUT, --output OUTPUT
                        The output path to store the generated inputs
  -s OFFSET, --offset OFFSET
                        the offset of the frame in the video
  -m MAXIMUM, --maximum MAXIMUM
                        The maximum number of the generated inputs
  -r RESOLUTION, --resolution RESOLUTION
                        The resolution of camera
  -f FORMAT, --format FORMAT
                        The color format of camera, [uyvy, nv12, nv21, rgb, p010]
```

## examples commands

```sh
python scripts/utils/data_reader/video2dr.py -i 4K_Street.mp4 -s 1000 -m 50 -r "3840 2176" -f nv12 -o 4K_street_1000_50_3840_2176_nv12
python scripts/utils/data_reader/video2dr.py -i 4K_Street.mp4 -s 1000 -m 50 -r "1920 1024" -f uyvy -o 4K_street_1000_50_1920_1024_uyvy
```

# Tools that convert stereo 2015 dataset to the inputs of the Sample DataReader.

## Usage

```sh
usage: stereo2dr.py [-h] -i INPUT [-o OUTPUT] [-s OFFSET] [-m MAXIMUM]
                    [-r RESOLUTION] [-f FORMAT]

convert Stereo Evaluation 2015 dataset to inputs of the Sample DataReader

optional arguments:
  -h, --help            show this help message and exit
  -i INPUT, --input INPUT
                        the input path to Stereo Evaluation 2015 dataset
  -o OUTPUT, --output OUTPUT
                        The output path to store the generated inputs
  -s OFFSET, --offset OFFSET
                        the offset of the dataset images
  -m MAXIMUM, --maximum MAXIMUM
                        The maximum number of the generated inputs
  -r RESOLUTION, --resolution RESOLUTION
                        The resolution of camera
  -f FORMAT, --format FORMAT
                        The color format of camera, [uyvy, nv12, nv21, rgb,
                        p010]
```

## example commands:

Note this tool is only verified with the dataset from [The KITTI Vision Benchmark Suite](https://www.cvlibs.net/datasets/kitti/eval_scene_flow.php?benchmark=stereo)

```sh
python scripts/utils/data_reader/stereo2dr.py -i /path/to/dataset/date_scene_flow -o stereo_2015_testing_1280_416_nv12
```

# Tools that convert point clound files to the inputs of the Sample DataReader.

## Usage

```sh
usage: lidar2dr.py [-h] -i INPUT [-o OUTPUT] [-s OFFSET] [-m MAXIMUM] [-r RANGE]

convert lidar pointcloud to inputs of the Sample DataReader

optional arguments:
  -h, --help            show this help message and exit
  -i INPUT, --input INPUT
                        The input directory that contains the pointclound
                        files
  -o OUTPUT, --output OUTPUT
                        The output path to store the generated inputs
  -s OFFSET, --offset OFFSET
                        the offset of the lidar frame in the input directory
  -m MAXIMUM, --maximum MAXIMUM
                        The maximum number of the generated inputs
  -r RANGE, --range RANGE
                        the range of points: [minX, maxX, minY, maxY, minZ, maxZ]
```


## example commands:

Note this tool is only verified with the data set from [semantic kitti velodyne](http://www.semantic-kitti.org/dataset.html).

```sh
python scripts/utils/data_reader/lidar2dr.py -i /path/to/dataset/sequences/00/velodyne -o LIDAR0 -m 500
```

Note: the "LIDAR0/info.txt" contains the "max points"/"offsetX"/"offsetY"/"ratioW"/"rationH" which will be used by the RideHalSampleApp as parameters.

```sh
# contents of LIDAR0/info.txt
max points: 126327
offsetX: 514.0645161290322
offsetY: 0
ratioW: 12.903225806451614
ratioH: 12.903225806451614
...
```