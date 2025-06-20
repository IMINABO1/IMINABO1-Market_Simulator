#include <librdkafka/rdkafka.h>
#include <iostream>
#include <cstring>

int main() {
    char errstr[512];

    // Create Kafka producer config
    rd_kafka_conf_t* conf = rd_kafka_conf_new();

    // Set the broker address
    if (rd_kafka_conf_set(conf, "bootstrap.servers", "localhost:9092", errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
        std::cerr << "Error setting broker: " << errstr << std::endl;
        return 1;
    }

    // Create producer instance
    rd_kafka_t* producer = rd_kafka_new(RD_KAFKA_PRODUCER, conf, errstr, sizeof(errstr));
    if (!producer) {
        std::cerr << "Failed to create producer: " << errstr << std::endl;
        return 1;
    }

    std::string topic = "order-updates";
    std::string message = "Hello from pure librdkafka!";

    // Produce message
    if (rd_kafka_producev(
            producer,
            RD_KAFKA_V_TOPIC(topic.c_str()),
            RD_KAFKA_V_MSGFLAGS(RD_KAFKA_MSG_F_COPY),
            RD_KAFKA_V_VALUE(const_cast<char*>(message.c_str()), message.size()),
            RD_KAFKA_V_END) != 0) {
        std::cerr << "Produce failed: " << rd_kafka_err2str(rd_kafka_last_error()) << std::endl;
    } else {
        std::cout << "Message sent!" << std::endl;
    }

    // Wait for delivery
    rd_kafka_flush(producer, 5000);

    // Destroy producer
    rd_kafka_destroy(producer);

    return 0;
}