#!/bin/bash
#set -x #echo on

gcc -lm assembler.c -std=c99 -o assembler
gcc -lm linker.c -std=c99 -o linker
echo ======================= Spec Test =============================
./assembler spec_0.txt out1.txt
./assember spec_1.txt out2.txt
./linker out1.txt out2.txt output.txt
DIFF=$(diff specoutcorrect.txt output.txt)
if [ "$DIFF" != "" ]
then
    echo SPEC TEST FAILED
    sdiff specoutcorrect.txt output.txt
else
    echo SPEC TEST PASSED
fi

echo ======================= Crash Global Duplicates Test =============================
./assembler globalduplicates_0.txt out1.txt
./assembler globalduplicates_1.txt out2.txt
./linker out1.txt out2.txt output.txt
if [ $? == 1 ]
then
    echo SUCCESS: PROGRAM CRASHED
else
    echo FAILED: PROGRAM SHOULD HAVE CRASHED
fi

echo ======================= Crash Undefined Global Test =============================
./assembler undefinedglobal_0.txt out1.txt
./assembler undefinedglobal_1.txt out2.txt
./linker out1.txt out2.txt output.txt
if [ $? == 1 ]
then
    echo SUCCESS: PROGRAM CRASHED
else
    echo FAILED: PROGRAM SHOULD HAVE CRASHED
fi

echo ======================= Crash 3 Test =============================
./assembler stack_0.txt out1.txt
./assembler stack_1.txt out2.txt
./linker out1.txt out2.txt output.txt
if [ $? == 1 ]
then
    echo SUCCESS: PROGRAM CRASHED
else
    echo FAILED: PROGRAM SHOULD HAVE CRASHED
fi

rm assembler
rm linker
