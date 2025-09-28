/*
	Original author of the starter code
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 2/8/20
	
	Please include your Name, UIN, and the date below
	Name: Avi Chaturvedi
	UIN: 733007543
	Date: 9/28/2025
*/
#include "common.h"
#include "FIFORequestChannel.h"
#include <sys/wait.h>
#include <chrono>

using namespace std;


int main (int argc, char *argv[]) {
	int opt;
	int p = 1;
	double t = 0.0;
	int e = 1;
	bool n = false;
	
	string filename = "";
	while ((opt = getopt(argc, argv, "p:t:e:f:c")) != -1) {
		switch (opt) {
			case 'p':
				p = atoi (optarg);
				break;
			case 't':
				t = atof (optarg);
				break;
			case 'e':
				e = atoi (optarg);
				break;
			case 'f':
				filename = optarg;
				break;
			case 'c':
				n = true;
				break;
		}
	}

	pid_t pid = fork();

	if (pid < 0) {
		exit(1);
	} else if (pid == 0) {
		char* args[] = {(char*)"./server", nullptr};
		if (execvp(args[0], args) == -1) {
			exit(1);
		}
	}

    FIFORequestChannel control_chan("control", FIFORequestChannel::CLIENT_SIDE);
	FIFORequestChannel* chan = &control_chan;
	if (n) {
		MESSAGE_TYPE nc = NEWCHANNEL_MSG;
		control_chan.cwrite(&nc, sizeof(MESSAGE_TYPE));

		char name[30];
		control_chan.cread(name, sizeof(name));

		chan = new FIFORequestChannel(name, FIFORequestChannel::CLIENT_SIDE);
	}

	if (!filename.empty()) {
		cout << "Starting file transfer of " << filename << endl;
		auto start = chrono::high_resolution_clock::now();

		filemsg fmsg(0, 0);
		int len = sizeof(filemsg) + filename.size() + 1;

		char* buf = new char[len];
		memcpy(buf, &fmsg, sizeof(filemsg));
		strcpy(buf + sizeof(filemsg), filename.c_str());
		chan->cwrite(buf, len);

		int filesize;
		chan->cread(&filesize, sizeof(int));
		cout << "File size: " << filesize << " bytes" << endl;
		
		string outpath = "received/" + filename;
		ofstream outfile(outpath, ios::binary);
		if (!outfile.is_open()) {
			delete[] buf;
			exit(1);
		}
		
		int max = MAX_MESSAGE - sizeof(filemsg) - filename.size() - 1;
		int remaining = filesize;
		int offset = 0;

		cout << "Transfer starting" << endl;
		while (remaining > 0) {
			int chunk_size = min(max, remaining);

			filemsg req(offset, chunk_size);
			memcpy(buf, &req, sizeof(filemsg));
			strcpy(buf + sizeof(filemsg), filename.c_str());

			chan->cwrite(buf, len);

			char recvbuf[MAX_MESSAGE];
			chan->cread(recvbuf, chunk_size);
			
			outfile.write(recvbuf, chunk_size);

			offset += chunk_size;
			remaining -= chunk_size;
		}

		outfile.close();
		delete[] buf;

		auto end = chrono::high_resolution_clock::now();
		double seconds = chrono::duration<double>(end - start).count();
		cout << "Transfer time: " << seconds << " seconds" << endl;
	} else if (t == 0.0 && e == 1) {
		ofstream csv("received/x1.csv");
		if (!csv.is_open()) {
			exit(1);
		}

		for (int i = 0; i < 1000; i++) {
			double time = i * .004;

			char buf[MAX_MESSAGE];

			datamsg d1(p, time, 1);
			memcpy(buf, &d1, sizeof(datamsg));

			chan->cwrite(&d1, sizeof(datamsg));
			double ecg1;
			chan->cread(&ecg1, sizeof(double));

			datamsg d2(p, time, 2);
			memcpy(buf, &d2, sizeof(datamsg));

			chan->cwrite(&d2, sizeof(datamsg));
			double ecg2;
			chan->cread(&ecg2, sizeof(double));

			csv << std::defaultfloat << time << "," << ecg1 << "," << ecg2 << "\n";
		}
	} else {
		char buf[MAX_MESSAGE]; // 256
		datamsg x(p, t, e);
		
		memcpy(buf, &x, sizeof(datamsg));
		chan->cwrite(buf, sizeof(datamsg)); // question
		double reply;
		chan->cread(&reply, sizeof(double)); //answer
		cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
	}
	
	// closing the channel    
    MESSAGE_TYPE m = QUIT_MSG;
    chan->cwrite(&m, sizeof(MESSAGE_TYPE));

	if (n) {
		delete chan;
	}

	control_chan.cwrite(&m, sizeof(MESSAGE_TYPE));

	wait(nullptr);
}
