#!/bin/bash

git log | grep "Bench:" | head -n 1 | awk '{print $NF}'
