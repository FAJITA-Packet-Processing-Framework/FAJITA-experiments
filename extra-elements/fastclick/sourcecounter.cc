/*
 * flowcounter.{cc,hh} -- remove insults in web pages
 * Tom Barbette
 */

#include <click/config.h>
#include <click/router.hh>
#include <click/args.hh>
#include <click/error.hh>
#include "sourcecounter.hh"

CLICK_DECLS

SourceCounter::SourceCounter()
{

}

int SourceCounter::configure(Vector<String> &conf, ErrorHandler *errh)
{
    if(Args(conf, this, errh)
        .read_or_set("CAPACITY", _capacity, 65536)
        .read_or_set("ISSRC", _is_src, 1)
        .read_or_set("CACHE", _cache, 1)
        .complete() < 0
    )
        return -1;
    
    _insertions = 0;
    if (_is_src)
        _offset = 26;
    else _offset = 30;
    
    struct rte_hash_parameters hash_params = {0};
    char buf[64];
    sprintf(buf, "%i-%s", click_random(), name().c_str());
    click_chatter("capacity is %d", _capacity);
    hash_params.name = buf;
    hash_params.entries = _capacity;

    hash_params.key_len = sizeof(local_flowID);
    hash_params.hash_func = ipv4_hash_crc_src_ip;
    hash_params.hash_func_init_val = 0;
    hash_params.extra_flag = RTE_HASH_EXTRA_FLAGS_RW_CONCURRENCY | RTE_HASH_EXTRA_FLAGS_MULTI_WRITER_ADD;

//    sprintf(buf, "Shared-Data-Structure-%d-%s",0 ,name().c_str());
    _table = rte_hash_create(&hash_params);

    if (!_table)
        return errh->error("Could not init flow table %s : error %d (%s)!", name().c_str(), rte_errno, rte_strerror(rte_errno));

    _local_fcbs = (atomic_uint32_t*) CLICK_ALIGNED_ALLOC(sizeof(atomic_uint32_t) * _capacity);
    CLICK_ASSERT_ALIGNED(_local_fcbs);
    bzero(_local_fcbs, sizeof(atomic_uint32_t) * _capacity);
    if (!_local_fcbs) {
            return errh->error("Could not init data for table %s!", name().c_str());
    }

    return 0;
}

Packet * SourceCounter::simple_action(Packet *packet){
    click_chatter("Unhandled funciton!");
    return packet;
}

#if HAVE_BATCH
PacketBatch * SourceCounter::simple_action_batch(PacketBatch *head) 
{
    int *positions = new int[head->count()];
    int **ret = new int*[head->count()];
    int index = 0;
    void **key_array = new void*[64];
    FOR_EACH_PACKET(head, p){
        key_array[index] = (local_flowID*) (p->data() + _offset);
        ret[index] = &(positions[index]);
        index++;
    }

//    auto *table = reinterpret_cast<rte_hash *> (_table);
//    rte_hash_lookup_bulk(table, const_cast<const void **>(&(key_array[0])), head->count(), ret);

    int i = 0;
    FOR_EACH_PACKET_SAFE(head, pkt) {
        int local_idx = 0;
//        int local_idx = positions[i];
//        if (local_idx < 0){
//            local_idx = rte_hash_add_key(table, key_array[i]);
//            if (local_idx < 0){
//                click_chatter("Problem with inserting data! %d", local_idx);
//                continue;
//            }
//        }

        _local_fcbs[local_idx]++;
        i++;
    }

    return head;
}
#endif

enum { h_count, h_insertions};

String
SourceCounter::read_handler(Element *e, void *thunk)
{
    SourceCounter *fd = static_cast<SourceCounter *>(e);
    switch ((intptr_t)thunk) {
        case h_count:{
            auto *table = reinterpret_cast<rte_hash *> (fd->_table);
            return String(rte_hash_count(table));
        }
        case h_insertions:
            return String(fd->_insertions);
        default:
    	    return "<error>";
    }
}

int
SourceCounter::write_handler(const String &s_in, Element *e, void *thunk, ErrorHandler *errh)
{
    return -1;
}

void
SourceCounter::add_handlers() {
    add_read_handler("count", read_handler, h_count);
    add_read_handler("insertions", read_handler, h_insertions);
}

CLICK_ENDDECLS
EXPORT_ELEMENT(SourceCounter)
ELEMENT_MT_SAFE(SourceCounter)
