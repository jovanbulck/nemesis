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
        divRegex = "4112(211)?12(111)?34211(1112(11)?12){16}"
        divPattern = re.compile(divRegex)
        divHit = divPattern.search(latenciesString)
        if divHit:
            print("Found signed modulo")
            divTrace = divHit.group()
            divTrace = divTrace[4:] # consume the three known instructions at the beginning
            if divTrace.startswith("211"):
                positiveDividend = False
                divTrace = divTrace[3:]
            else:
                positiveDividend = True
            divTrace = divTrace[2:]
            
            if divTrace.startswith("111"):
                positiveDivisor = False
                divTrace = divTrace[3:]
            else:
                positiveDivisor = True
            divTrace = divTrace[5:]
            
            bitsOfQuotient = ""
            bitEq1Trace = "11121112"
            bitEq0Trace = "111212"
            while divTrace.startswith(bitEq1Trace) or divTrace.startswith(bitEq0Trace):
                if divTrace.startswith(bitEq1Trace):
                    bitsOfQuotient += "1"
                    divTrace = divTrace[len(bitEq1Trace):]
                elif divTrace.startswith(bitEq0Trace):
                    bitsOfQuotient += "0"
                    divTrace[len(bitEq0Trace):]
                    divTrace = divTrace[len(bitEq0Trace):]
            quotient = int(bitsOfQuotient, 2)
            remainderIsPositive = True
            print("Positive dividend: {}".format(positiveDividend))
            print("Positive divisor: {}".format(positiveDivisor))
            if positiveDividend:
                if not positiveDivisor:
                    quotient = -quotient
            else:
                remainderIsPositive = False
                if positiveDivisor:
                    quotient = -quotient
                
            print("quotient = {}".format(quotient))
            print("Positive remainder: {}".format(remainderIsPositive))
        else:
            print("Found no signed division")
        

if __name__ == "__main__":
    main()
