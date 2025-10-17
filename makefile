CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -pthread
INCLUDES = -Iinclude
SRCDIR = src
OBJDIR = obj
BINDIR = bin

SOURCES = $(wildcard $(SRCDIR)/*.cpp)
OBJECTS = $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
TARGET = $(BINDIR)/trading_system

# Create directories if they don't exist
$(shell mkdir -p $(OBJDIR) $(BINDIR))

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Main executable
main: $(TARGET)

# Debug build
debug: CXXFLAGS += -g -DDEBUG
debug: $(TARGET)

# Release build
release: CXXFLAGS += -O3 -DNDEBUG
release: $(TARGET)

# Run tests
test: $(TARGET)
	./$(TARGET)

# Clean build artifacts
clean:
	rm -rf $(OBJDIR) $(BINDIR)

# Install dependencies (if any)
deps:
	@echo "No external dependencies required"

.PHONY: all main debug release test clean deps
