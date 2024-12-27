import matplotlib.pyplot as plt
import numpy as np

def hex_to_bit_array(hex_str):
    # Convert hex to integer
    num = int(hex_str, 16)
    # Convert integer to a binary string, stripping the "0b" prefix
    # and zero-padding to 4 bits per hex digit
    bit_str = bin(num)[2:].zfill(4 * len(hex_str))
    # Convert each character ('0' or '1') into an integer
    return [int(bit) for bit in bit_str]

def my_lines(ax, pos, *args, **kwargs):
    if ax == 'x':
        for p in pos:
            plt.axvline(p, *args, **kwargs)
    else:
        for p in pos:
            plt.axhline(p, *args, **kwargs)

upper = 0xFFFFFFFF
lower = 0x0


#bits = hex_to_bit_array(str((upper << 32) | lower))
bits = hex_to_bit_array("0xFFFFFFFF00000000")
data = np.repeat(bits, 2)
#clock = 1 - np.arange(len(data)) % 2
#manchester = 1 - np.logical_xor(clock, data)
t = 0.5 * np.arange(len(data))

#plt.hold(True)
my_lines('x', range(13), color='.5', linewidth=2)
my_lines('y', [0.5, 2, 4], color='.5', linewidth=2)
#plt.step(t, clock + 4, 'r', linewidth = 2, where='post')
plt.step(t, data + 2, 'r', linewidth = 2, where='post')
#plt.step(t, manchester, 'r', linewidth = 2, where='post')
plt.ylim([-1,6])

for tbit, bit in enumerate(bits):
    plt.text(tbit + 0.5, 1.5, str(bit))

plt.gca().axis('off')
plt.show()