import numpy as np
from scipy.signal import iirfilter, freqz

b, a = iirfilter(15, [2*np.pi*50, 2*np.pi*200], rs=50, btype='band', analog=True, ftype='cheby2')

w, h = freqz(b, a, 1000)
print("w = {},h={}".format(w,h))