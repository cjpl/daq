#!/usr/bin/env python
#-*- encoding: utf-8 -*-
'''
Mvme module provides utilities to handle VME bus. It includes:
  1. Standard VME bus information class: StdVME
  2. Standard VME bus interface in MIDAS/PSI:
     a. MVME_INTERFACE
     b. MVME_STATUS
     c. MvmeDev

Author: Exaos Lee <Exaos DOT Lee AT Gmail DOT com>, Aug. 2009
'''

from ctypes import *
from ctypes.util import find_library
from pprint import pprint
import os, os.path

class StdVME:
    '''
    Standard VME informations.
    '''
    DMODE = { "D8":     [1, "Data length 8-bit"],
              "D16":    [2, "Data length 16-bit"],
              "D32":    [3, "Data length 32-bit"],
              "D64":    [4, "Data length 64-bit"],
              "RAMD16": [5, "RAM memory of VME adapter: int 16"],
              "RAMD32": [6, "RAM memory of VME adapter: int 32"],
              "LM":     [7, "Local memory mapped to VME"],
              }
    default_dmode = "D32"

    @classmethod
    def get_dmode_sym(self, var):
        for k in self.DMODE:
            if self.DMODE[k][0] == var: return k
        return None

    @classmethod
    def get_dmode_all(self): return self.DMODE.keys()

    @classmethod
    def get_dmode_var(self, sym):
        if sym in self.DMODE: return self.DMODE[sym][0]
        return 0

    @classmethod
    def get_dmode_info(self, sym):
        if sym in self.DMODE: return self.DMODE[sym][1]
        return "Invalid Data Mode"

    BLT = { "NONE":      [1, "Normal programmed IO"],
            "BLT32":     [2, "32-bit block transfer"],
            "MBLT64":    [3, "Multiplexed 64-bit block transfer"],
            "2EVME":     [4, "Two edge block transfer"],
            "2ESST":     [5, "Two edge source synchronous transfer"],
            "BLT32FIFO": [6, "FIFO mode, don't increment address"],
            "MBLT64FIFO":[7, "FIFO mode, don't increment address"],
            "2EVMEFIFO": [8, "Tow edge block transfer with FIFO mode"],
            }
    default_blt = "NONE"

    @classmethod
    def get_blt_sym(self, var):
        for k in self.BLT:
            if self.BLT[k][0] == var: return k
        return None

    @classmethod
    def get_blt_all(self): return self.BLT.keys()

    @classmethod
    def get_blt_var(self, sym):
        if sym in self.BLT: return self.BLT[sym][0]
        return 0

    @classmethod
    def get_blt_info(self, sym):
        if sym in self.BLT: return self.BLT[sym][1]
        return "Invalid Data Mode"

    AM = { "A32_SB":   [0x0F, "A32 extended supervisory block"],
           "A32_SP":   [0x0E, "A32 extended supervisory program"],
           "A32_SD":   [0x0D, "A32 extended supervisory data"],
           "A32_NB":   [0x0B, "A32 extended Non-Privileged block"],
           "A32_NP":   [0x0A, "A32 extended Non-Privileged program"],
           "A32_ND":   [0x09, "A32 extended Non-Privileged data"],
           "A32_SMBLT":[0x0C, "A32 multiplexed block transfer (D64)"],
           "A32_NMBLT":[0x08, "A32 multiplexed block transfer (D64)"],
           
           "A32":      [0x0D, "Default A32 mode: MVME_AM_A32_SD"],
           "A32_D64":  [0x0C, "Default A32 (D64) mode: MVME_AM_A32_SMBLT"],
           
           "A24_SB":   [0x3F, "A24 extended supervisory block"],
           "A24_SP":   [0x3E, "A24 extended supervisory program"],
           "A24_SD":   [0x3D, "A24 extended supervisory data"],
           "A24_NB":   [0x3B, "A24 extended Non-Privileged block"],
           "A24_NP":   [0x3A, "A24 extended Non-Privileged program"],
           "A24_ND":   [0x39, "A24 extended Non-Privileged data"],
           "A24_SMBLT":[0x3C, "A24 multiplexed block transfer (D64)"],
           "A24_NMBLT":[0x38, "A24 multiplexed block transfer (D64)"],

           "A24":      [0x3D, "Default A24 mode: MVME_AM_A24_SD"],
           "A24_D64":  [0x3C, "Default A24 (D64) mode: MVME_AM_A24_SMBLT"],

           "A16_SD":   [0x2D, "A16 extended supervisory data access"],
           "A16_ND":   [0x29, "A16 extended Non-Privileged data access"],

           "A16":      [0x2D, "Default A16 mode: MVME_AM_A16_SD"],

           "DEFAULT":  [0x0D, "Default address modifier: MVME_AM_A32"],
           }
    default_am = "A32"

    @classmethod
    def get_am_sym(self, var):
        for k in self.AM:
            if self.AM[k][0] == var: return k
        return None

    @classmethod
    def get_am_all(self): return self.AM.keys()

    @classmethod
    def get_am_var(self, sym):
        if sym in self.AM: return self.AM[sym][0]
        return 0

    @classmethod
    def get_am_info(self, sym):
        if sym in self.AM: return self.AM[sym][1]
        return "Invalid Data Mode"

