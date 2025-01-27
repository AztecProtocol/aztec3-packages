#!/bin/bash

rm -rf ./test/noir_compiled_examples/**/target
nargo --program-dir ./test/noir_compiled_examples/assert_lt compile --force --pedantic-solving
nargo --program-dir ./test/noir_compiled_examples/assert_msg_runtime compile --force --pedantic-solving
nargo --program-dir ./test/noir_compiled_examples/fold_fibonacci compile --force --pedantic-solving
nargo --program-dir ./test/noir_compiled_examples/assert_raw_payload compile --force --pedantic-solving
nargo --program-dir ./test/noir_compiled_examples/databus compile --force --pedantic-solving
