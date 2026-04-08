#!/bin/bash

PASS=0
FAIL=0

for i in 1 2 3 4 5 6 7; do
    cp programs/code${i}.txt /tmp/test${i}.txt
    python3 Compiler.py /tmp/test${i}.txt
    ./main /tmp/test${i}.txt 2>&1 | tr -d '\r' > /tmp/actual${i}.txt

    if file programs/ans${i}.txt | grep -q "UTF-16"; then
        iconv -f UTF-16 -t UTF-8 programs/ans${i}.txt | tr -d '\r' > /tmp/expected${i}.txt
    else
        tr -d '\r' < programs/ans${i}.txt > /tmp/expected${i}.txt
    fi

    # Strip leading blank lines, trailing newlines, and trailing spaces per line
    sed '/./,$!d' /tmp/actual${i}.txt   | sed 's/[[:space:]]*$//' | perl -pe 'chomp if eof' > /tmp/actual${i}_clean.txt
    sed 's/[[:space:]]*$//' /tmp/expected${i}.txt | perl -pe 'chomp if eof' > /tmp/expected${i}_clean.txt

    if diff -q /tmp/actual${i}_clean.txt /tmp/expected${i}_clean.txt > /dev/null 2>&1; then
        echo "Test $i: PASS"
        PASS=$((PASS + 1))
    else
        echo "Test $i: FAIL"
        diff /tmp/actual${i}_clean.txt /tmp/expected${i}_clean.txt | head -5
        FAIL=$((FAIL + 1))
    fi
done

echo ""
echo "Results: $PASS passed, $FAIL failed"