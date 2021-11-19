#!/bin/sh

echo "\t\t '' Interpreter tests ''"

executable="./boolx"
prompt="> "

command="$executable"
echo "\n\n$prompt$command"
$command

command="$executable -d"
echo "\n\n$prompt$command"
$command

command="$executable -d arg1 arg2"
echo "\n\n$prompt$command"
$command

command="$executable arg1 arg2"
echo "\n\n$prompt$command"
$command

command="$executable programs/hello_world.bx"
echo "\n\n$prompt$command"
$command

command="$executable -d programs/hello_world.bx"
echo "\n\n$prompt$command"
$command

command="$executable --debug programs/hello_world.bx"
echo "\n\n$prompt$command"
$command

echo "\n\n\n\n"

