# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# All rights reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.

import ctypes
import argparse
import json

TraceTypeMap = {
    0: "B",
    1: "E",
    2: "X",
    3: "C",
}


class MyStructure(ctypes.Structure):
    _pack_ = 1

    def from_buffer(self, raw):
        ctypes.memmove(ctypes.pointer(self), raw, ctypes.sizeof(self))


class NodeTraceEventHeader(MyStructure):
    _fields_ = [
        ("timestamp", ctypes.c_uint64),
        ("traceType", ctypes.c_int32),
        ("lenName", ctypes.c_uint32),
        ("lenProcessor", ctypes.c_uint32),
        ("lenEventName", ctypes.c_uint32),
        ("coreIdsMask", ctypes.c_uint32),
        ("numArgs", ctypes.c_uint32),
    ]


class NodeTraceEventArg(MyStructure):
    _fields_ = [
        ("argType", ctypes.c_int32),
        ("lenName", ctypes.c_uint32),
        ("lenValue", ctypes.c_uint32),
    ]


parser = argparse.ArgumentParser(description="QCNode systrace parser")
parser.add_argument("-i", "--input", type=str, default="qcnode_systrace.bin", help="the QC systrace bin file path")
parser.add_argument(
    "-o",
    "--output",
    type=str,
    default="qcnode_systrace.json",
    required=False,
    help="the output json file of the QCNode systrace",
)
pargs = parser.parse_args()

events = []
with open(pargs.input, "rb") as f:
    record = f.read()
evthLen = ctypes.sizeof(NodeTraceEventHeader)
arghLen = ctypes.sizeof(NodeTraceEventArg)

lastBEEvents = {}  # cache the last B/E event

offset = 0
recordLen = len(record)


def toCoreIds(coreIdsMask):
    coreIds = []
    for i in range(32):
        if (coreIdsMask & (1 << i)) != 0:
            coreIds.append(i)
    return coreIds


while offset < recordLen:
    raw = record[offset : offset + evthLen]
    evth = NodeTraceEventHeader()
    evth.from_buffer(raw)
    ph = TraceTypeMap[evth.traceType]
    name = record[offset + evthLen : offset + evthLen + evth.lenName]
    name = name.decode("utf-8")
    processor = record[offset + evthLen + evth.lenName : offset + evthLen + evth.lenName + evth.lenProcessor]
    processor = processor.decode("utf-8")
    processor = processor.upper()
    evtName = record[
        offset
        + evthLen
        + evth.lenName
        + evth.lenProcessor : offset
        + evthLen
        + evth.lenName
        + evth.lenProcessor
        + evth.lenEventName
    ]
    evtName = evtName.decode("utf-8")
    offset += evthLen + evth.lenName + evth.lenProcessor + evth.lenEventName
    args = {"timestamp": evth.timestamp}
    if 0 != evth.coreIdsMask:
        args["coreIds"] = toCoreIds(evth.coreIdsMask)
    for i in range(evth.numArgs):
        raw = record[offset : offset + arghLen]
        argh = NodeTraceEventArg()
        argh.from_buffer(raw)
        argName = record[offset + arghLen : offset + arghLen + argh.lenName]
        argName = argName.decode("utf-8")
        argValue = record[offset + arghLen + argh.lenName : offset + arghLen + argh.lenName + argh.lenValue]
        offset += arghLen + argh.lenName + argh.lenValue
        if argh.argType == 0:  # string
            argValue = argValue.decode("utf-8")
        elif argh.argType == 1:
            argValue = ctypes.c_double.from_buffer_copy(argValue).value
        elif argh.argType == 2:
            argValue = ctypes.c_float.from_buffer_copy(argValue).value
        elif argh.argType == 3:
            argValue = ctypes.c_uint64.from_buffer_copy(argValue).value
        elif argh.argType == 4:
            argValue = ctypes.c_uint32.from_buffer_copy(argValue).value
        elif argh.argType == 5:
            argValue = ctypes.c_uint16.from_buffer_copy(argValue).value
        elif argh.argType == 6:
            argValue = ctypes.c_uint8.from_buffer_copy(argValue).value
        elif argh.argType == 7:
            argValue = ctypes.c_int64.from_buffer_copy(argValue).value
        elif argh.argType == 8:
            argValue = ctypes.c_int32.from_buffer_copy(argValue).value
        elif argh.argType == 9:
            argValue = ctypes.c_int16.from_buffer_copy(argValue).value
        elif argh.argType == 10:
            argValue = ctypes.c_int8.from_buffer_copy(argValue).value
        args[argName] = argValue
    cat = evtName
    ts = evth.timestamp
    if ph in ["B", "E"]:
        title = args.get("frameId", cat)
        evt = {
            "name": "%s" % (title),
            "cat": cat,
            "ph": ph,
            "pid": processor,
            "tid": name,
            "ts": ts,
            "args": args,
        }
        if cat == "Start": print(evt)
        if cat in ["Execute"] and ph == "B":
            # check the previous B and E is match, as if Execute failed, there will be no E
            if processor in lastBEEvents and title in lastBEEvents[processor]:
                levt = lastBEEvents[processor][title]
                if levt["ph"] != "E":
                    eevt = dict(levt)
                    eevt["ph"] = "E"
                    events.append(eevt)
                    print("WARNING: %s %s %s Execute failed" % (eevt["pid"], eevt["tid"], eevt["cat"]))
        if processor not in lastBEEvents:
            lastBEEvents[processor] = {}
        lastBEEvents[processor][title] = evt
    elif ph in ["X"]:
        evt = {
            "name": cat,
            "cat": cat,
            "ph": "X",
            "pid": processor,
            "dur": 1,
            "tid": name,
            "ts": ts,
            "args": args,
        }
        if cat in ["FrameReady"]:
            if "frameId" in args:
                evt["name"] = "%s" % (args["frameId"])
    else:
        raise NotImplementedError
    events.append(evt)


QCTrace = {"traceEvents": events, "displayTimeUnit": "ns", "otherData": {"version": "qcnode trace v1.0.0"}}

with open(pargs.output, "w") as f:
    json.dump(QCTrace, f)
    print("generare QCNode systrace:", f.name)
