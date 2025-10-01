# Dual-Stack IPC Architecture

QAULTRA-CPP now supports **two independent IPC implementations**:

1. **IceOryx (v1)** - Eclipse Foundation's C++ IPC middleware
2. **iceoryx2 (v2)** - Rust-based rewrite with C++/Rust interoperability

## Why Dual-Stack?

### IceOryx (v1) Advantages
- **Mature and stable** - Production-tested in automotive industry
- **Pure C++** - Native C++ API with no FFI overhead
- **RouDi daemon** - Centralized service discovery and management
- **Performance** - Proven zero-copy shared memory IPC

### iceoryx2 (v2) Advantages
- **Cross-language** - Seamless C++/Rust interoperability
- **Modern architecture** - Distributed service discovery, no daemon required
- **Memory safety** - Rust implementation reduces bugs
- **Future-proof** - Active development, modern design patterns

## Architecture

```
qaultra::ipc
├── v1 (IceOryx)
│   ├── DataBroadcaster
│   ├── DataSubscriber
│   └── BroadcastManager
└── v2 (iceoryx2)
    ├── DataBroadcaster
    ├── DataSubscriber
    └── BroadcastManager
```

## API Compatibility

Both versions provide **identical** high-level APIs:

```cpp
// V1 - IceOryx
#include "qaultra/ipc/broadcast_hub_v1.hpp"
using namespace qaultra::ipc::v1;

// V2 - iceoryx2
#include "qaultra/ipc/broadcast_hub_v2.hpp"
using namespace qaultra::ipc::v2;

// Both share the same interface
DataBroadcaster::initialize_runtime("myapp");  // V1 only
auto broadcaster = DataBroadcaster(config, "stream");
broadcaster.broadcast(data, size, count, type);
```

## Usage Examples

### V1 - IceOryx (C++ to C++)

```cpp
#include "qaultra/ipc/broadcast_hub_v1.hpp"

using namespace qaultra::ipc::v1;

// Start RouDi daemon first:
// $ /home/quantaxis/iceoryx/build/install/bin/iox-roudi

int main() {
    DataBroadcaster::initialize_runtime("publisher");

    BroadcastConfig config;
    config.service_name = "MarketData";

    DataBroadcaster broadcaster(config, "ticks");

    uint8_t data[1024];
    broadcaster.broadcast(data, 1024, 100, MarketDataType::Tick);
}
```

### V2 - iceoryx2 (C++ to C++ or C++ to Rust)

```cpp
#include "qaultra/ipc/broadcast_hub_v2.hpp"

using namespace qaultra::ipc::v2;

// No daemon required!

int main() {
    BroadcastConfig config;
    config.service_name = "MarketData";

    DataBroadcaster broadcaster(config, "ticks");

    uint8_t data[1024];
    broadcaster.broadcast(data, 1024, 100, MarketDataType::Tick);
}
```

### V2 - Cross-Language (C++ Publisher + Rust Subscriber)

**C++ Publisher:**
```cpp
#include "qaultra/ipc/broadcast_hub_v2.hpp"
using namespace qaultra::ipc::v2;

DataBroadcaster broadcaster(config, "market_data");
broadcaster.broadcast(data, size, count, MarketDataType::Tick);
```

**Rust Subscriber:**
```rust
use iceoryx2::prelude::*;

let node = NodeBuilder::new().create::<ipc::Service>().unwrap();
let service = node.service_builder(&"market_data".try_into().unwrap())
    .publish_subscribe::<ZeroCopyMarketBlock>()
    .open_or_create()
    .unwrap();

let subscriber = service.subscriber_builder().create().unwrap();

while let Some(sample) = subscriber.receive().unwrap() {
    println!("Received: {:?}", sample.payload());
}
```

## Build Configuration

Both versions can be enabled/disabled independently:

```cmake
# CMakeLists.txt
option(QAULTRA_USE_ICEORYX "Use IceOryx for zero-copy IPC" ON)
option(QAULTRA_USE_ICEORYX2 "Use iceoryx2 for zero-copy IPC" ON)
```

Build status:
```
-- IceOryx Available: TRUE
-- iceoryx2 Available: TRUE
```

## Performance Comparison

### IceOryx V1 (C++ to C++)
- **Throughput**: 63,291 msg/sec
- **Latency**: 7.97 μs average
- **Overhead**: Minimal (pure C++)

### iceoryx2 V2 (C++ to C++)
- **Throughput**: TBD (expected similar)
- **Latency**: TBD (expected slightly higher due to FFI)
- **Overhead**: Small FFI layer to Rust core

### iceoryx2 V2 (C++ to Rust)
- **Throughput**: TBD (zero-copy across languages)
- **Latency**: TBD (true zero-copy, no serialization)
- **Overhead**: None (shared memory direct access)

## Dependencies

### IceOryx V1
```
/home/quantaxis/iceoryx/build/install/
├── lib/libiceoryx_posh.a
├── lib/libiceoryx_hoofs.a
└── bin/iox-roudi (daemon)
```

### iceoryx2 V2
```
/home/quantaxis/iceoryx2/build/
├── iceoryx2-cxx/libiceoryx2_cxx.a (C++ bindings)
└── rust/native/release/libiceoryx2_ffi_c.a (Rust FFI layer)
```

## Runtime Requirements

### V1 - IceOryx
**Required**: RouDi daemon must be running
```bash
/home/quantaxis/iceoryx/build/install/bin/iox-roudi &
```

### V2 - iceoryx2
**No daemon required** - fully distributed architecture

## When to Use Which?

### Use IceOryx V1 when:
- ✅ Pure C++ application
- ✅ Mature, production-tested solution needed
- ✅ Automotive-grade reliability required
- ✅ Centralized management preferred

### Use iceoryx2 V2 when:
- ✅ Cross-language communication needed (C++/Rust)
- ✅ No daemon overhead acceptable
- ✅ Modern distributed architecture preferred
- ✅ Future integration with QARS Rust code

## Migration Path

Applications can:
1. **Start with V1** for immediate stability
2. **Migrate to V2** when cross-language features needed
3. **Run both** during transition period (different namespaces)

## Limitations

### V1 Limitations
- ❌ No cross-language support (C++ only)
- ❌ Requires RouDi daemon
- ❌ Single-point-of-failure (daemon)

### V2 Limitations
- ⚠️ Newer, less battle-tested
- ⚠️ Small FFI overhead for C++ bindings
- ⚠️ Requires Rust toolchain for building

## Future Work

- [ ] Performance benchmarks V1 vs V2
- [ ] Cross-language integration tests
- [ ] Python bindings via V2
- [ ] Unified abstraction layer (auto-select V1/V2)

## Summary

The dual-stack architecture provides **flexibility** and **future-proofing**:
- Use **V1** for immediate C++ needs
- Use **V2** for C++/Rust interoperability
- Both share common data structures (`ZeroCopyMarketBlock`)
- Seamless migration path as requirements evolve
