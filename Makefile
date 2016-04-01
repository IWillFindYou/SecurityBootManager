# empty makefile
MODE = "disk"

all: build

build:
	./scripts/compile-$(MODE)mode.sh

run:
	./scripts/run-$(MODE)mode.sh
