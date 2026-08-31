#!/bin/bash

# NestDAQ topology fragment for NestDAQ -> EICrecon via ZeroMQ.
# Keep this port synchronized with ZMQ_PORT used by the EICrecon launcher.
ZMQ_PORT="${ZMQ_PORT:-5501}"

echo "---------------------------------------------------------------------"
echo " config endpoint (socket)"
echo "---------------------------------------------------------------------"

endpoint STFBFilePlayer out \
    type push \
    method connect \
    autoSubChannel true

endpoint TimeFrameBuilder in \
    type pull \
    method bind

endpoint TimeFrameBuilder out \
    type push \
    method bind \
    enable-uds false \
    portRangeMin "${ZMQ_PORT}" \
    portRangeMax "${ZMQ_PORT}"

echo "---------------------------------------------------------------------"
echo " config link"
echo "---------------------------------------------------------------------"

link STFBFilePlayer out TimeFrameBuilder in

# No NestDAQ sink is linked to TimeFrameBuilder:out here.
# EICrecon connects externally using a ZeroMQ PULL socket.
