#dynamic_client
import time, random, itertools
import grpc
import my_service_pb2, my_service_pb2_grpc

def random_order_request(order_id):
    side       = random.choice([True, False])
    order_type = random.choice(['LIMIT', 'MARKET'])
    price      = 0.0 if order_type=='MARKET' else round(random.uniform(10,500),2)
    quantity   = random.randint(1,100)
    timestamp  = int(time.time())
    return my_service_pb2.OrderRequest(
        order_id   = order_id,
        price      = price,
        quantity   = quantity,
        side       = side,
        timestamp  = timestamp,
        order_type = order_type
    )

def main():
    channel = grpc.insecure_channel('localhost:50051')
    stub    = my_service_pb2_grpc.OrderBookServiceStub(channel)

    for oid in itertools.count(1):
        req  = random_order_request(oid)
        stub.AddOrder(req)
        print(f"[gRPC] sent #{oid} → {req.order_type}@${req.price}")

        # Add these lines:
        try:
            bid = stub.GetBestBid(my_service_pb2.Empty())
            print("Best Bid:", bid)
        except Exception as e:
            print("No best bid:", e)
        try:
            ask = stub.GetBestAsk(my_service_pb2.Empty())
            print("Best Ask:", ask)
        except Exception as e:
            print("No best ask:", e)

        time.sleep(random.uniform(0.1,4))

if __name__ == '__main__':
    main()
