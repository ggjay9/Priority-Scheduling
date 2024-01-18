simple Queue
{
    parameters:
        int n_prio;
        bool preemption;
        @display("i=block/queue;q=queue");
        
        //generate signals for each statistic we need to collect
        @signal[generalQueueingTime](type="simtime_t");
        @signal[queueingTime*](type="simtime_t");
        @signal[generalResponseTime](type="simtime_t");
        @signal[responseTime*](type="simtime_t");
        @signal[extendedServiceTime*](type="simtime_t");
        @signal[perClassUtilFactor*](type="long");
        @signal[generalUtilFactor](type="long");
        
        //generate statistics for fixed/general ones and statisticTemplates for the dynamically generated statistics (the number of per-class statistics is function of the number of classes, selected in the ini fle)
        @statistic[generalQueueingTime](title="general queueing time";unit=s;record=vector,mean;interpolationmode=none);
        @statisticTemplate[queueingTime](title="per-class queueing time template";unit=s;record=vector,mean;interpolationmode=none);
        @statistic[generalResponseTime](title="general response time at end of service";unit=s;record=vector,mean;interpolationmode=none);
        @statisticTemplate[responseTime](title="per-class response time template at end of service";unit=s;record=vector,mean;interpolationmode=none);
        @statisticTemplate[extendedServiceTime](title="extended service time (useful when preemption is active)";unit=s;record=vector,mean;interpolationmode=none);
        @statisticTemplate[utilFactor](title="general and per-class utilization factor computation template";record=vector,last;interpolationmode=none);
           
    gates:
        input in[n_prio];
        output out;
}