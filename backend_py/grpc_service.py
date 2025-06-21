#grpc_service
import time, json
from concurrent import futures

import grpc
from confluent_kafka import Producer
import my_service_pb2
import my_service_pb2_grpc

import orderbook_cpp  # your pybind11 C++ extension

class OrderBookService(my_service_pb2_grpc.OrderBookServiceServicer):
    def __init__(self):
        self.bids = []
        self.asks = []
        self.book = orderbook_cpp.OrderBook()
        self.kafka_producer = Producer({"bootstrap.servers":"localhost:9092"})

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
        trades = self.book.add_order(o)

        # Track in Python for visualization
        order_level = {"price": req.price, "quantity": req.quantity}
        if req.side:
            self.bids.append(order_level)
        else:
            self.asks.append(order_level)

        for t in trades:
            ts = int(t.timestamp.total_seconds())
            msg = {
              "buy_order_id":  t.buy_order_id,
              "sell_order_id": t.sell_order_id,
              "price":         t.price,
              "quantity":      t.quantity,
              "timestamp":     ts
            }
            self.kafka_producer.produce(
               "order-updates",
               key=str(t.buy_order_id),
               value=json.dumps(msg)
            )
        self.kafka_producer.flush()

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

        self.kafka_producer.produce(
            "best-bid-updates",
            key="best_bid",
            value=json.dumps({
                "price": best.get_price(),
                "quantity": best.get_qty(),
                "timestamp": best.get_timestamp()
            })
            )
        self.kafka_producer.flush()

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

        self.kafka_producer.produce(
            "best-ask-updates",
            key="best_ask",
            value=json.dumps({
                "price": best.get_price(),
                "quantity": best.get_qty(),
                "timestamp": best.get_timestamp()
            })
            )
        self.kafka_producer.flush()

        return my_service_pb2.OrderResponse(
            order_id   = best.get_order_id(),
            price      = best.get_price(),
            quantity   = best.get_qty(),
            side       = best.get_side(),
            timestamp  = best.get_timestamp(),
            order_type = best.get_order_type().name
        )

    def GetOrderBook(self, req, ctx):
        # Return Python-tracked bids and asks
        return my_service_pb2.OrderBookResponse(
            bids=[my_service_pb2.OrderBookLevel(**b) for b in self.bids],
            asks=[my_service_pb2.OrderBookLevel(**a) for a in self.asks]
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
