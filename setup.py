#!/usr/bin/env python3
import sys
import os
from distutils.core import setup, Extension

if sys.version_info[0] < 3:
    boost_numpy = 'boost_numpy'
    boost_python = 'boost_python'
else:
    if os.path.exists('/usr/local/lib/libboost_python3.so') or os.path.exists('/usr/lib/x86_64-linux-gnu/libboost_python3.so'):
        boost_numpy = 'boost_numpy3'
        boost_python = 'boost_python3'
    else:
        boost_numpy = 'boost_numpy%d%d' % (sys.version_info[0], sys.version_info[1])
        boost_python = 'boost_python%d%d' % (sys.version_info[0], sys.version_info[1])
    pass

glsampler = Extension('glsampler',
        language = 'c++',
        extra_compile_args = ['-O3', '-std=c++1y', '-frtti'], 
        libraries = [boost_numpy, boost_python, 'glog', 'glfw', 'GLEW', 'GL'],
        include_dirs = ['/usr/local/include'],
        library_dirs = ['/usr/local/lib'],
        sources = ['glsampler.cpp', 'python-api.cpp'],
        depends = ['glsampler.h']
        )

setup (name = 'glsampler',
       version = '0.0.1',
       author = 'Wei Dong',
       author_email = 'wdong@wdong.org',
       license = 'BSD',
       description = 'This is a demo package',
       ext_modules = [glsampler],
       )
