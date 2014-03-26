/*
	MID2CHART C++ REWRITE
	Because writing this stuff in console + DLL form is better than GUI :)

	The purpose of this console application is strictly to convert Rock Band 3 MIDIs to Frets on Fire/Keyboard Hero's *.chart format.
	Since all other converters don't work (And people do not like using Feedback Chart Editor)...
	I decided to make an app that converts to chart retaining the Keyboard Note Data. Putting it in [ExpertKeyboard], etc.

	Update (2014年3月26日):
		-Added Note conversions
		-Added Event conversions
		-The stuff actually converts like I wanted it to.
		-File writing implemented.

	By: Clara Eleanor Taylor

	Possible Usages:
		mid2chart.exe <input file> <output file>
		mid2chart.exe <input file>

	If <output file> is voided, the output the application spits out will be the <input file> with ".chart" slapped on.
	e.g. "mid2chart.exe song.mid" will create "song.mid.chart".
*/

#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>

using namespace std;

//These are GLOBAL application settings. Change them if you want to add in extra difficulties, instruments, etc.
const int num_of_ins = 4;
const int num_of_difficulties = 4;

//Define all of the names here.
const string difficulties[num_of_difficulties] = { "Expert", "Hard", "Medium", "Easy" };
const string instruments [num_of_ins         ] = { "Single", "DoubleBass", "Drums", "Keys" };
const string corris_inst [num_of_ins         ] = { "PART GUITAR", "PART BASS", "PART DRUMS", "PART KEYS" };

//This is for identifying the file later on. If you tack on another instrument, add {19, 20, 21, 22}, etc.
//If you add another difficulty, increase each array bracket by 1. e.g., {3, 4, 5, 6, 7}, etc.
const int inst_ind[num_of_ins][num_of_difficulties] = { { 3, 4, 5, 6 },
                                                        { 7, 8, 9, 10 },
                                                        { 11, 12, 13, 14 },
                                                        { 15, 16, 17, 18 } };
//Still confused? Here is a further example. { {ExpertSingle, HardSingle}, {ExpertDoubleBass, HardDoubleBass} };
//Left-to-Right = Difficulty
//Up-to-Down = Instrument


//Tell the application what values are actually notes...
const unsigned char note_hex[num_of_difficulties][5] = { { 0x60, 0x61, 0x62, 0x63, 0x64 },
                                                         { 0x54, 0x55, 0x56, 0x57, 0x58 },
                                                         { 0x48, 0x49, 0x4A, 0x4B, 0x4C },
                                                         { 0x3C, 0x3D, 0x3E, 0x3F, 0x40 } };
//ORDER
//Expert: Green, Red, Yellow, Blue, Orange
//Hard: Green, Red, Yellow, Blue, Orange
//etc.

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
	} while ((byte[pos - 1] & (1 << 7)) != 0);

	//Combine the bits in the most inefficient way possible.
	for (unsigned int i = 0; i < bits.length(); i++) {
		total += pow(2, bits.length() - i - 1) * (bits[i] != '0');
	}

	//Save the day.
	return total;
}

