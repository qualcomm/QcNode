# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: BSD-3-Clause-Clear


from data_online import *
from threading import Thread
import numpy as np
import time

'''
sudo ufw allow proto tcp to any port 6666 from any
'''

class OnlineInferenceGateWay(Thread):
    def __init__(self, **kwargs):
        Thread.__init__(self)
        self.target = kwargs.get('target', '192.168.1.1:6666')
        self.timeout = kwargs.get('timeout', 2)
        self.dol = DataOnline(target = self.target, timeout = self.timeout)
        self.modelInfoRaw = self.dol.GetModelInfoRaw()
        self.gateway_port = kwargs.get('gateway', 6666)
        self.gateway = DataOnline(gateway = self.gateway_port, timeout = self.timeout)

    def run(self):
        while(True):
            self.gateway.Accept()
            try:
                self.Loop()
            except Exception as e:
                print('GateWay: port=%s exception: %s'%(self.gateway_port, e))
            self.gateway.Close()

    def Loop(self):
        start = time.time()
        count = 0
        while(True):
            begin = time.time()
            payload = self.gateway.ReadPayload()
            if len(payload) == 128: # only header, must be a query model info command
                self.gateway.WritePayload(self.modelInfoRaw)
                continue
            self.dol.WritePayload(payload)
            payload = self.dol.ReadPayload()
            self.gateway.WritePayload(payload)
            cost = (time.time() - begin) * 1000
            elapsed = time.time() - start
            count += 1
            FPS = count / elapsed
            print('frame ready: FPS=%.2f cost=%.2f ms'%(FPS, cost))


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description='QC online inference gateway')
    parser.add_argument('--target', type=str, default='192.168.1.1:6666', help='the target device ip address')
    parser.add_argument('--gateway', type=int, default=6666, help='the gateway port')
    args = parser.parse_args()
    olifgw = OnlineInferenceGateWay(target=args.target, gateway=args.gateway)
    olifgw.start()
    olifgw.join()