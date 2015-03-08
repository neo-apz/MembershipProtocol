/**********************************
 * FILE NAME: MP1Node.cpp
 *
 * DESCRIPTION: Membership protocol run by this Node.
 * 				Definition of MP1Node class functions.
 **********************************/

#include "MP1Node.h"

#include <sstream>

/*
 * Note: You can change/add any functions in MP1Node.{h,cpp}
 */

/**
 * Overloaded Constructor of the MP1Node class
 * You can add new members to the class if you think it
 * is necessary for your logic to work
 */
MP1Node::MP1Node(Member *member, Params *params, EmulNet *emul, Log *log, Address *address) {
	for( int i = 0; i < 6; i++ ) {
		NULLADDR[i] = 0;
	}
	this->memberNode = member;
	this->emulNet = emul;
	this->log = log;
	this->par = params;
	this->memberNode->addr = *address;
}

/**
 * Destructor of the MP1Node class
 */
MP1Node::~MP1Node() {}

/**
 * FUNCTION NAME: recvLoop
 *
 * DESCRIPTION: This function receives message from the network and pushes into the queue
 * 				This function is called by a node to receive messages currently waiting for it
 */
int MP1Node::recvLoop() {
    if ( memberNode->bFailed ) {
    	return false;
    }
    else {
    	return emulNet->ENrecv(&(memberNode->addr), enqueueWrapper, NULL, 1, &(memberNode->mp1q));
    }
}

/**
 * FUNCTION NAME: enqueueWrapper
 *
 * DESCRIPTION: Enqueue the message from Emulnet into the queue
 */
int MP1Node::enqueueWrapper(void *env, char *buff, int size) {
	Queue q;
	return q.enqueue((queue<q_elt> *)env, (void *)buff, size);
}

/**
 * FUNCTION NAME: nodeStart
 *
 * DESCRIPTION: This function bootstraps the node
 * 				All initializations routines for a member.
 * 				Called by the application layer.
 */
void MP1Node::nodeStart(char *servaddrstr, short servport) {
    Address joinaddr;
    joinaddr = getJoinAddress();

    // Self booting routines
    if( initThisNode(&joinaddr) == -1 ) {
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "init_thisnode failed. Exit.");
#endif
        exit(1);
    }

    if( !introduceSelfToGroup(&joinaddr) ) {
        finishUpThisNode();
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Unable to join self to group. Exiting.");
#endif
        exit(1);
    }

    return;
}

/**
 * FUNCTION NAME: initThisNode
 *
 * DESCRIPTION: Find out who I am and start up
 */
int MP1Node::initThisNode(Address *joinaddr) {
	/*
	 * This function is partially implemented and may require changes
	 */
	int id = *(int*)(&memberNode->addr.addr);
	int port = *(short*)(&memberNode->addr.addr[4]);

	memberNode->bFailed = false;
	memberNode->inited = true;
	memberNode->inGroup = false;
    // node is up!
	memberNode->nnb = 0;
	memberNode->heartbeat = 0;
	memberNode->pingCounter = TPING;
	memberNode->timeOutCounter = -1;
    initMemberListTable(memberNode, id, port);

    return 0;
}

/**
 * FUNCTION NAME: introduceSelfToGroup
 *
 * DESCRIPTION: Join the distributed system
 */
