#!/usr/bin/env python3
import sys
sys.path.append('build/lib.linux-x86_64-' + sys.version[:3])
import time
import os
import numpy as np
import glsampler



sampler = glsampler.Sampler(False)
v = np.random.randint(0, 255, (512,512,512), dtype=np.uint8)
for i in range(1000):
    print(i)
    sampler.load(v)
    sampler.sample(256,256,256,0,0,0,1)
    pass

glsampler.cleanup()

