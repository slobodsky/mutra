LDFLAGS += -lasound

all: midi_player midi_input_test

midi_player: MIDIFileReader.o linux/player.o linux/Sequencer.o linux/input_device.o
	${CXX} -o $@ $^  ${LDFLAGS}

midi_input_test: MIDIFileReader.o linux/midi_input_test.o linux/Sequencer.o linux/input_device.o
	${CXX} -o $@ $^  ${LDFLAGS}

clean:
	rm -f midi_player midi_input_test *.o linux/*.o *~ linux/*~