int MP1Node::introduceSelfToGroup(Address *joinaddr) {
	MessageHdr *msg;
#ifdef DEBUGLOG
    static char s[1024];
#endif

    if ( 0 == memcmp((char *)&(memberNode->addr.addr), (char *)&(joinaddr->addr), sizeof(memberNode->addr.addr))) {
        // I am the group booter (first process to join the group). Boot up the group
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Starting up group...");
#endif
        memberNode->inGroup = true;
    }
    else {
        size_t msgsize = sizeof(MessageHdr) + sizeof(joinaddr->addr) + sizeof(long) + 1;
        msg = (MessageHdr *) malloc(msgsize * sizeof(char));

        // create JOINREQ message: format of data is {struct Address myaddr}
        msg->msgType = JOINREQ;
        memcpy((char *)(msg+1), &memberNode->addr.addr, sizeof(memberNode->addr.addr));
        memcpy((char *)(msg+1) + 1 + sizeof(memberNode->addr.addr), &memberNode->heartbeat, sizeof(long));

#ifdef DEBUGLOG
        sprintf(s, "Trying to join...");
        log->LOG(&memberNode->addr, s);
#endif

        // send JOINREQ message to introducer member
        emulNet->ENsend(&memberNode->addr, joinaddr, (char *)msg, msgsize);

        free(msg);
    }

    return 1;

}

/**
 * FUNCTION NAME: finishUpThisNode
 *
 * DESCRIPTION: Wind up this node and clean up state
 */
int MP1Node::finishUpThisNode(){
   /*
    * Your code goes here
    */
    return 1;
}

/**
 * FUNCTION NAME: nodeLoop
 *
 * DESCRIPTION: Executed periodically at each member
 * 				Check your messages in queue and perform membership protocol duties
 */
void MP1Node::nodeLoop() {
    if (memberNode->bFailed) {
    	return;
    }

    // Check my messages
    checkMessages();

    // Wait until you're in the group...
    if( !memberNode->inGroup ) {
    	return;
    }

    // ...then jump in and share your responsibilites!
    nodeLoopOps();

    return;
}

/**
 * FUNCTION NAME: checkMessages
 *
 * DESCRIPTION: Check messages in the queue and call the respective message handler
 */
void MP1Node::checkMessages() {
    void *ptr;
    int size;

    // Pop waiting messages from memberNode's mp1q
    while ( !memberNode->mp1q.empty() ) {
    	ptr = memberNode->mp1q.front().elt;
    	size = memberNode->mp1q.front().size;
    	memberNode->mp1q.pop();
    	recvCallBack((void *)memberNode, (char *)ptr, size);
    }
    return;
}

/**
 * FUNCTION NAME: recvCallBack
 *
 * DESCRIPTION: Message handler for different message types
 */
bool MP1Node::recvCallBack(void *env, char *data, int size ) {
	/*
	 * Your code goes here
	 */

    bool ret_value = false;
    MessageHdr* msg_header = (MessageHdr*) data;
    MsgTypes msg_type = msg_header->msgType;

    char *message_content = data+sizeof(MessageHdr);
    size_t message_content_size = size - sizeof(MessageHdr);
    
    switch (msg_type) {
        case JOINREQ:
            ret_value = handle_joinreq(message_content, (int) message_content_size);
            break;

        case JOINREP:
            ret_value = handle_join_reply(message_content, (int) message_content_size);
            break;

        case PING:
            ret_value = handle_ping(message_content, (int) message_content_size);
            break;
        default:
            break;
    }
    
    return ret_value;
}

static Address get_address(MemberListEntry entry){
    Address address;
    memcpy(address.addr, &entry.id, sizeof(int));
    memcpy(&address.addr[4], &entry.port, sizeof(short));
    return address;
}

char* MP1Node::entry_to_byte_array(MemberListEntry& node, char* entry){   // [Address(6) | Hearbeat(long)]
    
    Address addr = get_address(node);
    long heartbeat = node.getheartbeat();
    
    memcpy(entry, &addr, sizeof(Address));
    memcpy(entry+sizeof(Address), &heartbeat, sizeof(long));

    return entry;
}

MemberListEntry MP1Node::byte_array_to_entry(MemberListEntry& node, char* entry, long timestamp){   // [Address(6) | Hearbeat(long)]
    
    Address *addr = (Address*) entry;
    int id;
    short port;
    long heartbeat;
    memcpy(&id, addr->addr, sizeof(int));
    memcpy(&port, &(addr->addr[4]), sizeof(short));
    
    memcpy(&heartbeat, entry+sizeof(Address), sizeof(long));
    
    node.setid(id);
    node.setport(port);
    node.setheartbeat(heartbeat);
    node.settimestamp(timestamp);
    
    return node;
}

