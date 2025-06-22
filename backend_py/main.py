from fastapi import FastAPI, Request, Form
from fastapi.responses import HTMLResponse, RedirectResponse
from fastapi.templating import Jinja2Templates
from fastapi.staticfiles import StaticFiles
import grpc
import my_service_pb2, my_service_pb2_grpc
import time
import uvicorn

app = FastAPI()
templates = Jinja2Templates(directory="templates")

# gRPC stub setup
channel = grpc.insecure_channel('localhost:50051')
stub = my_service_pb2_grpc.OrderBookServiceStub(channel)

app.mount("/static", StaticFiles(directory="static"), name="static")

@app.get("/", response_class=HTMLResponse)
def index(request: Request):
    # Remove or comment out these lines:
    # recent_orders = get_recent_orders()  # Implement this
    # chart_data = get_chart_data()        # Implement this
    return templates.TemplateResponse("index.html", {
        "request": request
        # Remove recent_orders and chart_data from context for now
    })

@app.post("/add_order", response_class=HTMLResponse)
def add_order(
    request: Request,
    price: float = Form(...),
    quantity: int = Form(...),
    side: str = Form(...),
    order_type: str = Form(...),
):
    order = my_service_pb2.OrderRequest(
        order_id=int(time.time()),  # simple unique id
        price=price,
        quantity=quantity,
        side=(side == "buy"),
        timestamp=int(time.time()),
        order_type=order_type
    )
    resp = stub.AddOrder(order)
    return templates.TemplateResponse("index.html", {"request": request, "message": f"Order added: {resp}"})

@app.get("/best_prices", response_class=HTMLResponse)
def best_prices(request: Request):
    try:
        best_bid = stub.GetBestBid(my_service_pb2.Empty())
        best_ask = stub.GetBestAsk(my_service_pb2.Empty())
    except Exception as e:
        best_bid = best_ask = None
    return templates.TemplateResponse("index.html", {
        "request": request,
        "best_bid": best_bid,
        "best_ask": best_ask
    })

@app.get("/best_prices_json")
def best_prices_json():
    try:
        best_bid = stub.GetBestBid(my_service_pb2.Empty())
        best_ask = stub.GetBestAsk(my_service_pb2.Empty())
        print(f"Best bid: {best_bid}")
        print(f"Best ask: {best_ask}")
        return {
            "best_bid": {
                "order_id": best_bid.order_id,
                "price": best_bid.price,
                "quantity": best_bid.quantity,
                "side": best_bid.side,
                "timestamp": best_bid.timestamp,
                "order_type": best_bid.order_type
            } if best_bid.order_id else None,
            "best_ask": {
                "order_id": best_ask.order_id,
                "price": best_ask.price,
                "quantity": best_ask.quantity,
                "side": best_ask.side,
                "timestamp": best_ask.timestamp,
                "order_type": best_ask.order_type
            } if best_ask.order_id else None
        }
    except Exception as e:
        print(f"Error in best_prices_json: {e}")
        return {"best_bid": None, "best_ask": None}

@app.get("/orderbook")
def get_orderbook():
    try:
        ob = stub.GetOrderBook(my_service_pb2.Empty())
        bids = [{"price": o.price, "quantity": o.quantity} for o in ob.bids]
        asks = [{"price": o.price, "quantity": o.quantity} for o in ob.asks]
        return {"bids": bids, "asks": asks}
    except Exception as e:
        return {"bids": [], "asks": []}

@app.get("/trades")
def get_trades():
    trades = stub.GetTradeLog(my_service_pb2.Empty())
    return [
        {
            "timestamp": t.timestamp,
            "price": t.price,
            "quantity": t.quantity,
            "side": t.side
        }
        for t in trades
    ]

if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=8000)