/******************************************************************************/
/*                                                                            */
/* ENTITY IMPLEMENTATIONS                                                     */
/*                                                                            */
/******************************************************************************/

// Student names: Isaac Roberts, Joshua You
// Student computing IDs: itr9fc, jyy3gx
//
//
// This file contains the actual code for the functions that will implement the
// reliable transport protocols enabling entity "A" to reliably send information
// to entity "B".
//
// This is where you should write your code, and you should submit a modified
// version of this file.
//
// Notes:
// - One way network delay averages five time units (longer if there are other
//   messages in the channel for GBN), but can be larger.
// - Packets can be corrupted (either the header or the data portion) or lost,
//   according to user-defined probabilities entered as command line arguments.
// - Packets will be delivered in the order in which they were sent (although
//   some can be lost).
// - You may have global state in this file, BUT THAT GLOBAL STATE MUST NOT BE
//   SHARED BETWEEN THE TWO ENTITIES' FUNCTIONS. "A" and "B" are simulating two
//   entities connected by a network, and as such they cannot access each
//   other's variables and global state. Entity "A" can access its own state,
//   and entity "B" can access its own state, but anything shared between the
//   two must be passed in a `pkt` across the simulated network. Violating this
//   requirement will result in a very low score for this project (or a 0).
//
// To run this project you should be able to compile it with something like:
//
//     $ gcc entity.c simulator.c -o myproject
//
// and then run it like:
//
//     $ ./myproject 0.0 0.0 10 500 3 test1.txt
//
// Of course, that will cause the channel to be perfect, so you should test
// with a less ideal channel, and you should vary the random seed. However, for
// testing it can be helpful to keep the seed constant.
//
// The simulator will write the received data on entity "B" to a file called
// `output.dat`.

#include <stdio.h>
#include "simulator.h"
#include "limits.h"

/**** A ENTITY ****/



static int bufferSize = 1024;
static int windowSize = 8;

static char A_seq_num;
static char B_pack_num;

unsigned int getUnsigned(int myint);
int sender_createChecksum(struct pkt packet);
unsigned int reciever_createChecksum(struct pkt packet);
static struct pkt A_sent_memory[2];


void B_sendACK(struct pkt packet);

void A_send_window(void);
void A_send_packet(int index);
void A_REAL_start_timer(void);

struct Sender {
	int windowSize;
	int requestNumber;
	int lastAck;
	int sequenceNumber;
	int sequenceBase;
	int sequenceMax;
	int bufferIndex;
	float RTT;
	struct pkt send_buffer[1024];
	char timerON;
}A;

/**** B ENTITY ****/
struct Receiver {
	int requestNumber;
	struct pkt myAck;
	int ackNum;
	int timeOuts;
	char EnableTimer;
}B;

void A_init() {
	
	A.windowSize = 0;
	A.requestNumber = 0;
	A.sequenceNumber = 0;
	
	A.sequenceMax = 1;
	A.timerON = 0;
	A.bufferIndex = 0;
	A.RTT = 250.0;
	A.sequenceMax = windowSize;
	//using slides
	A.sequenceBase = 0;
	A.lastAck = 0;
	for (int i = 0; i < bufferSize; i++) {
		struct pkt myPacket;
		char ack_str[20] = "null";
		myPacket.length = 10;
		myPacket.checksum = 0;
		myPacket.seqnum = 59;
		strncpy(myPacket.payload, ack_str, 20);
		A.send_buffer[i] = myPacket;
	}

}

unsigned int getUnsigned(int myint) {

	unsigned int toReturn;
	toReturn = myint + UINT_MAX + 1;
	return toReturn;
}



int sender_createChecksum(struct pkt packet) {
	
	int sum = 0;
	int checksum;
	
	for (int x = 0; x < 20; x++) {
		sum += packet.payload[x];
	}
	sum += packet.seqnum + packet.acknum + packet.length;
	checksum = ~sum;
	return checksum;
}

unsigned int reciever_createChecksum(struct pkt packet) {

	unsigned int sum = 0;
	

	for (int x = 0; x < 20; x++) {
		sum += packet.payload[x];
	}
	sum += packet.seqnum + packet.acknum + packet.length + packet.checksum;

	
	return sum;
}




/* This is called by the simulator with data passed from the application layer to your transport layer
containing data that should be sent to B. It is the job of your protocol to ensure that the data in such a
message is delivered in-order, and correctly, to the receiving side upper layer.
*/


