#!/bin/bash

if [ $# -eq 0 ]; then
    echo "Использование: $0 <number1> <number2>... <numbern>"
    exit 1
fi

sum=0
countargs=$#

for i in "$@"; do
    if ! [[ "$i" =~ ^-?[0-9]+\.?[0-9]*$ ]]; then
        echo "Ошибка: '$i' не число" >&2
        exit 1
    fi
    sum=$(echo "$sum + $i" | bc -l)
done

middle=$(echo "scale=6; $sum / $countargs" | bc -l)

echo "Количество: $countargs"
echo "Среднее: $middle"
