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

lxd.pulse_gen_footprint.argtypes = []
lxd.pulse_gen_footprint.restype  = c_size_t

lxd.create_pulse_gen.argtypes = [c_void_p, c_float, POINTER(c_int)]
lxd.create_pulse_gen.restype  = c_void_p

lxd.destroy_pulse_gen.argtypes = [c_void_p]
lxd.destroy_pulse_gen.restype  = c_void_p

lxd.pulse_gen_strike.argtypes = [c_void_p]
lxd.pulse_gen_strike.restype  = c_int

lxd.pulse_gen_generate_samples.argtypes = [c_void_p, c_size_t, POINTER(c_float)]
lxd.pulse_gen_generate_samples.restype  = c_int
