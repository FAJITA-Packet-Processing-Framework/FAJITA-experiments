#ifndef CLICK_DYSSECTQUEUEOUT_HH_
#define CLICK_DYSSECTQUEUEOUT_HH_

#include <click/batchelement.hh>
#include <click/task.hh>
#include <click/dpdkdevice.hh>

#include "dyssectcontroller.hh"

CLICK_DECLS

class DyssectQueueOut : public BatchElement {
    private:        
        uint32_t portid;
        uint32_t _burst;
        uint32_t queueid;
        uint32_t total_packets;
        int process(Packet *p);

    public:
        DyssectQueueOut() CLICK_COLD;
        ~DyssectQueueOut() CLICK_COLD;

        const char *class_name() const override { return "DyssectQueueOut"; }
        const char *port_count() const { return "1/0"; }
        const char *processing() const { return PUSH; }

        int configure(Vector<String> &, ErrorHandler *) CLICK_COLD;
        int initialize(ErrorHandler *errh) CLICK_COLD;

        void push_batch(int, PacketBatch *);
};

CLICK_ENDDECLS

#endif // CLICK_DYSSECTQUEUEOUT_HH_