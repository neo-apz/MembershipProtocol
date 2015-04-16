/**********************************
 * FILE NAME: MP2Node.cpp
 *
 * DESCRIPTION: MP2Node class definition
 **********************************/
#include "MP2Node.h"

/**
 * constructor
 */
MP2Node::MP2Node(Member *memberNode, Params *par, EmulNet * emulNet, Log * log, Address * address) {
	this->memberNode = memberNode;
	this->par = par;
	this->emulNet = emulNet;
	this->log = log;
	ht = new HashTable();
	this->memberNode->addr = *address;
}

/**
 * Destructor
 */
MP2Node::~MP2Node() {
	delete ht;
	delete memberNode;
}

/**
 * FUNCTION NAME: updateRing
 *
 * DESCRIPTION: This function does the following:
 * 				1) Gets the current membership list from the Membership Protocol (MP1Node)
 * 				   The membership list is returned as a vector of Nodes. See Node class in Node.h
 * 				2) Constructs the ring based on the membership list
 * 				3) Calls the Stabilization Protocol
 */
void MP2Node::updateRing() {
	/*
	 * Implement this. Parts of it are already implemented
	 */
	vector<Node> curMemList;
	bool change = false;

	/*
	 *  Step 1. Get the current membership list from Membership Protocol / MP1
	 */
	curMemList = getMembershipList();

	/*
	 * Step 2: Construct the ring
	 */
	// Sort the list based on the hashCode
	sort(curMemList.begin(), curMemList.end());
	vector<Node> curNeighbors;

    if (ring.size() == 10){
        setNeighbors();
    }
    
	if (ring.size() >= 5){
//		setNeighbors();
		curNeighbors = findNeighbors(curMemList);
		if (curNeighbors[0].nodeHashCode != haveReplicasOf[0].nodeHashCode) change = true;
		else if (curNeighbors[1].nodeHashCode != haveReplicasOf[1].nodeHashCode) change = true;
		else if (curNeighbors[2].nodeHashCode != hasMyReplicas[0].nodeHashCode) change = true;
		else if (curNeighbors[3].nodeHashCode != hasMyReplicas[1].nodeHashCode) change = true;
	}


	this->ring = curMemList;

	/*
	 * Step 3: Run the stabilization protocol IF REQUIRED
	 */
	// Run stabilization protocol if the hash table size is greater than zero and if there has been a changed in the ring
	if (this->ht->currentSize()!=0 && change)
		stabilizationProtocol(curNeighbors);
    

    if (isCordinator)
        check_for_timeout();
}

/**
 * FUNCTION NAME: getMemberhipList
 *
 * DESCRIPTION: This function goes through the membership list from the Membership protocol/MP1 and
 * 				i) generates the hash code for each member
 * 				ii) populates the ring member in MP2Node class
 * 				It returns a vector of Nodes. Each element in the vector contain the following fields:
 * 				a) Address of the node
 * 				b) Hash code obtained by consistent hashing of the Address
 */
vector<Node> MP2Node::getMembershipList() {
	unsigned int i;
	vector<Node> curMemList;
	for ( i = 0 ; i < this->memberNode->memberList.size(); i++ ) {
		Address addressOfThisMember;
		int id = this->memberNode->memberList.at(i).getid();
		short port = this->memberNode->memberList.at(i).getport();
		memcpy(&addressOfThisMember.addr[0], &id, sizeof(int));
		memcpy(&addressOfThisMember.addr[4], &port, sizeof(short));
		curMemList.emplace_back(Node(addressOfThisMember));
		if (addressOfThisMember == this->memberNode->addr){ // myself
			myPosition = curMemList.end()-1;
		}
	}
	return curMemList;
}

/**
 * FUNCTION NAME: hashFunction
 *
 * DESCRIPTION: This functions hashes the key and returns the position on the ring
 * 				HASH FUNCTION USED FOR CONSISTENT HASHING
 *
 * RETURNS:
 * size_t position on the ring
 */
size_t MP2Node::hashFunction(string key) {
	std::hash<string> hashFunc;
	size_t ret = hashFunc(key);
	return ret%RING_SIZE;
}

