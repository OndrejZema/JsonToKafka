#include "Worker.h"
#include "../../../core/message_ipfix.h"
#include <thread>
#include <iostream>
#include <libfds.h>


Worker::Worker(std::shared_ptr<ConfigFormat> configFormat,
               std::shared_ptr<ConfigKafka> configKafka,
               std::shared_ptr<ConfigProcessing> configProcessing) {

    this->configFormat = configFormat;
    this->configKafka = configKafka;
    this->configProcessing = configProcessing;


    flags = FDS_CD2J_ALLOW_REALLOC;
    if (configFormat->tcp_flags) {
        flags |= FDS_CD2J_FORMAT_TCPFLAGS;
    }
    if (configFormat->timestamp) {
        flags |= FDS_CD2J_TS_FORMAT_MSEC;
    }
    if (configFormat->proto) {
        flags |= FDS_CD2J_FORMAT_PROTO;
    }
    if (configFormat->ignore_unknown) {
        flags |= FDS_CD2J_IGNORE_UNKNOWN;
    }
    if (!configFormat->white_spaces) {
        flags |= FDS_CD2J_NON_PRINTABLE;
    }
    if (configFormat->numeric_names) {
        flags |= FDS_CD2J_NUMERIC_ID;
    }
    if (configFormat->split_biflow) {
        flags |= FDS_CD2J_REVERSE_SKIP;
    }
    if (!configFormat->octets_as_uint) {
        flags |= FDS_CD2J_OCTETS_NOINT;
    }

    workerThreadsCount = std::thread::hardware_concurrency();

    init();
}

Worker::Worker(const Worker &w) {
    workerThreadsCount = w.workerThreadsCount;
    flags = w.flags;

    configFormat = w.configFormat;
    configKafka = w.configKafka;
    configProcessing = w.configProcessing;

    init();
}

void Worker::init() {

    indexProcess = 0;
    indexAdd = 0;
    restartIndexAdd = false;
    isPluginRunning = false;
    isKafkaProducerConnected = false;

    kafkaProducer = std::make_unique<KafkaProducer>
            (KafkaProducer(configKafka->hostName, configKafka->port,
                           configKafka->topicList));

    msgs = std::make_unique<std::unique_ptr<WorkerMsg>[]>
            (configProcessing->messagesBufferSize);

    processMsgsBuffer = std::make_unique<std::unique_ptr<ProcessMsgBuffer>[]>
            (workerThreadsCount);
    for (uint32_t i = 0; i < workerThreadsCount; i++) {
        processMsgsBuffer[i] = std::make_unique<ProcessMsgBuffer>
                (ProcessMsgBuffer(configProcessing->processMessageLength));
    }

    workerThreads = new std::thread[workerThreadsCount];
}

Worker::~Worker() {
    if (isPluginRunning) {
        stop();
    }
    delete[] workerThreads;

    if (isKafkaProducerConnected) {
        kafkaProducer->disconnect();
    }
}

void Worker::addMsg(std::unique_ptr<WorkerMsg> msg) {
    lck.lock();
    while (msgs[indexAdd].get() != nullptr) {
        Logger::logWarning("Buffer is full");

        lck.unlock();
        std::unique_lock<std::mutex> lock(inputMtx);
        inputCV.wait(lock);
        lck.lock();
    }

    msgs[indexAdd] = std::move(msg);
    if (indexAdd + 1 < configProcessing->messagesBufferSize) {
        indexAdd++;
    } else {
        indexAdd = 0;
        restartIndexAdd = true;
    }
    lck.unlock();
    workerCV.notify_one();
}


void Worker::start() {
    Logger::logInfo("Plugin JsonToKafka started");
    isKafkaProducerConnected = kafkaProducer->connect();
    isPluginRunning = true;
    for (uint32_t i = 0; i < workerThreadsCount; i++) {
        workerThreads[i] = std::thread(&Worker::work, this,
                                       processMsgsBuffer[i]->buffer,
                                       &processMsgsBuffer[i]->size);
    }
}

void Worker::stop() {
    Logger::logInfo("Plugin JsonToKafka stopped");
    isPluginRunning = false;
    workerCV.notify_all();
    for (uint32_t i = 0; i < workerThreadsCount; i++) {
        workerThreads[i].join();
    }
}

void Worker::work(char *processMsgBuffer, size_t *processMsgBufferSize) {
    while (isPluginRunning) {
        lck.lock();
        if ((indexProcess < indexAdd || restartIndexAdd) &&
        msgs[indexProcess].get() != nullptr) {
            std::unique_ptr<WorkerMsg> msg = std::move(msgs[indexProcess]);
            uint32_t indexCurrentMsg = indexProcess;
            if (indexProcess + 1 < configProcessing->messagesBufferSize) {
                indexProcess++;
            } else {
                indexProcess = 0;
                restartIndexAdd = false;
            }
            lck.unlock();
            processMessage(std::move(msg), processMsgBuffer,
                           processMsgBufferSize,
                           indexCurrentMsg);
            inputCV.notify_all();

        } else {
            lck.unlock();
            std::unique_lock<std::mutex> lock(workerMtx);
            workerCV.wait(lock);
        }
    }
}

int Worker::processMessage(std::unique_ptr<WorkerMsg> msg,
                           char *processMsgBuffer,
                           size_t *processMsgBufferSize,
                           uint32_t indexCurrentMsg) {

    //int returnCode = IPX_OK;
    const uint32_t recordSize = ipx_msg_ipfix_get_drec_cnt(msg->ipfix_msg);
    uint32_t messageLen = 0;
    for (uint32_t i = 0; i < recordSize; i++) {
        //get record from message
        ipx_ipfix_record *recordIpfix = ipx_msg_ipfix_get_drec(
                msg->ipfix_msg, i);
        if (configFormat->ignore_options &&
            recordIpfix->rec.tmplt->type == FDS_TYPE_TEMPLATE_OPTS) {
            continue;
        }
        messageLen = convertMessage(&recordIpfix->rec, msg->iemgr,
                                    processMsgBuffer,
                                    processMsgBufferSize);
        if (messageLen >= 0) {
            lckSend.lock();
            kafkaProducer->sendMessage(processMsgBuffer, messageLen);
            lckSend.unlock();
        } else {
            Logger::logError("Error conversion: error code = " + messageLen);
        }

        //delete record
        free(recordIpfix->rec.data);
        fds_template_destroy(
                const_cast<fds_template *>(recordIpfix->rec.tmplt));
    }

    ipx_msg_destroy((ipx_msg *) msg->ipfix_msg);
    return IPX_OK;
}


uint32_t Worker::convertMessage(fds_drec *rec, const fds_iemgr_t *iemgr,
                                char *processMsgBuffer, size_t *
processMsgBufferSize) {
    //record conversion
    return fds_drec2json(rec, flags, iemgr, &processMsgBuffer,
                         processMsgBufferSize);
}
