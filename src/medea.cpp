//============================================================================
// Name        : medusa.cpp
// Author      : Jason Pitt
// Version     : 0.1
// Copyright   : All rights reserved
// Description : controls a medusa flow controller controller
//============================================================================

//#pragma once

#define PORT "/dev/ttyACM0" //This is system-specific"/tmp/interceptty"

#define STATICTYPE 0
#define RAMPTYPE 1
#define PROGRAM 0
#define CONTROL 1

#define MAXPORTS 12

//segment types
#define STEP 10
#define RAMP 11
#define GOTO 12
#define END 13
#define FEEDBACK 14
#define ALLON 15
#define ALLOFF 16
#define DELAY 17

#define ON 1
#define OFF 0

#define RELAUNCH_INTERVAL 3600 //senconds in a hour



#include <iostream>
#include <sys/time.h>
#include <unistd.h>
#include <sstream>
#include <stdlib.h>
#include <inttypes.h>
#include <vector>
#include <SerialStream.h>
#include <fcntl.h>
#include <linux/kd.h>
#include <sys/ioctl.h>

#include <fstream>
#include <iomanip>
#include <unistd.h>
#include <cstdio>




using namespace std;
using namespace LibSerial;

bool recordcams;


int roundFloat(double dub){
	int bulk;
	double frac;
	bulk = dub;
	frac = dub - (double)bulk;
	if (frac >= 0.5) return bulk+1; else return bulk;
}//end roundfloat

class FlowController {
public:
	short flowrate;  //output voltage to controll flow controller 0-255
	int maxflowrate;	// maximum flow rate of the controller
	double deltaflow; //change in flow rate
	int ppm; //ppm concentration of gas
	int channel; //arduino pin the flow controller is on
	string gas;
	bool active; //if flowcontroller is used in the experiment
	bool carrier;

	FlowController(void){
		carrier=false;
		flowrate=0;
		maxflowrate=0;
		deltaflow=0;
		channel=0;
		gas="NULL";
		active=false;
		carrier=false;
		ppm=0;

	}//end constructor

	int StopFlow(void){

	}

	int MaxFlow(void){

	}

	int SetFlow(short){

	}

	unsigned char getVolts(double value){
		double answer = (value/((double)maxflowrate)*255.0);
		//cout << "answer: " << answer << endl;
			int bulk;
			double frac;
			bulk = answer;
			//cout << "bulk: " << bulk << endl;
			frac = answer - (double)bulk;
			//cout << "frac: " << frac << endl;
			if (frac >= 0.5) return (unsigned char)bulk+1; else return (unsigned char)bulk;


	}//end getVolts

	string printMe(void){
		stringstream ss;
		char act='X';
		if (active)act='A';
		ss << channel + 1 << "\t" << maxflowrate << "\t" << gas << "\t" << ppm << "\t" << act << "\n";
		return ss.str();

	}//end printMe



}; //end class flow controller



class Timer {
public:
	long systemstarttime,seconds,delay;
	double msdelay;
	struct timeval start;
	bool ms;


	Timer(long sseconds){

		gettimeofday(&start, NULL);
		delay = sseconds;
		ms=0;
		msdelay=0;
	}//end consntructor

	Timer(void){
		gettimeofday(&start, NULL);
		delay = 0;
		ms=0;
		msdelay=0;

	}

	Timer(double msec,bool mms){
		gettimeofday(&start,NULL);
		msdelay=msec;
		ms=1;


	}

