#!/bin/sh

echo "\t\t '' Compactor tests ''"

executable="./compactorx"
prompt="> "

command="$executable"
echo "\n\n$prompt$command"
$command

command="$executable -l 20"
echo "\n\n$prompt$command"
$command

command="$executable --lines_length 20 arg1 arg2"
echo "\n\n$prompt$command"
$command

command="$executable arg1 arg2"
echo "\n\n$prompt$command"
$command

test_file="programs/compactor_test_file.bx"
if test -f "$test_file"; then # file exists
    rm "$test_file" # delete it
fi
command="$executable programs/addition.bx $test_file"
echo "\n\n$prompt$command"
$command
command="cat $test_file"
echo "$prompt$command"
$command
echo ""

if test -f "$test_file"; then # file exists
    rm "$test_file" # delete it
fi
command="$executable -l 20 programs/addition.bx $test_file"
echo "\n\n$prompt$command"
$command
command="cat $test_file"
echo "$prompt$command"
$command
echo ""

if test -f "$test_file"; then # file exists
    rm "$test_file" # delete it
fi
command="$executable --lines_length 20 programs/addition.bx $test_file"
echo "\n\n$prompt$command"
$command
command="cat $test_file"
echo "$prompt$command"
$command
echo ""

echo "\n\n\n\n"

