# - Interact with TeamCity via serive messages
#
# See also: http://confluence.jetbrains.com/display/TCD8/Build+Script+Interaction+with+TeamCity
#
# TODO Docs!
#

#=============================================================================
# Copyright 2012-2014 by Alex Turbov <i.zaufi@gmail.com>
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file LICENSE for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of this repository, substitute the full
#  License text for the above reference.)

include(CMakeParseArguments)

macro(is_running_under_teamcity VAR)
    if(NOT "$ENV{TEAMCITY_PROCESS_FLOW_ID}" STREQUAL "")
        set(${VAR} TRUE)
    else()
        set(${VAR} FALSE)
    endif()
endmacro()

macro(tc_escape_string VAR)
    string(REPLACE "|"   "||" ${VAR} "${${VAR}}")
    string(REPLACE "\n" "|n" ${VAR} "${${VAR}}")
    string(REPLACE "'"   "|'" ${VAR} "${${VAR}}")
    string(REPLACE "["   "|[" ${VAR} "${${VAR}}")
    string(REPLACE "]"   "|]" ${VAR} "${${VAR}}")
endmacro()

function(_tc_bare_message NAME )
    if(NOT "$ENV{TEAMCITY_PROCESS_FLOW_ID}" STREQUAL "")
        # Setup message start
        set(_message "##teamcity[${NAME}")

        # Append preformatted key-value pairs
        foreach(_p ${ARGN})
            set(_message "${_message} ${_p}")
        endforeach()

        # Show message w/ closing ']'
        message("${_message}]")
    endif()
endfunction()

function(_tc_message NAME )
    if(NOT "$ENV{TEAMCITY_PROCESS_FLOW_ID}" STREQUAL "")
        # Setup message start
        string(TIMESTAMP _timestamp "%Y-%m-%dT%H:%M:%S.000" UTC)
        set(_message "##teamcity[${NAME} flowId='$ENV{TEAMCITY_PROCESS_FLOW_ID}' timestamp='${_timestamp}'")

        # Append preformatted key-value pairs
        foreach(_p ${ARGN})
            set(_message "${_message} ${_p}")
        endforeach()

        # Show message w/ closing ']'
        message("${_message}]")
    endif()
endfunction()

function(_tc_push_prop_to_list PROP_PART ID)
    get_property(_list_exists GLOBAL PROPERTY TC_${PROP_PART} SET)
    if(_list_exists)
        get_property(_list GLOBAL PROPERTY TC_${PROP_PART})
    endif()

    list(INSERT _list 0 "${ID}")
    set_property(GLOBAL PROPERTY TC_${PROP_PART} ${_list})
endfunction()

macro(_tc_get_and_pop_prop_from_list PROP_PART)
    get_property(_list_exists GLOBAL PROPERTY TC_${PROP_PART} SET)
    if(_list_exists)
        get_property(_list GLOBAL PROPERTY TC_${PROP_PART})
    else()
        message(FATAL_ERROR "Misbalanced block start/end calls")
    endif()
    list(GET _list 0 _id)
    list(REMOVE_AT _list 0)

    set_property(GLOBAL PROPERTY TC_${PROP_PART} ${_list})
endmacro()

macro(_tc_get_prop_from_list PROP_PART VAR)
    get_property(_list_exists GLOBAL PROPERTY TC_${PROP_PART} SET)
    if(_list_exists)
        get_property(_list GLOBAL PROPERTY TC_${PROP_PART})
    else()
        message(FATAL_ERROR "Misbalanced block start/end calls")
    endif()
    list(GET _list 0 ${VAR})
endmacro()

function(_tc_generic_bare_block_start MSGTYPE PROP_PART ID)
    _tc_bare_message("${MSGTYPE}" ${ID} ${ARGN})
    _tc_push_prop_to_list(${PROP_PART} ${ID})
endfunction()

function(_tc_generic_bare_block_end MSGTYPE PROP_PART)
    _tc_get_and_pop_prop_from_list(${PROP_PART})
    _tc_bare_message("${MSGTYPE}" ${_id})
endfunction()

function(_tc_generic_block_start MSGTYPE PROP_PART ID)
    _tc_message("${MSGTYPE}" ${ID} ${ARGN})
    _tc_push_prop_to_list(${PROP_PART} ${ID})
endfunction()

function(_tc_generic_block_end MSGTYPE PROP_PART)
    _tc_get_and_pop_prop_from_list(${PROP_PART})
    _tc_message("${MSGTYPE}" ${_id})
