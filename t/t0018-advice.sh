#!/bin/sh

test_description='Test advise_if_enabled functionality'

. ./test-lib.sh

cat > expected <<EOF
hint: This is a piece of advice
hint: Disable this message with "git config advice.nestedTag false"
EOF
test_expect_success 'advise should be printed when config variable is unset' '
	test-tool advise "This is a piece of advice" 2>actual &&
	test_i18ncmp expected actual
'

test_expect_success 'advise should be printed when config variable is set to true' '
	test_config advice.nestedTag true &&
	test-tool advise "This is a piece of advice" 2>actual &&
	test_i18ncmp expected actual
'

test_expect_success 'advise should not be printed when config variable is set to false' '
	test_config advice.nestedTag false &&
	test-tool advise "This is a piece of advice" 2>actual &&
	test_must_be_empty actual
'

test_done