bool MP1Node::ping_others(){
    size_t reply_size = sizeof(MessageHdr) + ((sizeof(Address) + sizeof(long))*memberNode->memberList.size());
    MessageHdr *reply_data = (MessageHdr *) malloc(reply_size);
    
    reply_data->msgType = PING;
    
    member_list_serialize((char*)(reply_data+1));
    
    Address peer;
    for (vector<MemberListEntry>::iterator iter = memberNode->memberList.begin()+1; iter != memberNode->memberList.end(); iter++) {
        peer = get_address(*iter);
        emulNet->ENsend(&memberNode->addr, &peer, (char *) reply_data, reply_size);
        
    }
    
    free(reply_data);
    return true;
}

void MP1Node::check_failures(){
    Address peer;
    for (vector<MemberListEntry>::iterator iter = memberNode->memberList.begin()+1; iter != memberNode->memberList.end(); iter++) {
        
        if (par->getcurrtime() - iter->gettimestamp() > TREMOVE){ // I should now remove from my table
            #ifdef DEBUGLOG
            peer = get_address(*iter);
            log->logNodeRemove(&memberNode->addr, &peer);
            #endif

            memberNode->memberList.erase(iter);
            iter--;

            continue;
        }
        
        if (par->getcurrtime() - iter->gettimestamp() > TFAIL){ // mark this peer as failed
            iter->setheartbeat(-1); // this means it's failed
        }
    }
}

bool MP1Node::handle_joinreq(char* data, int size){

    Address* requester = (Address*) data;

    update_membership_list(MemberListEntry(*((int *)requester->addr), *((short *)(requester->addr+4)), *((long *)(data+sizeof(Address)+1)), par->getcurrtime()));
    
    // Send my row to the requester
    size_t reply_size = sizeof(MessageHdr) + sizeof(Address) + sizeof(long)+1;
    MessageHdr *reply_data = (MessageHdr *) malloc(reply_size);
    reply_data->msgType = JOINREP;
    memcpy((char*)(reply_data+1), & (memberNode->addr), sizeof(Address));
    memcpy((char*)(reply_data+1) + sizeof(Address) + 1, &(memberNode->heartbeat), sizeof(long));
    emulNet->ENsend(&memberNode->addr, requester, (char *) reply_data, reply_size);

    free(reply_data);

    return true;
}

bool MP1Node::handle_ping(char *data, int size){
    
    int row_size = sizeof(Address) + sizeof(long);
    vector<MemberListEntry> recvd_membership_list = member_list_deserialize(data, size/row_size);
    
    for (vector<MemberListEntry>::iterator iter = recvd_membership_list.begin(); iter != recvd_membership_list.end(); iter++) {
        update_membership_list(*iter);
    }
    
    return true;
}

char *MP1Node::member_list_serialize(char *buffer) {
    int index = 0;
    int entry_size = sizeof(Address) + sizeof(long);
    char *entry = (char*) malloc(entry_size);
    
    for (vector<MemberListEntry>::iterator iter = memberNode->memberList.begin(); iter != memberNode->memberList.end(); iter++, index += entry_size) {
        entry_to_byte_array(*iter, entry);
        memcpy(buffer+index, entry, entry_size);
    }
    
    free(entry);

    return buffer;
}

vector<MemberListEntry> MP1Node::member_list_deserialize(char *table, int rows) {
    vector<MemberListEntry> memberlist;
    int entry_size = sizeof(Address) + sizeof(long);
    MemberListEntry temp_entry;
    
    for (int r = 0; r < rows; ++r, table += entry_size) {
        temp_entry = byte_array_to_entry(temp_entry, table, par->getcurrtime());
        memberlist.push_back(MemberListEntry(temp_entry));
    }

    return memberlist;
}

