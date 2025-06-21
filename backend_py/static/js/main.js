// Enhanced Order Book JavaScript
let priceHistory = [];
const MAX_HISTORY = 100;
let depthChart = null;

// Initialize Chart.js defaults for dark theme
Chart.defaults.color = '#e2e8f0';
Chart.defaults.borderColor = '#4a5568';
Chart.defaults.backgroundColor = 'rgba(0, 212, 170, 0.1)';

// Utility functions
function updateLastUpdateTime() {
    const lastUpdateEl = document.getElementById('lastUpdate');
    if (lastUpdateEl) {
        lastUpdateEl.textContent = new Date().toLocaleTimeString();
    }
}

function showToast(message, type = 'success') {
    const toast = document.getElementById('toast');
    const toastMessage = document.getElementById('toastMessage');
    if (toast && toastMessage) {
        toastMessage.textContent = message;
        toast.classList.remove('translate-x-full');
        setTimeout(() => {
            toast.classList.add('translate-x-full');
        }, 3000);
    }
}

// API functions
async function fetchBestPrices() {
    try {
        const res = await fetch('/best_prices_json');
        if (!res.ok) throw new Error(`HTTP error! status: ${res.status}`);
        return await res.json();
    } catch (error) {
        console.error('Error fetching best prices:', error);
        return { best_bid: null, best_ask: null };
    }
}

async function fetchOrderBook() {
    try {
        const res = await fetch('/orderbook');
        if (!res.ok) throw new Error(`HTTP error! status: ${res.status}`);
        const data = await res.json();
        console.log("Orderbook data:", data);
        return data;
    } catch (error) {
        console.error('Error fetching order book:', error);
        return { bids: [], asks: [] };
    }
}

async function fetchTrades() {
    try {
        const res = await fetch('/trades');
        if (!res.ok) throw new Error(`HTTP error! status: ${res.status}`);
        return await res.json();
    } catch (error) {
        console.error('Error fetching trades:', error);
        return [];
    }
}

// Update functions
function updateBestPrices(data) {
    const bestBidEl = document.getElementById('bestBid');
    const bestAskEl = document.getElementById('bestAsk');
    
    if (bestBidEl) {
        bestBidEl.textContent = data.best_bid ? 
            JSON.stringify(data.best_bid, null, 2) : "No active bids";
    }
    
    if (bestAskEl) {
        bestAskEl.textContent = data.best_ask ? 
            JSON.stringify(data.best_ask, null, 2) : "No active asks";
    }
    
    updateLastUpdateTime();
}

function updatePerformanceChart() {
    const ctx = document.getElementById('performanceChart')?.getContext('2d');
    if (!ctx) return;

    if (!window.performanceChart) {
        window.performanceChart = new Chart(ctx, {
            type: 'line',
            data: {
                labels: priceHistory.map(p => p.time),
                datasets: [
                    {
                        label: 'Best Bid',
                        data: priceHistory.map(p => p.bestBid),
                        borderColor: '#00d4aa',
                        backgroundColor: 'rgba(0, 212, 170, 0.1)',
                        tension: 0.4,
                        fill: false,
                        pointRadius: 2,
                        pointHoverRadius: 6,
                        borderWidth: 2,
                    },
                    {
                        label: 'Best Ask',
                        data: priceHistory.map(p => p.bestAsk),
                        borderColor: '#ff6b6b',
                        backgroundColor: 'rgba(255, 107, 107, 0.1)',
                        tension: 0.4,
                        fill: false,
                        pointRadius: 2,
                        pointHoverRadius: 6,
                        borderWidth: 2,
                    }
                ]
            },
            options: { 
                responsive: true,
                maintainAspectRatio: false,
                plugins: {
                    legend: {
                        position: 'top',
                        labels: {
                            usePointStyle: true,
                            padding: 20,
                        }
                    },
                    tooltip: {
                        mode: 'index',
                        intersect: false,
                        backgroundColor: 'rgba(26, 31, 46, 0.9)',
                        titleColor: '#e2e8f0',
                        bodyColor: '#e2e8f0',
                        borderColor: '#4a5568',
                        borderWidth: 1,
                    }
                },
                scales: {
                    y: {
                        beginAtZero: false,
                        grid: {
                            color: 'rgba(75, 85, 99, 0.3)',
                        },
                        ticks: {
                            callback: function(value) {
                                return ' + value.toFixed(2);
                            }
                        }
                    },
                    x: {
                        grid: {
                            color: 'rgba(75, 85, 99, 0.3)',
                        },
                    }
                },
                interaction: {
                    intersect: false,
                    mode: 'index',
                },
                animation: {
                    duration: 300,
                }
            }
        });
    } else {
        window.performanceChart.data.labels = priceHistory.map(p => p.time);
        window.performanceChart.data.datasets[0].data = priceHistory.map(p => p.bestBid);
        window.performanceChart.data.datasets[1].data = priceHistory.map(p => p.bestAsk);
        window.performanceChart.update('none');
    }
}

