# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear-Clear


from data_online import *
import numpy as np
import time

class OnlineInference():
    def __init__(self, **kwargs):
        self.dol = DataOnline(**kwargs)
        self.modelInfo = self.dol.GetModelInfo()

    def convert(self, x, encoding):
        dtype = QCTypeToNumpyType[encoding['tensor_type']]
        if x.dtype != dtype:
            quant_scale = encoding['quant_scale']
            quant_offset = encoding['quant_offset']
            if x.dtype in [np.float32, np.float64] and dtype == np.uint8:
                # print('  auto quantize %s with quant_scale=%f quant_offset=%d'%(encoding['name'], quant_scale, quant_offset))
                x = np.clip(np.round(x/quant_scale - quant_offset), 0, 255)
                x = x.astype(np.uint8)
            elif x.dtype in [np.float32, np.float64] and dtype == np.uint16:
                x = np.clip(np.round(x/quant_scale - quant_offset), 0, 0xFFFF)
                x = x.astype(np.uint16)
            elif x.dtype in [np.float32, np.float64] and dtype == np.uint32:
                x = np.clip(np.round(x/quant_scale - quant_offset), 0, 0xFFFFFFFF)
                x = x.astype(np.uint32)
            else:
                raise Exception("don't support from dtype %s to %s for %s"%(x.dtype, dtype, encoding['name']))
        oshape = x.shape
        if list(x.shape) != encoding['shape']:
            if len(x.shape) == 5:
                x = x.transpose(0, 1, 3, 4, 2)
            elif len(x.shape) == 4:
                x = x.transpose(0, 2, 3, 1)
            elif len(x.shape) == 3:
                x = x.transpose(0, 2, 1)
            # print('  auto transpose from shape %s to %s for %s'%(oshape, encoding['shape'], encoding['name']))
        if list(x.shape) != encoding['shape']:
            raise Exception("don't support from shape %s to %s for %s"%(oshape, encoding['shape'], encoding['name']))
        return x

    def Execute(self, inputs):
        inputs_ = inputs
        kwargs = {'names':[]}
        if type(inputs) in [list, tuple]:
            inputs_ = []
            for idx, inp in enumerate(inputs):
                encoding = self.modelInfo['inputs'][idx]
                inp = self.convert(inp, encoding)
                inputs_.append(inp)
                name = encoding['name']
                kwargs['names'].append(name)
                kwargs['%s.tensor_type'%(name)] = encoding['tensor_type']
                kwargs['%s.scale'%(name)] = encoding['quant_scale']
                kwargs['%s.offset'%(name)] = encoding['quant_offset']
        elif type(inputs) == dict:
            inputs_ = []
            for idx in range(len(self.modelInfo['inputs'])):
                encoding = self.modelInfo['inputs'][idx]
                name = encoding['name']
                inp = inputs[name]
                inp = self.convert(inp, encoding)
                inputs_.append(inp)
                kwargs['names'].append(name)
                kwargs['%s.tensor_type'%(name)] = encoding['tensor_type']
                kwargs['%s.scale'%(name)] = encoding['quant_scale']
                kwargs['%s.offset'%(name)] = encoding['quant_offset']
        else:
            raise NotImplementedError
        self.dol.Write(inputs_, **kwargs)
        return self.dol.Read()


if __name__ == '__main__':
    # NOTE: This is a demo about how to use OnlineInference
    import argparse
    parser = argparse.ArgumentParser(description='QC online inference')
    parser.add_argument('--target', type=str, default='192.168.1.1:6666', help='the target device ip address')
    args = parser.parse_args()
    olif = OnlineInference(target=args.target)
    infos = olif.modelInfo
    print(infos)
    start = time.time()
    count = 0
    while(True):
        inputs = [np.zeros(infos['inputs'][0]['shape'], np.float32)]
        begin = time.time()
        Id, timestamp, outputs = olif.Execute(inputs)
        cost = (time.time() - begin) * 1000
        elapsed = time.time() - start
        count += 1
        FPS = count / elapsed
        print('frame ready: id=%s, timestamp=%s FPS=%.2f cost=%.2f ms'%(Id, timestamp, FPS, cost))
