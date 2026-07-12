# Host build of the pure engine unit tests. No calculator toolchain needed.
CXX ?= c++
CXXFLAGS ?= -std=c++11 -Wall -Wextra -O2 -g

ENGINE_SRC = ../src/value.cpp ../src/stack.cpp ../src/input_field.cpp \
             ../src/rpn.cpp ../src/layout.cpp
TEST_SRC = test_engine.cpp

.PHONY: run
run: test_engine
	./test_engine

test_engine: $(ENGINE_SRC) $(TEST_SRC)
	$(CXX) $(CXXFLAGS) $(ENGINE_SRC) $(TEST_SRC) -o $@

# Visual checks: render sample layouts / a full screen to /tmp/render.png.
.PHONY: render render-screen
render:
	$(CXX) $(CXXFLAGS) ../src/value.cpp ../src/layout.cpp render_host.cpp -o render_host
	./render_host | python3 paint.py

render-screen:
	$(CXX) $(CXXFLAGS) $(ENGINE_SRC) render_screen_host.cpp ../src/screen.cpp -o render_screen_host
	./render_screen_host | python3 paint.py

.PHONY: clean
clean:
	rm -f test_engine render_host render_screen_host
