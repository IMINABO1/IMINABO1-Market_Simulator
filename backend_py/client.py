import grpc
import my_service_pb2
import my_service_pb2_grpc

def run():
    # Connect to the server
    with grpc.insecure_channel('localhost:50051') as channel:
        stub = my_service_pb2_grpc.OrderBookStub(channel)
        
        # Create a request (make sure symbol matches what the server expects)
        request = my_service_pb2.BidRequest(symbol="AAPL")
        
        # Call GetBestBid and get the response
        response = stub.GetBestBid(request)
        
        print(f"Best Bid: {response.price}, Quantity: {response.quantity}")

if __name__ == '__main__':
    run()
