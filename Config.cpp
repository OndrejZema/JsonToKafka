#include <cstdlib>
#include <limits.h>
#include "Config.h"

#include <memory>

Config::Config(const char *params) {
    configFormat = std::make_shared<ConfigFormat>(ConfigFormat());
    configKafka = std::make_shared<ConfigKafka>(ConfigKafka());
    configProcessing = std::make_shared<ConfigProcessing>(
            ConfigProcessing());
    setDefaultConfig();

    std::unique_ptr<fds_xml_t, decltype(&fds_xml_destroy)> xml(
            fds_xml_create(),
            &fds_xml_destroy);
    if (!xml) {
        throw std::runtime_error("Failed to create an XML parser!");
    }
    if (fds_xml_set_args(xml.get(), args_params) != FDS_OK) {
        throw std::runtime_error(
                "Failed to parse the description of an XML document!");
    }
    fds_xml_ctx_t *params_ctx = fds_xml_parse_mem(xml.get(), params, true);
    if (!params_ctx) {
        std::string err = fds_xml_last_err(xml.get());
        throw std::runtime_error(
                "Failed to parse the configuration: " + err);
    }
    parseParams(params_ctx);
}

Config::~Config() {
}

void Config::setDefaultConfig() {
    configFormat->proto = true;
    configFormat->tcp_flags = true;
    configFormat->timestamp = true;
    configFormat->white_spaces = true;
    configFormat->ignore_unknown = true;
    configFormat->ignore_options = true;
    configFormat->octets_as_uint = true;
    configFormat->numeric_names = false;
    configFormat->split_biflow = false;

    configKafka->hostName = "127.0.0.1";
    configKafka->port = "9092";
    configKafka->topicList = "netflow";

    configProcessing->processMessageLength = 1024;
    configProcessing->messagesBufferSize = 1024;
    configProcessing->loggerConfigFile = getenv("HOME") +
                                    std::string(
                                            "/ipfixcol2jsontokafka.conf");
}

void Config::parseParams(fds_xml_ctx_t *params) {
    const struct fds_xml_cont *content;
    while (fds_xml_next(params, &content) != FDS_EOC) {
        switch (content->id) {
            case FMT_TFLAGS: // Format TCP flags
                //assert(content->type == FDS_OPTS_T_STRING);
                configFormat->tcp_flags = check_or("tcpFlags",
                                                   content->ptr_string,
                                                   "formatted", "raw");
                break;
            case FMT_TIMESTAMP: // Format timestamp
                //assert(content->type == FDS_OPTS_T_STRING);
                configFormat->timestamp = check_or("timestamp",
                                                   content->ptr_string,
                                                   "formatted", "unix");
                break;
            case FMT_PROTO: // Format protocols
                //assert(content->type == FDS_OPTS_T_STRING);
                configFormat->proto = check_or("protocol",
                                               content->ptr_string,
                                               "formatted", "raw");
                break;
            case FMT_UNKNOWN: // Ignore unknown
                //assert(content->type == FDS_OPTS_T_BOOL);
                configFormat->ignore_unknown = content->val_bool;
                break;
            case FMT_OPTIONS: // Ignore Options Template records
                //assert(content->type == FDS_OPTS_T_BOOL);
                configFormat->ignore_options = content->val_bool;
                break;
            case FMT_NONPRINT: // Print non-printable characters
                //assert(content->type == FDS_OPTS_T_BOOL);
                configFormat->white_spaces = content->val_bool;
                break;
            case FMT_NUMERIC: // Use only numeric identifiers
                //assert(content->type == FDS_OPTS_T_BOOL);
                configFormat->numeric_names = content->val_bool;
                break;
            case FMT_OCTETASUINT:
                //assert(content->type == FDS_OPTS_T_BOOL);
                configFormat->octets_as_uint = content->val_bool;
                break;
            case FMT_BFSPLIT: // Split biflow records
                //assert(content->type == FDS_OPTS_T_BOOL);
                configFormat->split_biflow = content->val_bool;
                break;
            case KAFKA:
                parseKafka(content->ptr_ctx);
                break;
            case PROCESSING:
                parseProcessing(content->ptr_ctx);
                break;
            default:
                throw std::invalid_argument(
                        "Unexpected element within <params>!");
        }
    }
}

void Config::parseKafka(fds_xml_ctx_t *kafka) {
    const fds_xml_cont *content;
    while (fds_xml_next(kafka, &content) != FDS_EOC) {
        switch (content->id) {
            case KAFKA_HOST_NAME:
                //assert(content->type == FDS_OPTS_T_STRING);
                configKafka->hostName = content->ptr_string;
                break;
            case KAFKA_PORT:
                //assert(content->type == FDS_OPTS_T_STRING);
                configKafka->port = content->ptr_string;
                break;
            case KAFKA_TOPIC_LIST:
                //assert(content->type == FDS_OPTS_T_STRING);
                configKafka->topicList = content->ptr_string;
                break;
            default:
                throw std::invalid_argument(
                        "Unexpected element within <kafka>!");
        }
    }
}

void Config::parseProcessing(fds_xml_ctx_t *processing) {
    const fds_xml_cont *content;
    while (fds_xml_next(processing, &content) != FDS_EOC) {
        switch (content->id) {
            case PROCESSING_PROCESS_MESSAGE_LENGTH:
                configProcessing->processMessageLength = content->val_int;
                if(configProcessing->processMessageLength < 256){
                    configProcessing->processMessageLength = 256;
                }
                break;
            case PROCESSING_MESSAGES_BUFFER_SIZE:
                configProcessing->messagesBufferSize = content->val_int;
                if(configProcessing->messagesBufferSize < 256){
                    configProcessing->messagesBufferSize = 256;
                }
                break;
            case PROCESSING_LOGGER_CONFIG_FILE:
                configProcessing->loggerConfigFile = content->ptr_string;
                break;
            default:
                throw std::invalid_argument(
                        "Unexpected element within <parser>!");
        }
    }
}

bool Config::check_or(const std::string &elem, const char *value,
                      const std::string &val_true,
                      const std::string &val_false) {
    if (strcasecmp(value, val_true.c_str()) == 0) {
        return true;
    }

    if (strcasecmp(value, val_false.c_str()) == 0) {
        return false;
    }

    // Error
    throw std::invalid_argument(
            "Unexpected parameter of the element <" + elem +
            "> (expected '" + val_true + "' or '" + val_false + "')");
}