	void startTimer(long time){

		gettimeofday(&start, NULL);
		if (ms){
			msdelay=time;
		}else{
			delay=time;
		}
		//cout << "stmstimer" << delay << "." << msdelay << "\n";
	}//end start timer
	void startTimer(double time){

			gettimeofday(&start, NULL);
			if (ms){
				msdelay=time;
			}else{
				delay=time;
			}
			//cout << "stmstimer" << msdelay << "\n";
		}//end start timer
	double getTimeElapsed(void){
		struct timeval currtime;
		gettimeofday(&currtime, NULL);
		stringstream ss;
		ss << currtime.tv_sec;

		string seconds(ss.str());
		//cout << "currsec" << seconds << "\t";
		ss.str("");
		ss.clear();
		ss << (double)currtime.tv_usec/(double)1000000;
		string micros(ss.str());

		micros.erase(0,1);
		//cout << "currmicros" << micros << "\t";
		seconds.append(micros);
		//cout << "appended" << seconds << "\t";
		double curr = atof(seconds.c_str());
		//cout << "currtime" << curr << "\t";
		ss.str("");
		ss.clear();

			ss << start.tv_sec;

				string sseconds(ss.str());
				ss.str("");
				ss.clear();
				ss << (double)start.tv_usec/(double)1000000;
				string smicros(ss.str());
				smicros.erase(0,1);
				sseconds.append(smicros);
				double scurr = atof(sseconds.c_str());
				//cout << "starttime" << scurr << "\n";
		return curr-scurr;

	}

	int getSeconds(void){
		struct timeval currtime;
				gettimeofday(&currtime,NULL);

		return (int)(currtime.tv_sec-start.tv_sec);
	}

	void printTimer(void){
		struct timeval currtime;
		gettimeofday(&currtime,NULL);
		cout << (currtime.tv_sec-start.tv_sec);
	}

	bool checkTimer(void){
		struct timeval currtime;
		gettimeofday(&currtime, NULL);
		if(ms){
			//cout << getTimeElapsed() << "\n";
			if (getTimeElapsed() >= msdelay)return true; else return false;
			//cout << "check" << (currtime.tv_sec-start.tv_sec) << "." << (currtime.tv_usec-start.tv_usec) << "\n";
			//if ((currtime.tv_sec-start.tv_sec) >= delay && (currtime.tv_usec-start.tv_usec) >= msdelay) return true; else return false;
		}else{
			if (currtime.tv_sec-start.tv_sec >= delay) return true; else return false;
		}
	}//end checkTimer

};

Timer displayTimer(1);

class Oscillator {
	//square wave oscillator type
public:
	long ontime; //nitrogen segment length in seconds
	long offtime; //oxygen segment length in seconds
	long delaycycles;  //number of cycles before cycling is engadged
	long dutycycles;  //number of cycles before cycling is stopped
	bool offpolarity;  //polarity of valve when cycling is stopped
	int status; //if on 1 or off 0
	int port;  //which port the oscillator is coupled to
	long cycles;  //number of times the oscillator has cycled
	bool locked;  //if oscillator is locked open or closed

	Timer timer;

	Oscillator(int setport,int seton, int setoff){
		cycles=0;  //reset cycles to zero
		status=0;  //set nitrogen valve closed
		ontime=seton;
		offtime=setoff;
		port=setport;
		delaycycles=0; //no delay
		dutycycles=-1;  //negative equals infinity
		offpolarity=0;  //default off
	}//end constructor

	Oscillator(int setport,int seton, int setoff, long setdelaycycles, long setdutycycles, bool setoffpolarity){
			cycles=0;  //reset cycles to zero
			status=0;  //set nitrogen valve closed
			ontime=seton;
			offtime=setoff;
			port=setport;
			delaycycles=setdelaycycles;
			dutycycles=setdutycycles;
			offpolarity=setoffpolarity;

		}//end constructor

	Oscillator(int setport){
			cycles=0;  //reset cycles to zero
			status=0;  //set nitrogen valve closed
			ontime=-1;
			offtime=-1;
			port=setport;
			delaycycles=0; //no delay
					dutycycles=-1;  //negative equals infinity
					offpolarity=0;  //default off
		}//end constructor

	void go(void){
		if (!locked){
			status=offpolarity;
			timer.startTimer(offtime);
			//cout << " timer"<< port << "started ";
		}else{

		}//else if locked
	}

