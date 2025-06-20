from confluent_kafka import Producer
import json

p = Producer({'bootstrap.servers': 'localhost:9092'})
order = {
    "order_id": 1,
    "price": 100.5,
    "quantity": 10,
    "side": True,  # True=buy, False=sell
    "timestamp": 1750388329,
    "order_type": "LIMIT"
}
p.produce('order-requests', json.dumps(order).encode('utf-8'))
p.flush()
print("Order produced!")