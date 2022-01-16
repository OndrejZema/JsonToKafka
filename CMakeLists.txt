# Create a linkable module
add_library(json-to-kafka-output MODULE
        JsonToKafka.cpp
    Worker.cpp
    Worker.h
    Config.cpp
    Config.h
    KafkaProducer.cpp
    KafkaProducer.h
    Logger.h

)
find_package(LibRDKafka 0.9.3 REQUIRED)

include_directories(${LIBRDKAFKA_INCLUDE_DIRS})

target_link_libraries(json-to-kafka-output ${LIBRDKAFKA_LIBRARIES})

install(
    TARGETS json-to-kafka-output
    LIBRARY DESTINATION "${INSTALL_DIR_LIB}/ipfixcol2/"
)
