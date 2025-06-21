from confluent_kafka import Consumer
import json

c = Consumer({
    'bootstrap.servers': 'localhost:9092',
    'group.id': 'trade-listener',
    'auto.offset.reset': 'earliest'
})
c.subscribe(['order-updates'])  # or ['best-bid-updates','best-ask-updates']

print("Listening for trades…")
while True:
    msg = c.poll(1.0)
    if not msg: continue
    if msg.error():
        print("Error:", msg.error())
        continue

    trade = json.loads(msg.value())
    print(">> New trade:", trade)