	void printStatus(void){
		cout << "Oscillator=" << port + 1 << "\tStatus=";
		if (status) cout << "BACK"; else cout << "FRONT";
		cout << "\tCycles=";

		if (locked){
			cout << "LOCKED";
		}else{
			cout << cycles;
			cout << "\tNext flip in=";
			if (status==OFF){
				cout << offtime - timer.getSeconds();
			}else {
				cout << ontime - timer.getSeconds();
			}//end status ==on
			cout << "s  ";
		}//end if not locked
		cout << "\n";
	}//end printStatus

	int scan(void){
		if (displayTimer.checkTimer())printStatus();
		if (locked){
			//cout << "lockedosc.scan.port= " << port +1 << ".status" << status << "\t";
			return 0;
		}
		//cout << "unlockedosc.scan.port= " << port +1 << ".status" << status << "\t";

		if (timer.checkTimer()){
			cycles++;
			if (dutycycles > 0 && cycles > dutycycles+delaycycles){
				locked=1;

				if (offpolarity==OFF){
					status=OFF;

				}else if (offpolarity==ON){
					status=ON;

				}
				return 1;
			}//end if finished dutycycles
			if (delaycycles > 0 && cycles < delaycycles){
				if (status==OFF){
					timer.startTimer(ontime);
				}else if (status==ON){
					timer.startTimer(offtime);
				}
				if (offpolarity==OFF){
									status=OFF;

								}else if (offpolarity==ON){
									status=ON;

								}
							return 1;
						}//end if finished delaycycles
			if (status==OFF){
				status=ON;
				timer.startTimer(ontime);
				//cout << "FLIPPED " << port +1 << "status" << status << "\n";
				return 1;
			}//end if off
			else {
				status=OFF;
				timer.startTimer(offtime);
				//cout << "FLIPPED " << port +1 << "status" << status << "\n";
				return 1;
			}//end if on

		}//end if cycle done
		return 0;
	}//end scan()

	char getStatus(void){
		if (status==OFF) return 1; else return 100;
	}//end getStatus

	long getSeconds(string line){
		long segmentlength=0;
		stringstream inner(line);
					string chunk;


							getline(inner,chunk,':');
							int hours = atoi(chunk.c_str());
							getline(inner,chunk,':');
							int minutes = atoi(chunk.c_str());
							getline(inner,chunk);
							int seconds = atoi(chunk.c_str());
							segmentlength=(hours * 3600) + (minutes * 60) + seconds;
							//cout << "segment time" << segmentlength << "\t" << hours << ":" << minutes<< ":" << seconds << "\n\n";
							return segmentlength;
	}//end get seconds

	string printMe(void){
			stringstream ss;

			if (!locked){
				ss << port + 1 << "\t Front time " << offtime << " seconds \t Back Time " << ontime << " seconds\n";
			}else {
				ss << port + 1 << "\t LOCKED ";
				if (status==1) ss << "BACK\n"; else ss << "FRONT\n";
			}//else if locked
			return ss.str();

		}//end printMe

};

class ControlSegment {
public:

	Timer timer;
	int type;
	int complete;
	int gotonum;
	int gotocount;
	vector<FlowController> *controllers;
	unsigned char volts[MAXPORTS];
	double flows[MAXPORTS];
	bool sent;

	long segmentlength;



	long length; //length of time segment in seconds

	ControlSegment(vector<FlowController> *controller){
		complete=0;
		controllers=controller;
		sent=false;

	}//end constructor

	void writeFile(string path, int mynumber){
		stringstream ss;
		string filename;
		ss << path << "/" << mynumber+1 << ".txt";
		filename = ss.str();
		ofstream ofile(filename.c_str());

		ofile << "segment length:" << segmentlength << "\n\n";
		for (int i=0; i < MAXPORTS; i++){
			ofile << "Cntrl#\t\tMaxFlw\t\tGas\t\tPPM\t\tFlow\n";
			ofile << i+1 <<"\t\t" << controllers->at(i).maxflowrate << "\t\t"  << controllers->at(i).gas << "\t\t" <<controllers->at(i).ppm << "\t\t" << flows[i]  << "\n";

		}//end print shit
		ofile.close();

	}//end writeFile

