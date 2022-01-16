#include <ipfixcol2.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <memory>

#include "Config.h"
#include "Worker.h"

#include "Logger.h"
#define ELPP_THREAD_SAFE
INITIALIZE_EASYLOGGINGPP

/** Plugin description */
IPX_API struct ipx_plugin_info ipx_plugin_info = {
        // Plugin identification name
        "json-to-kafka",
        // Brief description of plugin
        "Conversion of IPFIX data into JSON format to kafka",
        // Plugin type
        IPX_PT_OUTPUT,
        // Configuration flags (reserved for future use)
        0,
        // Plugin version string (like "1.2.3")
        "1.1.0",
        // Minimal IPFIXcol version string (like "1.2.3")
        "2.1.0"};

/** Instance */
struct InstanceData {
    /** Parsed configuration of the instance  */
    std::shared_ptr<Config> config;
};

//worker instance for covert ipfix records to json and export to
// apache kafka
std::unique_ptr<Worker> worker;

int ipx_plugin_init(ipx_ctx_t *ctx, const char *params) {
    std::shared_ptr<Config> config;
    try {
        config = std::make_shared<Config>(params);
    }
    catch (std::exception &ex) {
        std::cout << ex.what() << std::endl;
        return IPX_ERR_DENIED;
    }
    //load current config for logger
    Logger::init(config->getConfigProcessing()->loggerConfigFile);
    Logger::logInfo("Succesful init plugin");
    //create worker instance for save and convert ipfix records
    worker = std::make_unique<Worker>(Worker(config->getConfigFormat(),
                                             config->getConfigKafka(),
                                             config->getConfigProcessing()));
    //start worker threads
    worker->start();

    InstanceData *data = new InstanceData();
    data->config = std::make_shared<Config>(*config);
    ipx_ctx_private_set(ctx, data);


    return IPX_OK;
}

void ipx_plugin_destroy(ipx_ctx_t *ctx, void *cfg) {
    (void) ctx; // Suppress warnings
    InstanceData *data = reinterpret_cast<InstanceData *>(cfg);
    worker->stop();
    delete data;
}
//===========================================================================
/*
  Stolen method
    ->/src/core/message_ipfix.c
        ->171 - 193
        ipx_msg_ipfix_add_drec_ref
 */
struct ipx_ipfix_record *add_drec_ref(struct ipx_msg_ipfix **msg_ref) {
    struct ipx_msg_ipfix *msg = *msg_ref;
    const size_t offset = msg->rec_info.cnt_valid * msg->rec_info.rec_size;
    msg->rec_info.cnt_valid++;
    return ((struct ipx_ipfix_record *) (((uint8_t *) msg->recs) + offset));
}
//===========================================================================

int ipx_plugin_process(ipx_ctx_t *ctx, void *cfg, ipx_msg_t *msg) {
    //get message
    ipx_msg_ipfix *m = ipx_msg_base2ipfix(msg);
    if (!m) {
        Logger::logInfo("Message is not IPFIX");
        return IPX_ERR_FORMAT;
    }

    //create empty copy
    ipx_msg_ipfix *copyMsg = ipx_msg_ipfix_create(ctx, &m->ctx, m->raw_pkt,
                                                  m->raw_size);

    //copy raw_pkt
    copyMsg->raw_pkt = (uint8_t *) malloc(m->raw_size);
    memcpy(copyMsg->raw_pkt, m->raw_pkt, m->raw_size);

    copyMsg->msg_header.ref_cnt = m->msg_header.ref_cnt;

    uint32_t recordSize = 0;
    //copy record
    for (int i = 0; i < m->rec_info.cnt_valid; i++) {

        ipx_ipfix_record *rec = add_drec_ref(&copyMsg);
        //ipx_ipfix_record* rec = ipx_msg_ipfix_add_drec_ref(&copyMsg); // symbol lookup error

        ipx_ipfix_record *record = ipx_msg_ipfix_get_drec(m, i);
        recordSize = record->rec.size;

        rec->rec.size = recordSize;
        rec->rec.data = (uint8_t *) malloc(recordSize);
        rec->rec.tmplt = fds_template_copy(record->rec.tmplt);

        memcpy(rec->rec.data, record->rec.data, recordSize);

    }
    //add copy message to plugin
    worker->addMsg(std::move(std::make_unique<WorkerMsg>(WorkerMsg(
            copyMsg, ipx_ctx_iemgr_get(ctx)))));
    return IPX_OK;
}
