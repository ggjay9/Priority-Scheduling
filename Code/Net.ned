network Net
{
    parameters:
        int n_prio;
        @display("bgb=818,273");
    submodules:
        source[n_prio]: Source {
            @display("row");
        }
        queue1: Queue {
            @display("p=328.8,107.8875");
        }
        sink1: Sink {
            @display("p=568.2075,107.8875");
        }
    connections:
        for i=0..n_prio-1 {
            source[i].out --> queue1.in[i];
        }
        queue1.out --> sink1.in;
}
