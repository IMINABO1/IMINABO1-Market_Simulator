from confluent_kafka import Producer
import time
import json

p = Producer({'bootstrap.servers': 'localhost:9092'})
order1 = {
    "order_id": 1,
    "price": 100.5,
    "quantity": 10,
    "side": True,  # True=buy, False=sell
    "timestamp": 1750388329,
    "order_type": "LIMIT"
}

order2 = {
    "order_id": 2,
    "price": 18.5,
    "quantity": 27,
    "side": False,  # True=buy, False=sell
    "timestamp": int(time.time()),
    "order_type": "LIMIT"
}

messages = [order1, order2]
for mssg in messages:
    p.produce('order-requests', json.dumps(mssg).encode('utf-8'))
p.flush()
print("Order produced!")