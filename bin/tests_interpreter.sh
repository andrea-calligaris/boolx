#!/bin/dash

clear

printf "\t\t ## Interpreter tests ##"

executable="./boolx"
separator="-------------------------------"
prompt=" > "

command="$executable"
printf "\n\n\n%s\n\n%s%s\n\n" "$separator" "$prompt" "$command"
$command

# command="$executable -d"
# printf "\n\n\n%s\n\n%s%s\n\n" "$separator" "$prompt" "$command"
# $command

# command="$executable -d arg1 arg2"
# printf "\n\n\n%s\n\n%s%s\n\n" "$separator" "$prompt" "$command"
# $command

command="$executable arg1 arg2"
printf "\n\n\n%s\n\n%s%s\n\n" "$separator" "$prompt" "$command"
$command

command="$executable programs/hello_world.bx"
printf "\n\n\n%s\n\n%s%s\n\n" "$separator" "$prompt" "$command"
$command

# command="$executable -d programs/hello_world.bx"
# printf "\n\n\n%s\n\n%s%s\n\n" "$separator" "$prompt" "$command"
# $command

# command="$executable --debug programs/hello_world.bx"
# printf "\n\n\n%s\n\n%s%s\n\n" "$separator" "$prompt" "$command"
# $command

# etc.

printf "\n\n\n\n"