endfunction()

##teamcity[blockOpened name='<blockName>']
function(tc_block_start NAME)
    tc_escape_string(NAME)
    _tc_generic_block_start("blockOpened" "BLOCK" "name='${NAME}'")
endfunction()

##teamcity[blockClosed name='<blockName>']
function(tc_block_end)
    _tc_generic_block_end("blockClosed" "BLOCK")
endfunction()

##teamcity[message text='<message text>' errorDetails='<error details>' status='<status value>']
list(APPEND _TC_MESSAGE_STATUSES NORMAL WARNING FAILURE ERROR)
function(tc_message STATUS TEXT)
    # Check status
    list(FIND _TC_MESSAGE_STATUSES ${STATUS} _status)
    if(_status EQUAL -1)
        message(FATAL_ERROR "tc_message called with invalid status")
    endif()

    tc_escape_string(TEXT)
    tc_escape_string(STATUS)
    if(STATUS STREQUAL "ERROR" AND (NOT ARG2 STREQUAL ""))
        tc_escape_string(ARGV2)
        _tc_message("message" "text='${TEXT}'" "status='${STATUS}'" "errorDetails='${ARGV2}'")
    else()
        _tc_message("message" "text='${TEXT}'" "status='${STATUS}'")
    endif()
endfunction()

##teamcity[compilationStarted compiler='<compiler name>']
function(tc_compile_start COMPILER)
    tc_escape_string(COMPILER)
    _tc_generic_block_start("compilationStarted" "COMPILE" "compiler='${COMPILER}'")
endfunction()

##teamcity[compilationFinished compiler='<compiler name>']
function(tc_compile_end)
    _tc_generic_block_end("compilationFinished" "COMPILE")
endfunction()

##teamcity[progressStart '<message>']
function(tc_progress_start TEXT)
    tc_escape_string(TEXT)
    _tc_generic_bare_block_start("progressStart" "PROGRESS" "'${TEXT}'")
endfunction()

##teamcity[progressFinish '<message>']
function(tc_progress_end)
    _tc_generic_bare_block_end("progressFinish" "PROGRESS")
endfunction()

##teamcity[progressMessage '<message>']
function(tc_progress MESSAGE)
    tc_escape_string(MESSAGE)
    _tc_bare_message("progressMessage" "'${MESSAGE}'")
endfunction()

##teamcity[testSuiteStarted name='suiteName']
function(tc_test_suite_start NAME)
    tc_escape_string(NAME)
    _tc_generic_bare_block_start("testSuiteStarted" "TEST_SUITE" "name='${NAME}'")
endfunction()

##teamcity[testSuiteFinished name='suiteName']
function(tc_test_suite_end)
    _tc_generic_bare_block_end("testSuiteFinished" "TEST_SUITE")
endfunction()

##teamcity[testStarted name='testName' captureStandardOutput='<true/false>']
function(tc_test_start NAME)
    tc_escape_string(NAME)
    if(ARGV1 STREQUAL "CAPTURE_OUTPUT")
        _tc_generic_bare_block_start("testStarted" "TEST" "name='${NAME}'" "captureStandardOutput='true'")
    else()
        _tc_generic_bare_block_start("testStarted" "TEST" "name='${NAME}'" "captureStandardOutput='false'")
    endif()
endfunction()

##teamcity[testFinished name='testName' duration='<test_duration_in_milliseconds>']
function(tc_test_end)
    set(one_value_args DURATION)
    cmake_parse_arguments(tc_test_end "" "${one_value_args}" "" ${ARGN})

    tc_escape_string(TEST)
    if(tc_test_end_DURATION)
        tc_escape_string(tc_test_end_DURATION)
        _tc_generic_bare_block_end("testFinished" "TEST" "duration='${tc_test_end_DURATION}'")
    else()
        _tc_generic_bare_block_end("testFinished" "TEST")
    endif()
endfunction()

