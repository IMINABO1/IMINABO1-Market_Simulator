#best_price
import json
from confluent_kafka import Consumer, KafkaError

# Create a single consumer that subscribes to both topics
c = Consumer({
    'bootstrap.servers': 'localhost:9092',
    'group.id': 'best-price-watcher',
    'auto.offset.reset': 'earliest'
})
c.subscribe(['best-bid-updates', 'best-ask-updates'])

print("Listening for best‐price updates…")
try:
    while True:
        msg = c.poll(1.0)
        if msg is None:
            continue
        if msg.error():
            # If it’s a serious error, print it
            if msg.error().code() != KafkaError._PARTITION_EOF:
                print("Consumer error:", msg.error())
            continue

        topic = msg.topic()
        payload = json.loads(msg.value().decode('utf-8'))

        if topic == 'best-bid-updates':
            print(f"[BEST BID]  price={payload['price']}  qty={payload['quantity']}  ts={payload['timestamp']}")
        else:  # best-ask-updates
            print(f"[BEST ASK]  price={payload['price']}  qty={payload['quantity']}  ts={payload['timestamp']}")
finally:
    c.close()
