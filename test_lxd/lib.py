from ctypes import *
import os

# needed for malloc/free
libc = cdll.LoadLibrary('libc.so.6')
libc.malloc.argtypes = [c_size_t]
libc.malloc.restype  = c_void_p

libc.free.argtypes = [c_void_p]
libc.free.restype  = None

# lxd
root = os.path.realpath(os.path.join(os.path.dirname(__file__), '../'))
lxd = cdll.LoadLibrary(str(os.path.join(root, 'build', 'liblxd.so')))

lxd.APP_SUCCESS = 0

lxd.additive_square_footprint.argtypes = []
lxd.additive_square_footprint.restype  = c_size_t

lxd.create_additive_square.argtypes = [c_void_p, c_size_t, POINTER(c_int)]
lxd.create_additive_square.restype  = c_void_p

lxd.destroy_additive_square.argtypes = [c_void_p]
lxd.destroy_additive_square.restype  = c_void_p

lxd.additive_square_generate_samples.argtypes = [c_void_p, c_size_t, c_float, POINTER(c_float)]
lxd.additive_square_generate_samples.restype  = c_int

class envelope_setting(Structure):
    # hack alert! all the structs in the union are currently the same, so this
    # will *probably* work
    _fields_ = [("type", c_int),
                ("param", c_float)]

lxd.envelope_footprint.argtypes = []
lxd.envelope_footprint.restype  = c_size_t

lxd.populate_envelope_setting.argtypes = [c_int, c_size_t, c_size_t, POINTER(envelope_setting)]
lxd.populate_envelope_setting.restype  = c_int

lxd.create_envelope.argtypes = [c_void_p, POINTER(envelope_setting), POINTER(c_int)]
lxd.create_envelope.restype  = c_void_p

lxd.destroy_envelope.argtypes = [c_void_p]
lxd.destroy_envelope.restype  = c_void_p

lxd.envelope_strike.argtypes = [c_void_p]
lxd.envelope_strike.restype  = c_int

lxd.envelope_zero.argtypes = [c_void_p]
lxd.envelope_zero.restype  = c_int

lxd.envelope_change_setting.argtypes = [c_void_p, POINTER(envelope_setting)]
lxd.envelope_change_setting.restype = c_int

lxd.envelope_generate_samples.argtypes = [c_void_p, c_size_t, POINTER(c_float)]
lxd.envelope_generate_samples.restype  = c_int
