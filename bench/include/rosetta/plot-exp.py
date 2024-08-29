import matplotlib.pyplot as plt
import math

def parseExperiment(inpt):
    params = {}
    results = {}
    for l in inpt:
        if l[0] == '\t':
            s = l.split('\t')
            params[s[1]] = s[2]
        else:
            if "Inserted keys to filter" in l:
                s = l.split(' ')
                results['insert time'] = float(s[4][:-2])
            elif "fpr:" in l:
                s = l.split(' ')
                results['fpr'] = float(s[1])
            elif "bits per key" in l:
                s = l.split(' ')
                results['bpk'] = float(s[1])
            elif "per query" in l:
                s = l.split(' ')
                results['query time'] = float(s[0][:-2])
    return params, results

def plotExperiments(fname):
    with open(fname) as f:
        lines = f.read().splitlines()
        split = [i for i,j in enumerate(lines) if "experiment with the following parameters" in j]
        experiments_DST = [lines[i+2:j] for i,j in zip(split[:-1],split[1:]) if "DST" in lines[i]]
        experiments_SuRF = [lines[i+2:j] for i,j in zip(split[:-1],split[1:]) if "SuRF" in lines[i]]
        parsed_DST = [parseExperiment(exp) for exp in experiments_DST]
        parsed_DST = [p for p in parsed_DST if 'fpr' in p[1]]
        parsed_SuRF = [parseExperiment(exp) for exp in experiments_SuRF]
        parsed_SuRF = [p for p in parsed_SuRF if 'fpr' in p[1]]

        rsize = [int(p[0]['Maximum range:']) for p in parsed_DST]
        fpr = [p[1]['fpr'] for p in parsed_DST]
        bpk = [p[1]['bpk'] for p in parsed_DST]

        for r in set(rsize):
            X = [b for b,r2 in zip(bpk, rsize) if r2 == r]
            Y = [f for f,r2 in zip(fpr, rsize) if r2 == r]
            X, Y = zip(*sorted(zip(X, Y)))
            plt.plot(X, Y, label='DST, range size: ' + str(r))

        rsize = [int(p[0]['Maximum range:']) for p in parsed_SuRF]
        fpr = [p[1]['fpr'] for p in parsed_SuRF]
        bpk = [p[1]['bpk'] for p in parsed_SuRF]

        for r in set(rsize):
            X = [b for b,r2 in zip(bpk, rsize) if r2 == r]
            Y = [f for f,r2 in zip(fpr, rsize) if r2 == r]
            plt.plot(X, Y, '.', label='SuRF, range size: ' + str(r))


#        surf_rsize = [4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768]
#        surf_fpr = [0.32937, 0.332769, 0.332468, 0.334338, 0.339453, 0.344506, 0.351235, 0.357192, 0.359537, 0.35834, 0.35808, 0.362443, 0.361729, 0.354918]
#        surf_kb = [9993, 9995, 9998, 9993, 9993, 9994, 9994, 9997, 10000, 9996, 9994, 9994, 9992, 9993]
#        surf_bpk = [i*1024*8/10000000 for i in surf_kb]
#        plt.plot(surf_bpk, surf_fpr, '.', label='SuRF-base, all ranges')
#
#        surf_kb = [14871, 14869, 14871, 14869, 14873, 14874, 14873, 14869, 14870, 14872, 14869, 14871, 14873, 14873]
#        surf_fpr = [0.0215436, 0.0216614, 0.0221905, 0.022285, 0.0228926, 0.024203, 0.0256383, 0.0266451, 0.0268501, 0.0267945, 0.0271986, 0.0267489, 0.0284348, 0.0273104]
#        surf_bpk = [i*1024*8/10000000 for i in surf_kb]
#        plt.plot(surf_bpk, surf_fpr, '.', label='SuRF-Real-4, all ranges')
#
#        surf_kb = [19748, 19751, 19751, 19746, 19752, 19748, 19746, 19749, 19749, 19746, 19749, 19749, 19747, 19749]
#        surf_fpr = [0.00233726, 0.00240064, 0.00238442, 0.00234849, 0.00238554, 0.00237737, 0.00236578, 0.00227421, 0.00241497, 0.00213704, 0.00199888, 0.00253879, 0.00266008, 0.00276243]
#        surf_bpk = [i*1024*8/10000000 for i in surf_kb]
#        plt.plot(surf_bpk, surf_fpr, '.', label='SuRF-Real-8, all ranges')

        plt.xlabel("bits per key")
        plt.ylabel("false positive rate")
#        plt.legend()
        plt.show()

plotExperiments("experiment_results_combined")