function updateDepthChart(data) {
    const ctx = document.getElementById('depthChart')?.getContext('2d');
    if (!ctx) return;

    const bids = data.bids.sort((a, b) => b.price - a.price);
    const asks = data.asks.sort((a, b) => a.price - b.price);

    // Calculate cumulative quantities for depth visualization
    let cumulativeBids = [];
    let cumulativeAsks = [];
    let bidSum = 0;
    let askSum = 0;

    for (let i = 0; i < bids.length; i++) {
        bidSum += bids[i].quantity;
        cumulativeBids.push({ price: bids[i].price, quantity: bidSum });
    }

    for (let i = 0; i < asks.length; i++) {
        askSum += asks[i].quantity;
        cumulativeAsks.push({ price: asks[i].price, quantity: askSum });
    }

    const bidPrices = cumulativeBids.map(b => b.price);
    const bidQtys = cumulativeBids.map(b => b.quantity);
    const askPrices = cumulativeAsks.map(a => a.price);
    const askQtys = cumulativeAsks.map(a => a.quantity);

    if (!depthChart) {
        depthChart = new Chart(ctx, {
            type: 'line',
            data: {
                labels: bidPrices.concat(askPrices),
                datasets: [
                    {
                        label: 'Bid Depth',
                        data: bidQtys.concat(new Array(askPrices.length).fill(null)),
                        borderColor: '#00d4aa',
                        backgroundColor: 'rgba(0, 212, 170, 0.2)',
                        fill: 'origin',
                        tension: 0.1,
                        pointRadius: 1,
                        borderWidth: 2,
                        stepped: true,
                    },
                    {
                        label: 'Ask Depth',
                        data: new Array(bidPrices.length).fill(null).concat(askQtys),
                        borderColor: '#ff6b6b',
                        backgroundColor: 'rgba(255, 107, 107, 0.2)',
                        fill: 'origin',
                        tension: 0.1,
                        pointRadius: 1,
                        borderWidth: 2,
                        stepped: true,
                    }
                ]
            },
            options: { 
                responsive: true,
                maintainAspectRatio: false,
                plugins: {
                    legend: {
                        position: 'top',
                        labels: {
                            usePointStyle: true,
                            padding: 20,
                        }
                    },
                    tooltip: {
                        mode: 'index',
                        intersect: false,
                        backgroundColor: 'rgba(26, 31, 46, 0.9)',
                        titleColor: '#e2e8f0',
                        bodyColor: '#e2e8f0',
                        borderColor: '#4a5568',
                        borderWidth: 1,
                        callbacks: {
                            title: function(context) {
                                return 'Price:  + context[0].label;
                            },
                            label: function(context) {
                                return context.dataset.label + ': ' + context.parsed.y + ' units';
                            }
                        }
                    }
                },
                scales: {
                    y: {
                        beginAtZero: true,
                        grid: {
                            color: 'rgba(75, 85, 99, 0.3)',
                        },
                        title: {
                            display: true,
                            text: 'Cumulative Quantity'
                        }
                    },
                    x: {
                        grid: {
                            color: 'rgba(75, 85, 99, 0.3)',
                        },
                        title: {
                            display: true,
                            text: 'Price ($)'
                        }
                    }
                },
                interaction: {
                    intersect: false,
                    mode: 'index',
                },
                animation: {
                    duration: 300,
                }
            }
        });
    } else {
        depthChart.data.labels = bidPrices.concat(askPrices);
        depthChart.data.datasets[0].data = bidQtys.concat(new Array(askPrices.length).fill(null));
        depthChart.data.datasets[1].data = new Array(bidPrices.length).fill(null).concat(askQtys);
        depthChart.update('none');
    }
}

function updateTape(trades) {
    const tbody = document.querySelector('#tape tbody');
    if (!tbody) return;

    tbody.innerHTML = "";
    
    // Show only the most recent 15 trades
    trades.slice(0, 15).forEach((trade, index) => {
        const row = document.createElement('tr');
        row.className = 'hover:bg-gray-700 transition-colors duration-150 slide-in';
        row.style.animationDelay = `${index * 50}ms`;
        
        const sideColor = trade.side ? 'text-trade-green' : 'text-trade-red';
        const sideText = trade.side ? 'BUY' : 'SELL';
        const sideBg = trade.side ? 'bg-green-900 bg-opacity-20' : 'bg-red-900 bg-opacity-20';
        
        row.innerHTML = `
            <td class="px-6 py-4 whitespace-nowrap text-sm font-mono text-gray-300">
                ${new Date(trade.timestamp * 1000).toLocaleTimeString()}
            </td>
            <td class="px-6 py-4 whitespace-nowrap text-sm font-mono font-semibold text-white">
                ${typeof trade.price === 'number' ? trade.price.toFixed(2) : trade.price}
            </td>
            <td class="px-6 py-4 whitespace-nowrap text-sm font-mono text-gray-300">
                ${trade.quantity.toLocaleString()}
            </td>
            <td class="px-6 py-4 whitespace-nowrap">
                <span class="inline-flex items-center px-2.5 py-0.5 rounded-full text-xs font-bold ${sideColor} ${sideBg}">
                    ${sideText}
                </span>
            </td>
        `;
        tbody.appendChild(row);
    });
}

