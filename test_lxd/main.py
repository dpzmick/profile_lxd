from .lib import libc
from .lib import lxd
from .lib import envelope_setting

from matplotlib import pyplot as plt
from scipy import signal
from scipy import fftpack
import ctypes
import numpy as np
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

class Envelope(object):
    def __init__(self, settings):
        ptr = libc.malloc(lxd.envelope_footprint())
        if not ptr:
            raise RuntimeError('failed to allocate memory')

        self._impl = lxd.create_envelope(ptr, ctypes.byref(settings), None)
        if not self._impl:
            raise RuntimeError('something went wrong')

    def __del__(self):
        if hasattr(self, "_impl"):
            ptr = lxd.destroy_envelope(self._impl)
            libc.free(ptr)

    def strike(self):
        ret = lxd.envelope_strike(self._impl)
        if ret != lxd.APP_SUCCESS:
            raise RuntimeError('something went wrong')

    def generate_samples(self, nframes):
        buffer = np.empty(nframes, dtype=np.single)
        ptr = buffer.ctypes.data_as(ctypes.POINTER(ctypes.c_float))

        ret = lxd.envelope_generate_samples(self._impl,
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

        for sample in samples:
            assert sample >= -1.0
            assert sample <= 1.0

        if plot:
            plt.plot(afft)
            plt.plot(peaks, afft[peaks], 'x')
            plt.xscale('log')
            plt.show()

            plt.plot(samples)
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

def test_constant():
    s = envelope_setting()
    s.type  = 0
    s.param = 0.5

    e = Envelope(s)
    s = e.generate_samples(100)
    e.strike()
    s = np.append(s, e.generate_samples(1000))
    plt.plot(s)
    plt.show()

def test_linear():
    s = envelope_setting()
    lxd.populate_envelope_setting(1, 4000000000, 44100, ctypes.byref(s))

    e = Envelope(s)
    e.strike()
    samples = e.generate_samples(176401)
    plt.plot(samples[176398:])
    plt.axhline(0.0, color='red')
    plt.show()

def test_exponential():
    s = envelope_setting()
    s.type  = 2
    s.param = 0.005

    e = Envelope(s)
    e.strike()
    plt.plot(e.generate_samples(1000))
    plt.show()

def test_log():
    s = envelope_setting()
    s.type  = 3
    s.param = 1000.

    e = Envelope(s)
    e.strike()
    plt.plot(e.generate_samples(1000))
    plt.show()

if __name__ == "__main__":
    # test_constant()
    # test_exponential()
    test_linear()
    # test_log()
    # import test_lxd.main as me
    # for name in dir(me):
    #     if name.startswith("test_"):
    #         getattr(me, name)()
