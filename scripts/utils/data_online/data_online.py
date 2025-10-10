# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear-Clear

import socket
import struct
import numpy as np
import time
import ctypes

META_COMMAND_DATA = 0
META_COMMAND_MODEL_INFO = 1

QC_BUFFER_TYPE_RAW = 1
QC_BUFFER_TYPE_IMAGE = 2
QC_BUFFER_TYPE_TENSOR = 3

QC_IMAGE_FORMAT_RGB888 = 0
QC_IMAGE_FORMAT_BGR888 = 1
QC_IMAGE_FORMAT_UYVY = 2
QC_IMAGE_FORMAT_NV12 = 3
QC_IMAGE_FORMAT_P010 = 4

QC_IMAGE_FORMAT_COMPRESSED_H264 = 100
QC_IMAGE_FORMAT_COMPRESSED_H265 = 101

QCFormatMap = {
    #      format, numPlanes, bytes_per_pixel
    "rgb": (QC_IMAGE_FORMAT_RGB888, 1, 3),
    "bgr": (QC_IMAGE_FORMAT_BGR888, 1, 3),
    "uyvy": (QC_IMAGE_FORMAT_UYVY, 1, 2),
    "nv12": (QC_IMAGE_FORMAT_NV12, 2, 1),
    "p010": (QC_IMAGE_FORMAT_P010, 2, 2),
    "h264": (QC_IMAGE_FORMAT_COMPRESSED_H264, 1, 1),
    "h265": (QC_IMAGE_FORMAT_COMPRESSED_H265, 1, 1),
}

QC_TENSOR_TYPE_INT_8 = 0
QC_TENSOR_TYPE_INT_16 = 1
QC_TENSOR_TYPE_INT_32 = 2
QC_TENSOR_TYPE_INT_64 = 3

QC_TENSOR_TYPE_UINT_8 = 4
QC_TENSOR_TYPE_UINT_16 = 5
QC_TENSOR_TYPE_UINT_32 = 6
QC_TENSOR_TYPE_UINT_64 = 7

QC_TENSOR_TYPE_FLOAT_16 = 8
QC_TENSOR_TYPE_FLOAT_32 = 9
QC_TENSOR_TYPE_FLOAT_64 = 10

QC_TENSOR_TYPE_SFIXED_POINT_8 = 11
QC_TENSOR_TYPE_SFIXED_POINT_16 = 12
QC_TENSOR_TYPE_SFIXED_POINT_32 = 13

QC_TENSOR_TYPE_UFIXED_POINT_8 = 14
QC_TENSOR_TYPE_UFIXED_POINT_16 = 15
QC_TENSOR_TYPE_UFIXED_POINT_32 = 16

QCTypeToNumpyType = {
    QC_TENSOR_TYPE_INT_8 : np.int8,
    QC_TENSOR_TYPE_INT_16 : np.int16,
    QC_TENSOR_TYPE_INT_32 : np.int32,
    QC_TENSOR_TYPE_INT_64 : np.int64,

    QC_TENSOR_TYPE_UINT_8 : np.uint8,
    QC_TENSOR_TYPE_UINT_16 : np.uint16,
    QC_TENSOR_TYPE_UINT_32 : np.uint32,
    QC_TENSOR_TYPE_UINT_64 : np.uint64,

    QC_TENSOR_TYPE_FLOAT_16 : np.float16,
    QC_TENSOR_TYPE_FLOAT_32 : np.float32,
    QC_TENSOR_TYPE_FLOAT_64 : np.float64,

    QC_TENSOR_TYPE_SFIXED_POINT_8 : np.int8,
    QC_TENSOR_TYPE_SFIXED_POINT_16 : np.int16,
    QC_TENSOR_TYPE_SFIXED_POINT_32 : np.int32,

    QC_TENSOR_TYPE_UFIXED_POINT_8 : np.uint8,
    QC_TENSOR_TYPE_UFIXED_POINT_16 : np.uint16,
    QC_TENSOR_TYPE_UFIXED_POINT_32 : np.uint32,
}

class MyStructure(ctypes.Structure):
    _pack_ = 1
    def from_buffer(self, raw):  
        ctypes.memmove(ctypes.pointer(self), raw, ctypes.sizeof(self))

    def tobytes(self):
        return bytes(self)

class Meta(MyStructure):
    _fields_ = [ ('payloadSize', ctypes.c_ulonglong),
                 ('Id', ctypes.c_ulonglong),
                 ('timestamp',ctypes.c_ulonglong),
                 ('numOfDatas',ctypes.c_uint),
                 ('command',ctypes.c_uint),
                 ('reserved',ctypes.c_ubyte*96) ]

