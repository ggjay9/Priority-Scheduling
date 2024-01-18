simple Source
{
    parameters:
        volatile double avgInterArrivalTime @unit(s);
        volatile double avgServiceTime @unit(s);
        @display("i=block/source");
    gates:
        output out;
}