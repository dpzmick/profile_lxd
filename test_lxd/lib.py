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