class MVME_INTERFACE(Structure):
    _fields_ = [ ("handle",  c_int),       # internal handle
                 ("index",   c_int),       # index of interface 0..n
                 ("info",    c_void_p),    # internal info structure
                 ("am",      c_int),       # Address modifier
                 ("dmode",   c_int),       # Data mode (D8, D16, D32, D64)
                 ("blt_mode",c_int),       # Block transfer mode
                 ("table",   c_void_p),    # Optional table for some drivers
                 ]

def print_mvme(v):
    print "MVME_INTERFACE information:"
    vp = pointer(v)
    s = str(vp)
    print "   Pointer:","<"+s[s.index("at")+3:]
    for k in ["handle","index","info","am","dmode","blt_mode","table"]:
        print "   %s: %s"%(k,str(eval("v.%s"%k)))

MVME_STATUS = { "SUCCESS":      1,  # Successfully
                "NO_INTERFACE": 2,  # No VME interface
                "NO_CRATE":     3,  # No VME crate response
                "UNSUPPORTED":  4,  # Unsupported
                "INVALID_PARAM":5,  # Invalid parameter
                "NO_MEM":       6,  # No memory
                "ACCESS_ERROR": 7,  # Access device error

                "NO_DEVICE":    0, # No VME device found (add by E.L.)
                }

def get_status_msg(v):
    for i in MVME_STATUS:
        if MVME_STATUS[i] == v: return i
    return None

mvme_addr_t    = c_uint  # VME standard address type
mvme_locaddr_t = c_uint  # VME local address type
mvme_size_t    = c_uint  # VME size type

