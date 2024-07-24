#ifndef CLICK_DYSSECTOFFLOADINGCORE_HH_
#define CLICK_DYSSECTOFFLOADINGCORE_HH_

#include <click/batchelement.hh>
#include <click/task.hh>
#include <vector>

#include "dyssectcontroller.hh"

CLICK_DECLS

class DyssectOffloadingCore : public BatchElement {
    private:
        uint32_t idx;
        uint32_t count;
        uint32_t _burst;
        uint32_t queueid;
        struct rte_ring *_ring;
        
        void process();
        rte_ring* choose();

    public:
        DyssectOffloadingCore() CLICK_COLD;
        ~DyssectOffloadingCore() CLICK_COLD;

        const char *class_name() const override { return "DyssectOffloadingCore"; }
        const char *port_count() const { return "0/1"; }
        const char *processing() const { return PUSH; }

        int configure(Vector<String> &, ErrorHandler *) CLICK_COLD;
        int initialize(ErrorHandler *errh) CLICK_COLD;
        void add_handlers() CLICK_COLD;

        bool run_task(Task*) override;

        Task _task;
        std::vector<rte_ring*> Q;
};

CLICK_ENDDECLS

#endif // CLICK_DYSSECTOFFLOADINGCORE_HH_