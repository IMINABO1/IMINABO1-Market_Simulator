# best_price.py
from confluent_kafka import Consumer
import json

c = Consumer({
    'bootstrap.servers': 'localhost:9092',
    'group.id': 'best-price-reader',
    'auto.offset.reset': 'earliest'
})
c.subscribe(['best-bid-updates','best-ask-updates'])

print("🎯 Listening for best‐price updates…")
while True:
    msg = c.poll(1.0)
    if msg is None:   continue
    if msg.error():
        print("Error:", msg.error())
        continue

    data = json.loads(msg.value())
    if msg.topic() == 'best-bid-updates':
        print(f"📈 New best‐bid → {data}")
    else:
        print(f"📉 New best‐ask → {data}")
