#!/bin/bash

python -B pymavlink/tools/mavgen.py message_definitions/v1.0/common.xml --wire-protocol 1.0 --lang C
