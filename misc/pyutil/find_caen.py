#!/usr/bin/env python
#-*- encoding: utf-8 -*-

from PyMVME import *
import sys, os

def check_mod_rom(mdev, addr):
    r_am = mdev.set_am("A32_ND")
    r_dm = mdev.set_dmode("D16")
    if r_dm[0]!="SUCCESS" or r_am[0]!="SUCCESS": return False

    i = mdev.read(addr+0x8026) & 0xFF
    j = mdev.read(addr+0x802A) & 0xFF
    k = mdev.read(addr+0x802E) & 0xFF
    manu = (i<<12) | (j<<8) | k

    ver = mdev.read(addr+0x8032) & 0xFF

    i = mdev.read(addr+0x8036) & 0xFF
    j = mdev.read(addr+0x803A) & 0xFF
    k = mdev.read(addr+0x803E) & 0xFF
    b_id = (i<<12) | (j<<8) | k

    rev = mdev.read(addr+0x804E) & 0xFF

    i = mdev.read(addr+0x8F02) & 0xFF
    j = mdev.read(addr+0x8F06) & 0xFF
    ser = (i<<8) | j

    return [manu, b_id, ver, ser, rev]

mdev = MvmeDev(libname="../sis3100/libmvme_sis3100.so")

print "Opening the VME bus:",  mdev.open()
print "Reseting the VME bus:", mdev.sysreset()

mdev.set_am("A32_ND")
mdev.set_dmode("D16")
fmt="0x%08x: Vendor=0x%06x; Board=0x%06x; Ver=0x%02x; Ser=0x%02x; Rev=0x%02x"

# if len(sys.argv)<=1: exit()
# for i in sys.argv[1:]:
#     out = [int(i,16)<<16, ]
#     res = check_mod_rom(mdev, out[0])
#     out.extend(res)
#     print fmt%tuple(out)

for addr in range(0xFFFF):
    out = [ addr << 16, ]
    res = check_mod_rom(mdev, out[0])
    out.extend(res)
    if out[1]==0x40e6: print fmt%tuple(out)