/**
 * FUNCTION NAME: clientCreate
 *
 * DESCRIPTION: client side CREATE API
 * 				The function does the following:
 * 				1) Constructs the message
 * 				2) Finds the replicas of this key
 * 				3) Sends a message to the replica
 */
void MP2Node::clientCreate(string key, string value) {
	/*
	 * Implement this
	 */
    isCordinator = true;
    

	Message msg(g_transID, this->memberNode->addr, MessageType::CREATE, key, value);
	vector<Node> replicas = findNodes(key);

	if (replicas.size() == 3) {
		msg.replica = ReplicaType::PRIMARY;
		this->emulNet->ENsend(&this->memberNode->addr, replicas[0].getAddress(), msg.toString());
		transactions.emplace(g_transID++, Transaction(msg, 0, TRANSACTION_TIMEOUT));

		msg.replica = ReplicaType::SECONDARY;
		this->emulNet->ENsend(&this->memberNode->addr, replicas[1].getAddress(), msg.toString());

		msg.replica = ReplicaType::TERTIARY;
		this->emulNet->ENsend(&this->memberNode->addr, replicas[2].getAddress(), msg.toString());
	}

	else{
		// TODO Complete Here!
	}
}

/**
 * FUNCTION NAME: clientRead
 *
 * DESCRIPTION: client side READ API
 * 				The function does the following:
 * 				1) Constructs the message
 * 				2) Finds the replicas of this key
 * 				3) Sends a message to the replica
 */
void MP2Node::clientRead(string key){
	/*
	 * Implement this
	 */

    isCordinator = true;
    
	Message msg(g_transID, this->memberNode->addr, MessageType::READ, key);
	vector<Node> replicas = findNodes(key);

	if (replicas.size() == 3) {
		this->emulNet->ENsend(&this->memberNode->addr, replicas[0].getAddress(), msg.toString());
		this->emulNet->ENsend(&this->memberNode->addr, replicas[1].getAddress(), msg.toString());
		this->emulNet->ENsend(&this->memberNode->addr, replicas[2].getAddress(), msg.toString());
		transactions.emplace(g_transID++, Transaction(msg, 0, TRANSACTION_TIMEOUT));
	}

	else{
		// TODO Complete Here!
	}
}

/**
 * FUNCTION NAME: clientUpdate
 *
 * DESCRIPTION: client side UPDATE API
 * 				The function does the following:
 * 				1) Constructs the message
 * 				2) Finds the replicas of this key
 * 				3) Sends a message to the replica
 */
void MP2Node::clientUpdate(string key, string value){
	/*
	 * Implement this
	 */

    isCordinator = true;

	Message msg(g_transID, this->memberNode->addr, MessageType::UPDATE, key, value);
	vector<Node> replicas = findNodes(key);

	if (replicas.size() == 3) {
		msg.replica = ReplicaType::PRIMARY;
		this->emulNet->ENsend(&this->memberNode->addr, replicas[0].getAddress(), msg.toString());
		transactions.emplace(g_transID++, Transaction(msg, 0, TRANSACTION_TIMEOUT));

		msg.replica = ReplicaType::SECONDARY;
		this->emulNet->ENsend(&this->memberNode->addr, replicas[1].getAddress(), msg.toString());

		msg.replica = ReplicaType::TERTIARY;
		this->emulNet->ENsend(&this->memberNode->addr, replicas[2].getAddress(), msg.toString());
	}

	else{
		// TODO Complete Here!
	}
}

/**
 * FUNCTION NAME: clientDelete
 *
 * DESCRIPTION: client side DELETE API
 * 				The function does the following:
 * 				1) Constructs the message
 * 				2) Finds the replicas of this key
 * 				3) Sends a message to the replica
 */
void MP2Node::clientDelete(string key){
	/*
	 * Implement this
	 */

    isCordinator = true;

	Message msg(g_transID, this->memberNode->addr, MessageType::DELETE, key);
	vector<Node> replicas = findNodes(key);

	if (replicas.size() == 3) {
		this->emulNet->ENsend(&this->memberNode->addr, replicas[0].getAddress(), msg.toString());
		this->emulNet->ENsend(&this->memberNode->addr, replicas[1].getAddress(), msg.toString());
		this->emulNet->ENsend(&this->memberNode->addr, replicas[2].getAddress(), msg.toString());
		transactions.emplace(g_transID++, Transaction(msg, 0, TRANSACTION_TIMEOUT));
	}

	else{
		// TODO Complete Here!
	}
}