// Refresh functions
async function refreshBestPrices() {
    try {
        const data = await fetchBestPrices();
        updateBestPrices(data);
    } catch (error) {
        console.error('Error refreshing best prices:', error);
    }
}

async function refreshPerformanceChart() {
    try {
        const data = await fetchBestPrices();
        const now = new Date().toLocaleTimeString();
        
        priceHistory.push({
            time: now,
            bestBid: data.best_bid ? data.best_bid.price : null,
            bestAsk: data.best_ask ? data.best_ask.price : null
        });
        
        if (priceHistory.length > MAX_HISTORY) {
            priceHistory.shift();
        }
        
        updatePerformanceChart();
    } catch (error) {
        console.error('Error refreshing performance chart:', error);
    }
}

async function refreshDepthChart() {
    try {
        const data = await fetchOrderBook();
        updateDepthChart(data);
    } catch (error) {
        console.error('Error refreshing depth chart:', error);
    }
}

async function refreshTape() {
    try {
        const trades = await fetchTrades();
        updateTape(trades);
    } catch (error) {
        console.error('Error refreshing tape:', error);
    }
}

// Form handling
function setupFormHandling() {
    const form = document.querySelector('form');
    if (!form) return;

    form.addEventListener('submit', async function(e) {
        e.preventDefault();
        
        const submitButton = form.querySelector('button[type="submit"]');
        const originalText = submitButton.textContent;
        
        // Show loading state
        submitButton.textContent = 'Placing Order...';
        submitButton.disabled = true;
        
        try {
            const formData = new FormData(this);
            const response = await fetch('/add_order', {
                method: 'POST',
                body: formData
            });
            
            if (response.ok) {
                showToast('Order placed successfully!', 'success');
                this.reset();
                // Immediately refresh data
                setTimeout(() => {
                    refreshBestPrices();
                    refreshDepthChart();
                    refreshTape();
                }, 100);
            } else {
                const errorText = await response.text();
                showToast(`Error placing order: ${errorText}`, 'error');
            }
        } catch (error) {
            console.error('Error:', error);
            showToast('Network error - please try again', 'error');
        } finally {
            // Reset button state
            submitButton.textContent = originalText;
            submitButton.disabled = false;
        }
    });
}

// Connection status monitoring
function setupConnectionMonitoring() {
    let isOnline = true;
    
    const checkConnection = async () => {
        try {
            const response = await fetch('/best_prices_json', { 
                method: 'HEAD',
                cache: 'no-cache'
            });
            
            if (!isOnline && response.ok) {
                isOnline = true;
                showToast('Connection restored', 'success');
            }
        } catch (error) {
            if (isOnline) {
                isOnline = false;
                showToast('Connection lost - retrying...', 'error');
            }
        }
    };
    
    // Check connection every 10 seconds
    setInterval(checkConnection, 10000);
}

// Initialization
function initializeApp() {
    console.log('Initializing Order Book Dashboard...');
    
    // Setup form handling
    setupFormHandling();
    
    // Setup connection monitoring
    setupConnectionMonitoring();
    
    // Initial data load
    refreshBestPrices();
    refreshPerformanceChart();
    refreshDepthChart();
    refreshTape();
    
    console.log('Dashboard initialized successfully');
}

// Set up intervals with error handling
function setupIntervals() {
    const intervals = [
        { fn: refreshBestPrices, interval: 1000, name: 'Best Prices' },
        { fn: refreshPerformanceChart, interval: 5000, name: 'Performance Chart' },
        { fn: refreshDepthChart, interval: 2000, name: 'Depth Chart' },
        { fn: refreshTape, interval: 1500, name: 'Trade Tape' }
    ];
    
    intervals.forEach(({ fn, interval, name }) => {
        setInterval(async () => {
            try {
                await fn();
            } catch (error) {
                console.error(`Error in ${name} refresh:`, error);
            }
        }, interval);
    });
}

// Handle window events
function setupWindowEvents() {
    // Handle window resize for charts
    window.addEventListener('resize', () => {
        if (window.performanceChart) window.performanceChart.resize();
        if (depthChart) depthChart.resize();
    });
    
    // Handle visibility change to pause/resume updates
    document.addEventListener('visibilitychange', () => {
        if (document.visibilityState === 'visible') {
            // Refresh data when tab becomes visible
            refreshBestPrices();
            refreshDepthChart();
            refreshTape();
        }
    });
}

// Main initialization
window.addEventListener('load', () => {
    initializeApp();
    setupIntervals();
    setupWindowEvents();
});

// Export functions for potential external use
window.OrderBookDashboard = {
    refreshBestPrices,
    refreshPerformanceChart,
    refreshDepthChart,
    refreshTape,
    priceHistory
};