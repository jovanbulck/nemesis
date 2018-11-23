#!/usr/bin/python3

import numpy as np
import matplotlib.pyplot as plt
import matplotlib.font_manager as font_manager

def plot_trace(latencies, start, end, out, hsize, vsize, o=''):
    fig = plt.figure(figsize=(hsize,vsize))
    ax = plt.gca()

    font = font_manager.FontProperties(family='monospace', size=12) #weight='bold', style='normal')

    plt.yticks(np.arange(1, 5, 1))

    plt.xlabel('Instruction (interrupt number)')
    plt.ylabel('IRQ latency (cycles)')

    plt.plot(latencies[start:end], '-r' + o)
    plt.savefig(out + '.pdf', bbox_inches='tight')
    plt.show()

latencies = []
with open('out_parsed.txt', 'r') as fi:
    for line in fi:
        latencies.append(int(line))
    
plot_trace(latencies, 0, len(latencies), 'keystroke_trace', 12, 2)
plot_trace(latencies, 37, 79, 'keystroke_trace_zoom', 6, 3, 'o')
