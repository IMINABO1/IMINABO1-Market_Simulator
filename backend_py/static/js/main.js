let priceHistory = []; // Array of {time, bestBid, bestAsk}
const MAX_HISTORY = 100; // Show last 100 points

async function fetchBestPrices() {
    const res = await fetch('/best_prices_json');
    return await res.json();
}

function updateBestPrices(data) {
    document.getElementById('bestBid').textContent = data.best_bid ? JSON.stringify(data.best_bid, null, 2) : "N/A";
    document.getElementById('bestAsk').textContent = data.best_ask ? JSON.stringify(data.best_ask, null, 2) : "N/A";
}

function updatePerformanceChart() {
    const ctx = document.getElementById('performanceChart').getContext('2d');
    if (!window.performanceChart) {
        window.performanceChart = new Chart(ctx, {
            type: 'line',
            data: {
                labels: priceHistory.map(p => p.time),
                datasets: [
                    {
                        label: 'Best Bid',
                        data: priceHistory.map(p => p.bestBid),
                        borderColor: 'green',
                        fill: false,
                    },
                    {
                        label: 'Best Ask',
                        data: priceHistory.map(p => p.bestAsk),
                        borderColor: 'red',
                        fill: false,
                    }
                ]
            },
            options: { responsive: true }
        });
    } else {
        window.performanceChart.data.labels = priceHistory.map(p => p.time);
        window.performanceChart.data.datasets[0].data = priceHistory.map(p => p.bestBid);
        window.performanceChart.data.datasets[1].data = priceHistory.map(p => p.bestAsk);
        window.performanceChart.update();
    }
}

async function refreshBestPrices() {
    const data = await fetchBestPrices();
    updateBestPrices(data);
}

async function refreshPerformanceChart() {
    const data = await fetchBestPrices();
    const now = new Date().toLocaleTimeString();
    priceHistory.push({
        time: now,
        bestBid: data.best_bid ? data.best_bid.price : null,
        bestAsk: data.best_ask ? data.best_ask.price : null
    });
    if (priceHistory.length > MAX_HISTORY) priceHistory.shift();
    updatePerformanceChart();
}

setInterval(refreshBestPrices, 1000);
setInterval(refreshPerformanceChart, 1000);
window.onload = () => {
    refreshBestPrices();
    refreshPerformanceChart();
}

let depthChart = null;

function updateDepthChart(data) {
    const bids = data.bids.sort((a, b) => b.price - a.price);
    const asks = data.asks.sort((a, b) => a.price - b.price);

    const bidPrices = bids.map(b => b.price);
    const bidQtys = bids.map(b => b.quantity);
    const askPrices = asks.map(a => a.price);
    const askQtys = asks.map(a => a.quantity);

    const ctx = document.getElementById('depthChart').getContext('2d');

    if (!depthChart) {
        depthChart = new Chart(ctx, {
            type: 'line',
            data: {
                labels: bidPrices.concat(askPrices),
                datasets: [
                    {
                        label: 'Bids',
                        data: bidQtys,
                        borderColor: 'green',
                        fill: false,
                    },
                    {
                        label: 'Asks',
                        data: askQtys,
                        borderColor: 'red',
                        fill: false,
                    }
                ]
            },
            options: { responsive: true }
        });
    } else {
        // Update data only
        depthChart.data.labels = bidPrices.concat(askPrices);
        depthChart.data.datasets[0].data = bidQtys;
        depthChart.data.datasets[1].data = askQtys;
        depthChart.update();
    }
}

async function fetchOrderBook() {
    const res = await fetch('/orderbook');
    const data = await res.json();
    console.log("Orderbook data:", data); // Add this line
    return data;
}