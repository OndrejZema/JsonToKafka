#ifndef WORKER_H
#define WORKER_H

#include <vector>
#include <mutex>
#include <condition_variable>
#include <ipfixcol2.h>
#include <thread>
#include <atomic>
#include "Config.h"
#include "KafkaProducer.h"
#include "Logger.h"
#include <string>
#include <vector>
#include "../../../core/message_ipfix.h"


/**
 * Wraper for IPFIX message and iemgr
 */
class WorkerMsg final {
public:
    ipx_msg_ipfix_t *ipfix_msg;
    const fds_iemgr_t *iemgr;

    WorkerMsg(ipx_msg_ipfix_t *ipfix_msg, const fds_iemgr_t *iemgr) {
        this->ipfix_msg = ipfix_msg;
        this->iemgr = iemgr;
    }
};

/**
 * wrapper for conversion buffer
 */
class ProcessMsgBuffer final {
public:
    char *buffer;
    size_t size;

    ProcessMsgBuffer(size_t size) {
        this->buffer = new char[size];
        this->size = size;
    }

    ProcessMsgBuffer(const ProcessMsgBuffer &ins) {
        this->size = ins.size;
        this->buffer = new char[size];
    }

    ~ProcessMsgBuffer() {
        delete[] buffer;
    }
};

class Worker final {
private:
    //input buffer for conversion
    std::unique_ptr<std::unique_ptr<WorkerMsg>[]> msgs;
    //buffer for conversion
    std::unique_ptr<std::unique_ptr<ProcessMsgBuffer>[]> processMsgsBuffer;

    //current index for add message
    std::atomic_uint32_t indexAdd;
    //current index for process message
    std::atomic_uint32_t indexProcess;

    //due to ring buffer
    std::atomic_bool restartIndexAdd;

    //settings json format for libfds (fds_drec2json)
    uint32_t flags;

    uint32_t workerThreadsCount;

    std::thread *workerThreads;

    std::mutex workerMtx;
    //lock for critical section "addMsg" and "work"
    std::mutex lck;
    //lock for sender
    std::mutex lckSend;

    std::mutex inputMtx;

    std::atomic_bool isPluginRunning;
    std::atomic_bool isKafkaProducerConnected;

    std::condition_variable workerCV;
    std::condition_variable inputCV;

    std::shared_ptr<ConfigFormat> configFormat;
    std::shared_ptr<ConfigKafka> configKafka;
    std::shared_ptr<ConfigProcessing> configProcessing;
    std::unique_ptr<KafkaProducer> kafkaProducer;

    /**
     * Select message for conversion and starts conversion, if input buffer
     * is empty wait
     *
     * @param[in] processMsgBuffer converted msg buffer for thread instance
     * @param[in] processMsgBufferSize size msg buffer for thread instance
     */
    void work(char *processMsgBuffer, size_t *processMsgBufferSize);

    /**
     * Splits message to records and after full process delete workerMsg
     *
     * @param[in] workerMsg message for conversion
     * @param[in] msgBuffer buffer for conversion
     * @param[in] convMsgBufferSize size of buffer for conversion
     * @param[in] indexCurrentMsg current index message for conversion (later
     * save to output buffer)
     * @return state code
     */
    int
    processMessage(std::unique_ptr<WorkerMsg> workerMsg, char *msgBuffer,
                   size_t *convMsgBufferSize, uint32_t indexCurrentMsg);

    /**
     * Conversions single records and save it
     *
     * @param rec[in] record for conversion
     * @param iemgr[in] Information element manager
     * @param processMsgBuffer[in, out] buffer for conversion record
     * @param processMsgBufferSize[in, out] size of buffer for conversion record
     * @param convertedMsg[in] final converted message
     * @return number of chars in procesMsgBuffer
     */
    uint32_t convertMessage(fds_drec *rec, const fds_iemgr_t *iemgr, char *
    processMsgBuffer,
                            size_t *processMsgBufferSize);

    /**
     * Init due to smart pointer
     */
    void init();

public:
    /**
     * \brief Constructor
     *
     * @param[in] configFormat configuration of the result JSON message
     * @param[in] configKafka configuration kafka producent
     * @param[in] configProcessing configuration plugin
     */
    Worker(std::shared_ptr<ConfigFormat> configFormat,
           std::shared_ptr<ConfigKafka> configKafka,
           std::shared_ptr<ConfigProcessing> configProcessing);

/**
 * \brief Copy constructor
 * @param w instance Worker
 */
    Worker(const Worker &w);

    /**
     * \brief Add msg to input buffer
     *
     * Add msg to input buffer. If input buffer is full, msg rewrite old msg.
     * @param[in] msg Message for conversion to json
     */
    void addMsg(std::unique_ptr<WorkerMsg> msg);

    /**
     * \brief Start plugin
     *
     * Fill worker threads pool, create sender thread and connect kafka producer
     * module
     */
    void start();

    /**
     * \brief Stop plugin
     *  Stop worker threads pool and sender thread
     */
    void stop();

    /**
     * \brief Destructor
     */
    ~Worker();
};

#endif // CONFIG_H
