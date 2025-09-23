# Define the C compiler
CC = gcc

# Define the name of the executable
TARGET = bt

# Define the source files
SRCS = lexer.c parser.c bt.c main.c

# Rule to build the executable
$(TARGET): $(SRCS)
	$(CC) -o $(TARGET) $(SRCS)

# Rule to clean up the executable
clean:
	rm -f $(TARGET)

# Rule to run the executable
run: $(TARGET)
	./$(TARGET)

# Run with args passed as ARGS='path/to/file'
runf: $(TARGET)
	./$(TARGET) $(ARGS)

# Short aliases
r: $(TARGET)
	./$(TARGET) $(FILE)

rr: $(TARGET)
	cat $(IN) | ./$(TARGET)

# Tests
.PHONY: test
test: $(TARGET)
	bash tests/test_cli.sh

# Web app
.PHONY: web-install web-run web-test
web-install:
	python3 -m venv .venv && . .venv/bin/activate && pip install -U pip && pip install -r requirements.txt

web-run:
	. .venv/bin/activate && FLASK_APP=web/app.py flask run --reload

web-test:
	. .venv/bin/activate && pytest -q