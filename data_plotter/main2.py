import numpy as np
import matplotlib.pyplot as plt

def read_64bit_values(filename):
    with open(filename, 'r') as f:
        lines = [line.strip() for line in f if line.strip()]
    values_64bit = []
    last_full_64 = 0
    for i in range(0, len(lines), 2):
        lower_val = int(lines[i], 10)
        upper_val = int(lines[i+1], 10)
        full_64 = (upper_val << 32) | lower_val
        if full_64 != last_full_64:
            values_64bit.append(full_64)
            last_full_64 = full_64
    return values_64bit

def bits_of_64bit_value(val):
    return [(val >> b) & 1 for b in range(38)]

def build_bit_matrix(values_64bit):
    bit_arrays = [bits_of_64bit_value(val) for val in values_64bit]
    return np.array(bit_arrays).T  # shape: (64, num_samples)

def plot_logic_analyzer(bit_matrix):
    num_bits, num_samples = bit_matrix.shape
    time = np.arange(num_samples)

    plt.figure(figsize=(12, 8))
    for bit_index in range(num_bits):
        offset = bit_index
        plt.step(time, offset + bit_matrix[bit_index] * .5, where='post', label=f'Bit {bit_index}')

    #for idx, bit_index in enumerate(range(num_bits - 1, -1, -1)):
        #offset = idx
        #plt.step(time,
                 #offset + bit_matrix[bit_index] * .8,
                 #where='post',
                 #label=f'Bit {bit_index}')

    plt.xlabel('Sample index')
    plt.ylabel('Bit index + digital level')
    plt.title('64-bit values over time (Logic Analyzer Style)')

    ticks = range(38)
    labels = [
        '0',
        '1',
        'CO_0',
        'CO_1',
        'CO_2',
        'CO_3',
        'CO_4',
        'CO_5',
        'CO_6',
        'CO_7',
        'CO_8',
        'CO_9',
        'CO_10',
        'CO_11',
        'CO_12',
        'CO_13',
        'CO_14',
        'CO_15',
        'CA_0',
        'CA_1',
        'CA_2',
        'CA_3',
        'CA_4',
        'CA_5',
        'CA_6',
        'CA_7',
        'CA_8',
        'CA_9',
        'CA_10',
        'CA_11',
        'CA_12',
        'CA_13',
        'CA_14',
        'CA_15',
        'ALE_H',
        'ALE_L',
        'WRITE',
        'READ',
    ]

    #labels = list(reversed(labels))

    plt.yticks(ticks, labels)
    #plt.yticks(ticks)
    # You might not want a legend for all 64 bits—this is optional
    # plt.legend(bbox_to_anchor=(1.05, 1), loc='upper left', ncol=2)
    plt.tight_layout()
    plt.show()

def main():
    filename = 'data.txt'  # your file with pairs of 32-bit hex values
    values_64bit = read_64bit_values(filename)
    bit_matrix = build_bit_matrix(values_64bit)
    plot_logic_analyzer(bit_matrix)

if __name__ == "__main__":
    main()