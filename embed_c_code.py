#!/usr/bin/env python3

################################################################################
#
# BSD 3-Clause License
#
# Copyright (c) 2019, snickerbockers
# All rights reserved.
# 
#    Redistribution and use in source and binary forms, with or without
#    modification, are permitted provided that the following conditions are
#    met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its
#    contributors may be used to endorse or promote products derived from
#    this software without specific prior written permission.
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
#
################################################################################

from getopt import getopt
import sys
import struct

inpath = None
outpath = None
varname = None
include_guard_const = None

arglist = getopt(sys.argv[1:], "i:o:t:h:")

for pair in arglist[0]:
    if pair[0] == "-i":
        inpath = pair[1]
    elif pair[0] == "-o":
        outpath = pair[1]
    elif pair[0] == "-t":
        varname = pair[1]
    elif pair[0] == "-h":
        include_guard_const = pair[1]

if (inpath is None) or (outpath is None) or (varname is None):
    print("usage: %s  -i <input_file> -o <output_file> -t <var> " + \
          "[-h include_guard_const]\n" % sys.argv[0])
    sys.exit(1)

try:
    infile = open(inpath, "rb")
    outfile = open(outpath, "w")
except OSError as err:
    print("unable to open %s" % err.filename)
    sys.exit(1)

if not (include_guard_const is None):
    outfile.write("#ifndef %s\n#define %s\n" % \
                  (include_guard_const, include_guard_const))

outfile.write("static char const %s[] = {" % varname)

inbyte = infile.read(1)
first_val = True
col = 0
bytes_total = 0
while inbyte != b"":
    if first_val:
        first_val = False
    else:
        outfile.write(",")
    if col == 0:
        outfile.write("\n\t")
    else:
        outfile.write(" ")

    col = (col + 1) % 10
    outfile.write("0x%02x" % int.from_bytes(inbyte, byteorder='little'))
    inbyte = infile.read(1)
    bytes_total += 1

print("%d bytes written" % bytes_total)
while (bytes_total % 4):
    outfile.write(",")
    if col == 0:
        outfile.write("\n\t")
    else:
        outfile.write(" ")

    col = (col + 1) % 10
    outfile.write("0x00")
    bytes_total += 1
print("After padding, size is %d" % bytes_total)

outfile.write("\n};\n");


if not (include_guard_const is None):
    outfile.write("#endif\n")

infile.close()
outfile.close()
