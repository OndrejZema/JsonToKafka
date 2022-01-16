#pragma once

#include <iostream>
#include <thread>
#include <unistd.h>

#include <ctype.h>
#include <librdkafka/rdkafka.h>
#include <signal.h>
#include <stdio.h>

#include <string>
#include <vector>
#include "Logger.h"


/**
 * \brief Kafka producent module
 * Class for sending messages to apache kafka
 */

class KafkaProducer final {
private:
    std::string ip;
    std::string port;
    // topics
    rd_kafka_topic_partition_list_t *topics;
    // list of topics separated by ','
    std::string topicList = "";
    // configuration
    rd_kafka_conf_t *conf;
    // handle
    rd_kafka_t *rk;

    void handleMessages(int id);

    // callback function / message delivery
    static void dr_msg_cb(rd_kafka_t *rk, const rd_kafka_message_t
    *rkmessage, void *opaque);


public:
    /** \brief Constructor
     *
     * @param[in] ip apache kafka
     * @param[in] port apache kafka
     * @param[in] topicList topic message
     * @param[in] canLog can log data
     * @param[in] logFilePath path to log file
     */
    KafkaProducer(std::string ip, std::string port,
                  std::string topicList);

    /**
     * \brief Copy constructor
     * @param instance kafkaProducer
     */
    KafkaProducer(const KafkaProducer &kafkaProducer);


/**
 * \brief Connect kafka producent
 * @return state if kafka producent sucessful connected
 */
    bool connect();

/**
 * \brief Send message by kafka producer
 *
 * @param[in] message message buffer
 * @param[in] len message buffer length
 * @return state if message sucessful delivered
 */
    bool sendMessage(const char *message, uint32_t len);

/**
 * \brief Flush final message and destroy producent instance
 */
    void disconnect();
};
