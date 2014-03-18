/*
	MID2CHART C++ REWRITE
	Because writing this stuff in console + DLL form is better than GUI :)

	The purpose of this console application is strictly to convert Rock Band 3 MIDIs to Frets on Fire/Keyboard Hero's *.chart format.
	Since all other converters don't work (And people do not like using Feedback Chart Editor)...
	I decided to make an app that converts to chart retaining the Keyboard Note Data. Putting it in [ExpertKeyboard], etc.

	NOTE: THIS APPLICATION IS SOOOOO INCOMPLETE ATM. ONLY PUT ON GIT BECAUSE I WANTED TO.
	
	By: Clara Eleanor Taylor
*/

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>

using namespace std;

const string difficulties[] = { "Expert", "Hard", "Medium", "Easy" };
const string instruments[] = { "Single", "DoubleBass", "Drums", "Keys" };
const string corris_inst[] = { "PART GUITAR", "PART BASS", "PART DRUMS", "PART KEYS" };
const int num_of_ins = 4;

unsigned int readbyte(unsigned int byte[], unsigned int&pos) {
	//I am used to C# and GML's method of reading bytes via function. This pretty much replicates it :3
	//Requires ibyte array and position.
	pos += 1;
	return byte[pos - 1];
}

unsigned int read2byte(unsigned int byte[], unsigned int&pos) {
	//This is a pain but when put in a function... it makes life so much easier...
	pos += 2;
	return (byte[pos - 2] * 256) + byte[pos - 1];
}

unsigned int read4byte(unsigned int byte[], unsigned int&pos) {
	//This is a pain but when put in a function... it makes life so much easier...
	pos += 4;
	return (byte[pos - 4] * 16777216) + (byte[pos - 3] * 65536) + (byte[pos - 2] * 256) + byte[pos - 1];
}

unsigned int VLQ_to_Int(unsigned int byte[], unsigned int&pos) {
	//This is literally THE MOST painful function in this entire program.
	//Get the last bit of the byte.
	unsigned int total = 0;

	//It is a little messy but gets the job done.
	string bits = "";
	do {
		string seq = "";
		for (int i = 0; i < 7; i++) {
			seq = to_string((byte[pos] & (1 << i)) != 0) + seq;
		}
		bits = bits + seq;
		pos += 1;
	}
	while ((byte[pos - 1] & (1 << 7)) != 0);

	//Combine the bits in the most inefficient way possible.
	for (unsigned int i = 0; i < bits.length(); i++) {
		total += pow(2, bits.length() - i - 1) * (bits[i] != '0');
	}

	//Save the day.
	return total;
}

