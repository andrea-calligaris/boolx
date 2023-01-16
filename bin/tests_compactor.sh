#!/bin/dash

clear

printf "\t\t ## Compactor tests ##"

executable="./compactorx"
separator="-------------------------------"
prompt=" > "

command="$executable"
printf "\n\n\n%s\n\n%s%s\n\n" "$separator" "$prompt" "$command"
$command

command="$executable -l 20"
printf "\n\n\n%s\n\n%s%s\n\n" "$separator" "$prompt" "$command"
$command

command="$executable --lines_length 20 arg1 arg2"
printf "\n\n\n%s\n\n%s%s\n\n" "$separator" "$prompt" "$command"
$command

command="$executable arg1 arg2"
printf "\n\n\n%s\n\n%s%s\n\n" "$separator" "$prompt" "$command"
$command

test_file="programs/compactor_test_file.bx"
if test -f "$test_file"; then # file exists
    rm "$test_file" # delete it
fi
command="$executable programs/addition.bx $test_file"
printf "\n\n\n%s\n\n%s%s\n\n" "$separator" "$prompt" "$command"
$command
command="cat $test_file"
printf "\n%s%s\n" "$prompt" "$command"
$command
printf "\n"

if test -f "$test_file"; then # file exists
    rm "$test_file" # delete it
fi
command="$executable -l 20 programs/addition.bx $test_file"
printf "\n\n\n%s\n\n%s%s\n\n" "$separator" "$prompt" "$command"
$command
command="cat $test_file"
printf "\n%s%s\n" "$prompt" "$command"
$command
printf "\n"

if test -f "$test_file"; then # file exists
    rm "$test_file" # delete it
fi
command="$executable --lines_length 20 programs/addition.bx $test_file"
printf "\n\n\n%s\n\n%s%s\n\n" "$separator" "$prompt" "$command"
$command
command="cat $test_file"
printf "\n%s%s\n" "$prompt" "$command"
$command
printf "\n"

# etc.

if test -f "$test_file"; then # file exists
    rm "$test_file" # delete it
fi

printf "\n\n\n\n"

