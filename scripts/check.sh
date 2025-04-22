#!/usr/bin/env bash

_TEST_MD_FILE="tests/test.md"
_TEST_HTML_RESULT_FILE="/tmp/mthc_test_result.html"
_TEST_EXPECTED_HTML_RESULT_FILE="tests/expected.html"

main() {
    echo "Compile mthc program..."
    make mthc

    echo "Generate html from test markdown..."
    ./mthc ${_TEST_MD_FILE} | sed -n -e '1,/^=== Generate HTML ===$/d' -e 'p' >${_TEST_HTML_RESULT_FILE}

    echo "Compare generated html with expected result..."
    diff ${_TEST_HTML_RESULT_FILE} ${_TEST_EXPECTED_HTML_RESULT_FILE}
    if (( $? != 0 )); then
        echo "Test failed: generated HTML does not match expected result."
        exit 1
    else 
        echo "Test passed: generated HTML matches expected result."
    fi
}

main "$@"
