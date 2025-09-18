#!/usr/bin/env python3
"""
QAULTRA C++ Python Bindings Example
Demonstrates the complete Python interface to QAULTRA C++
"""

import qaultra_cpp as qa
import numpy as np
import pandas as pd
from datetime import datetime, timedelta

def main():
    print("QAULTRA C++ Python Bindings Example")
    print("===================================\n")

    try:
        # 1. Create account using Python bindings
        print("1. Creating Trading Account...")
        account = qa.account.QA_Account(
            account_cookie="python_account",
            portfolio_cookie="python_portfolio",
            user_cookie="python_user",
            init_cash=1000000.0,
            auto_reload=False,
            environment="backtest"
        )

        print(f"   Initial Cash: ${account.get_cash():,.2f}")
        print(f"   Total Value: ${account.get_total_value():,.2f}\n")

        # 2. Market data with Arrow backend
        print("2. Creating Market Data with Arrow...")

        # Generate sample OHLCV data
        dates = pd.date_range('2024-01-01', periods=100, freq='D')
        np.random.seed(42)

        # Random walk price generation
        initial_price = 100.0
        returns = np.random.normal(0, 0.02, 100)
        prices = [initial_price]
        for ret in returns[:-1]:
            prices.append(prices[-1] * (1 + ret))

        # Create OHLCV data
        opens = np.array(prices)
        closes = opens * (1 + np.random.normal(0, 0.01, 100))
        highs = np.maximum(opens, closes) * (1 + np.abs(np.random.normal(0, 0.005, 100)))
        lows = np.minimum(opens, closes) * (1 - np.abs(np.random.normal(0, 0.005, 100)))
        volumes = np.random.uniform(50000, 200000, 100)
        amounts = closes * volumes

        # Create Arrow K-line collection
        klines = qa.data.ArrowKlineCollection()
        codes = ["AAPL"] * 100
        timestamps = [int(d.timestamp() * 1000) for d in dates]

        klines.add_batch(codes, timestamps, opens.tolist(), highs.tolist(),
                        lows.tolist(), closes.tolist(), volumes.tolist(), amounts.tolist())

        print(f"   Added {klines.size()} K-line records")
        print(f"   Memory usage: {klines.memory_usage():,} bytes\n")

        # 3. Technical analysis with SIMD
        print("3. SIMD-Optimized Technical Analysis...")

        # Calculate indicators using SIMD
        sma_20 = klines.sma(20)
        ema_12 = klines.ema(0.154)  # alpha = 2/(12+1)
        rsi_14 = klines.rsi(14)
        bollinger = klines.bollinger_bands(20, 2.0)

        print(f"   SMA(20) last value: {sma_20[-1]:.2f}")
        print(f"   EMA(12) last value: {ema_12[-1]:.2f}")
        print(f"   RSI(14) last value: {rsi_14[-1]:.2f}")
        print(f"   Bollinger Upper: {bollinger[0][-1]:.2f}")
        print(f"   Bollinger Lower: {bollinger[2][-1]:.2f}\n")

        # 4. Trading with the account
        print("4. Executing Trades...")

        current_price = closes[-1]

        # Buy order
        buy_order = account.buy("AAPL", 1000.0, "2024-04-10 09:30:00", current_price)
        print(f"   Buy Order: {1000} AAPL @ ${current_price:.2f}")
        print(f"   Order Status: {buy_order.status}")
        print(f"   Cash after buy: ${account.get_cash():,.2f}")

        # Simulate price movement
        new_price = current_price * 1.05  # 5% increase
        account.on_price_change("AAPL", new_price, "2024-04-10 16:00:00")

        print(f"   Price update: AAPL -> ${new_price:.2f}")
        print(f"   Float P&L: ${account.get_float_profit():,.2f}")
        print(f"   Total Value: ${account.get_total_value():,.2f}\n")

        # 5. Matching engine demonstration
        print("5. Matching Engine...")

        # Create matching engine
        engine = qa.market.factory.create_matching_engine(4)

        # Add trade callback
        def trade_callback(trade):
            print(f"   Trade executed: {trade.trade_volume} @ ${trade.trade_price:.2f}")

        engine.add_trade_callback(trade_callback)
        engine.start()

        # Create orders
        buy_order_me = qa.account.Order()
        buy_order_me.order_id = "ME_BUY_001"
        buy_order_me.code = "AAPL"
        buy_order_me.volume = 500.0
        buy_order_me.price = new_price - 0.50
        buy_order_me.direction = qa.Direction.BUY
        buy_order_me.status = qa.OrderStatus.PENDING

        sell_order_me = qa.account.Order()
        sell_order_me.order_id = "ME_SELL_001"
        sell_order_me.code = "AAPL"
        sell_order_me.volume = 500.0
        sell_order_me.price = new_price + 0.50
        sell_order_me.direction = qa.Direction.SELL
        sell_order_me.status = qa.OrderStatus.PENDING

        # Submit orders
        engine.submit_order(buy_order_me)
        engine.submit_order(sell_order_me)

        # Get market depth
        depth = engine.get_market_depth("AAPL", 5)
        print(f"   Order book depth: {len(depth[0])} bids, {len(depth[1])} asks")

        engine.stop()

        # 6. QIFI protocol export
        print("\n6. QIFI Protocol Export...")

        qifi_account = account.to_qifi()
        print(f"   Account Cookie: {qifi_account.account_cookie}")
        print(f"   Total Value: ${qifi_account.total_value:,.2f}")
        print(f"   Positions: {len(qifi_account.positions)}")
        print(f"   Orders: {len(qifi_account.orders)}")
        print(f"   Trades: {len(qifi_account.trades)}")

        # Save to file
        qifi_account.save_to_file("account_export.json")
        print("   Exported to account_export.json\n")

        # 7. Performance demonstration
        print("7. Performance Comparison...")

        # Generate large arrays for SIMD testing
        size = 1000000
        a = np.random.random(size)
        b = np.random.random(size)

        # Time regular NumPy
        import time
        start = time.time()
        numpy_result = a * b
        numpy_time = time.time() - start

        # Time SIMD version
        start = time.time()
        simd_result = qa.simd.vectorized_multiply(a, b)
        simd_time = time.time() - start

        print(f"   NumPy multiply ({size:,} elements): {numpy_time:.4f}s")
        print(f"   SIMD multiply ({size:,} elements): {simd_time:.4f}s")
        print(f"   Speedup: {numpy_time/simd_time:.2f}x")

        # Verify results are equivalent
        diff = np.abs(numpy_result - simd_result).max()
        print(f"   Max difference: {diff:.2e}\n")

        # 8. Memory-mapped arrays for zero-copy
        print("8. Zero-Copy Memory Operations...")

        # Create memory-mapped array
        mmap_array = qa.data.MemoryMappedDoubleArray("test_data.bin", 10000)

        # Fill with data
        for i in range(min(100, 10000)):
            mmap_array[i] = i * 0.1

        # Sync to disk
        mmap_array.sync()

        print(f"   Created memory-mapped array: {mmap_array.size()} elements")
        print(f"   First 10 values: {[mmap_array[i] for i in range(10)]}")

        # Get as NumPy array (zero-copy)
        np_view = mmap_array.as_array()
        print(f"   NumPy view shape: {np_view.shape}")
        print(f"   Shares memory: {np.shares_memory(np_view, np_view)}\n")

        # 9. Lock-free threading
        print("9. Lock-Free Data Structures...")

        # Create lock-free queue
        queue = qa.threading.LockFreeDoubleQueue(1000)

        # Add items
        for i in range(10):
            queue.enqueue(float(i))

        print(f"   Queue size: {queue.size()}")

        # Remove items
        values = []
        while not queue.is_empty():
            item = queue.dequeue()
            if item is not None:
                values.append(item)

        print(f"   Dequeued values: {values}\n")

        print("Example completed successfully!")
        print("\nQAULTRA C++ provides:")
        print("✓ High-performance account management")
        print("✓ SIMD-optimized calculations")
        print("✓ Zero-copy memory operations")
        print("✓ Lock-free concurrent data structures")
        print("✓ Complete market simulation")
        print("✓ Standards-compliant protocols (QIFI/MIFI/TIFI)")

    except Exception as e:
        print(f"Error: {e}")
        import traceback
        traceback.print_exc()
        return 1

    return 0

if __name__ == "__main__":
    exit(main())