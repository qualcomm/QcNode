# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# All rights reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.

import ctypes
import argparse
import json

ProcessorMap = {
  0 : "HTP0",
  1 : "HTP1",
  2 : "CPU",
  3 : "GPU",
  4 : "CAMERA",
  5 : "VPU",
  6 : "DATA_READER",
  7 : "DATA_ONLINE",
}

CatMap = {
  0 : "Init",
  1 : "Start",
  2 : "Execute",
  3 : "Stop",
  4 : "Deinit",
  5 : "FrameReady",
  6 : "VidcInputDone",
  7 : "Vidc2ndOutputDone",
  8 : "VidcInputDone",
  9 : "Vidc2ndOutputDone",
}

class MyStructure(ctypes.Structure):
    _pack_ = 1
    def from_buffer(self, raw):
        ctypes.memmove(ctypes.pointer(self), raw, ctypes.sizeof(self))

class SysTraceRecord(MyStructure):
    _fields_ = [ ('id', ctypes.c_ulonglong),
                 ('timestamp', ctypes.c_ulonglong),
                 ('processor',ctypes.c_uint),
                 ('cat',ctypes.c_uint),
                 ('name',ctypes.c_ubyte*32),
                 ('ph',ctypes.c_ubyte),
                 ('reserved',ctypes.c_ubyte*7) ]


parser = argparse.ArgumentParser(description='RideHal systrace parser')
parser.add_argument('-i', '--input', type=str, default='ridehal_systrace.bin', help='the RideHal systrace bin file path')
parser.add_argument('-o', '--output', type=str, default='ridehal_systrace.json', required=False, help='the output json file of the RideHal systrace')
args = parser.parse_args()

events = []
with open(args.input, 'rb') as f:
    record = f.read()
reLen = ctypes.sizeof(SysTraceRecord)
numRec = len(record)//reLen

lastBEEvents = {} # cache the last B/E event

for i in range(numRec):
    raw = record[i*reLen: (i+1)*reLen]
    rec = SysTraceRecord()
    rec.from_buffer(raw)
    name = bytes(rec.name).decode('utf-8').strip('\x00')
    ph = '%c'%(rec.ph)
    processor = ProcessorMap[rec.processor]
    cat = CatMap[rec.cat]
    ts = rec.timestamp
    if ph in ['B', 'E']:
        evt = { 'name': '%s'%(rec.id), 'cat': cat, 'ph': ph, 'pid': processor,
                'tid': name, 'ts': ts, 'args': { 'id': rec.id, 'timestamp': ts } }
        if cat  in ['Init', 'Start', 'Stop', 'Deinit']:
            evt['name'] = cat
        if cat in ['Execute'] and ph == 'B':
            # check the previous B and E is match, as if Execute failed, there will be no E
            if processor in lastBEEvents and name in lastBEEvents[processor]:
                levt = lastBEEvents[processor][name]
                if levt['ph'] != 'E':
                    eevt = dict(levt)
                    eevt['ph'] = 'E'
                    events.append(eevt)
                    print('WARNING: %s %s %s Execute failed' %(eevt['pid'], eevt['tid'], eevt['name']))
        if processor not in lastBEEvents:
            lastBEEvents[processor] = {}
        lastBEEvents[processor][name] = evt
    elif ph in ['X']:
        evt = { 'name': cat, 'cat': cat, 'ph': 'X', 'pid': processor, 'dur': 1,
                'tid': name, 'ts': ts, 'args': { 'id': rec.id, 'timestamp': ts } }
        if cat in ['FrameReady']:
            evt['name'] = '%s'%(rec.id)
    else:
        raise NotImplementedError
    events.append(evt)


RideHalTrace = {
  'traceEvents': events,
  'displayTimeUnit': 'ns',
  'otherData': {
    'version': 'ridehal trace v1.0.0'
  }
}

with open(args.output, 'w') as f:
    json.dump(RideHalTrace, f)
    print('generare RideHal systrace:', f.name)