##teamcity[testFailed name='MyTest.test1' message='failure message' details='message and stack trace']
##teamcity[testFailed type='comparisonFailure' name='MyTest.test2' message='failure message'
##         details='message and stack trace' expected='expected value' actual='actual value']
function(tc_test_failed MESSAGE)
    set(one_value_args DETAILS TYPE EXPECTED ACTUAL)
    cmake_parse_arguments(tc_test_failed "" "${one_value_args}" "" ${ARGN})

    set(_aux_args)
    if(tc_test_failed_DETAILS)
        tc_escape_string(tc_test_failed_DETAILS)
        list(APPEND _aux_args "details='${tc_test_failed_DETAILS}'")
    endif()
    if(tc_test_failed_TYPE)
        tc_escape_string(tc_test_failed_TYPE)
        list(APPEND _aux_args "type='${tc_test_failed_TYPE}'")
    endif()
    if(tc_test_failed_EXPECTED)
        tc_escape_string(tc_test_failed_EXPECTED)
        list(APPEND _aux_args "expected='${tc_test_failed_EXPECTED}'")
    endif()
    if(tc_test_failed_ACTUAL)
        tc_escape_string(tc_test_failed_ACTUAL)
        list(APPEND _aux_args "actual='${tc_test_failed_ACTUAL}'")
    endif()

    _tc_get_prop_from_list("TEST" _name)
    tc_escape_string(MESSAGE)
    _tc_bare_message("testFailed" "${_name}" "message='${MESSAGE}'" ${_aux_args})
    unset(_name)
endfunction()

##teamcity[testIgnored name='testName' message='ignore comment']
function(tc_test_ignored MESSAGE)
    _tc_get_prop_from_list("TEST" _name)
    tc_escape_string(MESSAGE)
    _tc_bare_message("testIgnored" "${_name}" "message='${MESSAGE}'")
    unset(_name)
endfunction()

##teamcity[testStdOut name='className.testName' out='text']
function(tc_test_output TEXT)
    tc_escape_string(TEXT)
    _tc_get_prop_from_list("TEST" _name)
    _tc_bare_message("testStdOut" "${_name}" "out='${TEXT}'")
    unset(_name)
endfunction()

##teamcity[testStdErr name='className.testName' out='error text']
function(tc_test_error_output TEXT)
    tc_escape_string(TEXT)
    _tc_get_prop_from_list("TEST" _name)
    _tc_bare_message("testStdErr" "${_name}" "out='${TEXT}'")
    unset(_name)
endfunction()

##teamcity[publishArtifacts '<path>']
function(tc_publish_artefacts PATH)
    tc_escape_string(PATH)
    _tc_bare_message("publishArtifacts" "'${PATH}'")
endfunction()

##teamcity[buildProblem description='<description>' identity='<identity>']
function(tc_build_problem TEXT)
    tc_escape_string(TEXT)
    if(ARGV1)
        tc_escape_string(ARGV1)
        _tc_bare_message("buildProblem" "description='${TEXT}'" "identity='${ARGV1}'")
    else()
        _tc_bare_message("buildProblem" "description='${TEXT}'")
    endif()
endfunction()

##teamcity[buildStatus status='<status value>' text='{build.status.text} and some aftertext']
function(tc_build_status TEXT)
    if(TEXT STREQUAL "SUCCESS")
        tc_escape_string(ARGV1)
        _tc_bare_message("buildStatus" "status='SUCCESS'" "text='${ARGV1}'")
    else()
        tc_escape_string(TEXT)
        _tc_bare_message("buildStatus" "text='${TEXT}'")
    endif()
endfunction()

##teamcity[buildNumber '<new build number>']
function(tc_set_build_number NEW_NUMBER)
    tc_escape_string(NEW_NUMBER)
    _tc_bare_message("buildNumber" "'${NEW_NUMBER}'")
endfunction()

##teamcity[setParameter name='ddd' value='fff']
function(tc_set_param NAME VALUE)
    tc_escape_string(NAME)
    tc_escape_string(VALUE)
    _tc_bare_message("setParameter" "'name=${NAME}'" "value='${VALUE}'")
endfunction()

##teamcity[buildStatisticValue key='<valueTypeKey>' value='<value>']
function(tc_build_stats KEY VALUE)
    tc_escape_string(KEY)
    tc_escape_string(VALUE)
    _tc_bare_message("buildStatisticValue" "'key=${KEY}'" "value='${VALUE}'")
endfunction()

##teamcity[enableServiceMessages]
function(tc_enable)
    _tc_bare_message("enableServiceMessages")
endfunction()

##teamcity[disableServiceMessages]
function(tc_disable)
    _tc_bare_message("disableServiceMessages")
endfunction()

# X-Chewy-RepoBase: https://raw.githubusercontent.com/mutanabbi/chewy-cmake-rep/master/
# X-Chewy-Path: TeamCityIntegration.cmake
# X-Chewy-Version: 1.1
# X-Chewy-Description: Interact w/ TeamCity via service messages
