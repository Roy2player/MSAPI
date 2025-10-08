#!/bin/bash

precisionExponent=5
calculationPrecision=0
volatilityPrecision=0
accumulativeCpu=0
accumulativeVolatility=1
passedCounter=0
failedCounter=0
unexpectedCounter=0
minimumCounter=100
maximumCounter=10000

dir="./perfBashResults/$1/$(date -I)"
file="$dir/$(date +%s)"

if [ ! -d $dir ]
then
	mkdir -p $dir
fi

touch $file

PrintResult() {
echo -ne "\n" | tee -a $file
cat <<EOF | tee -a $file
Total counter         : $((passedCounter+failedCounter+unexpectedCounter))
Passed counter        : $passedCounter
Failed counter        : $failedCounter
Unexpected counter    : $unexpectedCounter
Accumulative cpu time : $accumulativeCpu
Minimum counter       : $minimumCounter
Maximum counter       : $maximumCounter
Volatility precision  : $volatilityPrecision
EOF
}

CheckIfEnd() {
	if (( passedCounter > maximumCounter ))
        then
                echo "Limit of attempts is reached" | tee -a $file
                PrintResult
                exit 1
        fi

	if (( passedCounter < minimumCounter ))
	then
		return 0
	fi

	if [ $(awk -v x=$accumulativeVolatility -v y=$volatilityPrecision 'BEGIN { diff = x - 1; if (diff < 0) diff = -diff; print diff <= y }') -eq "1" ]
	then
		PrintResult
		exit 0
	fi
}

((calculationPrecision=precisionExponent+1))
volatilityPrecision=$( awk -v x=$precisionExponent 'BEGIN { y = 1; for (i = 0; i < x; ++i) { y = y / 10 }; printf "%."x"f", y }' )

while true
do
	echo -ne "Attempt №$passedCounter" | tee -a $file

	lastRow=$(./$1 | tail -n 1)
	result=$(echo $lastRow | cut -d ' ' -f 1 | head -c -2)

	if [ -z "$result" ]
	then
		((unexpectedCounter+=1))
		CheckIfEnd
		echo ": unexpected latest line" | tee -a $file
		continue
	fi

	if [ $result != "Passed" ]
	then
		if [ $result == "Failed" ]
		then
			echo -ne ": failed"\\r | tee -a $file
			((failedCounter+=1))
		else
			echo -ne ": unexpected latest line"\\r | tee -a $file
			((unexpectedCounter+=1))
		fi
	else
		((passedCounter+=1))
		cpuTime=$(echo $lastRow | cut -d ' ' -f 8)
		echo -ne ": cpu time $cpuTime" | tee -a $file

		if (( passedCounter > 1 ))
		then
			accumulativeCpu=$(( $accumulativeCpu + ($cpuTime-$accumulativeCpu)/$passedCounter))
			volatility=$(awk -v x=$cpuTime -v y=$accumulativeCpu -v p=$calculationPrecision 'BEGIN { printf "%."p"f", x / y }')
			accumulativeVolatility=$(awk -v x=$accumulativeVolatility -v y=$volatility -v z=$passedCounter -v p=$calculationPrecision 'BEGIN { printf "%."p"f", x + (y - x) / z }')

			echo -ne ", accCpu: $accumulativeCpu, vol: $volatility, accVol: $accumulativeVolatility" | tee -a $file
		else
			((accumulativeCpu=cpuTime))
		fi
	fi
	
	CheckIfEnd
	echo | tee -a $file
done
