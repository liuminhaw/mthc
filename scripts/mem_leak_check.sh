#!/usr/bin/env bash

_TEST_DIR="tests"

# Result will be in the format of "test_name|status"
_TESTS_RESULTS=()

# output color settings
_RED=$(tput setaf 1)
_GREEN=$(tput setaf 2)
_YELLOW=$(tput setaf 3)
_CYAN=$(tput setaf 6)
_RESET=$(tput sgr0)

run_test() {
    if [[ ${#} -ne 1 ]]; then
        echo "Usage: ${FUNCNAME[0]} <input_file>"
        return 1
    fi

    local _input_file="${1}"
    local _file_base
    _file_base="$(basename "${_input_file}" .md)"

    echo "===== Testcase: ${_file_base} ====="
    echo "Testing memory leak on ${_file_base}..."
    if ! valgrind --leak-check=full --error-exitcode=104 ./mthc "${_input_file}" &>/dev/null; then
        _TEST_RESULTS+=("${_file_base}|failed")
    else
        _TEST_RESULTS+=("${_file_base}|passed")
    fi
    echo ""
}

print_result() {
    local _total_count=0
    local _passed_count=0
    local _failed_count=0

    local _result
    echo "===== Result ====="
    for _result in "${_TEST_RESULTS[@]}"; do
        local _test_name
        local _status

        IFS='|' read -r _test_name _status <<<"${_result}"

        if [[ "${_status}" == "failed" ]]; then
            echo "${_RED}Test: ${_test_name} failed.${_RESET}"
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
}

main() {
    if [[ ${#} -eq 1 ]]; then
        echo "Calling with argument: ${1}"
    fi

    echo "Compile mthc program with memory leak check..."
    if ! make valgrind; then
        echo "Failed to compile mthc program with memory leak check."
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

    print_result
}

main "$@"
