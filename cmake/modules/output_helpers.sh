#!/bin/bash
#
# Pretty print helper functions
#
# Copyright 2005-2013 by Alex Turbov <i.zaufi@gmail.com>
#

COLOR_BRACKET='\033[1;34m'
COLOR_SUCCESS='\033[1;32m'
COLOR_FAILURE='\033[1;31m'
COLOR_WARNING='\033[1;33m'
COLOR_NORMAL='\033[0;39m'

_term_columns=${COLUMNS:-0}
(( _term_columns == 0 )) && _term_columns=$(stty size 2>/dev/null | cut -d' ' -f2)
(( _term_columns > 0 )) || (( _term_columns = 80 ))
(( _term_columns = _term_columns - 5 ))

_is_tty=$(tty -s && echo 'yes' || echo 'no')
_ebegin_in_action='no'
_initial_lines_count=1
_spammed_lines=${_initial_lines_count}

function move_to_col()
{
    test -n "$1" && printf "\033[$1G"
}
function move_above()
{
    test -n "$1" && printf "\033[$1A"
}
function move_below()
{
    test -n "$1" && printf "\033[$1B"
}

function ebegin()
{
    printf " ${COLOR_SUCCESS}*${COLOR_NORMAL} "
    printf "$1"
    _ebegin_in_action='yes'
}

function eend()
{
    _ebegin_in_action='no'
    if [ "${_spammed_lines}" != "${_initial_lines_count}" ]; then
        move_above ${_spammed_lines}
    fi
    if [ "${_is_tty}" = 'yes' ]; then
        move_to_col ${_term_columns}
        printf "${COLOR_BRACKET}["
        if test "$1" = '0'; then
            printf " ${COLOR_SUCCESS}ok"
        else
            printf " ${COLOR_FAILURE}!!"
        fi
        printf " ${COLOR_BRACKET}]${COLOR_NORMAL}\n"
    fi
    if [ "${_spammed_lines}" != "${_initial_lines_count}" ]; then
        move_below ${_spammed_lines}
    fi
    _spammed_lines=${_initial_lines_count}
    if [ "$1" != '0' ]; then
        exit 1
    fi
}

function inc_lines_printed()
{
    if [ "${_ebegin_in_action}" = 'yes' ]; then
        if [ "${_spammed_lines}" = "${_initial_lines_count}" ]; then
            printf '\n'
        fi
        _spammed_lines=$((_spammed_lines+1))
    fi
}

#
# TODO It seems that we need to take care about line length we want to print
#       to correct outputed lines counting...
#
function eerror()
{
    inc_lines_printed
    printf " ${COLOR_FAILURE}*${COLOR_NORMAL} Error: $1\n"
}

function ewarn()
{
    inc_lines_printed
    printf " ${COLOR_WARNING}*${COLOR_NORMAL} Warning: $1\n"
}

function einfo()
{
    inc_lines_printed
    printf " ${COLOR_SUCCESS}*${COLOR_NORMAL} $1\n"
}