void A_output(struct msg message) { 
	//create packet
	struct pkt myPacket;
	myPacket.seqnum = A.sequenceNumber;
	A.sequenceNumber++;

	strncpy(myPacket.payload, message.data, 20);
	myPacket.length = message.length;
	myPacket.acknum = 0; //not used for anything
	myPacket.checksum = sender_createChecksum(myPacket);

	A.send_buffer[A.bufferIndex] = myPacket;

	A.bufferIndex = (A.sequenceNumber % bufferSize);

	if (myPacket.seqnum < A.sequenceBase + windowSize) A_send_packet(myPacket.seqnum % bufferSize);
	

}

void A_send_packet(int index) {
	A_REAL_start_timer();
	tolayer3_A(A.send_buffer[index]);
	//printf("B   aknum   %d      A base num    %d  ", B.myAck.acknum, A.sequenceBase);
	//if (index == 0) exit(-1);
}

void A_send_window(void) {
	//printf("\n -------------- A seqnum is %d \n", A.sequenceNumber);
	if(A.sequenceNumber != A.lastAck) A_REAL_start_timer();
	int i = A.sequenceBase % bufferSize;
	//printf("\n ----------------------- message sent\n");
	//int x = (A.sequenceBase + windowSize) % windowSize;
	//printf("SequenceBase %d      comapred to %d \n", A.sequenceBase, x);

	while (i != ( A.sequenceBase + windowSize) % bufferSize ) {
		//printf("i is %d \n", i);
		//printf("pkt payload is %s", A.send_buffer[i].payload);
		printf("\n ----------Sending packet before ack------------\n");
		tolayer3_A(A.send_buffer[i]);
		

		i = (i + 1) % bufferSize;
	}

}

void A_input(struct pkt packet) {

	unsigned int checksum;
	checksum = reciever_createChecksum(packet);
	if (checksum == UINT_MAX && (packet.acknum <= A.sequenceNumber)) {
		//printf("\n  A receiving valid packet ---------------- packet acknum is %d     A.sequenceBase is %d \n", packet.acknum, A.sequenceBase);

		if (packet.acknum > A.lastAck) {
			A.lastAck = packet.acknum;
			A.sequenceBase = packet.acknum % bufferSize;
			//printf("\n -------------- A seqnum is %d ", A.sequenceNumber);
			
			//if ((A.sequenceNumber % bufferSize) - 1 != A.lastAck) A_send_packet(packet.acknum % bufferSize);
			if(packet.acknum != A.sequenceNumber) A_send_packet(packet.acknum % bufferSize);
		}
		
	}

}

void A_timerinterrupt() {

	//printf("TIMER A INTERRUPT ----------------");
	A.timerON = 0;
	printf("------------Sending due to timeout ------------------\n");
	A_send_window();
}


void A_REAL_start_timer(void) {
	if (A.timerON == 1) stoptimer_A();
	A.timerON = 1;
	//printf("TIMER A STARTED ----------------");
	starttimer_A(A.RTT);

}




void B_init() {
	B.requestNumber = 0;
	B.timeOuts = 0;
	
	struct pkt myPacket;
	myPacket.acknum = 0;
	myPacket.length = 20;
	myPacket.seqnum = 0;
	char ack_str[20] = "acknowledge";
	strncpy(myPacket.payload, ack_str, 20);
	myPacket.checksum = sender_createChecksum(myPacket);
	B.myAck = myPacket;
	B.EnableTimer = 1;

}

void B_sendACK(struct pkt packet) {

	packet.checksum = sender_createChecksum(packet);
	//printf("\n ------------ sending ack  %d               A base is %d -------------- \n", B.myAck.acknum, A.sequenceBase);
	tolayer3_B(packet);




}



void B_input(struct pkt packet) {
	//if checksum
	unsigned int checksum;
	checksum = reciever_createChecksum(packet);
	if (checksum == UINT_MAX) {
		//printf("\n message :: %s \n", packet.payload);
		//printf("\n---------- received packet number %d      B.myAck.acknum   %d -------------- \n", packet.seqnum, B.myAck.acknum);
		if (packet.seqnum == B.myAck.acknum) {
			//B.myAck.acknum++;
			//B.myAck.acknum = packet.seqnum + 1;
			//printf("acknum %d \n ", B.myAck.acknum);
			B.myAck.acknum = B.myAck.acknum + 1;
			

			struct msg msgFor5;
			strncpy(msgFor5.data, packet.payload, 20);
			msgFor5.length = packet.length;

			tolayer5_B(msgFor5);

			
		 }
	}
	//printf("\n ------------ sending ack  %d               A seqnum is %d -------------- \n", B.myAck.acknum, A.sequenceNumber);
	B_sendACK(B.myAck);

}

void B_timerinterrupt() {

}