	void setEnd(void){
		for (vector<FlowController>::iterator citer = controllers->begin(); citer != controllers->end(); citer++){
			(*citer).flowrate=0;
		}//end
		timer.startTimer((long)0);
	}//end set end

	bool notSent(void){
			if (sent) return false; else return true;
		}

	void setGOTO(string segline){
		stringstream ss(segline);
		string line;
		int j=0;
		while(getline(ss,line,' ')){
			if (j >0){ //strip goto
				switch (j){
				case 1:
					gotonum=atoi(line.c_str())-1;
					break;
				case 2:
					stringstream sss(line);
					string outp;
					getline(sss,outp,'X');
					gotocount=atoi(outp.c_str());
					break;

				}//end j switch
			}
			j++;
		}//end while commands

	}//end setgoto

	void setStep(string segline,int totalflow){
		stringstream ss(segline);
		string line;
		double targets[MAXPORTS];
		for (int i=0; i < MAXPORTS; i++) {targets[i]=0; flows[i]=0;}

		int j=0;
		while(getline(ss,line,' ')){//end parse commands
			stringstream inner(line);
			string chunk;
			if (j>0){ //strip off command string
				getline(inner,chunk,':');
				if (chunk.find("T") != string::npos){
					getline(inner,chunk,':');
					int hours = atoi(chunk.c_str());
					getline(inner,chunk,':');
					int minutes = atoi(chunk.c_str());
					getline(inner,chunk);
					int seconds = atoi(chunk.c_str());
					segmentlength=(hours * 3600) + (minutes * 60) + seconds;
					cout << "segment time" << segmentlength << "\t" << hours << ":" << minutes<< ":" << seconds << "\n\n";

				}else{//end if time segment
					int commandflow= atoi(chunk.c_str());
					getline(inner,chunk,':');
					int targetconc= atoi(chunk.c_str());
					getline(inner,chunk);
					targets[commandflow-1]=targetconc;

					//write stuff to segment

				}

			}
			j++;
		}//end while

		int carriernum=0;

		for (vector<FlowController>::iterator citer = controllers->begin(); citer != controllers->end(); citer++){
					if ((*citer).carrier) break; else carriernum++;
			}//end for each gas line
		targets[carriernum]=totalflow;

		for (int i=0; i < MAXPORTS; i++) {
			if (!controllers->at(i).active || controllers->at(i).carrier) continue;

			cout << "\n segment " << i << " concentration " << targets[i] << "\t";
			cout << "source conc:" << controllers->at(i).ppm << "\n";
			double ratio=((double)targets[i])/((double)controllers->at(i).ppm);

		cout << "ratio:" << ratio << "\t";
			double flow = ratio * (double)totalflow ;
		cout << "flow:" << flow << "\n";
			targets[carriernum]-=flow;
			flows[i]=flow;
		}
		cout << "\n carrier segment " << carriernum << " concentration " << targets[carriernum] << "\t";
		cout << "source conc:" << controllers->at(carriernum).ppm << "\n";

		cout << "flow:" << targets[carriernum] << "\n";
					flows[carriernum]=targets[carriernum];
		int i=0;
		for (vector<FlowController>::iterator citer = controllers->begin(); citer != controllers->end(); citer++){
			//if ((*citer).carrier)
			volts[i]=(*citer).getVolts(flows[i]);
			cout << "volts " << i << "=" << (int)volts[i] << "\n";


			      i++;
		}//end
		//for (int i=0; i < MAXPORTS; i++) cout << "volts " << i << ":" << volts[i] << ":\n";

	}//setStep

	int getGOTO(int segid){
		gotocount--;
		if (gotocount >= 0)
		return gotonum; else return segid+1;
	}