/**
 * FUNCTION NAME: createKeyValue
 *
 * DESCRIPTION: Server side CREATE API
 * 			   	The function does the following:
 * 			   	1) Inserts key value into the local hash table
 * 			   	2) Return true or false based on success or failure
 */
bool MP2Node::createKeyValue(string key, string value, ReplicaType replica) {
	/*
	 * Implement this
	 */
	// Insert key, value, replicaType into the hash table

	this->ht->create(key, Entry(value, par->getcurrtime(), replica).convertToString());
	vector<Node> replicas = findNodes(key);

	if (replicas.size() == 3){
//		switch (replica){
//			case ReplicaType::PRIMARY:
//				this->hasMyReplicas.emplace_back(replicas[1]);
//				this->hasMyReplicas.emplace_back(replicas[2]);
//				break;
//			case ReplicaType::SECONDARY:
//
//			case ReplicaType::TERTIARY:
//				this->haveReplicasOf.emplace_back(replicas[0]);
//				break;
//
//		}

		return true;
	}

	return false;
}

/**
 * FUNCTION NAME: readKey
 *
 * DESCRIPTION: Server side READ API
 * 			    This function does the following:
 * 			    1) Read key from local hash table
 * 			    2) Return value
 */
string MP2Node::readKey(string key) {
	/*
	 * Implement this
	 */
	// Read key from local hash table and return value
	string entryStr = this->ht->read(key);
	if (!entryStr.empty()){
		return Entry(entryStr).value;
	}

	return entryStr;
}

/**
 * FUNCTION NAME: updateKeyValue
 *
 * DESCRIPTION: Server side UPDATE API
 * 				This function does the following:
 * 				1) Update the key to the new value in the local hash table
 * 				2) Return true or false based on success or failure
 */
bool MP2Node::updateKeyValue(string key, string value, ReplicaType replica) {
	/*
	 * Implement this
	 */
	// Update key in local hash table and return true or false
	return this->ht->update(key, Entry(value, par->getcurrtime(), replica).convertToString());
}

/**
 * FUNCTION NAME: deleteKey
 *
 * DESCRIPTION: Server side DELETE API
 * 				This function does the following:
 * 				1) Delete the key from the local hash table
 * 				2) Return true or false based on success or failure
 */
bool MP2Node::deletekey(string key) {
	/*
	 * Implement this
	 */
	// Delete the key from the local hash table

	return this->ht->deleteKey(key);
}

/**
 * FUNCTION NAME: checkMessages
 *
 * DESCRIPTION: This function is the message handler of this node.
 * 				This function does the following:
 * 				1) Pops messages from the queue
 * 				2) Handles the messages according to message types
 */
void MP2Node::checkMessages() {
	/*
	 * Implement this. Parts of it are already implemented
	 */
	char * data;
	int size;

	/*
	 * Declare your local variables here
	 */

	// dequeue all messages and handle them
	while ( !memberNode->mp2q.empty() ) {
		/*
		 * Pop a message from the queue
		 */
		data = (char *)memberNode->mp2q.front().elt;
		size = memberNode->mp2q.front().size;
		memberNode->mp2q.pop();

		string message(data, data + size);
		/*
		 * Handle the message types here
		 */

		Message msg(message);

		switch (msg.type) {
			case MessageType::CREATE:
				handle_create_msg(msg);
				break;
			case MessageType::DELETE:
				handle_delete_msg(msg);
				break;
			case MessageType::READ:
				handle_read_msg(msg);
				break;
			case MessageType::UPDATE:
				handle_update_msg(msg);
				break;
			case MessageType::REPLY:
				handle_reply_msg(msg);
				break;
			case MessageType::READREPLY:
				handle_readreply_msg(msg);
				break;
			default:
				// TODO ERROR
				break;
		}

	}

	/*
	 * This function should also ensure all READ and UPDATE operation
	 * get QUORUM replies
	 */
}

// Message Handlers

