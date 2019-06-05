#!/usr/bin/python3

def main():
    import sys
    import re
    
    NB_BITS_INT = 16
    
    if len(sys.argv) < 2:
        print("Usage: {} <file> [-p]".format(sys.argv[0]))
        exit()
    with open(sys.argv[1], "r") as inFile:
        lines = inFile.readlines()
        latencies = []
        for line in lines:
            if line.startswith("latency:"):
                _, latency = line.split(" ")
                latencies.append(latency.strip())
    latenciesString = "".join(latencies)
    hardened = False
    if len(sys.argv) > 2:
        if sys.argv[2] == "-p":
            hardened = True
    multiplicationRegex = "11123|11(12112(1?)112)+(12)?3"
    if hardened:
        multiplicationRegex = "3211"+"11212112"*16+"23"
    multiplicationPattern = re.compile(multiplicationRegex)
    multiplicationHit = multiplicationPattern.search(latenciesString)
    if multiplicationHit:
        multiplicationTrace = multiplicationHit.group()
        if hardened:
            print("Full trace", multiplicationTrace)
            print("Iteration trace", "11212112")
        else:
            print("Found multiplication")
            print("Full trace", multiplicationTrace)
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
                    print(("partial recovery of b\nb = " + "x"*(16-len(bitsOfB))+"".join(bitsOfB[::-1])))
                else:
                    print("At least one of the {} least significant bits of a is 1".format(NB_BITS_INT - len(bitsOfB) + 1))
                    print("b = {}".format(int(bitsOfB[::-1], 2)))
    else:
        print("Found no multiplication")


if __name__ == "__main__":
    main()
