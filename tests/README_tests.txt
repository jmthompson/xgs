M65816 Test Suite
=================

This test suite will create a binary test816 that you can use to load
binary test suites and run them inside a very basic 65816 virtual
machine with just RAM (no ROM or I/O).

At the moment everything is hard coded to run the 6502/65C02 functional test
suite by Klaus Dormann, which is included in the test directory. I've
included not only the source code but also the assembler output, as it
requires the as65 assembler which is not open source and doesn't even want
to run on my Fedora system.

To run the test:

test816 -f 6502_functional_test.bin

The test will continue running until it encounters a test trap, which takes
the form of a branch of jump instruction that loops back on itself in an
infinite loop.

For more information on Klaus' excellent test suite check out the project
on GitHub: https://github.com/Klaus2m5/6502_65C02_functional_tests

At the moment everything passes, but this only covers emulation mode. I am
currently looking for more test code to test full 65816 functionality.
