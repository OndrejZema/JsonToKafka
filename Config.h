#ifndef CONFIG_H
#define CONFIG_H

#include <ipfixcol2.h>
#include <cstdint>
#include <memory>

#include <string>
#include <sstream>

/** XML nodes in configuration file*/
enum params_xml_nodes {
    // Formatting parameters
    FMT_TFLAGS,      /**< TCP flags                                          */
    FMT_TIMESTAMP,   /**< Timestamp                                          */
    FMT_PROTO,       /**< Protocol                                           */
    FMT_UNKNOWN,     /**< Unknown definitions                                */
    FMT_OPTIONS,     /**< Ignore Options Template Records                    */
    FMT_NONPRINT,    /**< Non-printable chars                                */
    FMT_OCTETASUINT, /**< OctetArray as unsigned integer                     */
    FMT_NUMERIC,     /**< Use numeric names                                  */
    FMT_BFSPLIT,     /**< Split biflow                                       */
    KAFKA,              /**< Apache Kafka config node                        */
    KAFKA_HOST_NAME,    /**< Apache Kafka host name                          */
    KAFKA_PORT,         /**< Apache Kafka port                               */
    KAFKA_TOPIC_LIST,   /**< Apache kafka topic list                         */
    PROCESSING,                         /**< Procesing node                  */
    PROCESSING_PROCESS_MESSAGE_LENGTH,  /**< message buffer size             */
    PROCESSING_MESSAGES_BUFFER_SIZE,    /**< input / output buffer size      */
    PROCESSING_LOGGER_CONFIG_FILE,            /**< path for log config file  */

};
/**
 * \brief Configuration for JSON output format
 * All values for configuration JSON format
 */
struct ConfigFormat {
    /** TCP flags format - true (formatted), false (raw)                       */
    bool tcp_flags;
    /** Timestamp format - true (formatted), false (UNIX)                      */
    bool timestamp;
    /** Protocol format  - true (formatted), false (raw)                       */
    bool proto;
    /** Skip unknown elements                                                  */
    bool ignore_unknown;
    /** Converter octetArray type as unsigned integer (only if field size <= 8)*/
    bool octets_as_uint;
    /** Convert white spaces in string (do not skip)                           */
    bool white_spaces;
    /** Ignore Options Template records                                        */
    bool ignore_options;
    /** Use only numeric identifiers of Information Elements                   */
    bool numeric_names;
    /** Split biflow records                                                   */
    bool split_biflow;
};
/**
 * \brief Configuration for kafka producent
 * All values for configuration kafka producent
 */
struct ConfigKafka {
    /** apache kafka host name      */
    std::string hostName;
    /** apache kafka port           */
    std::string port;
    /** apache kafka topic list     */
    std::string topicList;
};
/**
 * \brief Configuration for plugin process pipeline
 *  All values for configuration plugin process pipeline
 */
struct ConfigProcessing {
    /** message length for process  minimum 256   */
    uint32_t processMessageLength;
    /** input / output buffer size  minimum 256   */
    uint32_t messagesBufferSize;
    /** path for log file*/
    std::string loggerConfigFile;
};
/** Definition of the \<kafka>\*/
static const struct fds_xml_args args_kafka[] = {
        FDS_OPTS_ELEM(KAFKA_HOST_NAME, "hostName", FDS_OPTS_T_STRING,
                      FDS_OPTS_P_OPT),
        FDS_OPTS_ELEM(KAFKA_PORT, "port", FDS_OPTS_T_STRING,
                      FDS_OPTS_P_OPT),
        FDS_OPTS_ELEM(KAFKA_TOPIC_LIST, "topicList", FDS_OPTS_T_STRING,
                      FDS_OPTS_P_OPT),
        FDS_OPTS_END};
