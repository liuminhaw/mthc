#!/usr/bin/env bash

_TEST_DIR="tests"

# Result will be in the format of "test_name|status|generated_file|expected_file"
_TESTS_RESULTS=()

# output color settings
_RED=$(tput setaf 1)
_GREEN=$(tput setaf 2)
_YELLOW=$(tput setaf 3)
_CYAN=$(tput setaf 6)
_RESET=$(tput sgr0)

# --------------------------------------------------------------------------------------------------
# Running the test, result will be append to _TESTS_RESULT array in the format of
# "test_name|status|generated_file|expected_file"
#
# Globals:
#   _TEST_DIR
#   _TESTS_RESULTS
#
# Arguments:
#   $1: input_file
# --------------------------------------------------------------------------------------------------
run_test() {
    if [[ ${#} -ne 1 ]]; then
        echo "Usage: ${FUNCNAME[0]} <input_file>"
        return 1
    fi

    local _input_file="${1}"
    local _file_base
    _file_base="$(basename "${_input_file}" .md)"
    local _expected_file
    _expected_file="${_TEST_DIR}/${_file_base}.html"
    local _generated_file
    _generated_file="/tmp/mthc_${_file_base}.html"

    echo "===== Testcase: ${_file_base} ====="
    echo "Generate html from test markdown..."
    ./mthc --test "${_input_file}" 1>"${_generated_file}" 2>/dev/null

    sed -i 's/[[:blank:]]\+$//' "${_generated_file}"
    sed -i 's/[[:blank:]]\+$//' "${_expected_file}"

    echo "Compare generated html with expected result..."
    if ! diff "${_generated_file}" "${_expected_file}" &>/dev/null; then
        _TEST_RESULTS+=("${_file_base}|failed|${_generated_file}|${_expected_file}")
    else
        _TEST_RESULTS+=("${_file_base}|passed|${_generated_file}|${_expected_file}")
    fi
    echo ""
}

# --------------------------------------------------------------------------------------------------
# Show result information of all tests
#
# Globals:
#   _TEST_RESULTS
#
# Output:
#   The result information of all tests
# --------------------------------------------------------------------------------------------------
print_result() {
    local _total_count=0
    local _passed_count=0
    local _failed_count=0

    local _result
    echo "===== Result ====="
    for _result in "${_TEST_RESULTS[@]}"; do
        local _test_name
        local _status
        local _generated_file
        local _expected_file

        IFS='|' read -r _test_name _status _generated_file _expected_file <<<"${_result}"

        if [[ "${_status}" == "failed" ]]; then
            echo "${_RED}Test: ${_test_name} failed.${_RESET}"
            echo "Generated file: ${_generated_file}"
            echo "Expected file: ${_expected_file}"
            echo "Diff:"
            diff --color "${_generated_file}" "${_expected_file}"
            ((_failed_count++))
        else
            echo "${_GREEN}Test: ${_test_name} passed.${_RESET}"
            ((_passed_count++))
        fi
    done

    echo ""
    local _format="${_CYAN}"
    if [[ ${_failed_count} -ne 0 ]]; then
        _format="${_YELLOW}"
    fi
    echo "${_format}Total tests: ${#_TEST_RESULTS[@]}"
    echo "Passed tests: ${_passed_count}"
    echo "Failed tests: ${_failed_count}${_RESET}"

    if [[ ${_failed_count} -ne 0 ]]; then
        return 1
    fi
}

main() {
    if [[ ${#} -eq 1 ]]; then
        echo "Calling with argument: ${1}"
    fi

    echo "Compile mthc program..."
    if ! make; then
        echo "${_RED}Failed to compile mthc program.${_RESET}"
        exit 1
    fi
    echo ""

    local _files=()
    if [[ ${#} -eq 1 ]]; then
        _files=("${_TEST_DIR}"/*"${1}".md)
    else
        _files=("${_TEST_DIR}"/*.md)
    fi

    local _file
    for _file in "${_files[@]}"; do
        if [[ ! -f "${_file}" ]]; then
            echo "${_RED}File ${_file} does not exist.${_RESET}"
            continue
        fi
        run_test "${_file}"
    done

    if ! print_result; then
      exit 1
    fi
}

main "$@"
