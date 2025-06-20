from confluent_kafka import Consumer, KafkaException, KafkaError, Producer

# Kafka Producer setup
def create_producer():
    conf = {
        'bootstrap.servers': 'localhost:9092'
    }
    producer = Producer(**conf)
    return producer

def produce_message(producer, topic, message):
    producer.produce(topic, value=message.encode('utf-8'))
    producer.flush()  # Ensure the message is sent immediately

# Kafka Consumer setup
def create_consumer():
    conf = {
        'bootstrap.servers': 'localhost:9092',
        'group.id': 'order-book-group',
        'auto.offset.reset': 'earliest'  # Start from the earliest message
    }
    consumer = Consumer(conf)
    return consumer

def consume_message(consumer, topic):
    consumer.subscribe([topic])
    msg = consumer.poll(timeout=1.0)  # Wait for a message for up to 1 second
    
    if msg is None:
        print("No message received")
    elif msg.error():
        if msg.error().code() == KafkaError._PARTITION_EOF:
            print(f"End of partition reached: {msg.partition}, offset {msg.offset}")
        else:
            raise KafkaException(msg.error())
    else:
        print(f"Received message: {msg.value().decode('utf-8')}")
        # Implement logic to handle the message content
        
    consumer.commit()

def run_kafka_producer():
    producer = create_producer()
    topic = 'order-updates'
    # Produce a message to Kafka
    message = "This is my message"
    produce_message(producer, topic, message)
    print(f"Message produced to topic {topic}: {message}")

def run_kafka_consumer():
    consumer = create_consumer()
    try:
        while True:
            consume_message(consumer, 'order-updates')  # Specify your Kafka topic name
    except KeyboardInterrupt:
        print("Kafka consumer stopped")
    finally:
        consumer.close()

if __name__ == '__main__':
    # To produce a message
    run_kafka_producer()

    # To consume messages (separate execution or after producing)
    # run_kafka_consumer()