/** Definition of the \<processing>\*/
static const struct fds_xml_args args_processing[] = {
        FDS_OPTS_ELEM(PROCESSING_PROCESS_MESSAGE_LENGTH,
                      "processMessageLength",
                      FDS_OPTS_T_INT, FDS_OPTS_P_OPT),
        FDS_OPTS_ELEM(PROCESSING_MESSAGES_BUFFER_SIZE, "messagesBufferSize",
                      FDS_OPTS_T_INT, FDS_OPTS_P_OPT),
        FDS_OPTS_ELEM(PROCESSING_LOGGER_CONFIG_FILE, "loggerConfigFile",
                      FDS_OPTS_T_STRING, FDS_OPTS_P_OPT),
        FDS_OPTS_END};
/** Definition of the \<params>\*/
static const struct fds_xml_args args_params[] = {
        FDS_OPTS_ROOT("params"),
        FDS_OPTS_ELEM(FMT_TFLAGS, "tcpFlags", FDS_OPTS_T_STRING,
                      FDS_OPTS_P_OPT),
        FDS_OPTS_ELEM(FMT_TIMESTAMP, "timestamp", FDS_OPTS_T_STRING,
                      FDS_OPTS_P_OPT),
        FDS_OPTS_ELEM(FMT_PROTO, "protocol", FDS_OPTS_T_STRING,
                      FDS_OPTS_P_OPT),
        FDS_OPTS_ELEM(FMT_UNKNOWN, "ignoreUnknown", FDS_OPTS_T_BOOL,
                      FDS_OPTS_P_OPT),
        FDS_OPTS_ELEM(FMT_OPTIONS, "ignoreOptions", FDS_OPTS_T_BOOL,
                      FDS_OPTS_P_OPT),
        FDS_OPTS_ELEM(FMT_NONPRINT, "nonPrintableChar", FDS_OPTS_T_BOOL,
                      FDS_OPTS_P_OPT),
        FDS_OPTS_ELEM(FMT_NUMERIC, "numericNames", FDS_OPTS_T_BOOL,
                      FDS_OPTS_P_OPT),
        FDS_OPTS_ELEM(FMT_OCTETASUINT, "octetArrayAsUint", FDS_OPTS_T_BOOL,
                      FDS_OPTS_P_OPT),
        FDS_OPTS_ELEM(FMT_BFSPLIT, "splitBiflow", FDS_OPTS_T_BOOL,
                      FDS_OPTS_P_OPT),
        FDS_OPTS_NESTED(KAFKA, "kafka", args_kafka, FDS_OPTS_P_OPT),
        FDS_OPTS_NESTED(PROCESSING, "processing", args_processing,
                        FDS_OPTS_P_OPT),
        FDS_OPTS_END};

/**
 * \brief Wrapper class for all plugin configuration values
 */
class Config {
private:
    /**
     * \brief Set default values
     */
    void setDefaultConfig();

    /**
     *\brief Parse "format" parameters
     * @param params[in]
     * @throw invalid_argument or runtime_error
     */
    void parseParams(fds_xml_ctx_t *params);

    /**
     * \brief Parse "kafka" parameters
     * @param kafka[in]
     * @throw invalid_argument or runtime_error
     */
    void parseKafka(fds_xml_ctx_t *kafka);

    /**
     * \brief Parse "procesing" parameters
     * @param parser[in]
     * @throw invalid_argument or runtime_error
     */
    void parseProcessing(fds_xml_ctx_t *parser);

    bool check_or(const std::string &elem, const char *value,
                  const std::string &val_true,
                  const std::string &val_false);

    std::shared_ptr<ConfigFormat> configFormat;
    std::shared_ptr<ConfigKafka> configKafka;
    std::shared_ptr<ConfigProcessing> configProcessing;

public:
    /**
     *
     * @return configuration kafka
     */
    std::shared_ptr<ConfigKafka> getConfigKafka() {
        return configKafka;
    }

    /**
     *
     * @return configuration format
     */
    std::shared_ptr<ConfigFormat> getConfigFormat() {
        return configFormat;
    }

    /**
     *
     * @return configuration processing (plugin)
     */
    std::shared_ptr<ConfigProcessing> getConfigProcessing() {
        return configProcessing;
    }

    /**
     * \brief Constructor
     * Create new configuration
     * @param[in] params XML configuration
     */
    Config(const char *params);

    /**
     * \brief Destructor
     */
    ~Config();
};

#endif // CONFIG_H