	void getOutput(unsigned char *command){
		switch (type){
		case STEP:
			for (int i=0; i < MAXPORTS; i++){
				if (volts[i] < 1) volts[i]=1; //avoid null bits
				command[5+(i*2)]=volts[i];
			}//end for each port
			startSegment();
			break;
		case END:
			for (int i=0; i < MAXPORTS; i++){
				volts[i]=1;
				command[5+(i*2)]=volts[i];
			}

			break;
			}//end switch
	}//end getOutput

	void startSegment(void){
		timer.startTimer(segmentlength);
		sent=true;
	}//end start the segment

	bool isDone(void){
		return timer.checkTimer();
	}//end isDone

};//end class control segment



class Experiment {
public:
	vector<ControlSegment> segments;
	vector<FlowController> controllers;
	vector<Oscillator> oscillators;
	int totalflow;
	int carriergas;
	int segnumber;
	string workingdir;
	int filenumber;
	bool savevolts;
	double saverate;
	int first;

	Experiment(void){
		totalflow=0;
		carriergas=0;
		segnumber=0;
		filenumber=0;
		savevolts=0;
		saverate=0;
		first=1;

	}//end constructor

	void getOscCommand(unsigned char *command){
	int i=0;
		for(vector<Oscillator>::iterator citer = oscillators.begin(); citer != oscillators.end(); citer++){
			command[5+(i*2)]=(unsigned char)(*citer).getStatus();
			i++;
		}//end for each oscillator


	}//end getOutput

	void startOscillators(void){
		int c=1;
		for(vector<Oscillator>::iterator citer = oscillators.begin(); citer != oscillators.end(); citer++){
			//Timer startstagger(3);
			//while (!startstagger.checkTimer());
			(*citer).go();
			writeOscillatorFile(c,(*citer).cycles,(*citer).status);
			cout << "Launch oscillator " << c++ <<"\n";
				}//end for each oscillator
	}//end start all oscillators

	void writeOscillatorFile(int dirnum,long fnum, int output){
				stringstream ss;
				string filename;
				ss << workingdir << "/cam" << dirnum<< "/" << fnum << ".txt";
				filename = ss.str();
				ofstream ofile(filename.c_str());

				ofile << "status=";
				if (!output) ofile << "FRONT\n"; else ofile << "BACK\n";

				ofile.close();
	}//end writeOscillatorFile


	int scanOscillators(void){
		//cout << "scan oscs";
			int val=0;
			int c=1;
			int display=0;

			if (displayTimer.checkTimer()){
				system("clear");
				cout << "\nOscillator Status:\n";
				display=1;
			}
		for(vector<Oscillator>::iterator citer = oscillators.begin(); citer != oscillators.end(); citer++){
			int rez = (*citer).scan();
			if (rez) {
				val= 1;
				writeOscillatorFile(c,(*citer).cycles,(*citer).status);
			}
			c++;
			//cout << c++ << ":" << rez << "\t";
		}//end for each oscillator
			//cout << endl;
		if (displayTimer.checkTimer() && display) displayTimer.startTimer((long)1);
		return val;
	}//end scan oscillators for update commands

	void writeFile(void){
		segments.at(segnumber).writeFile(workingdir,filenumber);
	}
	void writeVolts(int voltage){
		cout << "volts "<<voltage << "\r";
		string fullname(workingdir);
		fullname+= "/";
		fullname+= "volts.txt";
		ofstream ofile(fullname.c_str(),fstream::app);
		ofile << voltage << " \n";
		ofile.close();
	}
	void addSegment(string segline){
		ControlSegment seg(&controllers);
	//	cout << "segline is:" << segline << endl;;
		if (segline.find("GAS")!= string::npos || segline.find("gas")!=string::npos){
		//	cout << "found a gas" <<  endl;
			if (segline.find("r") != string::npos || segline.find("R") != string::npos){
				seg.type=RAMP;
			//	cout << "found a ramp" <<  endl;
			} else { //end if ramp
				seg.type=STEP;
				//cout << "found a step" <<  endl;

				seg.setStep(segline,totalflow);




			}//end if step

		}else //end if gas segment
			if (segline.find("END") != string::npos || segline.find("end") != string::npos){
				seg.type=END;
				seg.setEnd();
			//	cout << "found an end" <<  endl;


			}//end if END
			else if (segline.find("GOTO") != string::npos || segline.find("goto") != string::npos){
				seg.type=GOTO;
				seg.setGOTO(segline);
			}//end if goto


		segments.push_back(seg);

	}//end addsegment

