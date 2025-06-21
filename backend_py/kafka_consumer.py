#kafka_consumer.py
'''
!!!!! ARCHAIC !!!!!
Script is not necessary cuz  the project is using a gRCP-centric approach and not a
Kafka-centric approach
if you run it with the dynamic client and server it will run and keep waiting for
messages but will receive none
no orders are sent to  order-requests topics cuz order-updates woould have consumed
them
but i will keep this file in case it may be needed
'''
import json
from confluent_kafka import Consumer
import grpc
import my_service_pb2
import my_service_pb2_grpc

# Set up Kafka consumer
c = Consumer({
    'bootstrap.servers': 'localhost:9092',
    'group.id': 'kafka-consumer',
    'auto.offset.reset': 'earliest'
})
c.subscribe(['order-requests'])

# Set up gRPC channel and stub
channel = grpc.insecure_channel('localhost:50051')
stub = my_service_pb2_grpc.OrderBookServiceStub(channel)

print("Waiting for order messages...")
while True:
    msg = c.poll(1.0)
    if msg is None:
        continue
    if msg.error():
        print("Consumer error:", msg.error())
        continue
    raw_order = json.loads(msg.value())
    print(raw_order)
    # Build gRPC request
    
    request = my_service_pb2.OrderRequest(
        order_id=raw_order['order_id'],
        price=raw_order['price'],
        quantity=raw_order['quantity'],
        side=raw_order['side'],
        timestamp=raw_order['timestamp'],
        order_type=raw_order.get('order_type', 'LIMIT')
    )
    
    # Call gRPC AddOrder
    response = stub.AddOrder(request)
    print("Order added via gRPC:", response)