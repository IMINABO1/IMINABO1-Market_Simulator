// Enhanced Order Book JavaScript
let priceHistory = [];
const MAX_HISTORY = 100;
let depthChart = null;
let connectionStatus = true;

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
    const toastIcon = document.getElementById('toastIcon');
    
    if (toast && toastMessage && toastIcon) {
        toastMessage.textContent = message;
        
        // Update toast appearance based on type
        if (type === 'success') {
            toast.className = toast.className.replace('toast-error', 'toast-success');
            toastIcon.className = 'w-6 h-6 text-cyber-green';
            toastIcon.innerHTML = '<path fill-rule="evenodd" d="M10 18a8 8 0 100-16 8 8 0 000 16zm3.707-9.293a1 1 0 00-1.414-1.414L9 10.586 7.707 9.293a1 1 0 00-1.414 1.414l2 2a1 1 0 001.414 0l4-4z" clip-rule="evenodd"></path>';
        } else {
            toast.className = toast.className.replace('toast-success', 'toast-error');
            toastIcon.className = 'w-6 h-6 text-danger-red';
            toastIcon.innerHTML = '<path fill-rule="evenodd" d="M10 18a8 8 0 100-16 8 8 0 000 16zM8.707 7.293a1 1 0 00-1.414 1.414L8.586 10l-1.293 1.293a1 1 0 101.414 1.414L10 11.414l1.293 1.293a1 1 0 001.414-1.414L11.414 10l1.293-1.293a1 1 0 00-1.414-1.414L10 8.586 8.707 7.293z" clip-rule="evenodd"></path>';
        }
        
        toast.classList.remove('translate-x-full');
        setTimeout(() => {
            toast.classList.add('translate-x-full');
        }, 4000);
    }
}

function updateConnectionStatus(status) {
    const connectionEl = document.getElementById('connectionStatus');
    if (connectionEl) {
        if (status) {
            connectionEl.textContent = 'LIVE FEED';
            connectionEl.className = 'text-sm font-medium text-cyber-green';
        } else {
            connectionEl.textContent = 'DISCONNECTED';
            connectionEl.className = 'text-sm font-medium text-danger-red';
        }
    }
    connectionStatus = status;
}

// API functions
async function fetchBestPrices() {
    try {
        const res = await fetch('/best_prices_json');
        if (!res.ok) throw new Error(`HTTP error! status: ${res.status}`);
        const data = await res.json();
        
        if (!connectionStatus) {
            updateConnectionStatus(true);
        }
        
        return data;
    } catch (error) {
        console.error('Error fetching best prices:', error);
        updateConnectionStatus(false);
        return { best_bid: null, best_ask: null };
    }
}

async function fetchOrderBook() {
    try {
        const res = await fetch('/orderbook');
        if (!res.ok) throw new Error(`HTTP error! status: ${res.status}`);
        const data = await res.json();
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
        const data = await res.json();
        return Array.isArray(data) ? data : [];
    } catch (error) {
        console.error('Error fetching trades:', error);
        return [];
    }
}

// Update functions
function updateBestPrices(data) {
    const bestBidEl = document.getElementById('bestBid');
    const bestAskEl = document.getElementById('bestAsk');
    const spreadEl = document.getElementById('spreadValue');
    
    if (bestBidEl) {
        bestBidEl.textContent = data.best_bid ? 
            JSON.stringify(data.best_bid, null, 2) : "No active bids";
    }
    
    if (bestAskEl) {
        bestAskEl.textContent = data.best_ask ? 
            JSON.stringify(data.best_ask, null, 2) : "No active asks";
    }
    
    // Update spread
    if (spreadEl && data.best_bid && data.best_ask) {
        const spread = data.best_ask.price - data.best_bid.price;
        spreadEl.textContent = `$${spread.toFixed(2)}`;
    } else if (spreadEl) {
        spreadEl.textContent = '$--';
    }
    
    updateLastUpdateTime();
}

