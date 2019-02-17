#!/usr/bin/python3

def main():
    import sys
    import re
    
    NB_BITS_INT = 16
    
    if len(sys.argv) != 2:
        print("Usage: {} <file>".format(sys.argv[0]))
        exit()
    with open(sys.argv[1], "r") as inFile:
        lines = inFile.readlines()
        latencies = []
        for line in lines:
            if line.startswith("latency:"):
                _, latency = line.split(" ")
                latencies.append(latency.strip())
    latenciesString = "".join(latencies)
    multiplicationRegex = "11123|11(12112(1?)112)+(12)?3"
    multiplicationPattern = re.compile(multiplicationRegex)
    multiplicationHit = multiplicationPattern.search(latenciesString)
    if multiplicationHit:
        print("Found multiplication")
        multiplicationTrace = multiplicationHit.group()
        if multiplicationTrace == "11123":
            print("a = 0")
            print("b = unknown")
        else:
            multiplicationTrace = multiplicationTrace[2:] # consume the two known instructions at the beginning
            bitsOfB = ""
            bitEq1Trace = "121121112"
            bitEq0Trace = "12112112"
            aIsZeroTrace = "123"
            while multiplicationTrace.startswith(bitEq1Trace) or multiplicationTrace.startswith(bitEq0Trace):
                if multiplicationTrace.startswith(bitEq1Trace):
                    bitsOfB += "1"
                    multiplicationTrace = multiplicationTrace[len(bitEq1Trace):]
                elif multiplicationTrace.startswith(bitEq0Trace):
                    bitsOfB += "0"
                    multiplicationTrace[len(bitEq0Trace):]
                    multiplicationTrace = multiplicationTrace[len(bitEq0Trace):]
            if multiplicationTrace.startswith(aIsZeroTrace):
                print("Result overflowed...incorrect result is returned")
                print("partial recovery of b\nb > {}".format(int(bitsOfB[::-1], 2)))
            else:
                print("At least one of the {} least significant bits of a is 1".format(NB_BITS_INT - len(bitsOfB) + 1))
                print("b = {}".format(int(bitsOfB[::-1], 2)))
    else:
        print("Found no multiplication")
        

if __name__ == "__main__":
    main()
