import time
import grpc

import my_service_pb2
import my_service_pb2_grpc

def run():
    channel = grpc.insecure_channel('localhost:50051')
    stub    = my_service_pb2_grpc.OrderBookServiceStub(channel)

    # 1) Add an order:
    buy_req1 = my_service_pb2.OrderRequest(
        order_id   = 1,
        price      = 100.5,
        quantity   = 10,
        side       = True,            # buy
        timestamp  = int(time.time()),
        order_type = "LIMIT"
    )
    buy_resp1 = stub.AddOrder(buy_req1)
    print("AddOrder Buy 1→", buy_resp1)

    buy_req2 = my_service_pb2.OrderRequest(
        order_id   = 2,
        price      = 100.5,
        quantity   = 10,
        side       = True,            # buy
        timestamp  = int(time.time()),
        order_type = "LIMIT"
    )
    buy_resp2 = stub.AddOrder(buy_req2)
    print("AddOrder Buy 2→", buy_resp2)

    # Add sell order
    #there needs to be at least one sell order or else bestbid will throw error
    # that is "No sell order available"

    sell_req1 = my_service_pb2.OrderRequest(
        order_id   = 3,
        price      = 99.5,
        quantity   =  5,
        side       = False,     # sell
        timestamp  = int(time.time()),
        order_type = "LIMIT"
    )
    sell_resp1 = stub.AddOrder(sell_req1)
    print("AddOrder Sell 1 →", sell_resp1)

    sell_req2 = my_service_pb2.OrderRequest(
        order_id = 4,
        price = 1400.5,
        quantity = 10,
        side = False, #sell
        timestamp = int(1750436055.9389596),
        order_type = "LIMIT"
    )
    sell_resp2 = stub.AddOrder(sell_req2)
    print("AddOrder Sell 2→", sell_resp2)

    sell_req3 = my_service_pb2.OrderRequest(
        order_id = 5,
        price = 348.5,
        quantity = 8,
        side = False, #sell
        timestamp = int(time.time()),
        order_type = "LIMIT"
    )
    sell_resp3 = stub.AddOrder(sell_req3)
    print("AddOrder Sell 3→", sell_resp3)

    # 2) Fetch best bid:
    best_bid = stub.GetBestBid(my_service_pb2.Empty())
    print("GetBestBid →", best_bid)

    # 3) Fetch best ask:
    best_ask = stub.GetBestAsk(my_service_pb2.Empty())
    print("GetBestAsk →", best_ask)

if __name__ == "__main__":
    run()
