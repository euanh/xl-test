#!/bin/bash

RESOURCES_DIR=$(dirname ${BASH_SOURCE[0]})
cd $RESOURCES_DIR
wget https://download.fedoraproject.org/pub/fedora/linux/releases/22/Cloud/x86_64/Images/Fedora-Cloud-Base-22-20150521.x86_64.qcow2
