#ifndef CLICK_DYSSECTCONTROLLER_HH_
#define CLICK_DYSSECTCONTROLLER_HH_

#include <click/element.hh>
#include <click/task.hh>
#include <click/notifier.hh>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <vector>
#include <math.h>
#include <float.h>

#include <rte_ethdev.h>

#include <google/dense_hash_map>
#include <rte_hash.h>

CLICK_DECLS

#define LONG                    0
#define SHORT                   1
#define SCALE                   1.5
#define EPSILON                 0.0000000001f
#define FLOW_SIZE               12
#define SHORT_TIME              100000
#define MEMPOOL_CACHE_SIZE      512
#define PKTMBUF_POOL_ELEMENTS   8*1024*1024 - 1
#define RETA_CONF_SIZE          (RTE_ETH_RSS_RETA_SIZE_512 / RTE_ETH_RETA_GROUP_SIZE)
//#define HASH_TABLE_SIZE 	    16777216
#define HASH_TABLE_SIZE 	    4096

#define MIN(a,b) ((a) < (b) ? (a) : (b))

struct DyssectFlow
{
    uint16_t src_port;
    uint16_t dst_port;
    uint32_t src_addr;
    uint32_t dst_addr;
    uint32_t hash_value;

    struct Hash
    {
        std::size_t operator()(const DyssectFlow &f) const
        {
            return f.hash_value;
        }
    };

    struct EqualTo
    {
        bool operator()(const DyssectFlow &a, const DyssectFlow &b) const 
        {
            return !memcmp((void*) &a, (void*) &b, FLOW_SIZE);
        }
    };
};

struct DyssectState {
    double cdf;
    double prob;
    double prob2;
    bool priority;
    bool offloaded;
    uint32_t epoch;
    uint64_t bytes;
    uint32_t shard;

    DyssectFlow flow;

    void **global_state;

    struct Hash
    {
        std::size_t operator()(const DyssectState *s) const
        {
            return s->flow.hash_value;
        }
    };

    struct EqualTo
    {
        bool operator()(const DyssectState *a, const DyssectState *b) const 
        {
            return !memcmp((void*) &(a->flow), (void*) &(b->flow), FLOW_SIZE);
        }
    };
};

using DyssectHashTable  = google::dense_hash_map<DyssectFlow, DyssectState*, DyssectFlow::Hash, DyssectFlow::EqualTo>;
using DyssectHashTable2 = google::dense_hash_map<DyssectFlow, uint32_t>;

struct ShardInfo
{
    double r;
    double V;
    bool use_2;
    rte_atomic32_t bytes;
    rte_atomic32_t packets;
    uint32_t old_bytes;
    uint32_t old_packets;

    rte_atomic32_t pause;
    rte_atomic32_t ref_count_1;
    rte_atomic32_t ref_count_2;
    
    std::vector<DyssectState*> *ordered_flows;

    double r_new;
    uint32_t owner;
    uint32_t owner_new;
    DyssectHashTable *table;
//    void *table;
    rte_ring local_queue;
    DyssectHashTable2 *flows;
    DyssectHashTable *flows3;
    DyssectHashTable *flows3_2;
    void *_table;
};

class DyssectController : public Element {
    public:
        DyssectController() CLICK_COLD;
        ~DyssectController() CLICK_COLD;

        const char *class_name() const override { return "DyssectController"; }
        const char *port_count() const { return "-/-"; }

        Task _task;

        static uint32_t iqueues;
        static rte_ring **rings;
        static rte_ring **queues;
        static ShardInfo *shards;
        static uint32_t sfc_length;
        static uint32_t total_shards;
        static rte_atomic32_t *epoch;
        static rte_atomic32_t **had_changes;
        static rte_atomic32_t **transfer_r;
        static rte_atomic32_t **transfer_shard;
        static rte_atomic32_t **transfer_offloading;
        static rte_atomic32_t **controller_signal;

        static rte_atomic32_t **offloading_from_working;
        static rte_atomic32_t **new_offloading_from_working;

        static rte_mempool *pktmbuf_pool;

        static rte_ring **toAddQueue;
        static rte_ring **toRemoveQueue;

        int configure(Vector<String> &, ErrorHandler *) CLICK_COLD;
        int initialize(ErrorHandler *errh) CLICK_COLD;
        void add_handlers() CLICK_COLD;

        bool run_task(Task*) override;

    private:
        uint32_t W;
        uint32_t E;
        uint32_t *A;
        uint32_t *O;
        uint32_t *newA;
        uint32_t *newO;
        uint32_t portid;
        uint32_t oqueues;
        uint32_t ndesc_in;
        uint32_t ndesc_out;
        uint64_t next_short;

        uint32_t total_flows;
        uint32_t total_bytes;
        uint32_t total_packets;
        uint32_t last_totalpackets;

        uint32_t reta_size;
        struct rte_eth_rss_reta_entry64 reta_conf[RETA_CONF_SIZE];

        bool solver;
        char *solver_IN;
        char *solver_OUT;
        uint32_t total_cores;

        double Tr, Tp;
        double Car, Cap;
        double Csr, Csp;
        double SLOr, SLOp;


        void init_dpdk();
        void init_shards();
        void init_queues();

        bool update_ratio();
        bool update_relationship();
        bool update_shards();

        void update_reta();
        void migration_shard(uint32_t, uint32_t, bool);

        void swap_shards();
        void clear_flows();
        void order_shards();
        void volume_shards();
        void update_short_epoch(bool solver);
        bool run_short_solver();
};

CLICK_ENDDECLS

#endif // CLICK_DYSSECTCONTROLLER_HH_
