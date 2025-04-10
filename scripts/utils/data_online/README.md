*Menu*:
- [1. Overview](#1-overview)
  - [1.1 Local PC online inference mode](#11-local-pc-online-inference-mode)
  - [1.2 Remote PC gateway online inference mode](#12-remote-pc-gateway-online-inference-mode)
  - [1.3 Cluster online inference mode](#13-cluster-online-inference-mode)
- [2. How to use this tool](#2-how-to-use-this-tool)
  - [2.1 Launch the related RideHalSampleApp Data Online pileine.](#21-launch-the-related-ridehalsampleapp-data-online-pileine)
  - [2.2 Launch a gateway if needed](#22-launch-a-gateway-if-needed)
  - [2.3 Run the accuracy tool on the PC with datasets](#23-run-the-accuracy-tool-on-the-pc-with-datasets)

# RideHal Data Online Framework

## 1. Overview

This is a python data online framework that through the python module [DataOnline](./data_online.py#L169) to setup TCP socket connections with RideHalSampleApp through the [SampleDataOnline](../../../tests/sample/include/ridehal/sample/SampleDataOnline.hpp#L18).

And the most common use case is the QNN model accuracy test, and this is supported through the bellowing 3 python module.

  - [OnlineInference](./online_inference.py#L8): Online inference with target device.
  - [OnlineInferenceGateWay](./online_inference_gateway.py#L8): A gateway to the online inference with target device.
  - [OnlineInferenceCluster](./online_inference_cluster.py#L8): A batch mode online inference with a list of target device(including gateway) to speed up.

### 1.1 Local PC online inference mode

```
       +------------------+
       |   Accuracy Tool  |
       +------------------+
              | ^
              | | Execute
              V |
       +------------------+
       | Online Inference |
       +------------------+
              | ^
        Write | | Read
              V |
       +------------------+
       |   Data Online    |
       +------------------+
              | ^
              | | TCP: 6666
              V |
       +------------------+
       | SampleDataOnline |
       +------------------+
              | ^
              | | DataBroker
              V |
       +-----------------+
       |    SampleQnn    |
       +-----------------+
```

### 1.2 Remote PC gateway online inference mode

```
       +------------------+
       |   Accuracy Tool  |
       +------------------+
              | ^
              | | Execute
              V |
       +------------------+
       | Online Inference |
       +------------------+
              | ^
        Write | | Read
              V |
       +------------------+
       |   Data Online    |
       +------------------+
              | ^
              | | TCP: 6666
              V |
  +--------------------------+
  | Online Inference GateWay |
  +--------------------------+
              | ^
 WritePayload | | ReadPayload
              V |
       +------------------+
       |   Data Online    |
       +------------------+
              | ^
              | | TCP: 6666
              V |
       +------------------+
       | SampleDataOnline |
       +------------------+
              | ^
              | | DataBroker
              V |
       +-----------------+
       |    SampleQnn    |
       +-----------------+
```

### 1.3 Cluster online inference mode

This is a mode that generally a PC with more than 1 target device connected through multiple ethernet port, to use this mode, the user must know how to configure each device IP and the PC ethernet IP to ensure each of the target device can be accessed by the PC.
But this also support that a PC can have several remote gateway PC.

As below diagram shows, 3 batched inputs to the online inference cluster will be divided into 3 one batch input and forward to the 3 online inference at the same time thus will improve the performance 3 times.

```
                                         +---------------+
                                         | Accuracy Tool |
                                         +---------------+
                                                 ^
                                                 |  Execute ( [ (batch, C, H, W) ])
                                                 V               here batch = 3, but can be any value in fact
                                    +--------------------------+
                                    | Online Inference Cluster |
                                    +--------------------------+
                                              ^  ^  ^
                     ._______________________/   |   \_______________________.
                     | batch 0                   | batch 1           batch 2 |
                     V                           V                           V
        +------------------+            +------------------+            +------------------+
        | Online Inference |            | Online Inference |            | Online Inference |
        +------------------+            +------------------+            +------------------+
                ^                                ^                               ^
                |                                |                               |
                V                                V                               V
      +-------------------+            +-------------------+           +-------------------+
      | Device or GATEWAY |            | Device or GATEWAY |           | Device or GATEWAY |
      +-------------------+            +-------------------+           +-------------------+
```

## 2. How to use this tool

### 2.1 Launch the related RideHalSampleApp Data Online pileine.

For how to use RideHalSampleApp, refer RideHalSampleApp [`user guide`](../../../tests/sample/README.md#212-ridehal-dataonline-sample).

For how to configure a typical data online inference pipeline, refer RideHalSampleApp [`1 QNN model data online inference pipeline`](../../../tests/sample/README.md#34-1-qnn-model-data-online-inference-pipeline).

### 2.2 Launch a gateway if needed

```sh
python scripts/utils/data_online/online_inference_gateway.py --target 192.168.1.1:6666
```

### 2.3 Run the accuracy tool on the PC with datasets

Please note to specify the correct target "IP:port" address for either the remote gateway PC or the target device.

```python
# remote is a gateway PC
olif = OnlineInference(target='10.91.9.9:6666')
```

```python
# remote is a target device
olif = OnlineInference(target='192.168.1.1:6666')
```

Using cluser mode to speed up.

```python
olif = OnlineInferenceCluster(targets=["192.168.1.1:6666", "192.168.1.2:6666"])
```

Run the accuracy tool by calling the API [Execute](./online_inference.py#L43) provided by the online inference, or the API [Execute](./online_inference_cluster.py#L33) by the online inference cluster. Please note that the input must has the same data type and shape as the QNN model, so transpose and quantization is required generally, but it was simple.

Below is some sample python code show how to do transpose and quantization. And this is optional, a float NCHW format can also be provided to the online inference and the online inference itself can help automatically do this and convert the float NCHW tensors to quantized uint8 NHWC tensors for QNN.

```python
# transpose to NHWC from NCHW
input = input.transpose(0, 2, 3, 1)
input = numpy.clip(numpy.round(input/scale + offset), 0, 255)
input = input.astype(numpy.uint8)

Id, timestamp, outputs = olif.Execute([input])
```

And for the "outputs" of the [Exeute](./online_inference.py#L43) API, the dequantization will be automatically done, so the outputs is generally a dict that contains float tensors.
