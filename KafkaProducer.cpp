#include <cstring>
#include "KafkaProducer.h"


void KafkaProducer::dr_msg_cb(rd_kafka_t *rk,
                              const rd_kafka_message_t *rkmessage,
                              void *opaque) {
    if (rkmessage->err) {
        fprintf(stderr, "%% Message delivery failed: %s\n",
                rd_kafka_err2str(rkmessage->err));
    } else {
        fprintf(stderr,
                "%% Message delivered (%zd bytes, "
                "partition %" PRId32 ")\n",
                rkmessage->len, rkmessage->partition);
    }

    /* The rkmessage is destroyed automatically by librdkafka */
}

KafkaProducer::KafkaProducer(std::string ip, std::string port,
                             std::string topicList) {
    this->ip = ip;
    this->port = port;
    this->topicList = topicList;
}

KafkaProducer::KafkaProducer(const KafkaProducer &kp) {
    ip = kp.ip;
    port = kp.port;
    topicList = kp.topicList;
}
bool KafkaProducer::connect() {
    conf = rd_kafka_conf_new();
    char errstr[512];


    std::string broker = ip + ":" + port;

    if (rd_kafka_conf_set(conf, "bootstrap.servers", broker.c_str(), errstr,
                          sizeof(errstr)) != RD_KAFKA_CONF_OK) {

        Logger::logError("Failed to connect server");

        return false;
    }


    if (!(rk = rd_kafka_new(RD_KAFKA_PRODUCER, conf, errstr,
                            sizeof(errstr)))) {

        Logger::logError("Failed to create new producer");

        return false;
    }

    rd_kafka_conf_set_dr_msg_cb(conf, KafkaProducer::dr_msg_cb);
    return true;
}

bool KafkaProducer::sendMessage(const char *message, const uint32_t len) {
    char buf[len];
    memmove(buf, message, len);

    rd_kafka_resp_err_t err;
    if (len == 0) {
        /* Empty line: only serve delivery reports */
        rd_kafka_poll(rk, 0 /*non-blocking */);
        return false;
    }

    retry:
    err = rd_kafka_producev(
            /* Producer handle */
            rk,
            /* Topic name */
            RD_KAFKA_V_TOPIC(topicList.c_str()),
            /* Make a copy of the payload. */
            RD_KAFKA_V_MSGFLAGS(RD_KAFKA_MSG_F_COPY),
            /* Message value and length */
            RD_KAFKA_V_VALUE(buf, len),
            /* Per-Message opaque, provided in
             * delivery report callback as
             * msg_opaque. */
            RD_KAFKA_V_OPAQUE(NULL),
            /* End sentinel */
            RD_KAFKA_V_END);

    if (err) {
        /*
         * Failed to *enqueue* message for producing.
         */

        Logger::logError("Failed to enqueue message for production");

        if (err == RD_KAFKA_RESP_ERR__QUEUE_FULL) {
            /* If the internal queue is full, wait for
             * messages to be delivered and then retry.
             * The internal queue represents both
             * messages to be sent and messages that have
             * been sent or failed, awaiting their
             * delivery report callback to be called.
             *
             * The internal queue is limited by the
             * configuration property
             * queue.buffering.max.messages */
            rd_kafka_poll(rk, 1000 /*block for max 1000ms*/);
            goto retry;
        }
    } else {
        //cout << "Enqueued message (" << len << " bytes) for topic " << topics.c_str() << endl;
    }

    return true;
}

void KafkaProducer::disconnect() {
    Logger::logInfo("Flushing last message");
    rd_kafka_flush(rk, 10 * 1000 /* wait for max 10 seconds */);

    /* If the output queue is still not empty there is an issue
     * with producing messages to the clusters. */
    if (rd_kafka_outq_len(rk) > 0) {

        Logger::logWarning("Message(s) were not delivered");
    }
    /* Destroy the producer instance */
    rd_kafka_destroy(rk);
}
