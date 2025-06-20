import time, json
from concurrent import futures

import grpc
from confluent_kafka import Producer
import my_service_pb2
import my_service_pb2_grpc

import orderbook_cpp  # your pybind11 C++ extension

class OrderBookService(my_service_pb2_grpc.OrderBookServiceServicer):
    def __init__(self):
        self.book = orderbook_cpp.OrderBook()

    def AddOrder(self, req, ctx):
        # turn the protobuf into your C++ Order
        o = orderbook_cpp.Order(
            req.order_id,
            req.price,
            req.quantity,
            req.side,
            req.timestamp,
            getattr(orderbook_cpp.OrderType, req.order_type)
        )
        # call into C++
        _ = self.book.add_order(o)

        # echo the request back as the response
        return my_service_pb2.OrderResponse(
            order_id   = req.order_id,
            price      = req.price,
            quantity   = req.quantity,
            side       = req.side,
            timestamp  = req.timestamp,
            order_type = req.order_type
        )

    def GetBestBid(self, req, ctx):
        best = self.book.get_best_bid()
        return my_service_pb2.OrderResponse(
            order_id   = best.get_order_id(),
            price      = best.get_price(),
            quantity   = best.get_qty(),
            side       = best.get_side(),
            timestamp  = best.get_timestamp(),
            order_type = best.get_order_type().name
        )

    def GetBestAsk(self, req, ctx):
        best = self.book.get_best_ask()
        return my_service_pb2.OrderResponse(
            order_id   = best.get_order_id(),
            price      = best.get_price(),
            quantity   = best.get_qty(),
            side       = best.get_side(),
            timestamp  = best.get_timestamp(),
            order_type = best.get_order_type().name
        )

def serve():
    server = grpc.server(futures.ThreadPoolExecutor(max_workers=4))
    my_service_pb2_grpc.add_OrderBookServiceServicer_to_server(
        OrderBookService(), server
    )
    server.add_insecure_port('[::]:50051')
    print("gRPC listening on 0.0.0.0:50051")
    server.start()
    try:
        while True:
            time.sleep(86400)
    except KeyboardInterrupt:
        server.stop(0)

if __name__ == "__main__":
    serve()
