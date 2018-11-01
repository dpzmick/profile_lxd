import struct
import array
import numpy as np
from matplotlib import pyplot as plt

float_size  = 4
size_t_size = 8

square    = np.array([], dtype=np.float32)
pulse     = np.array([], dtype=np.float32)
lxd_in    = np.array([], dtype=np.float32)
fft_taken = []

with open('/scratch/data_out', 'rb') as f:
    i = 0
    while True:
        header_bytes = f.read(2*size_t_size)
        if not header_bytes: break

        (n_samples, fft_bins) = struct.unpack('NN', header_bytes)
        print( (n_samples, fft_bins) )

        float_array = array.array('f')
        float_array.fromfile(f, n_samples)
        square = np.concatenate( (square, float_array) )

        float_array = array.array('f')
        float_array.fromfile(f, n_samples)
        pulse = np.concatenate( (pulse, float_array) )

        float_array = array.array('f')
        float_array.fromfile(f, n_samples)
        lxd_in = np.concatenate( (lxd_in, float_array) )

        if fft_bins:
            float_array = array.array('f')
            float_array.fromfile(f, fft_bins)

            fft_taken.append(i)

        i += n_samples

for loc in fft_taken:
    plt.axvline(loc, color='r')

#plt.plot(square)
plt.plot(lxd_in)
plt.plot(pulse)
plt.show()
