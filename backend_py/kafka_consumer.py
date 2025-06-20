import json
from confluent_kafka import Consumer
import grpc
import my_service_pb2
import my_service_pb2_grpc

# Set up Kafka consumer
c = Consumer({
    'bootstrap.servers': 'localhost:9092',
    'group.id': 'orderbook-updater',
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
    order = json.loads(msg.value())
    # Build gRPC request
    request = my_service_pb2.OrderRequest(
        order_id=order['order_id'],
        price=order['price'],
        quantity=order['quantity'],
        side=order['side'],
        timestamp=order['timestamp'],
        order_type=order.get('order_type', 'LIMIT')
    )
    # Call gRPC AddOrder
    response = stub.AddOrder(request)
    print("Order added via gRPC:", response)