function updateMarketStats(orderBookData, tradesData) {
    const ordersCountEl = document.getElementById('ordersCount');
    const tradesCountEl = document.getElementById('tradesCount');
    const activityBar = document.querySelector('.activity-bar');
    
    if (ordersCountEl) {
        const totalOrders = orderBookData.bids.length + orderBookData.asks.length;
        ordersCountEl.textContent = totalOrders.toString();
    }
    
    if (tradesCountEl) {
        tradesCountEl.textContent = tradesData.length.toString();
    }
    
    if (activityBar) {
        const activity = Math.min(100, (orderBookData.bids.length + orderBookData.asks.length) * 2);
        activityBar.style.width = `${activity}%`;
    }
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
                        borderColor: '#00ff88',
                        backgroundColor: 'rgba(0, 255, 136, 0.1)',
                        tension: 0.4,
                        fill: false,
                        pointRadius: 2,
                        pointHoverRadius: 6,
                        borderWidth: 2,
                    },
                    {
                        label: 'Best Ask',
                        data: priceHistory.map(p => p.bestAsk),
                        borderColor: '#ff3366',
                        backgroundColor: 'rgba(255, 51, 102, 0.1)',
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
                            color: '#e2e8f0'
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
                            color: '#e2e8f0',
                            callback: function(value) {
                                return '$' + value.toFixed(2);
                            }
                        }
                    },
                    x: {
                        grid: {
                            color: 'rgba(75, 85, 99, 0.3)',
                        },
                        ticks: {
                            color: '#e2e8f0'
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
        window.performanceChart.data.labels = priceHistory.map(p => p.time);
        window.performanceChart.data.datasets[0].data = priceHistory.map(p => p.bestBid);
        window.performanceChart.data.datasets[1].data = priceHistory.map(p => p.bestAsk);
        window.performanceChart.update('none');
    }
}

