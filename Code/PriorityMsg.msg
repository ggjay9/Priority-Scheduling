packet PriorityMsg {
    int Priority;
    simtime_t serviceTime;
    simtime_t originalServiceTime;
    simtime_t timeInServer=0;
    simtime_t startingTime;
    simtime_t firstTimeInServer;
    simtime_t timeInQueue=0;
    simtime_t startingTimeInQueue;
    
    }
