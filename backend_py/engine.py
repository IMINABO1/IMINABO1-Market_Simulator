#engine.py
'''
!!!!! ARCHAIC !!!!!
Script is not necessary cuz  the project is using a gRCP-centric approach and not a
Kafka-centric approach
if you run it with the dynamic client and server it will run and keep waiting for
messages but will receive none
no orders are sent to  order-requests topics cuz order-updates would have consumed
them
but i will keep this file in case it may be needed
'''
import json
from confluent_kafka import Consumer, Producer, KafkaError
import orderbook_cpp

# kafka setup
consumer = Consumer({
    'bootstrap.servers': 'localhost:9092',
    'group.id': 'orderbook-engine',
    'auto.offset.reset': 'earliest'
})
consumer.subscribe(['order-requests'])

producer = Producer({'bootstrap.servers': 'localhost:9092'})

# in‐process C++ orderbook
book = orderbook_cpp.OrderBook()

print("ENGINE ▶ listening for new orders…")
while True:
    msg = consumer.poll(1.0)
    if msg is None:
        continue
    if msg.error():
        if msg.error().code() != KafkaError._PARTITION_EOF:
            print("Engine consumer error:", msg.error())
        continue

    raw = json.loads(msg.value().decode('utf-8'))
    # map into C++ Order
    o = orderbook_cpp.Order(
        raw['order_id'],
        raw['price'],
        raw['quantity'],
        raw['side'],
        raw['timestamp'],
        getattr(orderbook_cpp.OrderType, raw.get('order_type', 'LIMIT'))
    )

    # 1) match & emit trades
    trades = book.add_order(o)
    for t in trades:
        ts = int(t.timestamp.total_seconds())
        trade_msg = json.dumps({
            "buy_order_id":  t.buy_order_id,
            "sell_order_id": t.sell_order_id,
            "price":         t.price,
            "quantity":      t.quantity,
            "timestamp":     ts
        })
        producer.produce('order-updates', trade_msg)

    # 2) emit new best bid / ask
    try:
        bid = book.get_best_bid()
        producer.produce('best-bid-updates', json.dumps({
            "price":    bid.get_price(),
            "quantity": bid.get_qty(),
            "timestamp": bid.get_timestamp()
        }))
    except RuntimeError:
        pass

    try:
        ask = book.get_best_ask()
        producer.produce('best-ask-updates', json.dumps({
            "price":    ask.get_price(),
            "quantity": ask.get_qty(),
            "timestamp": ask.get_timestamp()
        }))
    except RuntimeError:
        pass

    producer.flush()
