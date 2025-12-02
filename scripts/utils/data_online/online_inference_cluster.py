# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear


from online_inference import OnlineInference
from threading import Thread
from queue import Queue
import numpy as np
import time
import sys
import argparse
import json

class OnlineInferenceCluster():
    def __init__(self, **kwargs):
        self.olifs = []
        for target in kwargs['targets']:
            olif = OnlineInference(target = target)
            mqw = Queue()
            mqr = Queue()
            thr = Thread(target=self.threadRunner, args=(target, olif, mqw, mqr))
            self.olifs.append((thr, olif, mqw, mqr))
            thr.start()
        self.batch = len(self.olifs)
        self.session = 0

    def threadRunner(self, target, olif, mqw, mqr):
        print('target %s online'%(target))
        while(True):
          inputs = mqw.get()
          outputs = olif.Execute(inputs)
          mqr.put(outputs)

    def Execute(self, inputs):
        session = self.session
        self.session += 1
        if type(inputs) in [list, tuple]:
            batchs = [x.shape[0] for x in inputs]
            batch = batchs[0]
            if not all([batch == b for b in batchs]):
                raise Exception("not all inputs batch equal")
            for i in range(batch):
                inps = []
                for x in inputs:
                  x = x[i]
                  x = x.reshape([1] + list(x.shape))
                  inps.append(x)
                _, _, mqw, _ = self.olifs[i % self.batch]
                mqw.put(inps)
        elif type(inputs) == dict:
            batchs = [x.shape[0] for _, x in inputs.items()]
            batch = batchs[0]
            if not all([batch == b for b in batchs]):
                raise Exception("not all inputs batch equal")
            for i in range(batch):
                inps = {}
                for k, x in inputs.items():
                  x = x[i]
                  x = x.reshape([1] + list(x.shape))
                  inps[k] = x
                _, _, mqw, _ = self.olifs[i % self.batch]
                mqw.put(inps)
        else:
            raise
        timestamp=None
        outputs=None
        for i in range(batch):
            _, _, _, mqr = self.olifs[i % self.batch]
            _, timestamp_, outputs_ = mqr.get()
            if timestamp == None:
                timestamp, outputs = timestamp_, outputs_
            else:
                for k, v in outputs.items():
                    outputs[k] = np.concatenate((v, outputs_[k]))
        return session, timestamp, outputs

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='QC online inference cluster')
    parser.add_argument('--targets', nargs='*', type=str, default=['192.168.1.1:6666', '192.168.1.1:6667'],
      help='the target infomation list:\n\tip:port')
    parser.add_argument('--batch', type=int, default=10, help='the batch size to test')
    args = parser.parse_args()
    batch = args.batch
    olifc = OnlineInferenceCluster(targets=args.targets)
    start = time.time()
    count = 0
    while(True):
        inputs = [np.zeros([batch, 3, 800, 1152], np.float32)]
        begin = time.time()
        Id, timestamp, outputs = olifc.Execute(inputs)
        cost = (time.time() - begin) * 1000
        elapsed = time.time() - start
        count += 1
        FPS = batch * count / elapsed
        print('frame ready: id=%s, timestamp=%s FPS=%.2f cost=%.2f ms'%(Id, timestamp, FPS, cost))
        print('  outputs:', [(k, v.shape) for k,v in outputs.items()])