bool MP1Node::handle_join_reply(char* data, int size)
{
    Address* source = (Address*) data;
    memberNode->inGroup = true;
    
    update_membership_list(MemberListEntry(*(int*)(source->addr),
                                           *(short*)(source->addr+4),
                                           *(long*)(data+sizeof(Address)+1),
                                           par->getcurrtime()));

    return true;
}

bool MP1Node::update_membership_list(MemberListEntry entry){
    Address entry_addr = get_address(entry);
    long heartbeat = entry.getheartbeat();
    
    for (vector<MemberListEntry>::iterator iter = memberNode->memberList.begin(); iter != memberNode->memberList.end(); iter++) {
        if (get_address(*iter) == entry_addr) { // already exists
            if (heartbeat == -1){ // it should be marked as failed
                iter->setheartbeat(-1);
                return true;
            }
            if (iter->getheartbeat() == -1){ // ignore this
                return false;
            }
            if (iter->getheartbeat() < heartbeat){ // actual update
                iter->settimestamp(par->getcurrtime());
                iter->setheartbeat(heartbeat);
                return true;
            }
            // mine is up to date --> no change
            return false;
        }
    }
    
    if (heartbeat == -1){ // I've already removed it from my table.
        return false;
    }
    
    // It was not in my list --> new entry
    memberNode->memberList.push_back(MemberListEntry(entry));
    #ifdef DEBUGLOG
        log->logNodeAdd(&(memberNode->addr), &entry_addr);
    #endif
    
    return true;
}

/**
 * FUNCTION NAME: nodeLoopOps
 *
 * DESCRIPTION: Check if any node hasn't responded within a timeout period and then delete
 * 				the nodes
 * 				Propagate your membership list
 */
void MP1Node::nodeLoopOps() {

    // Check whether it's time to PING others
    
    if (memberNode->pingCounter == 0) { // it's time to ping others
        memberNode->heartbeat++;
//        memberNode->myPos->heartbeat++;
        memberNode->memberList[0].heartbeat++;
        ping_others();
        memberNode->pingCounter = TPING;
    }
    else{ // wait until it's time to PING
        memberNode->pingCounter--;
    }
    
    check_failures();
    
    
    return;
}

/**
 * FUNCTION NAME: isNullAddress
 *
 * DESCRIPTION: Function checks if the address is NULL
 */
int MP1Node::isNullAddress(Address *addr) {
	return (memcmp(addr->addr, NULLADDR, 6) == 0 ? 1 : 0);
}

/**
 * FUNCTION NAME: getJoinAddress
 *
 * DESCRIPTION: Returns the Address of the coordinator
 */
Address MP1Node::getJoinAddress() {
    Address joinaddr;

    memset(&joinaddr, 0, sizeof(Address));
    *(int *)(&joinaddr.addr) = 1;
    *(short *)(&joinaddr.addr[4]) = 0;

    return joinaddr;
}

/**
 * FUNCTION NAME: initMemberListTable
 *
 * DESCRIPTION: Initialize the membership list
 */
void MP1Node::initMemberListTable(Member *memberNode, int id, int port) {
	memberNode->memberList.clear();
    MemberListEntry myself = MemberListEntry(id, port, memberNode->heartbeat, par->getcurrtime());
    memberNode->memberList.push_back(myself);
//    memberNode->myPos = memberNode->memberList.begin();
}

/**
 * FUNCTION NAME: printAddress
 *
 * DESCRIPTION: Print the Address
 */
void MP1Node::printAddress(Address *addr)
{
    printf("%d.%d.%d.%d:%d \n",  addr->addr[0],addr->addr[1],addr->addr[2],
                                                       addr->addr[3], *(short*)&addr->addr[4]) ;    
}