	void setCarrier(void){
		controllers.at(carriergas).carrier=true;
	}

};//end class expeiment

//globals
//string read;
Timer mytime(0);//sec timer
Timer  volttime(0, 1);//ms timer

Experiment exp;
SerialStream ardu;
int toggle=0;
int ready=0;
int awk=0;
long c=0;
int progstate=PROGRAM;

unsigned char offcommand[] = {7,7,7,7,1,64,2,1,3,1,4,1,5,1,6,1,8,8,8,8}; //20 byte command string 0.1p
unsigned char oncommand[] =  {7,7,7,7,1,1,2,61,3,60,4,1,5,1,6,1,8,8,8,8}; //20 byte command string all on
unsigned char basecommand[]={7,7,7,7,1,1,2,1,3,1,4,1,5,1,6,1,7,1,8,1,9,1,10,1,11,1,12,1,8,8,8,8}; //32 byte
//unsigned char valveoffcommand[]={7,7,7,7,1,1,2,1,3,1,4,1,5,1,6,1,7,1,8,1,9,1,10,1,11,1,12,1,8,8,8,8}; //32 byte
//unsigned char valveoncommand[]={7,7,7,7,1,100,2,100,3,100,4,100,5,100,6,100,7,100,8,100,9,100,10,100,11,100,12,100,8,8,8,8}; //32 byte

Oscillator getOscillator(int channel, string line){
	Oscillator osc(channel);
	int j=0;
		stringstream ss(line);
		//ss << "\n";
		string comm;

		osc.locked=false;
		osc.port=channel;

		while (getline(ss,comm,'\t')){

			switch(j){

			case 0://locked or offtime fronttime
				if (comm.find("FRONT") != string::npos || comm.find("front") != string::npos){
					osc.locked=true;
					osc.status=OFF;
					return osc;

				}else if (comm.find("BACK") != string::npos || comm.find("back") != string::npos){
					osc.locked=true;
					osc.status=ON;
					return osc;

				}else

				{

					osc.offtime=osc.getSeconds(comm);
				}//end if good
				break;
			case 1://valve on time
				osc.ontime=osc.getSeconds(comm);
				break;

			case 2: //delay cycles
				osc.delaycycles=atoi(comm.c_str());
				break;

			case 3: //duty cycles
				osc.dutycycles=atoi(comm.c_str());
				break;

			case 4: //off Polarity
				if (comm.find("OFF") != string::npos) osc.offpolarity=OFF;
				else if (comm.find("ON") != string::npos) osc.offpolarity=ON;
				break;

			}//end switch
			 j++;
		}//while commands

		return osc;


}//end getOscil