class MetaModelInfo(MyStructure):
    _fields_ = [ ('payloadSize', ctypes.c_ulonglong),
                 ('Id', ctypes.c_ulonglong),
                 ('timestamp',ctypes.c_ulonglong),
                 ('numOfDatas',ctypes.c_uint),
                 ('command',ctypes.c_uint),
                 ('numInputs',ctypes.c_uint),
                 ('numOuputs',ctypes.c_uint),
                 ('reserved',ctypes.c_ubyte*88) ]

class DataImageMeta(MyStructure):
    _fields_ = [ ('dataType', ctypes.c_uint),
                 ('size', ctypes.c_uint),
                 ('format',ctypes.c_uint),
                 ('batchSize',ctypes.c_uint),
                 ('width',ctypes.c_uint),
                 ('height',ctypes.c_uint),
                 ('stride',ctypes.c_uint*4),
                 ('actualHeight',ctypes.c_uint*4),
                 ('planeBufSize',ctypes.c_uint*4),
                 ('numPlanes',ctypes.c_uint),
                 ('reserved',ctypes.c_ubyte*52) ]

class DataImage():
    def __init__(self, **kwargs):
        self.raw = kwargs['raw']
        if 'meta' in kwargs:
            self.meta = kwargs['meta']
        if 'format' in kwargs:
            format, numPlanes, bps  = QCFormatMap[kwargs['format']]
            self.meta = DataImageMeta()
            self.meta.dataType = QC_BUFFER_TYPE_IMAGE
            self.meta.size = len(self.raw)
            self.meta.format = format
            self.meta.batchSize = 1
            self.meta.width = kwargs['width']
            self.meta.height = kwargs['height']
            for i in range(numPlanes):
                self.meta.stride[i] = self.meta.width * bps
            self.meta.actualHeight[0] = self.meta.height
            self.meta.planeBufSize[0] = self.meta.height * self.meta.stride[i]
            if format in [QC_IMAGE_FORMAT_NV12,  QC_IMAGE_FORMAT_P010]:
                self.meta.actualHeight[1] = self.meta.height//2
                self.meta.planeBufSize[1] = self.meta.height * self.meta.stride[i] // 2
            self.meta.numPlanes = numPlanes

    def tobytes(self):
        return self.raw

    @property
    def nbytes(self):
        return len(self.raw)

class DataMeta(MyStructure):
    _fields_ = [ ('dataType', ctypes.c_uint),
                 ('size', ctypes.c_uint),
                 ('reserved',ctypes.c_ubyte*120) ]

class DataTensorMeta(MyStructure):
    _fields_ = [ ('dataType', ctypes.c_uint),
                 ('size', ctypes.c_uint),
                 ('type',ctypes.c_uint),
                 ('dims',ctypes.c_uint*8),
                 ('numDims',ctypes.c_uint),
                 ('quantScale',ctypes.c_float),
                 ('quantOffset',ctypes.c_int),
                 ('name',ctypes.c_ubyte*64),
                 ('reserved',ctypes.c_ubyte*8) ]