class MvmeDev:
    def __init__(self, mvme=None, libname=None):
        '''
        Initialize the instance.
        mvme:     MVME_INTERFACE structure instance.
        libname:  A shared library file name such as "lib_mvme.dylib",
                  or just a short name like "_mvme".
        '''
        if mvme: self._dev = mvme
        else:    self._dev = MVME_INTERFACE()
        self._devp = pointer(self._dev)
        if not libname:
            libname = os.path.join(os.path.dirname(__file__),"lib_mvme.so")
        self.LoadLibrary(libname)

    @classmethod
    def print_all_status(self): pprint(MVME_STATUS)

    def LoadLibrary(self, libname):
        '''
        Load shared library. The "libname" should be a shared library filename
        such as "lib_mvme.dylib", or just short name like "_mvme".
        '''
        try:
            if os.path.isfile(libname): fname = os.path.abspath(libname)
            else: fname = os.path.abspath(find_library(libname))
            self._libname = fname
            self._lib = CDLL(fname)
        except: 
            self._OK = False
            return False

        self._lib.mvme_open.restype  = c_int
        self._lib.mvme_open.argtypes = \
            [POINTER(POINTER(MVME_INTERFACE)), c_int]
        self._lib.mvme_close.restype  = c_int
        self._lib.mvme_close.argtypes = [POINTER(MVME_INTERFACE),]
        self._lib.mvme_sysreset.restype  = c_int
        self._lib.mvme_sysreset.argtypes = [POINTER(MVME_INTERFACE),]
        self._lib.mvme_read.restype  = c_int
        self._lib.mvme_read.argtypes = \
            [POINTER(MVME_INTERFACE),c_void_p, mvme_addr_t, mvme_size_t]
        self._lib.mvme_read_value.restype  = c_uint
        self._lib.mvme_read_value.argtypes = \
            [POINTER(MVME_INTERFACE),mvme_addr_t]
        self._lib.mvme_write.restype  = c_int
        self._lib.mvme_write.argtypes = \
            [POINTER(MVME_INTERFACE),mvme_addr_t,c_void_p, mvme_size_t]
        self._lib.mvme_write_value.restype  = c_int
        self._lib.mvme_write_value.argtypes = \
            [POINTER(MVME_INTERFACE),mvme_addr_t,c_uint]
        self._lib.mvme_set_am.restype  = c_int
        self._lib.mvme_set_am.argtypes = [POINTER(MVME_INTERFACE),c_int]
        self._lib.mvme_get_am.restype  = c_int
        self._lib.mvme_get_am.argtypes = \
            [POINTER(MVME_INTERFACE),POINTER(c_int)]
        self._lib.mvme_set_dmode.restype  = c_int
        self._lib.mvme_set_dmode.argtypes = [POINTER(MVME_INTERFACE),c_int]
        self._lib.mvme_get_dmode.restype  = c_int
        self._lib.mvme_get_dmode.argtypes = \
            [POINTER(MVME_INTERFACE),POINTER(c_int)]
        self._lib.mvme_set_blt.restype  = c_int
        self._lib.mvme_set_blt.argtypes = [POINTER(MVME_INTERFACE),c_int]
        self._lib.mvme_get_blt.restype  = c_int
        self._lib.mvme_get_blt.argtypes = \
            [POINTER(MVME_INTERFACE),POINTER(c_int)]

        self._OK = True
        return True

    def open(self, idx=0):
        '''
        Open VME interface. The param "idx" is the index interface number
        (default is 0), should be used to distingush multiple VME interface
        access within the same program.'''
        if not self._OK: return get_status_msg(0)

        res = self._lib.mvme_open(pointer(self._devp), idx)
        msg = get_status_msg(res)
        if msg == "SUCCESS":
            self.set_am()
            self.set_dmode()
            self.set_blt()
        return msg

    def close(self):
        '''
        Close and release ALL the opened VME channel.  ivme --- MVME_INTERFACE
        instance.  '''
        res = self._lib.mvme_close(self._devp)
        return get_status_msg(res)

    def sysreset(self):
        '''
        VME bus reset. Effect of the VME bus reset is dependent of the type of
        VME interface used.  '''
        res = self._lib.mvme_sysreset(self._devp)
        return get_status_msg(res)

    def read(self, addr, nbytes=1, dst=None):
        '''
        Read from VME bus. Implementation of the read can include automatic DMA
        trnasfer based on the size of the data.

        addr:    The VME address, should be an unsigned long.
        nbytes:  requested transfer size. Default value is 1, means read single
                 data from VME bus, which is useful for register access.

        Return value is a list like [res, var]:
              res  ---  The status returned.
              var  ---  The value returned, which length equals to nbytes.
              dst  ---  The ctypes.c_void_p value. 
        '''
        if nbytes==1:
            res = self._lib.mvme_read_value(self._devp, addr)
            return res
        elif nbytes>1:
            if not dst:
                arrlen = nbytes / sizeof(c_int)
                dst = (c_int * arrlen)()
            else:
                if sizeof(dst)<nbytes: nbytes = sizeof(dst)
            res = self._lib.mvme_read(self._devp, cast(dst, c_void_p).value,
                                      addr, nbytes)
        else:
            return "ERROR"

        return [get_status_msg(res), dst]

    def write(self, addr, data):
        '''
        Write data to VME bus. Implementation of the wirte can include automatic
        DMA transfer based on the size of the data.

        data:  Could be an int, or a list of int. If data is an int or length
               of data equals to 1, then write single data to VME bus, which
               is useful for register access.
        '''
        if not data: return None

        if type(data) != list or len(data)==1:
            if type(data) != list: var = c_uint(data)
            else: var = c_uint(data[0])
            res = self._lib.mvme_write_value(self._devp,addr,var)
        else:
            var = (c_uint * nbs)()
            nbs = len(data)*sizeof(c_uint)
            var = data
            res = self._lib.mvme_write(self._devp,addr,
                                       cast(var,c_void_p).value,nbs)

        return get_status_msg(res)

    def set_am(self, am="DEFAULT"):
        '''
        Set Address Modifier. Parameter "am" should be one key in MVME_AM.  '''
        if am == "DEFAULT": am = StdVME.default_am
        pre_am = self.get_am()[1]
        if am in StdVME.AM:
            res = self._lib.mvme_set_am(self._devp,
                                        StdVME.get_am_var(am))
            msg = get_status_msg(res)
        else: msg = "INVALID_AM"
        return [msg, pre_am]

    def get_am(self):
        '''
        Get Address Modifier. The 2nd value of return list should be one key in
        MVME_AM.  '''
        i = c_int()
        res = self._lib.mvme_get_am(self._devp, pointer(i))
        msg = get_status_msg(res)
        am  = StdVME.get_am_sym(i.value)
        return [msg, am]

    def set_dmode(self, dmode="DEFAULT"):
        '''
        Set Data Mode. The "dmode" should be one key of MVME_DMODE.  '''
        if dmode == "DEFAULT": dmode = StdVME.default_dmode
        pre_mode = self.get_dmode()[1]
        if dmode in StdVME.DMODE:
            mode = c_int(StdVME.get_dmode_var(dmode))
            res = self._lib.mvme_set_dmode(self._devp, mode)
            msg = get_status_msg(res)
        else: msg = "INVALID_DMODE"
        return [msg, pre_mode]

    def get_dmode(self):
        '''
        Get current Data Mode. The 2nd value of return list should be one key in
        MVME_DMODE.  '''
        i = c_int()
        res = self._lib.mvme_get_dmode(self._devp, pointer(i))
        msg = get_status_msg(res)
        dmode = StdVME.get_dmode_sym(i.value)
        return [msg, dmode]

    def set_blt(self, blt="DEFAULT"):
        '''
        Set Block Transfer mode.  '''
        if blt == "DEFAULT": blt = StdVME.default_blt
        msg = "INVALID_BLT"
        pre_blt = self.get_blt()[1]
        if blt in StdVME.BLT:
            bvar = c_int(StdVME.get_blt_var(blt))
            res = self._lib.mvme_set_blt(self._devp, bvar)
            msg = get_status_msg(res)

        return [msg, pre_blt]

    def get_blt(self):
        '''
        Get current Block transfer mode.  '''
        i = c_int()
        res = self._lib.mvme_get_blt(self._devp, pointer(i))
        msg = get_status_msg(res)
        blt = StdVME.get_blt_sym(i.value)
        return [msg, blt]

    def int_generate(self, level, vec, info):
        '''
        Interrupt generator.  '''
        res = self._lib.mvme_interrupt_generate(self._devp,c_int(level),
                                                c_int(vec), c_void_p(info))
        return get_status_msg(res)

    def int_attach(self, level, vec, isr, info):
        '''
        Interrupt attach.  '''
        if os.name == 'win': ## for Windows
            ISRFUNC = WINFUNCTYPE(c_void_p, c_int, c_void_p, c_void_p)
        else: ## For POSIX system: os.name == 'posix'
            ISRFUNC = CFUNCTYPE(c_void_p, c_int, c_void_p, c_void_p)
        res = self._lib.mvme_interrupt_attach(self._devp, c_int(level),
                                              c_int(vec),
                                              ISRFUNC(isr), c_void_p(info))
        return get_status_msg(res)

    def int_detach(self, level, vec, info):
        '''
        Interrupt detach.  '''
        res = self._lib.mvme_interrupt_detach(self._devp, c_int(level),
                                              c_int(vec), c_void_p(info))
        return get_status_msg(res)

    def int_enable(self, level, vec, info):
        '''
        Interrupt enable.  '''
        res = self._lib.mvme_interrupt_enable(self._devp, c_int(level),
                                              c_int(vec), c_void_p(info))
        return get_status_msg(res)
        

    def int_disable(self, level, vec, info):
        '''
        Interrupt disable.  '''
        res = self._lib.mvme_interrupt_disable(self._devp, c_int(level),
                                               c_int(vec), c_void_p(info))
        return get_status_msg(res)

