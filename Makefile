CXXFLAGS=-Oz -fno-stack-protector -ffunction-sections -fdata-sections -Wl,--gc-sections -fno-unwind-tables -fno-asynchronous-unwind-tables -fno-math-errno -ffast-math -fno-unroll-loops -fmerge-all-constants -fno-ident -ffast-math  -Wl,--hash-style=gnu -Wl,--build-id=none -fno-rtti -fno-exceptions -fomit-frame-pointer -fno-unroll-loops -fno-profile-use

.PHONY: app
app: main.cc
	g++ `pkg-config --cflags glfw3` -o app main.cc `pkg-config --static --libs glfw3 gl`
	strip -S \
	  --strip-unneeded \
	  --remove-section=.note.gnu.gold-version \
	  --remove-section=.comment \
	  app
#	du -b app | awk '{ print  (65536 - $$1 )} $$1 > 65536 { exit 1 }'

.PHONY: clean
clean:
	rm app 

run:
	make
	./app
	
run9:
	make
	./app 9
