#include <omnetpp.h>
#include "PriorityMsg_m.h"
#include <vector>

using namespace omnetpp;

class Queue : public cSimpleModule
{
  protected:
    // all the messages we use are of type PriorityMsg; see PriorityMsg.msg for the full data structure
    PriorityMsg *msgInServer;
    PriorityMsg *endOfServiceMsg;
    PriorityMsg *msgServer;

    cQueue queue;

    bool preemption;
    int n_prio;
    std::vector<simsignal_t> queueingTime, responseTime, timeInServer, extendedServiceTime, perClassUtilFactor;
    std::vector<simtime_t> totalBusyTimePerClass;

    simsignal_t qlenSignal;
    simsignal_t busySignal;
    simsignal_t generalQueueingTimeSignal;
    simsignal_t generalResponseTimeSignal;
    simsignal_t generalUtilFactorSignal;
    simtime_t busyTimeStart;
    simtime_t totalBusyTime=SIMTIME_ZERO;

    bool serverBusy;
public:
    Queue();
    virtual ~Queue();

protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    void startPacketService(cMessage *msg);
    void putPacketInQueue(PriorityMsg *msg);
    static int compare(cObject* a, cObject* b);
    void HandlePreemption(PriorityMsg *msg);

};

Define_Module(Queue);

Queue::Queue()
{
    msgInServer = endOfServiceMsg = nullptr;
}

Queue::~Queue()
{
    delete msgInServer;
    cancelAndDelete(endOfServiceMsg);
}

void Queue::initialize()
{
    //generate message to trigger the end of service
    endOfServiceMsg = new PriorityMsg("end-service");
    //setup the queue to be ordered: messages are sorted from higher to lower priorities and from older to newer timestamps
    queue.setName("queue");
    queue.setup(compare); //see compare function at the bottom for the ordering scheme
    serverBusy = false;
    //get parameters from the ned file
    n_prio = par("n_prio").intValue();
    preemption=par("preemption").boolValue();

    //initialize the signal recording
    generalQueueingTimeSignal = registerSignal("generalQueueingTime");
    generalResponseTimeSignal = registerSignal("generalResponseTime");

    //generate the template properties according to the ned file
    cProperty *queueingTemplate = getProperties()->get("statisticTemplate", "queueingTime");
    cProperty *responseTemplate = getProperties()->get("statisticTemplate", "responseTime");
    cProperty *extendedTemplate = getProperties()->get("statisticTemplate", "extendedServiceTime");
    cProperty *utilfactorTemplate = getProperties()->get("statisticTemplate", "utilFactor");

    //generate the dynamic signals and statistics (for the per-class statistics) using the given template and save them in a vector structure
    for (int i=0; i< n_prio; i++) {
        char str[32];
        sprintf(str, "queueingTime%d", i+1);
        queueingTime.push_back(registerSignal(str));
        getEnvir()->addResultRecorders(this, queueingTime[i], str, queueingTemplate);

        sprintf(str, "responseTime%d", i+1);
        responseTime.push_back(registerSignal(str));
        getEnvir()->addResultRecorders(this, responseTime[i], str, responseTemplate);

        sprintf(str, "extendedServiceTime%d", i+1);
        extendedServiceTime.push_back(registerSignal(str));
        getEnvir()->addResultRecorders(this, extendedServiceTime[i], str, extendedTemplate);

        sprintf(str, "perClassUtilFactor%d", i+1);
        perClassUtilFactor.push_back(registerSignal(str));
        getEnvir()->addResultRecorders(this, perClassUtilFactor[i], str, utilfactorTemplate);

        //initialize the busy time per-class counter vector to all zeros
        totalBusyTimePerClass.push_back(SIMTIME_ZERO);
    }

    generalUtilFactorSignal = registerSignal("generalUtilFactor");
    getEnvir()->addResultRecorders(this, generalUtilFactorSignal, "generalUtilFactor", utilfactorTemplate);
}

