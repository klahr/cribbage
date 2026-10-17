CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra
SRC = test_rules.cpp ../src/core/scoring.cpp ../src/core/game.cpp ../src/core/bot.cpp

test: test_rules
	./test_rules

test_rules: $(SRC) $(wildcard ../src/core/*.h)
	$(CXX) $(CXXFLAGS) -o $@ $(SRC)

clean:
	rm -f test_rules

.PHONY: test clean
