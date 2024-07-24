#include <click/config.h>
#include "dyssectqueueout.hh"
#include <click/args.hh>
#include <click/router.hh>
#include <click/standard/scheduleinfo.hh>

#include <clicknet/ip.h>
#include <clicknet/tcp.h>
#include <clicknet/ether.h>

CLICK_DECLS

DyssectQueueOut::DyssectQueueOut()
    : total_packets(0)
{
}

DyssectQueueOut::~DyssectQueueOut()
{
}

int DyssectQueueOut::configure(Vector<String> &conf, ErrorHandler *errh)
{
    return Args(conf, this, errh)
        .read("PORT", portid)
        .read("QUEUE", queueid)
        .read_or_set("BURST", _burst, 32)
        .complete();
}

int DyssectQueueOut::initialize(ErrorHandler *errh)
{
    return 0;
}

void DyssectQueueOut::push_batch(int, PacketBatch *batch)
{
    Packet *p = batch->first();
    Packet *next;

    struct rte_mbuf *pkts[_burst];

    int count = 0;
    while(p && count < _burst)
    {
        next = p->next();
        struct rte_mbuf *mbuf = DPDKDevice::get_mbuf(p, true, 0);

        if(likely(mbuf != NULL))
            pkts[count++] = mbuf;

        p = next;
    }

    total_packets += rte_eth_tx_burst(portid, queueid, pkts, count);
}

CLICK_ENDDECLS
EXPORT_ELEMENT(DyssectQueueOut)