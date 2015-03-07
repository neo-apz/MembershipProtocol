/**********************************
 * FILE NAME: MP1Node.cpp
 *
 * DESCRIPTION: Membership protocol run by this Node.
 * 				Header file of MP1Node class.
 **********************************/

#ifndef _MP1NODE_H_
#define _MP1NODE_H_

#include "stdincludes.h"
#include "Log.h"
#include "Params.h"
#include "Member.h"
#include "EmulNet.h"
#include "Queue.h"

/**
 * Macros
 */
#define TREMOVE 20
#define TFAIL 5

/*
 * Note: You can change/add any functions in MP1Node.{h,cpp}
 */

/**
 * Message Types
 */
enum MsgTypes{
    JOINREQ,
    JOINREP,
	PING,
	FAILNOTIFY,
    DUMMYLASTMSGTYPE
};

/**
 * STRUCT NAME: MessageHdr
 *
 * DESCRIPTION: Header and content of a message
 */
typedef struct MessageHdr {
	enum MsgTypes msgType;
}MessageHdr;

/**
 * CLASS NAME: MP1Node
 *
 * DESCRIPTION: Class implementing Membership protocol functionalities for failure detection
 */
class MP1Node {
private:
	EmulNet *emulNet;
	Log *log;
	Params *par;
	Member *memberNode;
	char NULLADDR[6];

    // Here are the handler functions
    bool handle_joinreq(Address* source, char* data, int size);
	bool handle_join_reply(Address* source, char *data, int size);
	bool handle_ping(Address* source, char *data, int size);
    
    // Here are extra functions
    bool update_membership_list(MemberListEntry entry);
    bool ping_others();
    
    MemberListEntry byte_array_to_entry(MemberListEntry& node, char* entry, long timestamp);
    char* entry_to_byte_array(MemberListEntry& node, char* entry);
    
public:
	MP1Node(Member *, Params *, EmulNet *, Log *, Address *);
	Member * getMemberNode() {
		return memberNode;
	}
	int recvLoop();
	static int enqueueWrapper(void *env, char *buff, int size);
	void nodeStart(char *servaddrstr, short serverport);
	int initThisNode(Address *joinaddr);
	int introduceSelfToGroup(Address *joinAddress);
	int finishUpThisNode();
	void nodeLoop();
	void checkMessages();
	bool recvCallBack(void *env, char *data, int size);
	void nodeLoopOps();
	int isNullAddress(Address *addr);
	Address getJoinAddress();
	void initMemberListTable(Member *memberNode, int id, int port);
	void printAddress(Address *addr);
	virtual ~MP1Node();


	char *member_list_serialize(char *buffer);
	vector<MemberListEntry> member_list_deserialize(char *buffer, int rows);
};

#endif /* _MP1NODE_H_ */
