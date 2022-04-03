OBJS = \
./src/BeepBoxMain.o \
./src/Mixer.o \
./src/LoudnessStats.o \
./src/ebur128/ebur128.o

all: BeepBox

DEPS=$(OBJS:.o=.d)

depend: $(DEPS)

include $(DEPS)

BeepBox: $(OBJS)	
	mkdir -p ./bin
	g++ $(OBJS) -L. -L./lib -lBeepingCore -lm /usr/local/lib/libsndfile.a /usr/local/lib/libFLAC.a /usr/local/lib/libogg.a /usr/local/lib/libvorbis.a /usr/local/lib/libvorbisenc.a -o ./bin/$@	

clean:
	rm -rf $(OBJS) $(DEPS) ./bin/BeepBox
	rm -rf $(OBJS) $(DEPS) ./bin

CXXFLAGS= -w -DLINUX -DOSX -I. -I/usr/local/include -I./lib \
          -I./lib/include  -I./src/ebur128  \
          -I/opt/local/include -O3 -DNDEBUG -ffast-math -funroll-loops

%.o: %.c
	gcc $(CXXFLAGS) -c -o $@ $<

%.d: %.c
	echo $(@:.d=.o): \\> $@
	gcc $(CXXFLAGS) -M -o - $< | sed s/.*:// >> $@

%.o: %.cxx
	g++ $(CXXFLAGS) -c -o $@ $<

%.d: %.cxx
	echo $(@:.d=.o): \\> $@
	g++ $(CXXFLAGS) -M -o - $< | sed s/.*:// >> $@

%.o: %.cpp
	g++ $(CXXFLAGS) -c -o $@ $<

%.d: %.cpp
	echo $(@:.d=.o): \\> $@
	g++ $(CXXFLAGS) -M -o - $< | sed s/.*:// >> $@