void MP2Node::handle_create_msg(Message msg) {
	bool success = createKeyValue(msg.key, msg.value, msg.replica);

	if (success){
		log->logCreateSuccess(&this->memberNode->addr, false, msg.transID, msg.key, msg.value);
	}
	else{
		log->logCreateFail(&this->memberNode->addr, false, msg.transID, msg.key, msg.value);
	}

	if (msg.transID != STABLIZER_TRANS){
		Message reply(msg.transID, this->memberNode->addr, MessageType::REPLY, success);
		this->emulNet->ENsend(&this->memberNode->addr, &msg.fromAddr, reply.toString());
	}
}

void MP2Node::handle_read_msg(Message msg) {
	string value = readKey(msg.key);
	bool success = (value != "");

	if (success){
		log->logReadSuccess(&this->memberNode->addr, false, msg.transID, msg.key, value);
	}
	else{
		log->logReadFail(&this->memberNode->addr, false, msg.transID, msg.key);
	}

	Message reply(msg.transID, this->memberNode->addr, value);
	this->emulNet->ENsend(&this->memberNode->addr, &msg.fromAddr, reply.toString());
}

void MP2Node::handle_update_msg(Message msg) {
	bool success = updateKeyValue(msg.key, msg.value, msg.replica);

	if (success){
		log->logUpdateSuccess(&this->memberNode->addr, false, msg.transID, msg.key, msg.value);
	}
	else{
		log->logUpdateFail(&this->memberNode->addr, false, msg.transID, msg.key, msg.value);
	}

	Message reply(msg.transID, this->memberNode->addr, MessageType::REPLY, success);
	this->emulNet->ENsend(&this->memberNode->addr, &msg.fromAddr, reply.toString());
}

void MP2Node::handle_delete_msg(Message msg) {
	bool success = deletekey(msg.key);

	if (success){
		log->logDeleteSuccess(&this->memberNode->addr, false, msg.transID, msg.key);
	}
	else{
		log->logDeleteFail(&this->memberNode->addr, false, msg.transID, msg.key);
	}

	Message reply(msg.transID, this->memberNode->addr, MessageType::REPLY, success);
	this->emulNet->ENsend(&this->memberNode->addr, &msg.fromAddr, reply.toString());
}

void MP2Node::handle_reply_msg(Message msg) {
	Message original_msg = get_trans_message(msg.transID);
	switch (get_trans_type(msg.transID)){
		case MessageType::CREATE:
			if (msg.success){
				if (inc_trans_success(msg.transID) == REPLICA_COUNT){
					log->logCreateSuccess(&this->memberNode->addr, true, msg.transID, original_msg.key, original_msg.value);
                    invalidate_trans(msg.transID);
				}
			}
			else{
				if (inc_trans_success(msg.transID) == 1){
					log->logCreateFail(&this->memberNode->addr, true, msg.transID, msg.key, msg.value);
                    invalidate_trans(msg.transID);
				}
			}
			break;

		case MessageType::DELETE:
			if (msg.success){
				if (inc_trans_success(msg.transID) == REPLICA_COUNT){
					log->logDeleteSuccess(&this->memberNode->addr, true, msg.transID, original_msg.key);
                    invalidate_trans(msg.transID);
				}
			}
			else{
				if (inc_trans_success(msg.transID) == 1){
					log->logDeleteFail(&this->memberNode->addr, true, msg.transID, original_msg.key);
                    invalidate_trans(msg.transID);
				}
			}

			break;

		case MessageType::UPDATE:
			if (msg.success){
				if (inc_trans_success(msg.transID) == QUORUM_COUNT){
					log->logUpdateSuccess(&this->memberNode->addr, true, msg.transID, original_msg.key, original_msg.value);
                    invalidate_trans(msg.transID);
                }
			}
			else{
				if (inc_trans_success(msg.transID) == 1){
					log->logUpdateFail(&this->memberNode->addr, true, msg.transID, original_msg.key, original_msg.value);
                    invalidate_trans(msg.transID);
				}
			}
			break;
	}


}

