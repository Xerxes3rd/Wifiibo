# coding=utf-8
# !/usr/bin/env python
# Copyright (c) 2016, Jeffrey Pfau
# Modified by Ryan Jarvis (2017)
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
"""Create a C source and header file out of one or more binaries"""

import argparse
import os
import re
import struct
import sys
import subprocess


def output_bin(in_name, out_name, h):
    basename = os.path.basename(out_name)
    symbol_name = re.sub(r"\W", "_", basename)
    define_name = "GENERATED_{}_H_INCLUDED".format(symbol_name.upper())
    gz_len = 0
	
    proc = subprocess.Popen("gzip -9 {} -c".format(in_name), stdout=subprocess.PIPE)
    #i = proc.communicate()[0]
    #print out.upper()
	
    #with open(in_name, "rb") as i:
    h.write("#ifndef {}\n".format(define_name))
    h.write("#define {}\n".format(define_name))
    h.write("\n");
    h.write("const uint8_t {}[] PROGMEM = {{\n".format(symbol_name))
    while True:
        block = proc.stdout.read(16)
        if len(block) < 16:
            if len(block):
                h.write("\t")
                for b in block:
                    # Python 2/3 compat
                    if type(b) is str:
                        b = ord(b)
                    h.write("0x{:02x}, ".format(b))
                h.write("\n")
                gz_len += len(block)
            break
        h.write("\t0x{:02x}, 0x{:02x}, 0x{:02x}, 0x{:02x}, "
                "0x{:02x}, 0x{:02x}, 0x{:02x}, 0x{:02x}, "
                "0x{:02x}, 0x{:02x}, 0x{:02x}, 0x{:02x}, "
                "0x{:02x}, 0x{:02x}, 0x{:02x}, 0x{:02x},\n"
                .format(*struct.unpack("BBBBBBBBBBBBBBBB", block)))
        gz_len += 16
    
    
    h.write("};\n")
    h.write("\n");
    h.write("#define {}_len {}\n".format(symbol_name, gz_len))
    h.write("const size_t {}_length = {};\n".format(symbol_name, gz_len))
    h.write("#endif //{}\n".format(define_name))


def output_set(out_name, in_names):
    with open(out_name + ".h", "w") as h:
        for in_name in in_names:
            output_bin(in_name, out_name, h)


def _main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('-o', metavar="OUTPUT", type=str,
                        help="output file basename")
    parser.add_argument('input', type=str, nargs='+', help="input files")
    args = parser.parse_args()
    if not args:
        sys.exit(1)
    if not args.o:
        for in_name in args.input:
            output_set(in_name, [in_name])
    else:
        output_set(args.o, args.input)


if __name__ == "__main__":
    _main()