int main(int argc, char* argv[]) {
	ifstream midi;

	//Show the console our pretty little art.
	cout << "/---------------------------------------------------------\\" << endl
		<< "|*                                                       *|" << endl
		<< "|                  < Clara's Mid2Chart >                  |" << endl
		<< "|                     Version 1.0.0.0                     |" << endl
		<< "|                                                         |" << endl
		<< "|    For people who know what the hell they are doing.    |" << endl
		<< "|*                                                       *|" << endl
		<< "\\---------------------------------------------------------/\n\n";

	if (argc < 2) {
		cerr << "Usage: mid2chart.exe <input path> <output path>"
			<< endl
			<< endl
			<< "    <output path> is purely optional."
			<< endl
			<< "    If blank, uses input name + \".chart\" (not affecting the MIDI!)"
			<< endl;
		return 1;
	}

	string path, opath;

	if (argc == 2) {
		path = argv[1];
		opath = string(argv[1]) + ".chart";
	}

	if (argc == 3) {
		path = argv[1];
		opath = argv[2];
	}

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
	string* track_title = new string[track_number];

	cout << "Track Names:     ";

	bool ins_exists[num_of_ins];
	bool events_exist = false;
	unsigned int ins_pos[num_of_ins], events_pos;

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
					ins_pos[a] = position_of_track[i] + 12 + name.length();
				}
			}

			if (name_of_track[i] == "EVENTS") {
				events_exist = true;
				events_pos = position_of_track[i] + 12 + name.length();
			}
		}
		else {
			//Phuck
			name_of_track[i] = "";
		}

		pos = position_of_track[i] + size_of_track[i] + 8;
	}

	//The first one is the chart data. This is very important for song information. [Song]
	track_title[0] = "Song";
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
	track_title[1] = "SyncTrack";
	track_string[1] = "[SyncTrack]\n";
	track_string[1] += "{\n";

	//Collect MIDI Data
	pos = position_of_track[0] + 9 + name_of_track[0].length();
	bool hit_end = false;
	unsigned int cur_pos = 0;

	unsigned int bpm_count = 0, ts_count = 0;
	double bpm_average = 0;

	int bpm = 0;
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
	cout << endl;

	//Next up is notes. We have configured the note data above as constants. So we are just going to speed by this.
	for (int ins = 0; ins < num_of_ins; ins++) {
		if (!ins_exists[ins])
			continue; //Just get rid of it. We do not want to waste bytes of space.

		//Carry on.
		unsigned int track_pos[num_of_difficulties];
		for (int _i = 0; _i < num_of_difficulties; _i++)
		{
			track_title[inst_ind[ins][_i]] = difficulties[_i] + instruments[ins];
			track_string[inst_ind[ins][_i]] = "[" + difficulties[_i] + instruments[ins] + "]\n";
			track_string[inst_ind[ins][_i]] += "{\n";
			track_pos[_i] = 0;
		}

		//This is for sustains. Notice how 5 is used here.
		//Every lane has 5 frets, therefore, 5 is fixed and hardcoded (TODO: Jagged Vector & Custom size for flexibility).
		int note_queue[num_of_difficulties][5];
		for (int i = 0; i < num_of_difficulties; i++) {
			for (int _i = 0; _i < 5; _i++) {
				note_queue[i][_i] = 0;
			}
		}

		pos = ins_pos[ins];
		bool hit_end = false;
		unsigned int tmp_move;
		cout << "Found " << setw(12) << left << corris_inst[ins];
		while (hit_end == false) {
			tmp_move = VLQ_to_Int(ibyte, pos);
			for (int _i = 0; _i < num_of_difficulties; _i++)
			{
				track_pos[_i] += tmp_move;
			}

			unsigned char skittles = readbyte(ibyte, pos);
			if (skittles == 0xFF) {
				string text_event = "";
				unsigned int count;

				switch (readbyte(ibyte, pos))
				{
				case 0x01: //This is a Text Event... get it out of here.
					count = readbyte(ibyte, pos);

					//Read the chars
					for (int i = 0; i < count; i++) {
						text_event += (char)readbyte(ibyte, pos);
					}

					//Write them to the strings
					for (unsigned int a = 0; a < num_of_difficulties; a++) {
						track_string[inst_ind[ins][a]] += "	" + to_string(track_pos[a]) + " = E " + text_event + "\n";
					}
					break;
				case 0x2F: //el fin... oh wait I hate that language (seriously).
					hit_end = true;
					break;
				}
			}
			else
			if (skittles >= 0x80 && skittles <= 0x9F) {
				unsigned char note = readbyte(ibyte, pos);

				//Alright, the C# version had a HUGE switch-and-case statement. We are going to simply that right now.
				for (int a = 0; a < num_of_difficulties; a++) {
					for (int b = 0; b < 5; b++) {
						if (note_hex[a][b] == note) {
							//YES. It matches. Now let's do some fun stuff.
							if (note_queue[a][b] == 0) {
								note_queue[a][b] = track_pos[a];
							}
							else {
								if (track_pos[a] - note_queue[a][b] > delta_time / 4) {
									//This allows for sustainable notes. Anything lower will have the sustain removed, period.
									track_string[inst_ind[ins][a]] += "    " + to_string(note_queue[a][b])
										+ " = N " + to_string(b)
										+ " " + to_string(track_pos[a] - note_queue[a][b])
										+ "\n";
								}
								else {
									//Snip that tiny (or non-existant) sustain off.
									track_string[inst_ind[ins][a]] += "    " + to_string(note_queue[a][b])
										+ " = N " + to_string(b)
										+ " 0"
										+ "\n";
								}
								note_queue[a][b] = 0;
							}

							//Kill the chain afterwards.
							break;
						}
					}
				}
				pos += 1;
			}
		}


		//Add in the final mark.
		for (int _i = 0; _i < 4; _i++)
		{
			track_string[inst_ind[ins][_i]] += "}\n";
		}

		cout << "...Converted!" << endl;
	}

	//There is one more track to write... and that is the events.
	if (events_exist) {
		pos = events_pos;
		cout << "Found " << setw(12) << left << "EVENTS";
		bool hit_end = false;
		unsigned int tmp_move, e_pos = 0;
		track_title[2] = "Events";
		track_string[2] = "[EVENTS]\n{\n";

		while (hit_end == false) {
			tmp_move = VLQ_to_Int(ibyte, pos);
			e_pos += tmp_move;

			unsigned char skittles = readbyte(ibyte, pos);
			if (skittles == 0xFF) {
				string text_event = "";
				unsigned int count;

				switch (readbyte(ibyte, pos))
				{
				case 0x01: //This is a Text Event... get it out of here.
					count = readbyte(ibyte, pos);

					//Read the chars
					for (int i = 0; i < count; i++) {
						char tmp_str = readbyte(ibyte, pos);
						if (tmp_str != '[' && tmp_str != ']') {
							text_event += tmp_str;
						}
					}

					//Write them to the strings
					track_string[2] += "	" + to_string(e_pos) + " = E \"" + text_event + "\"\n";
					break;
				case 0x2F: //le fin... French is better than Spanish.
					hit_end = true;
					break;
				}
			}
			else
			if (skittles >= 0x80 && skittles <= 0x9F) {
				switch (readbyte(ibyte, pos))
				{
					//lol
				case 24: break;
				case 26: break;
				}
				pos += 1;
			}

		}
		track_string[2] += "}\n";
		cout << "...Converted!" << endl;
	}

	cout << endl << "Writing to file: " << opath << endl;

	//Now we are going to write it to a file... THE EASY WAY.
	ofstream chart;
	chart.open(opath);
	for (unsigned int i = 0; i < track_number; i++) {
		chart << track_string[i];
		if (track_title[i] != "") {
			cout << "        Wrote: " << track_title[i] << endl;
		}
	}
	cout << endl;

	cout << "Write successful. Cleaning up." << endl;

	//We're done. Close the files.
	chart.close();
	midi.close();

	cout << "Press Enter to Exit.";
	getchar();

	//Return 0 to the Operating System. Apparently that is "good practice"... not like it matters.
	return 0;
}