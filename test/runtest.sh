#! /bin/sh

export PROG=pico

if [ "$1" = "-clean" ]; then
	rm -f $PROG *.res *.stderr
	exit 0
fi

# compilation:
if [ "$1" = "-extension" ]; then
	(cd ../src; CFLAGS=-DSHORTCUTS_EXTENSION make clean all; cp $PROG ../test/; make clean);
	shift
else
	(cd ../src; make clean all; cp $PROG ../test/; make clean);
	echo "Extensions are disabled!" >&2
fi

if [ "$1" = "-valgrind" ]; then
	VALGRIND=valgrind
else
	VALGRIND=""
fi

TEST_SET="counter int_test picoprog uclock all_opcodes extension"
# tests from http://bleyer.org/pacoblaze
TEST_SET="$TEST_SET adc_ctrl auto_pwm clock control dac_ctrl fc_ctrl fg_ctrl"
TEST_SET="$TEST_SET led_ctrl ls_test progctrl pwm_ctrl security sha1prog spi_prog"

for TEST in $TEST_SET; do
	echo "==== $TEST ===="
	$VALGRIND ./$PROG < $TEST.in > $TEST.res 2> $TEST.stderr

	if diff $TEST.res $TEST.out > /dev/null; then
		echo "== [SUCCESS] =="
	else
		echo "== [FAILED] =="
	fi
done
