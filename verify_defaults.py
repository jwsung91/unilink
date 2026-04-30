import sys
import os

sys.path.append(os.path.join(os.getcwd(), "build/lib"))
sys.path.append(os.path.join(os.getcwd(), "build/bindings/python"))
import unilink_py as unilink

print("--- Verifying Dynamic Defaults (Python Bindings) ---")

# 1. Reliable (Default)
client_r = unilink.TcpClient("127.0.0.1", 10040)
print(f"Reliable (Initial): Strategy={client_r.backpressure_strategy}, Threshold={client_r.backpressure_threshold / 1024 / 1024:.2f} MB")

# 2. Switch to BestEffort
client_r.backpressure_strategy = unilink.BackpressureStrategy.BestEffort
# Note: In C++ builder, this is set during build. 
# In Python, if we just set strategy, does the threshold update? 
# Usually, the threshold is a separate property. 
# Let's check what it is now.
print(f"BestEffort (Switched): Strategy={client_r.backpressure_strategy}, Threshold={client_r.backpressure_threshold / 1024 / 1024:.2f} MB")

# 3. Using the Builder (if available in Python) - actually Python uses constructors mostly.

print("\n--- Verifying with different strategies at creation ---")
# The Python bindings for TcpClient(host, port) likely use the default constructor of TcpClientConfig.
# Let's see what the default threshold is in TcpClientConfig.
cfg = unilink.TcpClientConfig()
print(f"Default Config: Strategy={cfg.backpressure_strategy}, Threshold={cfg.backpressure_threshold / 1024 / 1024:.2f} MB")
