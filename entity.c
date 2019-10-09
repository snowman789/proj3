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

static char A_seq_num;
static char B_pack_num;

unsigned int getUnsigned(int myint);
int sender_createChecksum(struct pkt packet);
unsigned int reciever_createChecksum(struct pkt packet);
static struct pkt A_sent_memory[2];
int Add(int x, int y);

struct pkt B_sendACK(struct pkt packet, char ACK);

void A_flip_seq_num(void);



void A_init() {
	A_seq_num = 0;
}

unsigned int getUnsigned(int myint) {

	unsigned int toReturn;
	toReturn = myint + UINT_MAX + 1;
	return toReturn;
}

int Add(int x, int y)
{
	// Iterate till there is no carry   
	while (y != 0)
	{
		// carry now contains common  
		//set bits of x and y 
		int carry = x & y;

		// Sum of bits of x and y where at  
		//least one of the bits is not set 
		x = x ^ y;

		// Carry is shifted by one so that adding 
		// it to x gives the required sum 
		y = carry << 1;
	}
	return x;
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

	struct pkt myPacket;
	myPacket.seqnum = A_seq_num;
	strncpy(myPacket.payload, message.data, 20);
	myPacket.length = message.length;
	//need to flip bits;
	myPacket.checksum = sender_createChecksum(myPacket);
	//create checksum




	A_sent_memory[A_seq_num] = myPacket;
	A_flip_seq_num();
	tolayer3_A(myPacket);

}

void A_input(struct pkt packet) {

	//also need checksum
	if (packet.seqnum == A_seq_num && packet.acknum == 1) {
		//send next packet
	}
}

void A_timerinterrupt() {
}


/**** B ENTITY ****/

void B_init() {
}

struct pkt B_sendACK(struct pkt packet, char ACK) {
	
	struct pkt ackPacket;

	if (ACK == 1) ackPacket.acknum = 1;
	else ackPacket.acknum = 0; //send NACK

	char ack_str[20] = "acknowledge";
	strncpy(ackPacket.payload, ack_str, 20);
	ackPacket.seqnum = packet.seqnum;
	ackPacket.checksum = 6969;
	ackPacket.length = 20;
	ackPacket.checksum = sender_createChecksum(ackPacket);
	tolayer3_B(ackPacket);

	return ackPacket;
}



void B_input(struct pkt packet) {
	

	
	//if checksum
	unsigned int checksum;
	checksum = reciever_createChecksum(packet);
	
	//packet is valid
	if (checksum == UINT_MAX) {
		B_sendACK(packet, 1);
	}
	else { //packet corrupted
		B_sendACK(packet, 0);
		
	}


	
	struct msg msgFor5;
	strncpy(msgFor5.data, packet.payload, 20);
	msgFor5.length = packet.length;
	tolayer5_B(msgFor5);

	//myPacket.seqnum = B_pack_num;
}

void B_timerinterrupt() {
}
