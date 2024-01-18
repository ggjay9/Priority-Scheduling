#include <omnetpp.h>
#include <stdlib.h>
#include "PriorityMsg_m.h"

using namespace omnetpp;


class Source : public cSimpleModule
{
  private:
    PriorityMsg *sendMessageEvent;
    int nbGenMessages;

    double avgInterArrivalTime;
    double avgServiceTime;

  public:
    Source();
    virtual ~Source();

  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    int priorityClass;
};

Define_Module(Source);

Source::Source()
{
    sendMessageEvent = nullptr;
}

Source::~Source()
{
    cancelAndDelete(sendMessageEvent);
}

void Source::initialize()
{
    sendMessageEvent = new PriorityMsg("sendMessageEvent");
    nbGenMessages = 0;
    //get avg interarrival time and service from ini -> ned file
    avgInterArrivalTime = par("avgInterArrivalTime").doubleValue();
    avgServiceTime = par("avgServiceTime").doubleValue();

    //start sending packets
    scheduleAt(simTime(), sendMessageEvent);
    //retrieve the priority class of a given source from its ID
    priorityClass=getId()-1;

}

void Source::handleMessage(cMessage *msg)
{
    //generate packet name and insert our custom data in the message structure
    char msgname[20];
    sprintf(msgname, "message-%d, priority %d", ++nbGenMessages, priorityClass);
    PriorityMsg *message = new PriorityMsg(msgname);
    message->setPriority(priorityClass);
    message->setServiceTime(avgServiceTime*priorityClass); //to be used to save the residual service time
    message->setOriginalServiceTime(avgServiceTime*priorityClass); //to be used to get the full service time
    send(message, "out");

    //schedule next packet (same things happen)
    message->setPriority(priorityClass);
    message->setServiceTime(avgServiceTime*priorityClass);
    message->setOriginalServiceTime(avgServiceTime*priorityClass);
    //second message will be sent out after an exponentially distributed amount of time given by our parameters and the priority class
    scheduleAt(simTime()+exponential(avgInterArrivalTime/priorityClass), sendMessageEvent);
}