int main() {
	ifstream midi;
	string path = "C:\\test\\notes.mid";

	//Show the console our pretty little art.
	cout << "/---------------------------------------------------------\\" << endl
		<< "|*                                                       *|" << endl
		<< "|                  < Clara's Mid2Chart >                  |" << endl
		<< "|                     Version 1.0.0.0                     |" << endl
		<< "|                                                         |" << endl
		<< "|    For people who know what the hell they are doing.    |" << endl
		<< "|*                                                       *|" << endl
		<< "\\---------------------------------------------------------/\n\n";

	//Now tell the end-user that we are actually trying to do something...
	cout << "Attempting to open MIDI File: " << path << "\n\n";

	//Actually open the file
	midi.open(path, ios::in | ios::binary | ios::ate);

	if (midi.fail() == true) {
		//You failed at life
		cout << "Failed to open the file (" << path << ") to convert..." << endl;
		getchar();
		return -1;
	}

	cout << "Managed to actually open the file. Checking Header...\n";

	unsigned int size = 0;
	midi.seekg(0, ios::end);
	size = (unsigned int)midi.tellg(); //Get the length of the file.

	//Set up the array for byte reading.
	char* byte = 0;
	byte = new char[size];

	//Afterwards, set the position back to 0 then read from the goddamn file.
	midi.seekg(0, ios::beg);
	midi.read(byte, size);

	//Now be aware, this merely puts it in a char array. We can recall all of the bytes if we wanted to.
	//Personally, I think we should convert it to ints. ;)
	unsigned int* ibyte = new unsigned int[size];
	for (unsigned int i = 0; i < size; i++) {
		//That function, read(char, size), required a char... not an unsigned char. So we are fixing it here.
		ibyte[i] = (unsigned int)byte[i] + ((byte[i] < 0) * 256);
	}

	delete[] byte; //Obliterate this. We no longer need it.

	//Header Check
	if (ibyte[0] != 0x4D || ibyte[1] != 0x54 || ibyte[2] != 0x68 || ibyte[3] != 0x64) {
		//The hell are you throwing into this application?
		cout << "Not a MIDI File" << endl;
		getchar();
		return -1;
	}

	cout << "MIDI Header Check Successful.\n\n"
		<< "Processing MIDI File...\n\n"
		<< "Size of MIDI:    " << size << " bytes (" << (double)size / 1024 << " KB)\n";

	//Where are we? Oh right we need to set up a position variable.
	unsigned int pos = 8; //Why 8? We read the first 4 bytes of the file. The next 4 are always 0x00, 0x00, 0x00, 0x06. No point in checking them.

	cout << "MIDI Track Type: ";
	//Get the file format of the MIDI
	switch (read2byte(ibyte, pos)) {
	case 0: cout << "Single Track MIDI" << endl; break;
	case 1: cout << "Multi Synchronous Track MIDI" << endl; break;
	case 2: cout << "Multi Asynchronous Track MIDI" << endl; break;
	default: cout << "Unknown Track Type MIDI" << endl; break;
	}

	int* i = 0x0;
	delete[] i;

	//Get the Number of Tracks, being "nn nn"
	unsigned int track_number = read2byte(ibyte, pos);
	cout << "Track Number:    " << track_number << endl;

	//Delta time! The resolution of the MIDI. The ticks per quarter note. The... eh you get the idea.
	unsigned int delta_time = read2byte(ibyte, pos);
	cout << "Delta Time:      " << delta_time << endl;

	unsigned int* size_of_track = new unsigned int[track_number];
	unsigned int* position_of_track = new unsigned int[track_number];
	string* name_of_track = new string[track_number];
	string* track_string = new string[track_number];

	cout << "Track Names:     ";

	bool ins_exists[num_of_ins];
	unsigned int ins_pos[num_of_ins];

	//Get the track data... At least the base of it.
	for (unsigned int i = 0; i < track_number; i++) {
		position_of_track[i] = pos;
		pos += 4;
		size_of_track[i] = read4byte(ibyte, pos);
		pos += 1;
		//Get name of the track.
		if (ibyte[pos] == 0xFF && ibyte[pos + 1] == 0x03) {
			pos += 2;
			//It passed the check. We will now read values.
			name_of_track[i] = "";
			int length = readbyte(ibyte, pos);
			string name = "";
			for (int a = 0; a < length; a++) {
				name += readbyte(ibyte, pos);
			}
			name_of_track[i] = name;

			//Because we have to be fancy with our console.
			if (i != 0)
				cout << "                 ";
			cout << name_of_track[i] << endl;

			//Checking if each instrument exists.
			for (int a = 0; a < num_of_ins; a++) {
				if (name_of_track[i] == corris_inst[a]) {
					ins_exists[a] = true;
					ins_pos[a] = position_of_track[i];
				}
			}
		}
		else {
			//Phuck
			name_of_track[i] = "";
		}

		pos = position_of_track[i] + size_of_track[i] + 8;
	}

	//The first one is the chart data. This is very important for song information. [Song]
	track_string[0] = "[Song]\n";
	track_string[0] += "{\n";
	track_string[0] += "	Name = \"" + name_of_track[0] + "\"\n";
	track_string[0] += "	Artist = \"\"\n";
	track_string[0] += "	Charter = \"\"\n";
	track_string[0] += "	Offset = 0\n";
	track_string[0] += "	Resolution = " + to_string(delta_time) + "\n";
	track_string[0] += "	Player2 = bass\n";
	track_string[0] += "	Difficulty = 0\n";
	track_string[0] += "	PreviewStart = 0.00\n";
	track_string[0] += "	PreviewEnd = 0.00\n";
	track_string[0] += "	Genre = unknown\n";
	track_string[0] += "	MediaType = \"cd\"\n";
	track_string[0] += "	MusicStream = \"song.ogg\"\n";
	track_string[0] += "	GuitarStream = \"guitar.ogg\"\n";
	track_string[0] += "	BassStream = \"rhythm.ogg\"\n";
	track_string[0] += "}\n";

	//The second one has the BPM Changes and Time Signature Changes.
	track_string[1] = "[SyncTrack]\n";
	track_string[1] += "{\n";

	//Collect MIDI Data
	pos = position_of_track[0] + 9 + name_of_track[0].length();
	bool hit_end = false;
	unsigned int cur_pos = 0;

	unsigned int bpm_count = 0, ts_count = 0;
	double bpm_average = 0;

	double bpm = 0;
	while (hit_end == false) {
		if (readbyte(ibyte, pos) == 0xFF) {
			switch (readbyte(ibyte, pos))
			{
				case 0x58: //Time Signature
					pos += 1;
					track_string[1] += "	" + to_string(cur_pos) + " = TS " + to_string(readbyte(ibyte, pos)) + "\n";
					pos += 3;
					cur_pos += VLQ_to_Int(ibyte, pos);
					ts_count += 1;
					break;
				case 0x51: //BPM Setup
					pos += 1;
					bpm = 60000000000 / ((readbyte(ibyte, pos) * 65536) + (readbyte(ibyte, pos) * 256) + readbyte(ibyte, pos));
					track_string[1] += "	" + to_string(cur_pos) + " = B " + to_string(bpm) + "\n";
					bpm_average += bpm;
					cur_pos += VLQ_to_Int(ibyte, pos);
					bpm_count += 1;
					break;
				case 0x2F: //End.
					hit_end = true;
					break;
			}
		}
	}
	track_string[1] += "}\n";

	bpm_average /= bpm_count;

	//Enlighten our Console on our wonderful discovery of statistics no one actually cares about.
	cout << endl;
	cout << "Time Signatures: " << ts_count << endl;
	cout << "BPM Changes:     " << bpm_count << " (" << bpm_average / 1000 << " BPM average)" << endl;

	//Next up is notes. We have configured the note data above as constants. So we are just going to speed by this.
	for (int a = 0; a < num_of_ins; a++) {
		if (ins_exists[a]) {
			pos = ins_pos[a];

		}
	}

	//We're done. Close the file.
	midi.close();
	getchar();
	return 0;
}
