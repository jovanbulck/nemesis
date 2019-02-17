#!/usr/bin/python3

def main():
    import sys
    import re
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
        udivRegex = "211(1112(11)?12){16}"
        udivPattern = re.compile(udivRegex)
        udivHit = udivPattern.search(latenciesString)
        if udivHit:
            print("Found unsigned division")
            udivTrace = udivHit.group()
            udivTrace = udivTrace[3:] # consume the three known instructions at the beginning
            bitsOfQuotient = ""
            bitEq1Trace = "11121112"
            bitEq0Trace = "111212"
            while udivTrace.startswith(bitEq1Trace) or udivTrace.startswith(bitEq0Trace):
                if udivTrace.startswith(bitEq1Trace):
                    bitsOfQuotient += "1"
                    udivTrace = udivTrace[len(bitEq1Trace):]
                elif udivTrace.startswith(bitEq0Trace):
                    bitsOfQuotient += "0"
                    udivTrace[len(bitEq0Trace):]
                    udivTrace = udivTrace[len(bitEq0Trace):]
            print("quotient = {}".format(int(bitsOfQuotient, 2)))
        else:
            print("Found no unsigned division")
        

if __name__ == "__main__":
    main()