class DataOnline():
    def __init__(self, **kwargs):
        self.target = kwargs.get('target', '192.168.1.1:6666')
        self.timeout = kwargs.get('timeout', 2)
        ip, port = self.target.split(':')
        if 'target' in kwargs:
            self.target = kwargs['target']
            print('Create DataOnline Target: target = %s' % (self.target))
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.connect((ip, int(port)))
            if self.timeout > 0:
                timeout = struct.pack('<QQ', self.timeout, 0)
                self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_SNDTIMEO, timeout)
        else:
            self.gateway_port = kwargs['gateway']
            print('Create DataOnline Gateway: port = %s' % (self.gateway_port))
            self.server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self.server.bind(('', self.gateway_port))
            self.server.listen(1)
            
        self.session = 0
        self.start_time = time.time()

    def Accept(self):
        if hasattr(self, 'server'):
            self.sock, addr = self.server.accept()
            self.peer_addr = addr
        else:
            raise Exception("this is not a gateway DataOnline")

    def Close(self):
        if hasattr(self, 'server'):
            self.sock.close()
        else:
            raise Exception("this is not a gateway DataOnline")

    def GetModelInfoRaw(self):
        meta = Meta()
        meta.command = META_COMMAND_MODEL_INFO
        self.sock.send(meta.tobytes())
        start = time.time()
        meta = self.sock.recv(128)
        while(len(meta) < 128):
            meta += self.sock.recv(128 - len(meta))
            elapsed = time.time() - start
            if (self.timeout > 0 and elapsed > self.timeout):
                raise Exception('timeout')
        modelInfo = MetaModelInfo()
        modelInfo.from_buffer(meta)
        if 0 == modelInfo.numOfDatas:
            raise Exception('No model information')
        else:
            size = modelInfo.numOfDatas * 128
            start = time.time()
            payload = self.sock.recv(size)
            while(len(payload) < size):
                payload += self.sock.recv(size - len(payload))
                elapsed = time.time() - start
                if (self.timeout > 0 and elapsed > self.timeout):
                    raise Exception('timeout')
        return meta + payload

    def GetModelInfo(self):
        infos= {'inputs': [], 'outputs': []}
        meta = Meta()
        meta.command = META_COMMAND_MODEL_INFO
        self.sock.send(meta.tobytes())
        start = time.time()
        meta = self.sock.recv(128)
        while(len(meta) < 128):
            meta += self.sock.recv(128 - len(meta))
            elapsed = time.time() - start
            if (self.timeout > 0 and elapsed > self.timeout):
                raise Exception('timeout')
        modelInfo = MetaModelInfo()
        modelInfo.from_buffer(meta)
        if 0 == modelInfo.numOfDatas:
            raise Exception('No model information')
        else:
            size = modelInfo.numOfDatas * 128
            start = time.time()
            payload = self.sock.recv(size)
            while(len(payload) < size):
                payload += self.sock.recv(size - len(payload))
                elapsed = time.time() - start
                if (self.timeout > 0 and elapsed > self.timeout):
                    raise Exception('timeout')
            for i in range(modelInfo.numInputs):
                raw = payload[i*128: (i+1)*128]
                tsMeta = DataTensorMeta()
                tsMeta.from_buffer(raw)
                name = bytes(tsMeta.name).decode('utf-8').strip('\x00')
                dims = [tsMeta.dims[i] for i in range(tsMeta.numDims)]
                info = { 'name': name, 'shape': dims, 'tensor_type': tsMeta.type,
                         'quant_scale': tsMeta.quantScale,
                         'quant_offset': tsMeta.quantOffset }
                infos['inputs'].append(info)
            for i in range(modelInfo.numOuputs):
                i += modelInfo.numInputs
                raw = payload[i*128: (i+1)*128]
                tsMeta = DataTensorMeta()
                tsMeta.from_buffer(raw)
                name = bytes(tsMeta.name).decode('utf-8').strip('\x00')
                dims = [tsMeta.dims[i] for i in range(tsMeta.numDims)]
                info = { 'name': name, 'shape': dims, 'tensor_type': tsMeta.type,
                         'quant_scale': tsMeta.quantScale,
                         'quant_offset': tsMeta.quantOffset }
                infos['outputs'].append(info)
        return infos
    
    def WritePayload(self, payload):
        self.sock.send(payload)

    # datas: a list of numpy tensors
    # kwargs: additional information for the data
    def Write(self, datas, **kwargs):
        meta = Meta()
        meta.Id = kwargs.get('Id', self.session)
        timestamp = int((time.time() - self.start_time) * 1000000000) # timestamp in ns
        meta.timestamp = kwargs.get('timestamp', timestamp)
        meta.payloadSize = 128 * len(datas)
        for data in datas:
            meta.payloadSize += data.nbytes
        meta.numOfDatas = len(datas)
        meta.commad = META_COMMAND_DATA
        payload = meta.tobytes()
        names = kwargs.get('names', ['input:%s'%(i) for i in range(len(datas))])
        for index, data in enumerate(datas):
            name = names[index]
            data_type = kwargs.get('%s.data_type'%(name), QC_BUFFER_TYPE_TENSOR)
            if QC_BUFFER_TYPE_TENSOR == data_type:
                tsM = DataTensorMeta()
                namebs = name.encode('utf-8')
                ctypes.memmove(ctypes.pointer(tsM.name), namebs, len(namebs))
                tsM.dataType = data_type
                tsM.size = data.nbytes
                tsM.type = kwargs.get('%s.tensor_type'%(name), QC_TENSOR_TYPE_UFIXED_POINT_8)
                tsM.dims = data.shape
                tsM.numDims = len(data.shape)
                tsM.quantScale = kwargs.get('%s.scale'%(name), 1.0)
                tsM.quantOffset = kwargs.get('%s.offset'%(name), 0)
                payload += tsM.tobytes()
            else:
                assert(isinstance(data, DataImage))
                payload += data.meta.tobytes()
        for index, data in enumerate(datas):
            payload += data.tobytes()
        self.sock.send(payload)
        self.session += 1

    def ReadPayload(self):
        start = time.time()
        payload = self.sock.recv(128)
        while(len(payload) < 128):
            payload += self.sock.recv(128 - len(payload))
            elapsed = time.time() - start
            if (self.timeout > 0 and elapsed > self.timeout):
                raise Exception('timeout')
        meta = Meta()
        meta.from_buffer(payload)
        size = meta.payloadSize + 128
        start = time.time()
        payload += self.sock.recv(size - len(payload))
        while(len(payload) < size):
            payload += self.sock.recv(size - len(payload))
            elapsed = time.time() - start
            if (self.timeout > 0 and elapsed > self.timeout):
                raise Exception('timeout')
        return payload

    def Read(self):
        datas = {}
        start = time.time()
        payload = self.sock.recv(128)
        while(len(payload) < 128):
            payload += self.sock.recv(128 - len(payload))
            elapsed = time.time() - start
            if (self.timeout > 0 and elapsed > self.timeout):
                raise Exception('timeout')
        meta = Meta()
        meta.from_buffer(payload)
        if 0 == meta.numOfDatas or META_COMMAND_DATA != meta.command:
            raise Exception('no datas')
        size = meta.payloadSize
        start = time.time()
        payload = self.sock.recv(size)
        while(len(payload) < size):
            payload += self.sock.recv(size - len(payload))
            elapsed = time.time() - start
            if (self.timeout > 0 and elapsed > self.timeout):
                raise Exception('timeout')
        offset = 128 * meta.numOfDatas
        for i in range(meta.numOfDatas):
            raw = payload[i*128: (i+1)*128]
            daMeta = DataMeta()
            daMeta.from_buffer(raw)
            if daMeta.dataType == QC_BUFFER_TYPE_TENSOR:
                tsMeta = DataTensorMeta()
                tsMeta.from_buffer(raw)
                name = bytes(tsMeta.name).decode('utf-8').strip('\x00')
                dims = [tsMeta.dims[i] for i in range(tsMeta.numDims)]
                rawDa = payload[offset:offset+tsMeta.size]
                dtype = QCTypeToNumpyType[tsMeta.type]
                data = np.frombuffer(rawDa, dtype)
                if tsMeta.type >= QC_TENSOR_TYPE_SFIXED_POINT_8 and tsMeta.type <= QC_TENSOR_TYPE_UFIXED_POINT_32:
                    data = tsMeta.quantScale*(data.astype(np.float32) + tsMeta.quantOffset)
                dims = [tsMeta.dims[i] for i in range(tsMeta.numDims)]
                datas[name] = data.reshape(dims)
                offset += tsMeta.size
            else:
                imgMeta = DataImageMeta()
                imgMeta.from_buffer(raw)
                rawDa = payload[offset:offset+imgMeta.size]
                offset += imgMeta.size
                datas[i] = DataImage(meta=imgMeta, raw=rawDa)
        return meta.Id, meta.timestamp, datas


