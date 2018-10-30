from .lib import libc
from .lib import lxd

from matplotlib import pyplot as plt
from scipy import signal
from scipy import fftpack
import ctypes
import numpy as np
import pytest
import itertools
import sys

class AdditiveSquare(object):
    def __init__(self, sample_rate):
        ptr = libc.malloc(lxd.additive_square_footprint())
        if not ptr:
            raise RuntimeError('Failed to allocate memory')

        self._impl = lxd.create_additive_square(ptr,
                                                ctypes.c_size_t(sample_rate),
                                                None)

        if not self._impl:
            raise ValueError('Something went wrong')

    def __del__(self):
        ptr = lxd.destroy_additive_square(self._impl)
        libc.free(ptr)

    def generate_samples(self, nframes, frequency):
        """Returns a numpy array of floats"""
        buffer = np.empty(nframes, dtype=np.single)
        ptr = buffer.ctypes.data_as(ctypes.POINTER(ctypes.c_float))

        ret = lxd.additive_square_generate_samples(self._impl,
                                                   ctypes.c_size_t(nframes),
                                                   ctypes.c_float(frequency),
                                                   ptr)
        if ret != lxd.APP_SUCCESS:
            raise RuntimeError('something went wrong')

        return buffer

class PulseGen(object):
    def __init__(self, c):
        ptr = libc.malloc(lxd.pulse_gen_footprint())
        if not ptr:
            raise RuntimeError('failed to allocate memory')

        self._impl = lxd.create_pulse_gen(ptr, ctypes.c_float(c), None)
        if not self._impl:
            raise RuntimeError('something went wrong')

    def __del__(self):
        ptr = lxd.destroy_pulse_gen(self._impl)
        libc.free(ptr)

    def strike(self):
        ret = lxd.pulse_gen_strike(self._impl)
        if ret != lxd.APP_SUCCESS:
            raise RuntimeError('something went wrong')

    def generate_samples(self, nframes):
        buffer = np.empty(nframes, dtype=np.single)
        ptr = buffer.ctypes.data_as(ctypes.POINTER(ctypes.c_float))

        ret = lxd.pulse_gen_generate_samples(self._impl,
                                             ctypes.c_size_t(nframes),
                                             ptr)

        if ret != lxd.APP_SUCCESS:
            raise RuntimeError('something went wrong')

        return buffer

def test_asquare():
    def inner(sample_rate, frequency, plot=False):
        # one cycle of the wave
        N = int(float(sample_rate)/frequency)
        print(sample_rate, frequency, N)

        a = AdditiveSquare(sample_rate)
        samples = a.generate_samples(N, frequency)
        fft     = fftpack.fft(samples)
        afft    = np.abs(fft)**2

        afft     = afft[:N//2]
        peaks, _ = signal.find_peaks(afft)

        if plot:
            plt.plot(afft)
            plt.plot(peaks, afft[peaks], 'x')
            plt.xscale('log')
            plt.show()

        for peak in peaks:
            if afft[peak] < 1000.: continue # some peaks don't matter
            assert (peak%2) == 1

    sample_rate = 48000
    frequency   = 440

    sample_rates = [41000, 48000, 96000, 192000]
    freqs        = range(10, 10000, 2)

    # inner(192000, 10, plot=True)
    for (s,f) in itertools.product(sample_rates, freqs):
        if s/2 < f: continue
        inner(s,f)

if __name__ == "__main__":
    # g = PulseGen(0.05)
    # g.strike();
    # res1 = g.generate_samples(50)

    # g.strike();
    # res2 = g.generate_samples(100)
    # res1 = np.append(res1, res2)

    # plt.plot(res1)
    # plt.show()
    # sys.exit(0)

    import test_lxd.main as me
    for name in dir(me):
        if name.startswith("test_"):
            getattr(me, name)()