void MP2Node::handle_readreply_msg(Message msg) {
	Message original_msg = get_trans_message(msg.transID);

	if (msg.value != ""){
		if (inc_trans_success(msg.transID) == QUORUM_COUNT){
			log->logReadSuccess(&this->memberNode->addr, true, msg.transID, original_msg.key, msg.value);
            invalidate_trans(msg.transID);
		}
	}
	else{
		if (inc_trans_success(msg.transID) == 1){
			log->logReadFail(&this->memberNode->addr, true, msg.transID, msg.key);
            invalidate_trans(msg.transID);
		}
	}
}

/**
 * FUNCTION NAME: findNodes
 *
 * DESCRIPTION: Find the replicas of the given keyfunction
 * 				This function is responsible for finding the replicas of a key
 */
vector<Node> MP2Node::findNodes(string key) {
	size_t pos = hashFunction(key);
	vector<Node> addr_vec;
	if (ring.size() >= 3) {
		// if pos <= min || pos > max, the leader is the min
		if (pos <= ring.at(0).getHashCode() || pos > ring.at(ring.size()-1).getHashCode()) {
			addr_vec.emplace_back(ring.at(0));
			addr_vec.emplace_back(ring.at(1));
			addr_vec.emplace_back(ring.at(2));
		}
		else {
			// go through the ring until pos <= node
			for (int i=1; i<ring.size(); i++){
				Node addr = ring.at(i);
				if (pos <= addr.getHashCode()) {
					addr_vec.emplace_back(addr);
					addr_vec.emplace_back(ring.at((i+1)%ring.size()));
					addr_vec.emplace_back(ring.at((i+2)%ring.size()));
					break;
				}
			}
		}
	}
	return addr_vec;
}

/**
 * FUNCTION NAME: recvLoop
 *
 * DESCRIPTION: Receive messages from EmulNet and push into the queue (mp2q)
 */
bool MP2Node::recvLoop() {
    if ( memberNode->bFailed ) {
    	return false;
    }
    else {
    	return emulNet->ENrecv(&(memberNode->addr), this->enqueueWrapper, NULL, 1, &(memberNode->mp2q));
    }
}

/**
 * FUNCTION NAME: enqueueWrapper
 *
 * DESCRIPTION: Enqueue the message from Emulnet into the queue of MP2Node
 */
int MP2Node::enqueueWrapper(void *env, char *buff, int size) {
	Queue q;
	return q.enqueue((queue<q_elt> *)env, (void *)buff, size);
}
/**
 * FUNCTION NAME: stabilizationProtocol
 *
 * DESCRIPTION: This runs the stabilization protocol in case of Node joins and leaves
 * 				It ensures that there always 3 copies of all keys in the DHT at all times
 * 				The function does the following:
 *				1) Ensures that there are three "CORRECT" replicas of all the keys in spite of failures and joins
 *				Note:- "CORRECT" replicas implies that every key is replicated in its two neighboring nodes in the ring
 */
void MP2Node::stabilizationProtocol(vector<Node> curNeighbors) {
	/*
	 * Implement this
	 */

	if (!isSameNode(hasMyReplicas[1], curNeighbors[3])){
		// My TERTIARY replica has failed

		for(map<string, string>::iterator kvPair = ht->hashTable.begin(); kvPair != ht->hashTable.end(); kvPair++){
			 Entry entry(kvPair->second);

			if (entry.replica == ReplicaType::PRIMARY){
				Message msg(STABLIZER_TRANS, this->memberNode->addr, MessageType::CREATE,
							kvPair->first, entry.value, ReplicaType::TERTIARY);
				emulNet->ENsend(&memberNode->addr, &curNeighbors[3].nodeAddress, msg.toString());
			}

		}
	}
	if (!isSameNode(hasMyReplicas[2], curNeighbors[2])){
		// My SECONDARY replica has failed,

		// TODO check whether this is the previous TERTIARY replica or not!!
		for(map<string, string>::iterator kvPair = ht->hashTable.begin(); kvPair != ht->hashTable.end(); kvPair++){
			Entry entry(kvPair->second);

			if (entry.replica == ReplicaType::PRIMARY){
				Message msg(STABLIZER_TRANS, this->memberNode->addr, MessageType::CREATE,
							kvPair->first, entry.value, ReplicaType::SECONDARY);
				emulNet->ENsend(&memberNode->addr, &curNeighbors[2].nodeAddress, msg.toString());
			}

		}
	}

	if (!isSameNode(haveReplicasOf[1], curNeighbors[1])){
		// The PRIMARY replica of which I'm its SECONDARY has failed, so I'm now PRIMARY
		for(map<string, string>::iterator kvPair = ht->hashTable.begin(); kvPair != ht->hashTable.end(); kvPair++){
			Entry entry(kvPair->second);

			if (entry.replica == ReplicaType::SECONDARY){
				entry.replica = ReplicaType::PRIMARY;
				Message msg(STABLIZER_TRANS, this->memberNode->addr, MessageType::CREATE,
							kvPair->first, entry.value, ReplicaType::SECONDARY);
				emulNet->ENsend(&memberNode->addr, &curNeighbors[2].nodeAddress, msg.toString());
				msg.replica = ReplicaType::TERTIARY;
				emulNet->ENsend(&memberNode->addr, &curNeighbors[3].nodeAddress, msg.toString());
			}

		}
	}

	// update neighbor list to the new version
	setNeighbors();

}

