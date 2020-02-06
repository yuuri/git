#!/bin/sh

test_description='Test advise_ng functionality'

. ./test-lib.sh

cat > expected <<EOF
hint: This is a piece of advice
hint: Turn this message off by running
hint: "git config advice.configVariable false"
EOF
test_expect_success 'advise should be printed when config variable is unset' '
	test-tool advise "advice.configVariable" "This is a piece of advice" 2>actual &&
	test_i18ncmp expected actual
'

test_expect_success 'advise should be printed when config variable is set to true' '
	test_config advice.configVariable true &&
	test-tool advise "advice.configVariable" "This is a piece of advice" 2>actual &&
	test_i18ncmp expected actual
'

test_expect_success 'advise should not be printed when config variable is set to false' '
	test_config advice.configVariable false &&
	test-tool advise "advice.configVariable" "This is a piece of advice" 2>actual &&
	test_must_be_empty actual
'

test_done
