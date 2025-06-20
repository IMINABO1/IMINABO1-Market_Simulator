import grpc
from concurrent import futures
import time

import my_service_pb2
import my_service_pb2_grpc

# Implement the service defined in your .proto file
class OrderBookService(my_service_pb2_grpc.OrderBookServicer):
    def GetBestBid(self, request, context):
        # Your logic for getting the best bid goes here
        return my_service_pb2.BidResponse(price=100.0, quantity=10)  # Example response

# Server setup
def serve():
    server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
    my_service_pb2_grpc.add_OrderBookServicer_to_server(OrderBookService(), server)
    
    server.add_insecure_port('[::]:50051')
    print("Server started at [::]:50051")
    server.start()
    
    try:
        while True:
            time.sleep(86400)  # Keep the server running
    except KeyboardInterrupt:
        server.stop(0)

if __name__ == '__main__':
    serve()
