#include <click/config.h>
#include "dyssectoffloadingcore.hh"
#include <click/args.hh>
#include <click/router.hh>
#include <click/standard/scheduleinfo.hh>

#include <clicknet/ip.h>
#include <clicknet/tcp.h>
#include <clicknet/ether.h>

CLICK_DECLS

inline rte_ring* DyssectOffloadingCore::choose()
{
    return Q[count++ % Q.size()];
}

DyssectOffloadingCore::DyssectOffloadingCore()
    : idx(0), count(0), _task(this)
{
    in_batch_mode = BATCH_MODE_YES;
}

DyssectOffloadingCore::~DyssectOffloadingCore()
{
}

int DyssectOffloadingCore::configure(Vector<String> &conf, ErrorHandler *errh)
{
    if(Args(conf, this, errh)
        .read("INDEX", idx)
        .read("QUEUE", queueid)
        .read_or_set("BURST", _burst, 32)
        .complete() < 0)
        return -1;

    return 0;
}

int DyssectOffloadingCore::initialize(ErrorHandler *errh)
{
    ScheduleInfo::initialize_task(this, &_task, true, errh);
    return 0;
}

inline void DyssectOffloadingCore::process()
{

}

bool DyssectOffloadingCore::run_task(Task *)
{
    rte_ring *q;
    while(rte_ring_dequeue(DyssectController::toRemoveQueue[idx], (void**) &q) == 0)
    {
        Q.erase(std::remove(Q.begin(), Q.end(), q), Q.end());
    }    

    while(rte_ring_dequeue(DyssectController::toAddQueue[idx], (void**) &q) == 0)
    {
        Q.push_back(q);
    } 

    if(Q.empty())
    {
        _task.fast_reschedule();
        return true;
    }

    PacketBatch    *head = NULL;
    WritablePacket *last = NULL;
    Packet *pkts[_burst];

    _ring = choose();

    int n = rte_ring_dequeue_burst(_ring, (void **)pkts, _burst, 0);

    for(int i = 0; i < n; i++)
    {
        WritablePacket *p = static_cast<WritablePacket*>(pkts[i]);
        rte_prefetch0(p->data());
        p->set_packet_type_anno(Packet::HOST);
        
        if(head == NULL)
            head = PacketBatch::start_head(p);
        else
            last->set_next(p);
        last = p;
    }

    if(head) {
        head->make_tail(last, n);
        output_push_batch(0, head);
    }

    _task.fast_reschedule();
    return true;
}

void DyssectOffloadingCore::add_handlers()
{
    add_task_handlers(&_task, 0, TASKHANDLER_WRITE_ALL);
}

CLICK_ENDDECLS
EXPORT_ELEMENT(DyssectOffloadingCore)