int MP2Node::inc_trans_success(int transID) {
	Transaction &transaction = transactions.at(transID);
	(get<1>(transaction))++;
	return (int)(get<1>(transaction));
}

int MP2Node::dec_trans_timeout(int transID) {
    Transaction &transaction = transactions.at(transID);
    (get<2>(transaction))--;
    return (int)(get<2>(transaction));
}

void MP2Node::invalidate_trans(int transID) {
    Transaction &transaction = transactions.at(transID);
    get<2>(transaction) = -1;
}

MessageType MP2Node::get_trans_type (int transID) {
	return ((Message)get<0>((Transaction) transactions.at(transID))).type;
}

Message MP2Node::get_trans_message (int transID) {
	return ((Message)get<0>((Transaction) transactions.at(transID)));
}

vector<Node> MP2Node::findNeighbors(vector<Node> ringOfNodes) {
    vector<Node>::iterator forwardNode, backwardNode;
    
    for (vector<Node>::iterator iter = ringOfNodes.begin(); iter != ringOfNodes.end(); iter++) {
        if (iter->nodeAddress == memberNode->addr){
            forwardNode = iter;
            backwardNode = iter;
            break;
        }
    }
    
	vector<Node> neighbors(4);

	if (backwardNode == ringOfNodes.begin()){
		backwardNode = ringOfNodes.end();
	}
	backwardNode--;
	neighbors[1] = *backwardNode;

	if (backwardNode == ringOfNodes.begin()){
		backwardNode = ringOfNodes.end();
	}
	backwardNode--;
	neighbors[0] = *backwardNode;

	forwardNode++;
	if (forwardNode == ringOfNodes.end())
		forwardNode = ringOfNodes.begin();

	neighbors[2] = *forwardNode;
	forwardNode++;
	if (forwardNode == ringOfNodes.end())
		forwardNode = ringOfNodes.begin();

	neighbors[3] = *forwardNode;

	return neighbors;
}

void MP2Node::setNeighbors(){
	vector<Node> neighbors = findNeighbors(this->ring);
	haveReplicasOf.clear();
	haveReplicasOf.emplace_back(neighbors[0]);
	haveReplicasOf.emplace_back(neighbors[1]);

	hasMyReplicas.clear();
	hasMyReplicas.emplace_back(neighbors[2]);
	hasMyReplicas.emplace_back(neighbors[3]);
}

bool MP2Node::isSameNode(Node n1, Node n2){
	return n1.nodeHashCode == n2.nodeHashCode;
}

void MP2Node::check_for_timeout(){
    for (map<int, Transaction>::iterator trans_pair = transactions.begin(); trans_pair != transactions.end(); trans_pair++) {
        Message msg = get_trans_message(trans_pair->first);
        
        if (dec_trans_timeout(trans_pair->first) == 0){
            switch (get_trans_type(trans_pair->first)) {
                case READ:
                    log->logReadFail(&this->memberNode->addr, true, msg.transID, msg.key);
                    break;
                case UPDATE:
                    log->logUpdateFail(&this->memberNode->addr, true, msg.transID, msg.key, msg.value);
                    break;
                default:
                    break;
            };
            
            
        }
    }
}