void doProgram(void){

	ofstream ofile ("/tmp/temp.medusa");



	//cout << "MEDUSA:\n";
	cout << "Load file or Define new program file (L/D):";
	string loadfile;
	getline(cin,loadfile);

	if ((loadfile.find("L") != string::npos) || (loadfile.find("l") != string::npos)){
				cout << "Medusa file name:";
				string medfile;
				getline(cin,medfile);
				ifstream ifile(medfile.c_str());


				for (int i=0; i < MAXPORTS; i++){
					string command;

					cout << i+1 <<":";

					getline(ifile,command);
					//ofile << command << endl;
					//cout << command;
					exp.oscillators.push_back(getOscillator(i,command));

				}//end for each available port

				cout << "\n\nOscillator List\n\nCtrl#\tFront\tBack\n";
						for (vector<Oscillator>::iterator citer = exp.oscillators.begin(); citer != exp.oscillators.end(); citer++){
							cout << (*citer).printMe();
						}//end through each controller
						string command;
						cout << "\nRecord movies(y/n)? " ;
								getline(ifile,command);
								//ofile << command << endl;
								if (command.find("Y") != string::npos || command.find("y") != string::npos) recordcams=1;




				cout << "Set working directory:";
				getline(cin,exp.workingdir);

				stringstream sss;
						sss << "mkdir " << exp.workingdir;
						string mains = sss.str();
						system (mains.c_str());

				for (int i=1; i < MAXPORTS+1; i++){
							stringstream ss;
							ss << "mkdir " << exp.workingdir << "/cam" << i << "\n";
							string co = ss.str();
							system(co.c_str());
						}

				cout << "Press S to execute the program\n";
				string execute;
				cin >> execute;
				if (execute.find("S") != string::npos || execute.find("s") != string::npos){
					//ofile.close();
					progstate=CONTROL;
					exp.segnumber=0;
				}//end if execute
	}else {//end if loadfile else if define new

		cout << "\nDefine Oscillators:\n\nFormat=Front Gas Time in h:m:s <tab> Back Gas Time in h:m:s \n\nexamples:\n1:0:1:30\t0:1:30\n2:1000\tN2\tstraight\n3:FRONT\t if oscillator is not used front gas one\n4:BACK if oscillator is not used back gas on\n\n";

		for (int i=0; i < MAXPORTS; i++){
			string command;

			cout << i+1 <<":";

			getline(cin,command);
			ofile << command << endl;
			//cout << command;
			exp.oscillators.push_back(getOscillator(i,command));

		}//end for each available port
		cout << "\n\nOscillator List\n\nCtrl#\tFront\tBack\n";
		for (vector<Oscillator>::iterator citer = exp.oscillators.begin(); citer != exp.oscillators.end(); citer++){
			cout << (*citer).printMe();
		}//end through each controller
		string command;
		cout << "\nRecord movies(y/n)? " ;
		getline(cin,command);
		ofile << command << endl;
		if (command.find("Y") != string::npos || command.find("y") != string::npos) recordcams=1;


		cout << "Save program to file name:";
		string filen;
		getline(cin,filen);
		ofile.close();
		ofstream sfile(filen.c_str());
		ifstream rfile("/tmp/temp.medusa");
		string line;
		while (getline(rfile,line)){
			sfile << line << endl;
		}//end while lines to copy
		rfile.close();
		sfile.close();

		cout << "Set working directory:";
		getline(cin,exp.workingdir);

		stringstream sss;
		sss << "mkdir " << exp.workingdir;
		string mains = sss.str();
		system (mains.c_str());

		for (int i=1; i < MAXPORTS+1; i++){
			stringstream ss;
			ss << "mkdir " << exp.workingdir << "/cam" << i << "\n";
			string co = ss.str();
			system(co.c_str());
		}



		cout << "Press S to execute the program\n";
		string execute;
		cin >> execute;
		if (execute.find("S") != string::npos || execute.find("s") != string::npos){
			//ofile.close();
			progstate=CONTROL;
			exp.segnumber=0;
		}//end if execute


	}//end if define new programfile

}//end doprogram




void relaunch_cameras(int numcameras, string workingdir, int currday, int duration){
	if (!recordcams) return;
	for (int i=0; i <numcameras; i++){
			stringstream ss;
			string command;
			ss << "mkdir " << workingdir <<  "/cam" << i+1 << "/hour" << currday << " &";

							command = ss.str();
							cout << command << endl;
							system(command.c_str());

		}//end for each camera
	for (int i=0; i <numcameras; i++){
		stringstream ss;
		string command;
		ss << "streamer -t 0:59:0 -c /dev/camera" << i+1 << " -s 640x480 -r 0.4167 -o " << workingdir << "/cam" << i+1 << "/hour" << currday << "/hour" << currday << "_0000000.pgm  &";

						command = ss.str();
						cout << command << endl;
						system(command.c_str());

	}//end for each camera




}//end_relaunch cameras