if __name__ == '__main__':
    # NOTE: This is a demo about how to use DataOnline
    import argparse
    import glob
    parser = argparse.ArgumentParser(description='QC online inference')
    parser.add_argument('--target', type=str, default='192.168.1.1:6666', help='the target device ip address')
    parser.add_argument('--data_path', type=str, default=None, required=False, help='the image data path')
    parser.add_argument('--format', type=str, default='nv12', required=False, help='the image format')
    parser.add_argument('--resolution', type=str, default="1920 1024", required=False, help='the image resolution')
    args = parser.parse_args()
    dol = DataOnline(target=args.target)
    start = time.time()
    count = 0
    if args.data_path == None:
        infos = dol.GetModelInfo()
        print(infos)
        tensors = [np.zeros(infos['inputs'][0]['shape'], np.uint8)]
        while(True):
            dol.Write(tensors)
            Id, timestamp, outputs = dol.Read()
            elapsed = time.time() - start
            FPS = count / elapsed
            print('frame ready: id=%s, timestamp=%s FPS=%.2f'%(Id, timestamp, FPS))
            for name, data in outputs.items():
                print('  %s: min=%s, max=%s, shape=%s'%(name, data.min(), data.max(), data.shape))
            count += 1
    else:
        # test image write
        W,H = [int(x) for x in args.resolution.split(' ')]
        imgFiles = glob.glob('%s/*.%s'%(args.data_path, args.format))
        for imgF in imgFiles:
            img = np.fromfile(imgF, np.uint8)
            drimg = DataImage(format=args.format, width=W, height=H, raw=img.tobytes())
            kwargs = {'input:0.data_type': QC_BUFFER_TYPE_IMAGE}
            dol.Write([drimg], **kwargs)
            count += 1
            elapsed = time.time() - start
            FPS = count / elapsed
            print('image: id=%s, FPS=%.2f'%(count, FPS))
