#!/usr/bin/python3

import re
import sys
from collections import defaultdict, OrderedDict

import statistics as stat
import pandas as pd, numpy as np
import matplotlib.pyplot as plt
import matplotlib.font_manager as font_manager

LATENCY_MIN = 7700
LATENCY_MAX = 8500

def dump_latency_avg_trace(latencies, out_file='latency_trace_mean.txt'):
    with open(out_file, 'w') as fo:
        for l in latencies:
            fo.write(str(stat.median(l)) + '\n')

def dump_latency(latencies, idx, out_file='latency_bench.txt'):
    with open(out_file, 'w') as fo:
        for l in latencies[idx]:
            fo.write(l + '\n')

def plot_latency_avg_trace(latencies, rips, start, end, marker_idx=[]):
    avg_trace = [ stat.median(l) for l in latencies[start:end] ]
    fig = plt.figure(figsize=(10,4))
    ax = plt.gca()

    for i,l in enumerate(avg_trace):
        print(i, hex(rips[start+i]), l)
        if start+i in marker_idx:
            print("^^ cmp")

    for i in marker_idx:
        x = i-start
        y = avg_trace[x]
        ax.annotate('cmp', xy=(x,y), xytext=(x,y-15),
                    arrowprops=dict(facecolor='black', width=1, headwidth=9, shrink=0.05), 
                   )

    #ax.set_ylim([LATENCY_MIN,LATENCY_MAX])

    font = font_manager.FontProperties(family='monospace', size=12) #weight='bold', style='normal')

    plt.xlabel('Instruction (interrupt number)')
    plt.ylabel('IRQ latency (cycles)')

    plt.plot(avg_trace, '-bo')
    plt.savefig('trace.pdf', bbox_inches='tight')
    plt.show()
    

def boxplot(latencies):
    d = OrderedDict()
    fig = plt.figure(figsize=(10,4))

    for i, (a, b, c, (la,lb,lc)) in enumerate(latencies):
        d[la + '-a-{}'.format(i)] = a
        d[lb + '-b-{}'.format(i)] = b
        d[lc + '-c-{}'.format(i)] = c

    df = pd.DataFrame(data=d, columns=d.keys())
    df = df.clip_upper(LATENCY_MAX-300)
    df = df.clip_lower(LATENCY_MIN)
    #print(df)
    bp = plt.boxplot(x=df.T.values.tolist(), labels=d.keys(), patch_artist=True)

    colors = ['lightblue', 'lightgreen', 'pink']
    for i, patch in enumerate(bp['boxes']):
        patch.set_facecolor(colors[i % 3]) #2])

    plt.xticks(rotation=70)
    plt.savefig('boxplot.pdf', bbox_inches='tight')
    plt.show()

def boxplot2(latencies):
    d = OrderedDict()
    fig = plt.figure(figsize=(10,4))

    for i, (a, b, (la,lb)) in enumerate(latencies):
        d[la + '-a-{}'.format(i)] = a
        d[lb + '-b-{}'.format(i)] = b

    df = pd.DataFrame(data=d, columns=d.keys())
    df = df.clip_upper(LATENCY_MAX-300)
    df = df.clip_lower(LATENCY_MIN)
    #print(df)
    bp = plt.boxplot(x=df.T.values.tolist(), labels=d.keys(), patch_artist=True)

    colors = ['lightblue', 'lightgreen', 'pink']
    for i, patch in enumerate(bp['boxes']):
        patch.set_facecolor(colors[i % 3]) #2])

    plt.xticks(rotation=70)
    plt.savefig('boxplot.pdf', bbox_inches='tight')
    plt.show()

def hist(latencies, labels=[], filename='hist', legend_loc='best', legend_ncol=1):
    plt.figure(figsize=(10,5))
    plt.hist(latencies, label=labels, bins=200, histtype='step', range=[LATENCY_MIN, LATENCY_MAX])
    axes = plt.gca()
    axes.set_ylim([0,800])

    font = font_manager.FontProperties(family='monospace', size=12) #weight='bold', style='normal')
    if len(labels) > 0:
        plt.legend(prop=font, loc=legend_loc, ncol=legend_ncol)

    plt.xlabel('IRQ latency (cycles)')
    plt.ylabel('Frequency')

    plt.savefig(filename + '.pdf', bbox_inches='tight')
    plt.show() 

def parse_latencies(in_file='out.txt'):
    ecall = step = rip = rip_prev = irq_tot = irq_zero = 0
    # NOTE: we abuse that Python 3.6 preserves dict order..
    # https://stackoverflow.com/a/39537308
    latencies = defaultdict(list)
    rips      = defaultdict(list)

    with open(in_file, 'r') as fi:
        for line in fi:

            # single-stepping mode entered by #PF handler 
            if re.search('Restoring enclave page', line):
                ecall += 1
                step = rip_prev = 0
                continue

            # latency + edbgrd rip dumped by IRQ handler
            #m = re.search('RIP=0x([0-9A-Fa-f]+); ACCESSED=([0-9]); latency=([0-9]+)', line)
            m = re.search('RIP=0x([0-9A-Fa-f]+); latency=([0-9]+)', line)
            if m:
                rip = int(m.groups()[0], base=16)
                #a = int(m.groups()[1])
                l = int(m.groups()[1])

                #if (not a):
                #    print('libnemesis.py: INFO: ignoring zero-step with PTE A bit set result @{} (ecall={}, step={})'.format(hex(rip), ecall, step-1))
                #    continue

                if (rip != rip_prev):
                    latencies[step].append(l)
                    rips[step] = rip
                    step += 1
                else:
                    latencies[step-1].pop()
                    latencies[step-1].append(l)
                    print('libnemesis.py: WARNING: discarding zero-step result @{} (ecall={}, step={})'.format(hex(rip), ecall, step-1))
                    irq_zero += 1

                rip_prev = rip
                irq_tot += 1

    print('libnemesis.py: parsed {} ecalls with total {} IRQs ({} zero-step)'.format(ecall, irq_tot, irq_zero))
    return (list(latencies.values()), list(rips.values()))

#print(parse_latencies('test.txt'))
