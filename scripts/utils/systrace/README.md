
# RideHal SysTrace parse and generation tool

The RideHalSampleApp supports to collect RideHal component events and generate a final systrace report that can be visualized by the web browser tool [chrome://tracing/](chrome://tracing/) or [edge://tracing/](edge://tracing/). Through the "Load" button to load the RideHal systrace json file.

## Collect the RideHal Systrace bin file

Before start the RideHalSampleApp, the environment variable "RIDEHAL_SYSTRACE" must be specified to turn on the RideHalSampleApp systrace. if no environment "RIDEHAL_SYSTRACE" or it was not correct specified, the systrace will be off.

Below is a sample command to specify environment "RIDEHAL_SYSTRACE" that the RideHalSampleApp will save the systrace events into file "/tmp/ridehal_systrace.bin"

```sh
export RIDEHAL_SYSTRACE=/tmp/ridehal_systrace.bin
```

After the environment "RIDEHAL_SYSTRACE" was specified, start to run the RideHalSampleApp and let it run for a while as wished and then stop it.

And pull the file "/tmp/ridehal_systrace.bin" from the target device to a ubuntu host PC.

## Parse and generate the systrace json file.

Using below command to convert the RideHal systrace bin file to a json file, but please use python version "3.x".

```sh
python scripts/utils/systrace/systrace.py -i ridehal_systrace.bin  -o ridehal_systrace.json
```

Then the file ridehal_systrace.json is the final output file that can be loaded by web tool [chrome://tracing/](chrome://tracing/) or [edge://tracing/](edge://tracing/).

Below image shows a demo of this tool.

![RideHal SysTrace Sample 01](./ridehal-systrace-sample-01.png)


# Desing reference

[[0]: Trace Event Format](https://docs.google.com/document/d/1CvAClvFfyA5R-PhYUmn5OOQtYMH4h6I0nSsKchNAySU/preview#heading=h.yr4qxyxotyw)
