# Host build of the pure engine unit tests. No calculator toolchain needed.
CXX ?= c++
CXXFLAGS ?= -std=c++11 -Wall -Wextra -O2 -g

ENGINE_SRC = ../src/value.cpp ../src/stack.cpp ../src/input_field.cpp ../src/rpn.cpp
TEST_SRC = test_engine.cpp

.PHONY: run
run: test_engine
	./test_engine

test_engine: $(ENGINE_SRC) $(TEST_SRC)
	$(CXX) $(CXXFLAGS) $(ENGINE_SRC) $(TEST_SRC) -o $@

.PHONY: clean
clean:
	rm -f test_engine
