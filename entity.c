/******************************************************************************/
/*                                                                            */
/* ENTITY IMPLEMENTATIONS                                                     */
/*                                                                            */
/******************************************************************************/

// Student names:
// Student computing IDs:
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

struct Sender {
	int windowSize;
	int requestNumber;
	int sequenceNumber;
	int sequenceBase;
	int sequenceMax;
	int bufferIndex;
	float RTT;
	struct pkt send_buffer[1024];
	char timerON;
}A;

static int bufferSize = 1024;
static int windowSize = 8;

static char A_seq_num;
static char B_pack_num;

unsigned int getUnsigned(int myint);
int sender_createChecksum(struct pkt packet);
unsigned int reciever_createChecksum(struct pkt packet);
static struct pkt A_sent_memory[2];


struct pkt B_sendACK(struct pkt packet, char ACK);

void A_flip_seq_num(void);
void A_send_packets(void);
void A_REAL_start_timer(void);

void A_init() {
	A_seq_num = 0;
	A.windowSize = 0;
	A.requestNumber = 0;
	A.sequenceNumber = 0;
	A.sequenceBase = 0;
	A.sequenceMax = 0;
	A.timerON = 0;
	A.bufferIndex = 0;
	A.RTT = 250.0;
	A.sequenceMax = windowSize;
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


void A_flip_seq_num(void) {
	if (A_seq_num == 0) A_seq_num = 1;
	else A_seq_num = 0;
}

/* This is called by the simulator with data passed from the application layer to your transport layer
containing data that should be sent to B. It is the job of your protocol to ensure that the data in such a
message is delivered in-order, and correctly, to the receiving side upper layer.
*/


void A_output(struct msg message) { 
	//create packet
	struct pkt myPacket;
	myPacket.seqnum = A.sequenceNumber;
	printf("test %d \n ", myPacket.seqnum);
	A.sequenceNumber = (A.sequenceNumber + 1) % bufferSize; // what happens when this wraps?
	printf("test %d \n ", myPacket.seqnum);
	strncpy(myPacket.payload, message.data, 20);
	myPacket.length = message.length;
	myPacket.checksum = sender_createChecksum(myPacket);
	
	//add packet to buffer
	A.send_buffer[A.bufferIndex] = myPacket; // TODO: Account for overflow




	A_sent_memory[A_seq_num] = myPacket;
	A_send_packets();

}

void A_send_packets(void) {

	//timer?
	A_REAL_start_timer();

		int windowIndex = A.sequenceBase;
		while (windowIndex != A.sequenceMax && windowIndex != A.sequenceNumber) {
			tolayer3_A(A.send_buffer[windowIndex++]);
			
			//printf("---------------printing windowIndex %d  and printing sequence base %d \n", windowIndex, A.sequenceBase);
		}

	

}

void A_input(struct pkt packet) {

	unsigned int checksum;
	checksum = reciever_createChecksum(packet);
	
	//packet is valid
	if (checksum == UINT_MAX) {
		
		A_REAL_start_timer();
		if (packet.seqnum == A.sequenceNumber) {
			printf("A_input exit");
			exit(-1);
		}
		printf("\n--------packet seqnum %d        sequence base is %d --------\n ",packet.seqnum, A.sequenceBase);
		
		if (packet.seqnum > A.sequenceBase) {
			A.sequenceMax = (A.sequenceMax - A.sequenceBase + packet.seqnum) % bufferSize;
			A.sequenceBase = packet.seqnum;
			printf("\n--------sequence base is %d --------\n ", A.sequenceBase);
		}
		A_send_packets();
	}
	


	////also need checksum
	//if (packet.seqnum == A_seq_num && packet.acknum == 1) {
	//	//send next packet
	//}
}

void A_timerinterrupt() {
	printf("TIMER A INTERRUPT ----------------");
	A.timerON = 0;
	A_send_packets();
}


void A_REAL_start_timer(void) {
	if (A.timerON == 1) stoptimer_A();
	A.timerON = 1;
	printf("TIMER A STARTED ----------------");
	starttimer_A(A.RTT);
}


/**** B ENTITY ****/
struct Receiver {
	int requestNumber;
	struct pkt lastSent;
	char lastAck;
	int timeOuts;
	char EnableTimer;
}B;

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
	B.lastSent = myPacket;
	B.EnableTimer = 1;
	B.lastAck = 0;
}

struct pkt B_sendACK(struct pkt packet, char ACK) {
	stoptimer_B();
	starttimer_B(250.0);
	struct pkt ackPacket;

	if (ACK == 1) ackPacket.acknum = 1;
	else ackPacket.acknum = 0; //send NACK

	char ack_str[20] = "acknowledge";
	strncpy(ackPacket.payload, ack_str, 20);
	ackPacket.seqnum = B.requestNumber;
	ackPacket.length = 20;
	ackPacket.checksum = sender_createChecksum(ackPacket);
	tolayer3_B(ackPacket);

	if(ACK == 1) B.lastSent = ackPacket;

	return ackPacket;
}



void B_input(struct pkt packet) {
	

	
	//if checksum
	unsigned int checksum;
	checksum = reciever_createChecksum(packet);
	
	//packet is valid
	printf("reciever: packet seqnum %d      -- B.requestNum %d  \n", packet.seqnum, B.requestNumber);
	//exit(-1);
	if (checksum == UINT_MAX && packet.seqnum == B.requestNumber) {
		
		struct msg msgFor5;
		strncpy(msgFor5.data, packet.payload, 20);
		msgFor5.length = packet.length;
		tolayer5_B(msgFor5);
		

		B.requestNumber = (B.requestNumber + 1) % bufferSize;
		B_sendACK(packet, 1); // packet memory
		B.lastAck = 1; // ack or nack memory
		B.timeOuts = 0;

		
	}

	 if (checksum == UINT_MAX && B.EnableTimer ==1) {
		B.EnableTimer = 0;
		starttimer_B(250);
	}

	
	//else { //packet corrupted
	//	B_sendACK(packet, 0);
	//	B.lastAck = 0;
	//}


	
	

	//myPacket.seqnum = B_pack_num;
}

void B_timerinterrupt() {
	if(B.timeOuts > 10){
		printf("exiting from timout from timerB");
		exit(-1);
	}
	B_sendACK(B.lastSent, B.lastAck);
	++B.timeOuts;
}
