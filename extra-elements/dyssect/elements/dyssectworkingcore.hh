#ifndef CLICK_DYSSECTWORKINGCORE_HH_
#define CLICK_DYSSECTWORKINGCORE_HH_

#include <click/batchelement.hh>
#include <click/notifier.hh>
#include <click/task.hh>
#include <click/dpdkdevice.hh>

#include "dyssectcontroller.hh"

CLICK_DECLS

class DyssectWorkingCore : public BatchElement {
    private:
        Task _task;
        uint32_t qsize;
        uint32_t portid;
        uint32_t _burst;
        uint32_t queueid;
        uint32_t offloading_idx;
        uint32_t off_idx;
        struct rte_mbuf *off_pkts[128];

        bool myown;
        struct rte_ring *q;
        struct rte_ring *myring;
        bool _solver;
        
        PacketBatch *process();
        int run(Packet *p);
        DyssectState *extract_state(Packet *p);

        void transfer_r();
        void transfer_shard();
        void transfer_offloading();

    public:
        DyssectWorkingCore() CLICK_COLD;
        ~DyssectWorkingCore() CLICK_COLD;

        const char *class_name() const override { return "DyssectWorkingCore"; }
        const char *port_count() const { return PORTS_0_1; }
        const char *processing() const { return PUSH; }

        int configure(Vector<String> &, ErrorHandler *) override CLICK_COLD;
        int initialize(ErrorHandler *errh) override CLICK_COLD;
        bool run_task(Task *) override;

        static int xstats_handler(int operation, String &input, Element *e,
                              const Handler *handler, ErrorHandler *errh);
        void add_handlers() override CLICK_COLD;
//        static String read_handler(Element *, void *) CLICK_COLD;
};

CLICK_ENDDECLS

#endif // CLICK_DYSSECTWORKINGCORE_HH_