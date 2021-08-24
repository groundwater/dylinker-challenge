all: run

.build: 
	docker build -t build .
	touch $@

myapp: mylinker main.c .build
	docker run \
		--rm \
		-v "$$PWD":/usr/src/myapp \
		-w /usr/src/myapp \
		build \
		gcc -Wl,-dynamic-linker,/usr/src/myapp/mylinker -o myapp main.c
	# patchelf --set-interpreter=$(shell pwd)/mylinker myapp

mylinker: linker.c .build
	docker run \
		--rm \
		-v "$$PWD":/usr/src/myapp \
		-w /usr/src/myapp \
		build \
		gcc -static -o mylinker linker.c

i: myapp
	docker run \
		--rm \
		-v "$$PWD":/usr/src/myapp \
		-w /usr/src/myapp \
		-it \
		build \
		bash

debug: myapp
	docker run \
		--privileged \
		--rm \
		-v "$$PWD":/usr/src/myapp \
		-w /usr/src/myapp \
		-it \
		build \
		gdb mylinker

run: myapp
	docker run \
		--rm \
		-v "$$PWD":/usr/src/myapp \
		-w /usr/src/myapp \
		build \
		./myapp

clean:
	rm -f myapp
	rm -f mylinker

# -Wl,-dynamic-linker,/my/lib/ld-linux.so.2