int main() {
	  struct timeval start, end;

	  long stime, seconds;

	  int oldvolt=0;
	  Timer movietimer; ///timer to relaunch streamer processes
	  int currday=0;  //day counter
	  recordcams=0;





	cout << "beep\a\n\n\n";
	cout << "sssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss\n";
	cout << "SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS\n";
	cout << "|  MEDEA  Gas VALVE Controller  Jason Pitt all rights reserved |\n";
	cout << "SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS\n";
	cout << "sssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss\n";
	cout << "\n\n\n\a\a\a\a";



	cout << "Connecting to Medea Controller at: " << PORT << "    port can be changed in medusa.prefs\n";

	ardu.Open(PORT);
    /*The arduino must be setup to use the same baud rate*/
    ardu.SetBaudRate(SerialStreamBuf::BAUD_9600);
    ardu.SetCharSize(SerialStreamBuf::CHAR_SIZE_8);
    //ardu.SetFlowControl(SerialStreamBuf::FLOW_CONTROL_NONE);
   // ardu.SetParity(SerialStreamBuf::PARITY_NONE);
    cout << "set baud rate 9600 and char size 8 bits\n";
  //  sleep(2);



cout << endl;


    while (1){

    	switch(progstate){
    	case PROGRAM:
    		doProgram();
    		break;
    	case CONTROL:
    		string read;

				//	cout << " main loop ready=" << ready << "\n";
    		if (movietimer.checkTimer()){

    			relaunch_cameras(12,exp.workingdir,currday,RELAUNCH_INTERVAL-600);

    			movietimer.startTimer((long)RELAUNCH_INTERVAL);
    			currday++;
    		}//end if timer up

    		    	if (!ready){
    		    		if (exp.first){

    		    			exp.startOscillators();


    		    			exp.getOscCommand(basecommand);

							ardu.write((const char*)basecommand,32);
							//exp.writeFile();

							cout << "sent command:";
							for (int i=0; i < 32; i++) cout << (int)basecommand[i] << ",";
							    		    			cout << endl;

							ready=true;
							cout << "Launch sent to Medea \n";
							exp.first=0;


    		    		}else{
    		    			//cout << " not ready wait ";
							//ardu >> read;
							getline(ardu,read);
							//cout << "wait";


							//cout << read << endl;
							if (read.find("startup") != string::npos || read.find("ready") != string::npos || read.find("akak") != string::npos)	ready=true;
							c=0;
    		    		}
    		    	}
    		    	if (ready){
    		    	//	cout << "ready again ";
    		    		if (exp.scanOscillators()){ //end if command finished launch the next
    		    		//	cout << "oscscan go";
    		    		//	exp.segnumber++;
    		    			//exp.filenumber++;

    		    			while (exp.scanOscillators()) cout << "scan wait\n"; //wait for oscillators to lock


							exp.getOscCommand(basecommand);

    		    			ardu.write((const char*)basecommand,32);



    		    			//ready=false;  //2013
    		    			//exp.writeFile();
    		    			//cout << "Segment " << exp.segnumber+1 << " sent to Medusa \n";
    		    			cout << "sent command:";
    		    			for (int i=0; i < 32; i++) cout << (int)basecommand[i] << ",";
    		    			cout << endl;
    		    		}//end



    		    	}//end read equal startup ready
    		    	//if (exp.segments.at(exp.segnumber).type==END)progstate=PROGRAM;
    		    	//cout << "empty";T
    		    	//volttime.printTimer();
    		    	//cout << "bottom read \n";
    		    //ouch	getline(ardu,read);
    		    	if (read.find("startup") != string::npos || read.find("ready") != string::npos || read.find("akak") != string::npos)	ready=true;

    		    	    		    								stringstream ss(read);
    		    	    		    							//	cout << read << "\n";



    		break;
    	}//end switch progstate


    }//end while

}//end main