function updateDepthChart(data) {
    const ctx = document.getElementById('depthChart')?.getContext('2d');
    if (!ctx || !data.bids || !data.asks) return;

    const bids = data.bids.sort((a, b) => b.price - a.price).slice(0, 10);
    const asks = data.asks.sort((a, b) => a.price - b.price).slice(0, 10);

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

    const allPrices = [...bidPrices.reverse(), ...askPrices];
    const bidData = [...bidQtys.reverse(), ...new Array(askPrices.length).fill(null)];
    const askData = [...new Array(bidPrices.length).fill(null), ...askQtys];

    if (!depthChart) {
        depthChart = new Chart(ctx, {
            type: 'line',
            data: {
                labels: allPrices,
                datasets: [
                    {
                        label: 'Bid Depth',
                        data: bidData,
                        borderColor: '#00ff88',
                        backgroundColor: 'rgba(0, 255, 136, 0.2)',
                        fill: 'origin',
                        tension: 0.1,
                        pointRadius: 1,
                        borderWidth: 2,
                        stepped: true,
                    },
                    {
                        label: 'Ask Depth',
                        data: askData,
                        borderColor: '#ff3366',
                        backgroundColor: 'rgba(255, 51, 102, 0.2)',
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
                            color: '#e2e8f0'
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
                                return 'Price: $' + context[0].label;
                            },
                            label: function(context) {
                                return context.dataset.label + ': ' + (context.parsed.y || 0) + ' units';
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
                        ticks: {
                            color: '#e2e8f0'
                        },
                        title: {
                            display: true,
                            text: 'Cumulative Quantity',
                            color: '#e2e8f0'
                        }
                    },
                    x: {
                        grid: {
                            color: 'rgba(75, 85, 99, 0.3)',
                        },
                        ticks: {
                            color: '#e2e8f0',
                            callback: function(value, index) {
                                return '$' + this.getLabelForValue(value);
                            }
                        },
                        title: {
                            display: true,
                            text: 'Price ($)',
                            color: '#e2e8f0'
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
        depthChart.data.labels = allPrices;
        depthChart.data.datasets[0].data = bidData;
        depthChart.data.datasets[1].data = askData;
        depthChart.update('none');
    }
}

function updateTape(trades) {
    const tbody = document.getElementById('tradesTableBody');
    if (!tbody || !Array.isArray(trades)) return;

    if (trades.length === 0) {
        tbody.innerHTML = `
            <tr>
                <td colspan="4" class="px-6 py-8 text-center text-gray-400">
                    <div class="flex flex-col items-center">
                        <svg class="w-8 h-8 mb-2 opacity-50" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                            <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M9 12l2 2 4-4m6 2a9 9 0 11-18 0 9 9 0 0118 0z"></path>
                        </svg>
                        No trades yet
                    </div>
                </td>
            </tr>
        `;
        return;
    }

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
                $${typeof trade.price === 'number' ? trade.price.toFixed(2) : trade.price}
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
        
        // Update price history
        const now = new Date().toLocaleTimeString();
        if (data.best_bid || data.best_ask) {
            priceHistory.push({
                time: now,
                bestBid: data.best_bid?.price || null,
                bestAsk: data.best_ask?.price || null
            });
            
            if (priceHistory.length > MAX_HISTORY) {
                priceHistory.shift();
            }
        }
    } catch (error) {
        console.error('Error refreshing best prices:', error);
    }
}

async function refreshDepthChart() {
    try {
        const data = await fetchOrderBook();
        updateDepthChart(data);
        return data;
    } catch (error) {
        console.error('Error refreshing depth chart:', error);
        return { bids: [], asks: [] };
    }
}

async function refreshTape() {
    try {
        const trades = await fetchTrades();
        updateTape(trades);
        return trades;
    } catch (error) {
        console.error('Error refreshing tape:', error);
        return [];
    }
}

async function refreshAllData() {
    try {
        const [, orderBookData, tradesData] = await Promise.all([
            refreshBestPrices(),
            refreshDepthChart(),
            refreshTape()
        ]);
        
        updateMarketStats(orderBookData, tradesData);
        updatePerformanceChart();
    } catch (error) {
        console.error('Error refreshing data:', error);
    }
}

// Form handling
function setupFormHandling() {
    const form = document.getElementById('orderForm');
    if (!form) return;

    form.addEventListener('submit', async function(e) {
        e.preventDefault();
        
        const submitButton = form.querySelector('button[type="submit"]');
        const originalText = submitButton.textContent;
        
        // Show loading state
        submitButton.textContent = 'PLACING ORDER...';
        submitButton.disabled = true;
        submitButton.classList.add('opacity-50');
        
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
                setTimeout(refreshAllData, 100);
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
            submitButton.classList.remove('opacity-50');
        }
    });
}

// Connection status monitoring
function setupConnectionMonitoring() {
    const checkConnection = async () => {
        try {
            const controller = new AbortController();
            const timeoutId = setTimeout(() => controller.abort(), 5000);
            
            const response = await fetch('/best_prices_json', { 
                method: 'HEAD',
                cache: 'no-cache',
                signal: controller.signal
            });
            
            clearTimeout(timeoutId);
            
            if (!connectionStatus && response.ok) {
                updateConnectionStatus(true);
                showToast('Connection restored', 'success');
            }
        } catch (error) {
            if (connectionStatus) {
                updateConnectionStatus(false);
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
    refreshAllData();
    
    // Show initialization toast
    setTimeout(() => {
        showToast('Trading dashboard initialized', 'success');
    }, 1000);
    
    console.log('Dashboard initialized successfully');
}

// Set up intervals with error handling
function setupIntervals() {
    // Refresh all data every 2 seconds
    setInterval(async () => {
        try {
            await refreshAllData();
        } catch (error) {
            console.error('Error in data refresh:', error);
        }
    }, 2000);
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
            refreshAllData();
        }
    });
    
    // Handle beforeunload
    window.addEventListener('beforeunload', () => {
        console.log('Page unloading - cleaning up...');
    });
}

// Main initialization
document.addEventListener('DOMContentLoaded', () => {
    console.log('DOM loaded, initializing app...');
    initializeApp();
    setupIntervals();
    setupWindowEvents();
});

// Export functions for potential external use
window.OrderBookDashboard = {
    refreshAllData,
    refreshBestPrices,
    refreshDepthChart,
    refreshTape,
    priceHistory,
    showToast
};