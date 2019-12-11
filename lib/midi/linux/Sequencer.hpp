#ifndef LINUX_SEQUENCER_HPP
#define LINUX_SEQUENCER_HPP
#include "../MIDIFileReader.hpp"
#include <iostream>
#include <alsa/asoundlib.h>

class LinuxSequencer : public Sequencer
{
protected:
	std::ostream& Device;
	int MaxDiff;
	double TotalDiff;
	int MaxInDiff;
	double TotalInDiff;
	int Number;
public:
	LinuxSequencer( std::ostream& Device0 );
	~LinuxSequencer();
	void note_on( int Channel, int Note, int Velocity );
	void note_off( int Channel, int Note, int Velocity );
	void after_touch( int Channel, int Note, int Velocity );
	void program_change( int Channel, int NewProgram );
	void control_change( int Channel, int Control, int Value );
	void pitch_bend( int Channel, int Bend );

	void start();
	double average_diff() { return Number > 0 ? TotalDiff / Number : 0; }
	int max_diff() { return MaxDiff; }
	double average_in_diff() { return Number > 0 ? TotalInDiff / Number : 0; }
	int max_in_diff() { return MaxInDiff; }
protected:
	void wait_for_usec( double WaitMicroSecs );
}; // LinuxSequencer

class ALSASequencer : public LinuxSequencer
{
public:
	ALSASequencer( std::ostream& Device0 );
	~ALSASequencer();
	void note_on( int Channel, int Note, int Velocity );
	void note_off( int Channel, int Note, int Velocity );
	void after_touch( int Channel, int Note, int Velocity );
	void program_change( int Channel, int NewProgram );
	void control_change( int Channel, int Control, int Value );
	void pitch_bend( int Channel, int Bend );

	void start();
protected:
  snd_seq_t* Seq;
  int Client;
  int OutClient;
  int OutPort;
}; // ALSASequencer

#endif // LINUX_SEQUENCER_HPP