void Queue::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) { //here enters if packet in server has been processed (the script receives the endOfServiceMsg scheduled before)

        EV << "END of service: " << msgInServer->getName() << endl;

        //Send processed message to sink, compute and emit all the statistics for the latest message processed
        send(msgInServer, "out");

        emit(extendedServiceTime[msgInServer->getPriority()-1], simTime() - msgInServer->getFirstTimeInServer());
        //count the total time in server for the priority class of the given message (and send it out)
        totalBusyTimePerClass[msgInServer->getPriority()-1] = totalBusyTimePerClass[msgInServer->getPriority()-1] + msgInServer->getOriginalServiceTime();
        emit(perClassUtilFactor[msgInServer->getPriority()-1], totalBusyTimePerClass[msgInServer->getPriority()-1]/simTime());
        //count the total time the server was busy, adding this last service duration
        totalBusyTime=totalBusyTime + msgInServer->getOriginalServiceTime();
        //emit general utilization factor, response time (general and for the class of the message that has just finished), queuing time (same as for response time)
        emit(generalUtilFactorSignal, totalBusyTime/simTime());
        emit(generalResponseTimeSignal, simTime() - msgInServer->getTimestamp());
        emit(responseTime[msgInServer->getPriority()-1], simTime() - msgInServer->getTimestamp());
        emit(generalQueueingTimeSignal, msgInServer->getTimeInQueue());
        emit(queueingTime[msgInServer->getPriority()-1], msgInServer->getTimeInQueue());

        //start next packet processing if queue not empty
        if (!queue.isEmpty()) {
            //put the next message from the queue inside the server
            msgInServer = (PriorityMsg *)queue.pop();
            //save the time spent in queue up to that moment
            msgInServer->setTimeInQueue(msgInServer->getTimeInQueue() + simTime() - msgInServer->getStartingTimeInQueue());

            //start service
            startPacketService(msg);
        }
        //otherwise just set as null the message in server and wait for another message to be delivered
        else {
            msgInServer = nullptr;
            serverBusy = false;
            EV << "IDLE period starts" <<endl;
        }

    }
    //if the message is not an endOfService, it is a new packet from one source
    else {

        //Setting arrival timestamp as msg field and cast the message as PriorityMsg
        msg->setTimestamp();
        msgServer = (PriorityMsg*) msg;

        // determine whether to put the message in queue or service, based on the actual working conditions
        if (serverBusy) {
            if (preemption && msgInServer->getPriority() > msgServer->getPriority()) {
                    //put packet in service if the priority of the message received is higher that the one in the server
                    HandlePreemption(msgServer);
            }
            else {
                putPacketInQueue(msgServer);
            }
        }
        else { //server idle, start service right away
            //put the message in server and start service

            msgInServer = (PriorityMsg*) msg;
            startPacketService(msg);
            //server is now busy
            serverBusy=true;

        }
    }
}

void Queue::startPacketService(cMessage *msg)
{
    //if never been in server, timestamp the first entrance time
    if(msgInServer->getTimeInServer() == 0) {
        msgInServer->setFirstTimeInServer(simTime());
    }
    //generate service time and schedule endOfService accordingly
    scheduleAt(simTime() + msgInServer->getServiceTime(), endOfServiceMsg);
    //save the current start of service timestamp (useful when preemption is active)
    msgInServer->setStartingTime(simTime());
    //log service start
    EV << "SERVICE of " << msgInServer->getName() << " starts at " << msgInServer->getStartingTime() << endl;
}

void Queue::putPacketInQueue(PriorityMsg *msg)
{
    //save timestamp of time packet enters queue
    msg->setStartingTimeInQueue(simTime());
    queue.insert(msg); //normally insert, the sorting function will order the packets

    //log new message in queue
    //EV << msg->getName() << " Queued at "<<msg->getStartingTimeInQueue()<< endl;
}

//Ordering packets in the queue according to priority
int Queue::compare(cObject* a, cObject* b)
{
    //compare the packet I want to insert with each packet in the queue from top to bottom until reaches the point to insert the packet
    PriorityMsg *first = (PriorityMsg*) a;
    PriorityMsg *last = (PriorityMsg*) b;
    //check if priority ordering is correct (between the packet I am inserting and each of those already in the queue)
    if (first->getPriority() > last->getPriority()) {
        return 1;
    }
    else if (first->getPriority() < last->getPriority()) {
        return -1;
    }
    else {  //check if timestamps ordering is correct
        if (first->getTimestamp() < last->getTimestamp())
        {return -1;}
        else {return 1;}
    }
}

//Handle preemption
void Queue::HandlePreemption(PriorityMsg *msg)
{
    //cancel the scheduled end of service
    cancelEvent(endOfServiceMsg);
    EV << "PREEMPTION of: " << msgInServer->getName() << endl;
    //update the time spent in server and the residual service time
    msgInServer->setTimeInServer(simTime() - msgInServer->getStartingTime());
    msgInServer->setServiceTime(msgInServer->getServiceTime() - msgInServer->getTimeInServer());
    //put the packet in queue again as usual
    putPacketInQueue(msgInServer);
    //serve the new higher-priority packet
    msgInServer = msg;
    startPacketService(msgInServer);
}
