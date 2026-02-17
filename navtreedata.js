/*
 @licstart  The following is the entire license notice for the JavaScript code in this file.

 The MIT License (MIT)

 Copyright (C) 1997-2020 by Dimitri van Heesch

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 and associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 @licend  The above is the entire license notice for the JavaScript code in this file
*/
var NAVTREE =
[
  [ "unilink", "index.html", [
    [ "Unilink System Architecture", "index.html", [
      [ "Table of Contents", "index.html#autotoc_md99", null ],
      [ "Overview", "index.html#autotoc_md101", [
        [ "Design Goals", "index.html#autotoc_md102", null ]
      ] ],
      [ "Layered Architecture", "index.html#autotoc_md104", [
        [ "Layer Responsibilities", "index.html#autotoc_md105", [
          [ "1. Builder API Layer", "index.html#autotoc_md106", null ],
          [ "2. Wrapper API Layer", "index.html#autotoc_md107", null ],
          [ "3. Transport Layer", "index.html#autotoc_md108", null ],
          [ "4. Common Utilities Layer", "index.html#autotoc_md109", null ]
        ] ]
      ] ],
      [ "Core Components", "index.html#autotoc_md111", [
        [ "1. Builder System", "index.html#autotoc_md112", null ],
        [ "2. Wrapper System", "index.html#autotoc_md113", null ],
        [ "3. Transport System", "index.html#autotoc_md114", null ],
        [ "4. Common Utilities", "index.html#autotoc_md115", null ]
      ] ],
      [ "Design Patterns", "index.html#autotoc_md117", [
        [ "1. Builder Pattern", "index.html#autotoc_md118", null ],
        [ "2. Dependency Injection", "index.html#autotoc_md119", null ],
        [ "3. Observer Pattern", "index.html#autotoc_md120", null ],
        [ "4. Singleton Pattern", "index.html#autotoc_md121", null ],
        [ "5. RAII (Resource Acquisition Is Initialization)", "index.html#autotoc_md122", null ],
        [ "6. Template Method Pattern", "index.html#autotoc_md123", null ]
      ] ],
      [ "Threading Model", "index.html#autotoc_md125", [
        [ "Overview", "index.html#autotoc_md126", null ],
        [ "Thread Safety Guarantees", "index.html#autotoc_md127", [
          [ "1. API Methods", "index.html#autotoc_md128", null ],
          [ "2. Callbacks", "index.html#autotoc_md129", null ],
          [ "3. Shared State", "index.html#autotoc_md130", null ]
        ] ],
        [ "IO Context Management", "index.html#autotoc_md131", [
          [ "Shared Context (Default)", "index.html#autotoc_md132", null ],
          [ "Independent Context", "index.html#autotoc_md133", null ]
        ] ]
      ] ],
      [ "Memory Management", "index.html#autotoc_md135", [
        [ "1. Smart Pointers", "index.html#autotoc_md136", null ],
        [ "2. Memory Pool", "index.html#autotoc_md137", null ],
        [ "3. Memory Tracking", "index.html#autotoc_md138", null ],
        [ "4. Safe Data Buffer", "index.html#autotoc_md139", null ]
      ] ],
      [ "Error Handling", "index.html#autotoc_md141", [
        [ "Error Propagation Flow", "index.html#autotoc_md142", null ],
        [ "Error Categories", "index.html#autotoc_md143", null ],
        [ "Error Recovery Strategies", "index.html#autotoc_md144", [
          [ "1. Automatic Retry", "index.html#autotoc_md145", null ],
          [ "2. Circuit Breaker (Planned)", "index.html#autotoc_md146", null ]
        ] ]
      ] ],
      [ "Configuration System", "index.html#autotoc_md148", [
        [ "Compile-Time Configuration", "index.html#autotoc_md149", null ],
        [ "Runtime Configuration", "index.html#autotoc_md150", null ]
      ] ],
      [ "Performance Considerations", "index.html#autotoc_md152", [
        [ "1. Asynchronous I/O", "index.html#autotoc_md153", null ],
        [ "2. Zero-Copy Operations", "index.html#autotoc_md154", null ],
        [ "3. Connection Pooling (Planned)", "index.html#autotoc_md155", null ],
        [ "4. Memory Pooling", "index.html#autotoc_md156", null ]
      ] ],
      [ "Extension Points", "index.html#autotoc_md158", [
        [ "1. Custom Transports", "index.html#autotoc_md159", null ],
        [ "2. Custom Builders", "index.html#autotoc_md160", null ],
        [ "3. Custom Error Handlers", "index.html#autotoc_md161", null ]
      ] ],
      [ "Development & Tooling", "index.html#autotoc_md163", [
        [ "1. Docker-based Environment", "index.html#autotoc_md164", null ],
        [ "2. Documentation Generation", "index.html#autotoc_md165", null ]
      ] ],
      [ "Testing Architecture", "index.html#autotoc_md167", [
        [ "1. Dependency Injection", "index.html#autotoc_md168", null ],
        [ "2. Independent Contexts", "index.html#autotoc_md169", null ],
        [ "3. State Verification", "index.html#autotoc_md170", null ]
      ] ],
      [ "Summary", "index.html#autotoc_md172", null ]
    ] ],
    [ "Channel Contract: Ensuring Predictable and Robust Communication", "md_docs_architecture_channel_contract.html", [
      [ "1. Introduction", "md_docs_architecture_channel_contract.html#autotoc_md1", null ],
      [ "2. Core Principles", "md_docs_architecture_channel_contract.html#autotoc_md2", null ],
      [ "3. Stop Semantics: No Callbacks After Stop()", "md_docs_architecture_channel_contract.html#autotoc_md3", null ],
      [ "4. Backpressure Policy", "md_docs_architecture_channel_contract.html#autotoc_md4", null ],
      [ "5. Error Handling Consistency", "md_docs_architecture_channel_contract.html#autotoc_md5", null ],
      [ "6. API Whitelist and State Transitions", "md_docs_architecture_channel_contract.html#autotoc_md6", null ],
      [ "7. Versioning and Evolution", "md_docs_architecture_channel_contract.html#autotoc_md7", null ]
    ] ],
    [ "Memory Safety Architecture", "md_docs_architecture_memory_safety.html", [
      [ "Table of Contents", "md_docs_architecture_memory_safety.html#autotoc_md10", null ],
      [ "Overview", "md_docs_architecture_memory_safety.html#autotoc_md12", [
        [ "Memory Safety Guarantees", "md_docs_architecture_memory_safety.html#autotoc_md13", null ],
        [ "Safety Levels", "md_docs_architecture_memory_safety.html#autotoc_md15", null ]
      ] ],
      [ "Safe Data Handling", "md_docs_architecture_memory_safety.html#autotoc_md17", [
        [ "SafeDataBuffer", "md_docs_architecture_memory_safety.html#autotoc_md18", null ],
        [ "Features", "md_docs_architecture_memory_safety.html#autotoc_md20", [
          [ "1. Construction Validation", "md_docs_architecture_memory_safety.html#autotoc_md21", null ],
          [ "2. Safe Type Conversions", "md_docs_architecture_memory_safety.html#autotoc_md23", null ],
          [ "3. Memory Validation", "md_docs_architecture_memory_safety.html#autotoc_md25", null ]
        ] ],
        [ "Safe Span (C++17 Compatible)", "md_docs_architecture_memory_safety.html#autotoc_md27", null ]
      ] ],
      [ "Thread-Safe State Management", "md_docs_architecture_memory_safety.html#autotoc_md29", [
        [ "ThreadSafeState", "md_docs_architecture_memory_safety.html#autotoc_md30", null ],
        [ "AtomicState", "md_docs_architecture_memory_safety.html#autotoc_md32", null ],
        [ "ThreadSafeCounter", "md_docs_architecture_memory_safety.html#autotoc_md34", null ],
        [ "ThreadSafeFlag", "md_docs_architecture_memory_safety.html#autotoc_md36", null ],
        [ "Thread Safety Summary", "md_docs_architecture_memory_safety.html#autotoc_md38", null ]
      ] ],
      [ "Memory Tracking", "md_docs_architecture_memory_safety.html#autotoc_md40", [
        [ "Built-in Memory Tracking", "md_docs_architecture_memory_safety.html#autotoc_md41", null ],
        [ "Features", "md_docs_architecture_memory_safety.html#autotoc_md43", [
          [ "1. Allocation Tracking", "md_docs_architecture_memory_safety.html#autotoc_md44", null ],
          [ "2. Leak Detection", "md_docs_architecture_memory_safety.html#autotoc_md46", null ],
          [ "3. Performance Monitoring", "md_docs_architecture_memory_safety.html#autotoc_md48", null ],
          [ "4. Debug Reports", "md_docs_architecture_memory_safety.html#autotoc_md50", null ]
        ] ],
        [ "Zero Overhead in Release", "md_docs_architecture_memory_safety.html#autotoc_md52", null ]
      ] ],
      [ "AddressSanitizer Support", "md_docs_architecture_memory_safety.html#autotoc_md54", [
        [ "Enable AddressSanitizer", "md_docs_architecture_memory_safety.html#autotoc_md55", null ],
        [ "What ASan Detects", "md_docs_architecture_memory_safety.html#autotoc_md57", null ],
        [ "Running with ASan", "md_docs_architecture_memory_safety.html#autotoc_md59", null ],
        [ "Performance Impact", "md_docs_architecture_memory_safety.html#autotoc_md61", null ]
      ] ],
      [ "Best Practices", "md_docs_architecture_memory_safety.html#autotoc_md63", [
        [ "1. Buffer Management", "md_docs_architecture_memory_safety.html#autotoc_md64", [
          [ "✅ DO", "md_docs_architecture_memory_safety.html#autotoc_md65", null ],
          [ "❌ DON'T", "md_docs_architecture_memory_safety.html#autotoc_md66", null ]
        ] ],
        [ "2. Type Conversions", "md_docs_architecture_memory_safety.html#autotoc_md68", [
          [ "✅ DO", "md_docs_architecture_memory_safety.html#autotoc_md69", null ],
          [ "❌ DON'T", "md_docs_architecture_memory_safety.html#autotoc_md70", null ]
        ] ],
        [ "3. Thread Safety", "md_docs_architecture_memory_safety.html#autotoc_md72", [
          [ "✅ DO", "md_docs_architecture_memory_safety.html#autotoc_md73", null ],
          [ "❌ DON'T", "md_docs_architecture_memory_safety.html#autotoc_md74", null ]
        ] ],
        [ "4. Memory Tracking", "md_docs_architecture_memory_safety.html#autotoc_md76", [
          [ "✅ DO", "md_docs_architecture_memory_safety.html#autotoc_md77", null ],
          [ "❌ DON'T", "md_docs_architecture_memory_safety.html#autotoc_md78", null ]
        ] ],
        [ "5. Sanitizers", "md_docs_architecture_memory_safety.html#autotoc_md80", [
          [ "✅ DO", "md_docs_architecture_memory_safety.html#autotoc_md81", null ],
          [ "❌ DON'T", "md_docs_architecture_memory_safety.html#autotoc_md82", null ]
        ] ]
      ] ],
      [ "Memory Safety Benefits", "md_docs_architecture_memory_safety.html#autotoc_md84", [
        [ "Prevents Common Vulnerabilities", "md_docs_architecture_memory_safety.html#autotoc_md85", null ],
        [ "Performance", "md_docs_architecture_memory_safety.html#autotoc_md87", null ]
      ] ],
      [ "Testing Memory Safety", "md_docs_architecture_memory_safety.html#autotoc_md89", [
        [ "Unit Tests", "md_docs_architecture_memory_safety.html#autotoc_md90", null ],
        [ "Integration Tests", "md_docs_architecture_memory_safety.html#autotoc_md92", null ],
        [ "Continuous Integration", "md_docs_architecture_memory_safety.html#autotoc_md94", null ]
      ] ],
      [ "Next Steps", "md_docs_architecture_memory_safety.html#autotoc_md96", null ]
    ] ],
    [ "Runtime Behavior Model", "md_docs_architecture_runtime_behavior.html", [
      [ "Table of Contents", "md_docs_architecture_runtime_behavior.html#autotoc_md176", null ],
      [ "Threading Model & Callback Execution", "md_docs_architecture_runtime_behavior.html#autotoc_md178", [
        [ "Architecture Diagram", "md_docs_architecture_runtime_behavior.html#autotoc_md179", null ],
        [ "Key Points", "md_docs_architecture_runtime_behavior.html#autotoc_md181", [
          [ "✅ Thread-Safe API Methods", "md_docs_architecture_runtime_behavior.html#autotoc_md182", null ],
          [ "✅ Callback Execution Context", "md_docs_architecture_runtime_behavior.html#autotoc_md184", null ],
          [ "⚠️ Never Block in Callbacks", "md_docs_architecture_runtime_behavior.html#autotoc_md186", null ],
          [ "✅ Thread-Safe State Access", "md_docs_architecture_runtime_behavior.html#autotoc_md188", null ]
        ] ],
        [ "Threading Model Summary", "md_docs_architecture_runtime_behavior.html#autotoc_md190", null ]
      ] ],
      [ "Reconnection Policy & State Machine", "md_docs_architecture_runtime_behavior.html#autotoc_md192", [
        [ "State Machine Diagram", "md_docs_architecture_runtime_behavior.html#autotoc_md193", null ],
        [ "Connection States", "md_docs_architecture_runtime_behavior.html#autotoc_md195", null ],
        [ "Configuration Example", "md_docs_architecture_runtime_behavior.html#autotoc_md197", null ],
        [ "Retry Behavior", "md_docs_architecture_runtime_behavior.html#autotoc_md199", [
          [ "Default Behavior", "md_docs_architecture_runtime_behavior.html#autotoc_md200", null ],
          [ "Retry Interval Configuration", "md_docs_architecture_runtime_behavior.html#autotoc_md201", null ],
          [ "State Callbacks", "md_docs_architecture_runtime_behavior.html#autotoc_md203", null ],
          [ "Manual Control", "md_docs_architecture_runtime_behavior.html#autotoc_md205", null ]
        ] ],
        [ "Reconnection Best Practices", "md_docs_architecture_runtime_behavior.html#autotoc_md207", [
          [ "1. Choose Appropriate Retry Interval", "md_docs_architecture_runtime_behavior.html#autotoc_md208", null ],
          [ "2. Handle State Transitions", "md_docs_architecture_runtime_behavior.html#autotoc_md210", null ],
          [ "3. Graceful Shutdown", "md_docs_architecture_runtime_behavior.html#autotoc_md212", null ]
        ] ]
      ] ],
      [ "Backpressure Handling", "md_docs_architecture_runtime_behavior.html#autotoc_md214", [
        [ "Backpressure Flow", "md_docs_architecture_runtime_behavior.html#autotoc_md215", null ],
        [ "Backpressure Configuration", "md_docs_architecture_runtime_behavior.html#autotoc_md217", null ],
        [ "Backpressure Strategies", "md_docs_architecture_runtime_behavior.html#autotoc_md219", [
          [ "Strategy 1: Pause Sending", "md_docs_architecture_runtime_behavior.html#autotoc_md220", null ],
          [ "Strategy 2: Rate Limiting", "md_docs_architecture_runtime_behavior.html#autotoc_md222", null ],
          [ "Strategy 3: Drop Data", "md_docs_architecture_runtime_behavior.html#autotoc_md224", null ]
        ] ],
        [ "Backpressure Monitoring", "md_docs_architecture_runtime_behavior.html#autotoc_md226", null ],
        [ "Memory Safety", "md_docs_architecture_runtime_behavior.html#autotoc_md228", null ]
      ] ],
      [ "Best Practices", "md_docs_architecture_runtime_behavior.html#autotoc_md230", [
        [ "1. Threading Best Practices", "md_docs_architecture_runtime_behavior.html#autotoc_md231", [
          [ "✅ DO", "md_docs_architecture_runtime_behavior.html#autotoc_md232", null ],
          [ "❌ DON'T", "md_docs_architecture_runtime_behavior.html#autotoc_md233", null ]
        ] ],
        [ "2. Reconnection Best Practices", "md_docs_architecture_runtime_behavior.html#autotoc_md235", [
          [ "✅ DO", "md_docs_architecture_runtime_behavior.html#autotoc_md236", null ],
          [ "❌ DON'T", "md_docs_architecture_runtime_behavior.html#autotoc_md237", null ]
        ] ],
        [ "3. Backpressure Best Practices", "md_docs_architecture_runtime_behavior.html#autotoc_md239", [
          [ "✅ DO", "md_docs_architecture_runtime_behavior.html#autotoc_md240", null ],
          [ "❌ DON'T", "md_docs_architecture_runtime_behavior.html#autotoc_md241", null ]
        ] ]
      ] ],
      [ "Performance Considerations", "md_docs_architecture_runtime_behavior.html#autotoc_md243", [
        [ "Threading Overhead", "md_docs_architecture_runtime_behavior.html#autotoc_md244", null ],
        [ "Reconnection Overhead", "md_docs_architecture_runtime_behavior.html#autotoc_md245", null ],
        [ "Backpressure Overhead", "md_docs_architecture_runtime_behavior.html#autotoc_md246", null ]
      ] ],
      [ "Next Steps", "md_docs_architecture_runtime_behavior.html#autotoc_md248", null ]
    ] ],
    [ "Performance Guide", "md_docs_guides_advanced_performance.html", [
      [ "Table of Contents", "md_docs_guides_advanced_performance.html#autotoc_md251", null ],
      [ "Performance Overview", "md_docs_guides_advanced_performance.html#autotoc_md253", [
        [ "Characteristics & Goals", "md_docs_guides_advanced_performance.html#autotoc_md254", null ]
      ] ],
      [ "Build Configuration Optimization", "md_docs_guides_advanced_performance.html#autotoc_md256", [
        [ "Minimal Build vs Full Build", "md_docs_guides_advanced_performance.html#autotoc_md257", null ],
        [ "Compiler Optimization Levels", "md_docs_guides_advanced_performance.html#autotoc_md258", null ]
      ] ],
      [ "Runtime Optimization", "md_docs_guides_advanced_performance.html#autotoc_md260", [
        [ "1. Threading Model & IO Context", "md_docs_guides_advanced_performance.html#autotoc_md261", null ],
        [ "2. Async Logging", "md_docs_guides_advanced_performance.html#autotoc_md262", null ],
        [ "3. Non-Blocking Callbacks", "md_docs_guides_advanced_performance.html#autotoc_md263", null ]
      ] ],
      [ "Memory Optimization", "md_docs_guides_advanced_performance.html#autotoc_md265", [
        [ "1. Memory Pool Usage", "md_docs_guides_advanced_performance.html#autotoc_md266", null ],
        [ "2. Avoid Data Copies", "md_docs_guides_advanced_performance.html#autotoc_md267", null ],
        [ "3. Reserve Vector Capacity", "md_docs_guides_advanced_performance.html#autotoc_md268", null ]
      ] ],
      [ "Network Optimization", "md_docs_guides_advanced_performance.html#autotoc_md270", [
        [ "1. Batch Small Messages", "md_docs_guides_advanced_performance.html#autotoc_md271", null ],
        [ "2. Connection Reuse", "md_docs_guides_advanced_performance.html#autotoc_md272", null ],
        [ "3. Socket Tuning", "md_docs_guides_advanced_performance.html#autotoc_md273", null ]
      ] ],
      [ "Benchmarking & Profiling", "md_docs_guides_advanced_performance.html#autotoc_md275", [
        [ "Built-in Performance Tests", "md_docs_guides_advanced_performance.html#autotoc_md276", null ],
        [ "Profiling with Tools", "md_docs_guides_advanced_performance.html#autotoc_md277", null ],
        [ "Simple Throughput Benchmark Code", "md_docs_guides_advanced_performance.html#autotoc_md278", null ]
      ] ],
      [ "Real-World Case Studies", "md_docs_guides_advanced_performance.html#autotoc_md280", [
        [ "Case Study 1: High-Throughput Data Streaming", "md_docs_guides_advanced_performance.html#autotoc_md281", null ],
        [ "Case Study 2: Low-Latency Trading System", "md_docs_guides_advanced_performance.html#autotoc_md282", null ],
        [ "Case Study 3: IoT Gateway (1000+ Connections)", "md_docs_guides_advanced_performance.html#autotoc_md283", null ]
      ] ]
    ] ],
    [ "Unilink Best Practices Guide", "md_docs_guides_core_best_practices.html", [
      [ "Table of Contents", "md_docs_guides_core_best_practices.html#autotoc_md286", null ],
      [ "Error Handling", "md_docs_guides_core_best_practices.html#autotoc_md288", [
        [ "✅ DO: Always Register Error Callbacks", "md_docs_guides_core_best_practices.html#autotoc_md289", null ],
        [ "✅ DO: Check Connection Status Before Sending", "md_docs_guides_core_best_practices.html#autotoc_md290", null ],
        [ "✅ DO: Implement Graceful Error Recovery", "md_docs_guides_core_best_practices.html#autotoc_md291", null ],
        [ "✅ DO: Use Centralized Error Handler", "md_docs_guides_core_best_practices.html#autotoc_md292", null ]
      ] ],
      [ "Resource Management", "md_docs_guides_core_best_practices.html#autotoc_md294", [
        [ "✅ DO: Use RAII and Smart Pointers", "md_docs_guides_core_best_practices.html#autotoc_md295", null ],
        [ "✅ DO: Stop Connections Before Shutdown", "md_docs_guides_core_best_practices.html#autotoc_md296", null ],
        [ "✅ DO: Reuse Connections When Possible", "md_docs_guides_core_best_practices.html#autotoc_md297", null ]
      ] ],
      [ "Thread Safety", "md_docs_guides_core_best_practices.html#autotoc_md299", [
        [ "✅ DO: Protect Shared State", "md_docs_guides_core_best_practices.html#autotoc_md300", null ],
        [ "✅ DO: Use ThreadSafeState for Complex States", "md_docs_guides_core_best_practices.html#autotoc_md301", null ],
        [ "❌ DON'T: Block in Callbacks", "md_docs_guides_core_best_practices.html#autotoc_md302", null ]
      ] ],
      [ "Performance Optimization", "md_docs_guides_core_best_practices.html#autotoc_md304", [
        [ "✅ DO: Use Move Semantics", "md_docs_guides_core_best_practices.html#autotoc_md305", null ],
        [ "✅ DO: Enable Async Logging", "md_docs_guides_core_best_practices.html#autotoc_md306", null ],
        [ "✅ DO: Use Shared IO Context (Default)", "md_docs_guides_core_best_practices.html#autotoc_md307", null ],
        [ "✅ DO: Batch Small Messages", "md_docs_guides_core_best_practices.html#autotoc_md308", null ]
      ] ],
      [ "Code Organization", "md_docs_guides_core_best_practices.html#autotoc_md310", [
        [ "✅ DO: Use Classes for Complex Logic", "md_docs_guides_core_best_practices.html#autotoc_md311", null ],
        [ "✅ DO: Separate Concerns", "md_docs_guides_core_best_practices.html#autotoc_md312", null ]
      ] ],
      [ "Testing", "md_docs_guides_core_best_practices.html#autotoc_md314", [
        [ "✅ DO: Use Dependency Injection", "md_docs_guides_core_best_practices.html#autotoc_md315", null ],
        [ "✅ DO: Test Error Scenarios", "md_docs_guides_core_best_practices.html#autotoc_md316", null ],
        [ "✅ DO: Use Independent Context for Tests", "md_docs_guides_core_best_practices.html#autotoc_md317", null ]
      ] ],
      [ "Security", "md_docs_guides_core_best_practices.html#autotoc_md319", [
        [ "✅ DO: Validate All Input", "md_docs_guides_core_best_practices.html#autotoc_md320", null ],
        [ "✅ DO: Implement Rate Limiting", "md_docs_guides_core_best_practices.html#autotoc_md321", null ],
        [ "✅ DO: Set Connection Limits", "md_docs_guides_core_best_practices.html#autotoc_md322", null ]
      ] ],
      [ "Logging and Debugging", "md_docs_guides_core_best_practices.html#autotoc_md324", [
        [ "✅ DO: Use Appropriate Log Levels", "md_docs_guides_core_best_practices.html#autotoc_md325", null ],
        [ "✅ DO: Enable Debug Logging During Development", "md_docs_guides_core_best_practices.html#autotoc_md326", null ],
        [ "✅ DO: Log Context Information", "md_docs_guides_core_best_practices.html#autotoc_md327", null ]
      ] ]
    ] ],
    [ "Unilink Quick Start Guide", "md_docs_guides_core_quickstart.html", [
      [ "Installation", "md_docs_guides_core_quickstart.html#autotoc_md329", [
        [ "Prerequisites", "md_docs_guides_core_quickstart.html#autotoc_md330", null ],
        [ "Build & Install", "md_docs_guides_core_quickstart.html#autotoc_md331", null ]
      ] ],
      [ "Your First TCP Client (30 seconds)", "md_docs_guides_core_quickstart.html#autotoc_md333", null ],
      [ "Your First TCP Server (30 seconds)", "md_docs_guides_core_quickstart.html#autotoc_md335", null ],
      [ "Your First Serial Device (30 seconds)", "md_docs_guides_core_quickstart.html#autotoc_md337", null ],
      [ "Common Patterns", "md_docs_guides_core_quickstart.html#autotoc_md339", [
        [ "Pattern 1: Auto-Reconnection", "md_docs_guides_core_quickstart.html#autotoc_md340", null ],
        [ "Pattern 2: Error Handling", "md_docs_guides_core_quickstart.html#autotoc_md341", null ],
        [ "Pattern 3: Member Function Callbacks", "md_docs_guides_core_quickstart.html#autotoc_md342", null ],
        [ "Pattern 4: Single vs Multi-Client Server (choose one)", "md_docs_guides_core_quickstart.html#autotoc_md343", null ]
      ] ],
      [ "Next Steps", "md_docs_guides_core_quickstart.html#autotoc_md345", null ],
      [ "Troubleshooting", "md_docs_guides_core_quickstart.html#autotoc_md347", [
        [ "Can't connect to server?", "md_docs_guides_core_quickstart.html#autotoc_md348", null ],
        [ "Port already in use?", "md_docs_guides_core_quickstart.html#autotoc_md349", null ],
        [ "Need independent IO thread?", "md_docs_guides_core_quickstart.html#autotoc_md350", null ]
      ] ],
      [ "Support", "md_docs_guides_core_quickstart.html#autotoc_md352", null ]
    ] ],
    [ "Testing Guide", "md_docs_guides_core_testing.html", [
      [ "Table of Contents", "md_docs_guides_core_testing.html#autotoc_md355", null ],
      [ "Quick Start", "md_docs_guides_core_testing.html#autotoc_md357", [
        [ "Build and Run All Tests", "md_docs_guides_core_testing.html#autotoc_md358", null ],
        [ "Windows Build & Test Workflow", "md_docs_guides_core_testing.html#autotoc_md360", null ]
      ] ],
      [ "Running Tests", "md_docs_guides_core_testing.html#autotoc_md362", [
        [ "Run All Tests", "md_docs_guides_core_testing.html#autotoc_md363", null ],
        [ "Run Specific Test Categories", "md_docs_guides_core_testing.html#autotoc_md365", null ],
        [ "Run Tests with Verbose Output", "md_docs_guides_core_testing.html#autotoc_md367", null ],
        [ "Run Tests in Parallel", "md_docs_guides_core_testing.html#autotoc_md369", null ]
      ] ],
      [ "UDP-specific test policies", "md_docs_guides_core_testing.html#autotoc_md371", null ],
      [ "Test Categories", "md_docs_guides_core_testing.html#autotoc_md372", [
        [ "Core Tests", "md_docs_guides_core_testing.html#autotoc_md373", null ],
        [ "Integration Tests", "md_docs_guides_core_testing.html#autotoc_md375", null ],
        [ "Memory Safety Tests", "md_docs_guides_core_testing.html#autotoc_md377", null ],
        [ "Concurrency Safety Tests", "md_docs_guides_core_testing.html#autotoc_md379", null ],
        [ "Performance Tests", "md_docs_guides_core_testing.html#autotoc_md382", null ],
        [ "Stress Tests", "md_docs_guides_core_testing.html#autotoc_md384", null ]
      ] ],
      [ "Memory Safety Validation", "md_docs_guides_core_testing.html#autotoc_md386", [
        [ "Built-in Memory Tracking", "md_docs_guides_core_testing.html#autotoc_md387", null ],
        [ "AddressSanitizer (ASan)", "md_docs_guides_core_testing.html#autotoc_md389", null ],
        [ "ThreadSanitizer (TSan)", "md_docs_guides_core_testing.html#autotoc_md391", null ],
        [ "Valgrind", "md_docs_guides_core_testing.html#autotoc_md393", null ]
      ] ],
      [ "Continuous Integration", "md_docs_guides_core_testing.html#autotoc_md395", [
        [ "GitHub Actions Integration", "md_docs_guides_core_testing.html#autotoc_md396", null ],
        [ "CI/CD Build Matrix", "md_docs_guides_core_testing.html#autotoc_md398", null ],
        [ "Ubuntu 20.04 Support", "md_docs_guides_core_testing.html#autotoc_md400", null ],
        [ "View CI/CD Results", "md_docs_guides_core_testing.html#autotoc_md402", null ]
      ] ],
      [ "Writing Custom Tests", "md_docs_guides_core_testing.html#autotoc_md404", [
        [ "Test Structure", "md_docs_guides_core_testing.html#autotoc_md405", null ],
        [ "Example: Custom Integration Test", "md_docs_guides_core_testing.html#autotoc_md407", null ],
        [ "Running Custom Tests", "md_docs_guides_core_testing.html#autotoc_md409", null ]
      ] ],
      [ "Test Configuration", "md_docs_guides_core_testing.html#autotoc_md411", [
        [ "CTest Configuration", "md_docs_guides_core_testing.html#autotoc_md412", null ],
        [ "Environment Variables", "md_docs_guides_core_testing.html#autotoc_md414", null ]
      ] ],
      [ "Troubleshooting Tests", "md_docs_guides_core_testing.html#autotoc_md416", [
        [ "Test Failures", "md_docs_guides_core_testing.html#autotoc_md417", null ],
        [ "Port Conflicts", "md_docs_guides_core_testing.html#autotoc_md419", null ],
        [ "Memory Issues", "md_docs_guides_core_testing.html#autotoc_md421", null ]
      ] ],
      [ "Performance Regression Testing", "md_docs_guides_core_testing.html#autotoc_md423", [
        [ "Benchmark Baseline", "md_docs_guides_core_testing.html#autotoc_md424", null ],
        [ "Compare Against Baseline", "md_docs_guides_core_testing.html#autotoc_md425", null ],
        [ "Automated Regression Detection", "md_docs_guides_core_testing.html#autotoc_md426", null ]
      ] ],
      [ "Code Coverage", "md_docs_guides_core_testing.html#autotoc_md428", [
        [ "Generate Coverage Report", "md_docs_guides_core_testing.html#autotoc_md429", null ],
        [ "View HTML Coverage Report", "md_docs_guides_core_testing.html#autotoc_md430", null ]
      ] ],
      [ "Next Steps", "md_docs_guides_core_testing.html#autotoc_md432", null ]
    ] ],
    [ "Troubleshooting Guide", "md_docs_guides_core_troubleshooting.html", [
      [ "Table of Contents", "md_docs_guides_core_troubleshooting.html#autotoc_md435", null ],
      [ "Connection Issues", "md_docs_guides_core_troubleshooting.html#autotoc_md437", [
        [ "Problem: Connection Refused", "md_docs_guides_core_troubleshooting.html#autotoc_md438", [
          [ "1. Server Not Running", "md_docs_guides_core_troubleshooting.html#autotoc_md439", null ],
          [ "2. Wrong Host/Port", "md_docs_guides_core_troubleshooting.html#autotoc_md440", null ],
          [ "3. Firewall Blocking", "md_docs_guides_core_troubleshooting.html#autotoc_md441", null ]
        ] ],
        [ "Problem: Connection Timeout", "md_docs_guides_core_troubleshooting.html#autotoc_md443", [
          [ "1. Network Unreachable", "md_docs_guides_core_troubleshooting.html#autotoc_md444", null ],
          [ "2. Server Overloaded", "md_docs_guides_core_troubleshooting.html#autotoc_md445", null ],
          [ "3. Slow Network", "md_docs_guides_core_troubleshooting.html#autotoc_md446", null ]
        ] ],
        [ "Problem: Connection Drops Randomly", "md_docs_guides_core_troubleshooting.html#autotoc_md448", [
          [ "1. Network Instability", "md_docs_guides_core_troubleshooting.html#autotoc_md449", null ],
          [ "2. Server Closing Connection", "md_docs_guides_core_troubleshooting.html#autotoc_md450", null ],
          [ "3. Keep-Alive Not Set", "md_docs_guides_core_troubleshooting.html#autotoc_md451", null ]
        ] ],
        [ "Problem: Port Already in Use", "md_docs_guides_core_troubleshooting.html#autotoc_md453", [
          [ "1. Kill Existing Process", "md_docs_guides_core_troubleshooting.html#autotoc_md454", null ],
          [ "2. Use Different Port", "md_docs_guides_core_troubleshooting.html#autotoc_md455", null ],
          [ "3. Enable Port Retry", "md_docs_guides_core_troubleshooting.html#autotoc_md456", null ],
          [ "4. Set SO_REUSEADDR (Advanced)", "md_docs_guides_core_troubleshooting.html#autotoc_md457", null ]
        ] ]
      ] ],
      [ "Compilation Errors", "md_docs_guides_core_troubleshooting.html#autotoc_md459", [
        [ "Problem: unilink/unilink.hpp Not Found", "md_docs_guides_core_troubleshooting.html#autotoc_md460", [
          [ "1. Install unilink", "md_docs_guides_core_troubleshooting.html#autotoc_md461", null ],
          [ "2. Add Include Path", "md_docs_guides_core_troubleshooting.html#autotoc_md462", null ],
          [ "3. Use as Subdirectory", "md_docs_guides_core_troubleshooting.html#autotoc_md463", null ]
        ] ],
        [ "Problem: Undefined Reference to unilink Symbols", "md_docs_guides_core_troubleshooting.html#autotoc_md465", [
          [ "1. Link unilink Library", "md_docs_guides_core_troubleshooting.html#autotoc_md466", null ],
          [ "2. Check Library Path", "md_docs_guides_core_troubleshooting.html#autotoc_md467", null ]
        ] ],
        [ "Problem: Boost Not Found", "md_docs_guides_core_troubleshooting.html#autotoc_md469", [
          [ "Ubuntu/Debian", "md_docs_guides_core_troubleshooting.html#autotoc_md470", null ],
          [ "macOS", "md_docs_guides_core_troubleshooting.html#autotoc_md471", null ],
          [ "Windows (vcpkg)", "md_docs_guides_core_troubleshooting.html#autotoc_md472", null ],
          [ "Manual Boost Path", "md_docs_guides_core_troubleshooting.html#autotoc_md473", null ]
        ] ]
      ] ],
      [ "Runtime Errors", "md_docs_guides_core_troubleshooting.html#autotoc_md475", [
        [ "Problem: Segmentation Fault", "md_docs_guides_core_troubleshooting.html#autotoc_md476", [
          [ "1. Enable Core Dumps", "md_docs_guides_core_troubleshooting.html#autotoc_md477", null ],
          [ "2. Use AddressSanitizer", "md_docs_guides_core_troubleshooting.html#autotoc_md478", null ],
          [ "3. Common Causes", "md_docs_guides_core_troubleshooting.html#autotoc_md479", null ]
        ] ],
        [ "Problem: Callbacks Not Being Called", "md_docs_guides_core_troubleshooting.html#autotoc_md481", [
          [ "1. Callback Not Registered", "md_docs_guides_core_troubleshooting.html#autotoc_md482", null ],
          [ "2. Client Not Started", "md_docs_guides_core_troubleshooting.html#autotoc_md483", null ],
          [ "3. Application Exits Too Quickly", "md_docs_guides_core_troubleshooting.html#autotoc_md484", null ]
        ] ]
      ] ],
      [ "Performance Issues", "md_docs_guides_core_troubleshooting.html#autotoc_md486", [
        [ "Problem: High CPU Usage", "md_docs_guides_core_troubleshooting.html#autotoc_md487", [
          [ "1. Busy Loop in Callback", "md_docs_guides_core_troubleshooting.html#autotoc_md488", null ],
          [ "2. Too Many Retries", "md_docs_guides_core_troubleshooting.html#autotoc_md489", null ],
          [ "3. Excessive Logging", "md_docs_guides_core_troubleshooting.html#autotoc_md490", null ]
        ] ],
        [ "Problem: High Memory Usage", "md_docs_guides_core_troubleshooting.html#autotoc_md492", [
          [ "1. Enable Memory Tracking (Debug)", "md_docs_guides_core_troubleshooting.html#autotoc_md493", null ],
          [ "2. Fix Memory Leaks", "md_docs_guides_core_troubleshooting.html#autotoc_md494", null ],
          [ "3. Limit Buffer Sizes", "md_docs_guides_core_troubleshooting.html#autotoc_md495", null ]
        ] ],
        [ "Problem: Slow Data Transfer", "md_docs_guides_core_troubleshooting.html#autotoc_md497", [
          [ "1. Batch Small Messages", "md_docs_guides_core_troubleshooting.html#autotoc_md498", null ],
          [ "2. Use Binary Protocol", "md_docs_guides_core_troubleshooting.html#autotoc_md499", null ],
          [ "3. Enable Async Logging", "md_docs_guides_core_troubleshooting.html#autotoc_md500", null ]
        ] ]
      ] ],
      [ "Memory Issues", "md_docs_guides_core_troubleshooting.html#autotoc_md502", [
        [ "Problem: Memory Leak Detected", "md_docs_guides_core_troubleshooting.html#autotoc_md503", null ]
      ] ],
      [ "Thread Safety Issues", "md_docs_guides_core_troubleshooting.html#autotoc_md505", [
        [ "Problem: Race Condition / Data Corruption", "md_docs_guides_core_troubleshooting.html#autotoc_md506", [
          [ "1. Protect Shared State", "md_docs_guides_core_troubleshooting.html#autotoc_md507", null ],
          [ "2. Use Thread-Safe Containers", "md_docs_guides_core_troubleshooting.html#autotoc_md508", null ]
        ] ]
      ] ],
      [ "Debugging Tips", "md_docs_guides_core_troubleshooting.html#autotoc_md510", [
        [ "Enable Debug Logging", "md_docs_guides_core_troubleshooting.html#autotoc_md511", null ],
        [ "Use GDB for Debugging", "md_docs_guides_core_troubleshooting.html#autotoc_md512", null ],
        [ "Network Debugging with tcpdump", "md_docs_guides_core_troubleshooting.html#autotoc_md513", null ],
        [ "Test with netcat", "md_docs_guides_core_troubleshooting.html#autotoc_md514", null ]
      ] ],
      [ "Getting Help", "md_docs_guides_core_troubleshooting.html#autotoc_md516", null ]
    ] ],
    [ "Build Guide", "md_docs_guides_setup_build_guide.html", [
      [ "</blockquote>", "md_docs_guides_setup_build_guide.html#autotoc_md519", null ],
      [ "Table of Contents", "md_docs_guides_setup_build_guide.html#autotoc_md520", null ],
      [ "Quick Build", "md_docs_guides_setup_build_guide.html#autotoc_md522", [
        [ "Basic Build (Recommended)", "md_docs_guides_setup_build_guide.html#autotoc_md523", null ]
      ] ],
      [ "Important Build Notes", "md_docs_guides_setup_build_guide.html#autotoc_md525", null ],
      [ "Build Configurations", "md_docs_guides_setup_build_guide.html#autotoc_md527", [
        [ "Minimal Build (Builder API only)", "md_docs_guides_setup_build_guide.html#autotoc_md528", null ],
        [ "Full Build (includes Configuration Management API)", "md_docs_guides_setup_build_guide.html#autotoc_md530", null ]
      ] ],
      [ "Build Options Reference", "md_docs_guides_setup_build_guide.html#autotoc_md532", [
        [ "Core Options", "md_docs_guides_setup_build_guide.html#autotoc_md533", null ],
        [ "Development Options", "md_docs_guides_setup_build_guide.html#autotoc_md534", null ],
        [ "Installation Options", "md_docs_guides_setup_build_guide.html#autotoc_md535", null ]
      ] ],
      [ "Build Types Comparison", "md_docs_guides_setup_build_guide.html#autotoc_md537", [
        [ "Release Build (Default)", "md_docs_guides_setup_build_guide.html#autotoc_md538", null ],
        [ "Debug Build", "md_docs_guides_setup_build_guide.html#autotoc_md540", null ],
        [ "RelWithDebInfo Build", "md_docs_guides_setup_build_guide.html#autotoc_md542", null ]
      ] ],
      [ "Advanced Build Examples", "md_docs_guides_setup_build_guide.html#autotoc_md544", [
        [ "Example 1: Minimal Production Build", "md_docs_guides_setup_build_guide.html#autotoc_md545", null ],
        [ "Example 2: Development Build with Examples", "md_docs_guides_setup_build_guide.html#autotoc_md547", null ],
        [ "Example 3: Testing with Sanitizers", "md_docs_guides_setup_build_guide.html#autotoc_md549", null ],
        [ "Example 4: Build with Custom Boost Location", "md_docs_guides_setup_build_guide.html#autotoc_md551", null ],
        [ "Example 5: Build with Specific Compiler", "md_docs_guides_setup_build_guide.html#autotoc_md553", null ]
      ] ],
      [ "Platform-Specific Builds", "md_docs_guides_setup_build_guide.html#autotoc_md555", [
        [ "Ubuntu 22.04 (Recommended)", "md_docs_guides_setup_build_guide.html#autotoc_md556", null ],
        [ "Ubuntu 20.04 Build", "md_docs_guides_setup_build_guide.html#autotoc_md558", [
          [ "Prerequisites", "md_docs_guides_setup_build_guide.html#autotoc_md559", null ],
          [ "Build Steps", "md_docs_guides_setup_build_guide.html#autotoc_md560", null ],
          [ "Notes", "md_docs_guides_setup_build_guide.html#autotoc_md561", null ]
        ] ],
        [ "Debian 11+", "md_docs_guides_setup_build_guide.html#autotoc_md563", null ],
        [ "Fedora 35+", "md_docs_guides_setup_build_guide.html#autotoc_md565", null ],
        [ "Arch Linux", "md_docs_guides_setup_build_guide.html#autotoc_md567", null ]
      ] ],
      [ "Build Performance Tips", "md_docs_guides_setup_build_guide.html#autotoc_md569", [
        [ "Parallel Builds", "md_docs_guides_setup_build_guide.html#autotoc_md570", null ],
        [ "Ccache for Faster Rebuilds", "md_docs_guides_setup_build_guide.html#autotoc_md571", null ],
        [ "Ninja Build System (Faster than Make)", "md_docs_guides_setup_build_guide.html#autotoc_md572", null ]
      ] ],
      [ "Installation", "md_docs_guides_setup_build_guide.html#autotoc_md574", [
        [ "System-Wide Installation", "md_docs_guides_setup_build_guide.html#autotoc_md575", null ],
        [ "Custom Installation Directory", "md_docs_guides_setup_build_guide.html#autotoc_md576", null ],
        [ "Uninstall", "md_docs_guides_setup_build_guide.html#autotoc_md577", null ]
      ] ],
      [ "Verifying the Build", "md_docs_guides_setup_build_guide.html#autotoc_md579", [
        [ "Run Unit Tests", "md_docs_guides_setup_build_guide.html#autotoc_md580", null ],
        [ "Run Examples", "md_docs_guides_setup_build_guide.html#autotoc_md581", null ],
        [ "Check Library Symbols", "md_docs_guides_setup_build_guide.html#autotoc_md582", null ]
      ] ],
      [ "Troubleshooting", "md_docs_guides_setup_build_guide.html#autotoc_md584", [
        [ "Problem: CMake Can't Find Boost", "md_docs_guides_setup_build_guide.html#autotoc_md585", null ],
        [ "Problem: Compiler Not Found", "md_docs_guides_setup_build_guide.html#autotoc_md586", null ],
        [ "Problem: Out of Memory During Build", "md_docs_guides_setup_build_guide.html#autotoc_md587", null ],
        [ "Problem: Permission Denied During Install", "md_docs_guides_setup_build_guide.html#autotoc_md588", null ]
      ] ],
      [ "CMake Package Integration", "md_docs_guides_setup_build_guide.html#autotoc_md590", [
        [ "Using the Installed Package", "md_docs_guides_setup_build_guide.html#autotoc_md591", null ],
        [ "Custom Installation Prefix", "md_docs_guides_setup_build_guide.html#autotoc_md592", null ],
        [ "Package Components", "md_docs_guides_setup_build_guide.html#autotoc_md593", null ],
        [ "Verification", "md_docs_guides_setup_build_guide.html#autotoc_md594", null ]
      ] ],
      [ "Next Steps", "md_docs_guides_setup_build_guide.html#autotoc_md596", null ]
    ] ],
    [ "Installation Guide", "md_docs_guides_setup_installation.html", [
      [ "Prerequisites", "md_docs_guides_setup_installation.html#autotoc_md598", null ],
      [ "Installation Methods", "md_docs_guides_setup_installation.html#autotoc_md599", [
        [ "Method 1: vcpkg (Recommended)", "md_docs_guides_setup_installation.html#autotoc_md600", [
          [ "Step 1: Install via vcpkg", "md_docs_guides_setup_installation.html#autotoc_md601", null ],
          [ "Step 2: Use in your project", "md_docs_guides_setup_installation.html#autotoc_md602", null ]
        ] ],
        [ "Method 2: Install from Source (CMake Package)", "md_docs_guides_setup_installation.html#autotoc_md603", [
          [ "Step 1: Build and install", "md_docs_guides_setup_installation.html#autotoc_md604", null ],
          [ "Step 2: Use in your project", "md_docs_guides_setup_installation.html#autotoc_md605", null ]
        ] ],
        [ "Method 3: Release Packages", "md_docs_guides_setup_installation.html#autotoc_md606", [
          [ "Step 1: Download and extract", "md_docs_guides_setup_installation.html#autotoc_md607", null ],
          [ "Step 2: Install", "md_docs_guides_setup_installation.html#autotoc_md608", null ],
          [ "Step 3: Use in your project", "md_docs_guides_setup_installation.html#autotoc_md609", null ]
        ] ],
        [ "Method 4: Git Submodule Integration", "md_docs_guides_setup_installation.html#autotoc_md610", [
          [ "Step 1: Add submodule", "md_docs_guides_setup_installation.html#autotoc_md611", null ],
          [ "Step 2: Use in CMake", "md_docs_guides_setup_installation.html#autotoc_md612", null ]
        ] ]
      ] ],
      [ "Packaging Notes", "md_docs_guides_setup_installation.html#autotoc_md613", null ],
      [ "Build Options (Source Builds)", "md_docs_guides_setup_installation.html#autotoc_md614", null ],
      [ "Next Steps", "md_docs_guides_setup_installation.html#autotoc_md615", null ]
    ] ],
    [ "System Requirements", "md_docs_guides_setup_requirements.html", [
      [ "System Requirements", "md_docs_guides_setup_requirements.html#autotoc_md618", [
        [ "Recommended Platform", "md_docs_guides_setup_requirements.html#autotoc_md619", null ],
        [ "Supported Platforms", "md_docs_guides_setup_requirements.html#autotoc_md620", null ]
      ] ],
      [ "Dependencies", "md_docs_guides_setup_requirements.html#autotoc_md622", [
        [ "Core Library Dependencies", "md_docs_guides_setup_requirements.html#autotoc_md623", null ],
        [ "Dependency Details", "md_docs_guides_setup_requirements.html#autotoc_md624", null ]
      ] ],
      [ "Optional Dependencies", "md_docs_guides_setup_requirements.html#autotoc_md626", [
        [ "For Documentation Generation", "md_docs_guides_setup_requirements.html#autotoc_md627", null ],
        [ "For Development", "md_docs_guides_setup_requirements.html#autotoc_md628", null ]
      ] ],
      [ "Compiler Requirements", "md_docs_guides_setup_requirements.html#autotoc_md630", [
        [ "Minimum Compiler Versions", "md_docs_guides_setup_requirements.html#autotoc_md631", null ],
        [ "C++ Standard", "md_docs_guides_setup_requirements.html#autotoc_md632", null ],
        [ "Compiler Features Required", "md_docs_guides_setup_requirements.html#autotoc_md633", null ]
      ] ],
      [ "Build Environment", "md_docs_guides_setup_requirements.html#autotoc_md635", [
        [ "Disk Space", "md_docs_guides_setup_requirements.html#autotoc_md636", null ],
        [ "Memory Requirements", "md_docs_guides_setup_requirements.html#autotoc_md637", null ],
        [ "CPU", "md_docs_guides_setup_requirements.html#autotoc_md638", null ]
      ] ],
      [ "Runtime Requirements", "md_docs_guides_setup_requirements.html#autotoc_md640", [
        [ "For Applications Using unilink", "md_docs_guides_setup_requirements.html#autotoc_md641", null ],
        [ "Thread Support", "md_docs_guides_setup_requirements.html#autotoc_md642", null ]
      ] ],
      [ "Platform-Specific Notes", "md_docs_guides_setup_requirements.html#autotoc_md644", [
        [ "Ubuntu 22.04 LTS", "md_docs_guides_setup_requirements.html#autotoc_md645", null ],
        [ "Ubuntu 20.04 LTS", "md_docs_guides_setup_requirements.html#autotoc_md646", null ],
        [ "Other Linux Distributions", "md_docs_guides_setup_requirements.html#autotoc_md647", null ]
      ] ],
      [ "Verifying Your Environment", "md_docs_guides_setup_requirements.html#autotoc_md649", [
        [ "Check Compiler Version", "md_docs_guides_setup_requirements.html#autotoc_md650", null ],
        [ "Check CMake Version", "md_docs_guides_setup_requirements.html#autotoc_md651", null ],
        [ "Check Boost Version", "md_docs_guides_setup_requirements.html#autotoc_md652", null ],
        [ "Quick Environment Test", "md_docs_guides_setup_requirements.html#autotoc_md653", null ]
      ] ],
      [ "Troubleshooting", "md_docs_guides_setup_requirements.html#autotoc_md655", [
        [ "Problem: Compiler Too Old", "md_docs_guides_setup_requirements.html#autotoc_md656", null ],
        [ "Problem: Boost Not Found", "md_docs_guides_setup_requirements.html#autotoc_md657", null ],
        [ "Problem: CMake Too Old", "md_docs_guides_setup_requirements.html#autotoc_md658", null ]
      ] ],
      [ "Next Steps", "md_docs_guides_setup_requirements.html#autotoc_md660", null ]
    ] ],
    [ "Unilink Documentation Index", "md_docs_index.html", [
      [ "📚 Documentation Structure", "md_docs_index.html#autotoc_md663", null ],
      [ "🚀 Getting Started", "md_docs_index.html#autotoc_md665", [
        [ "New to Unilink?", "md_docs_index.html#autotoc_md666", null ],
        [ "Core Documentation", "md_docs_index.html#autotoc_md667", null ]
      ] ],
      [ "📖 Tutorials", "md_docs_index.html#autotoc_md669", [
        [ "Beginner Tutorials", "md_docs_index.html#autotoc_md670", null ],
        [ "Coming Soon", "md_docs_index.html#autotoc_md671", null ]
      ] ],
      [ "📋 Guides", "md_docs_index.html#autotoc_md673", [
        [ "Setup Guides", "md_docs_index.html#autotoc_md674", null ],
        [ "Core Guides", "md_docs_index.html#autotoc_md675", null ],
        [ "Advanced Guides", "md_docs_index.html#autotoc_md676", null ],
        [ "Quick Reference", "md_docs_index.html#autotoc_md677", null ]
      ] ],
      [ "📚 API Reference", "md_docs_index.html#autotoc_md679", [
        [ "Core APIs", "md_docs_index.html#autotoc_md680", null ],
        [ "Advanced Features", "md_docs_index.html#autotoc_md681", null ]
      ] ],
      [ "🏗️ Architecture", "md_docs_index.html#autotoc_md683", [
        [ "Architecture Documentation", "md_docs_index.html#autotoc_md684", null ],
        [ "Key Concepts", "md_docs_index.html#autotoc_md685", null ]
      ] ],
      [ "🔧 Development", "md_docs_index.html#autotoc_md687", [
        [ "Development Documentation", "md_docs_index.html#autotoc_md688", null ],
        [ "Build Options", "md_docs_index.html#autotoc_md689", null ]
      ] ],
      [ "💡 Examples", "md_docs_index.html#autotoc_md691", [
        [ "Example Applications", "md_docs_index.html#autotoc_md692", null ],
        [ "Code Snippets", "md_docs_index.html#autotoc_md693", null ]
      ] ],
      [ "🔍 Search & Find", "md_docs_index.html#autotoc_md695", [
        [ "By Topic", "md_docs_index.html#autotoc_md696", null ],
        [ "By Use Case", "md_docs_index.html#autotoc_md697", null ]
      ] ],
      [ "📊 Documentation Stats", "md_docs_index.html#autotoc_md699", null ],
      [ "🆘 Need Help?", "md_docs_index.html#autotoc_md701", [
        [ "Quick Links", "md_docs_index.html#autotoc_md702", null ],
        [ "Learning Path", "md_docs_index.html#autotoc_md703", null ]
      ] ],
      [ "📝 Document History", "md_docs_index.html#autotoc_md705", null ],
      [ "🤝 Contributing", "md_docs_index.html#autotoc_md707", null ]
    ] ],
    [ "Unilink API Guide", "md_docs_reference_api_guide.html", [
      [ "Table of Contents", "md_docs_reference_api_guide.html#autotoc_md735", null ],
      [ "Builder API", "md_docs_reference_api_guide.html#autotoc_md737", [
        [ "Core Concept", "md_docs_reference_api_guide.html#autotoc_md738", null ],
        [ "Common Methods (All Builders)", "md_docs_reference_api_guide.html#autotoc_md739", null ],
        [ "Efficient Data Handling with SafeSpan", "md_docs_reference_api_guide.html#autotoc_md740", null ],
        [ "IO Context Ownership (advanced)", "md_docs_reference_api_guide.html#autotoc_md741", null ]
      ] ],
      [ "TCP Client", "md_docs_reference_api_guide.html#autotoc_md743", [
        [ "Basic Usage", "md_docs_reference_api_guide.html#autotoc_md744", null ],
        [ "API Reference", "md_docs_reference_api_guide.html#autotoc_md745", [
          [ "Constructor", "md_docs_reference_api_guide.html#autotoc_md746", null ],
          [ "Builder Methods", "md_docs_reference_api_guide.html#autotoc_md747", null ],
          [ "Instance Methods", "md_docs_reference_api_guide.html#autotoc_md748", null ]
        ] ],
        [ "Advanced Examples", "md_docs_reference_api_guide.html#autotoc_md749", [
          [ "With Member Functions", "md_docs_reference_api_guide.html#autotoc_md750", null ],
          [ "With Lambda Capture", "md_docs_reference_api_guide.html#autotoc_md751", null ]
        ] ]
      ] ],
      [ "TCP Server", "md_docs_reference_api_guide.html#autotoc_md753", [
        [ "Basic Usage", "md_docs_reference_api_guide.html#autotoc_md754", null ],
        [ "API Reference", "md_docs_reference_api_guide.html#autotoc_md755", [
          [ "Constructor", "md_docs_reference_api_guide.html#autotoc_md756", null ],
          [ "Builder Methods", "md_docs_reference_api_guide.html#autotoc_md757", null ],
          [ "Instance Methods", "md_docs_reference_api_guide.html#autotoc_md758", null ]
        ] ],
        [ "Advanced Examples", "md_docs_reference_api_guide.html#autotoc_md759", [
          [ "Single Client Mode", "md_docs_reference_api_guide.html#autotoc_md760", null ],
          [ "Port Retry", "md_docs_reference_api_guide.html#autotoc_md761", null ],
          [ "Echo Server Pattern", "md_docs_reference_api_guide.html#autotoc_md762", null ]
        ] ]
      ] ],
      [ "Serial Communication", "md_docs_reference_api_guide.html#autotoc_md764", [
        [ "Basic Usage", "md_docs_reference_api_guide.html#autotoc_md765", null ],
        [ "API Reference", "md_docs_reference_api_guide.html#autotoc_md766", [
          [ "Constructor", "md_docs_reference_api_guide.html#autotoc_md767", null ],
          [ "Builder Methods", "md_docs_reference_api_guide.html#autotoc_md768", null ],
          [ "Instance Methods", "md_docs_reference_api_guide.html#autotoc_md769", null ]
        ] ],
        [ "Device Paths", "md_docs_reference_api_guide.html#autotoc_md770", null ],
        [ "Advanced Examples", "md_docs_reference_api_guide.html#autotoc_md771", [
          [ "Arduino Communication", "md_docs_reference_api_guide.html#autotoc_md772", null ],
          [ "GPS Module", "md_docs_reference_api_guide.html#autotoc_md773", null ]
        ] ]
      ] ],
      [ "UDP Communication", "md_docs_reference_api_guide.html#autotoc_md775", [
        [ "Basic Usage", "md_docs_reference_api_guide.html#autotoc_md776", [
          [ "UDP Receiver (Server)", "md_docs_reference_api_guide.html#autotoc_md777", null ],
          [ "UDP Sender (Client)", "md_docs_reference_api_guide.html#autotoc_md778", null ]
        ] ],
        [ "API Reference", "md_docs_reference_api_guide.html#autotoc_md779", [
          [ "Constructor", "md_docs_reference_api_guide.html#autotoc_md780", null ],
          [ "Builder Methods", "md_docs_reference_api_guide.html#autotoc_md781", null ],
          [ "Instance Methods", "md_docs_reference_api_guide.html#autotoc_md782", null ]
        ] ],
        [ "Advanced Examples", "md_docs_reference_api_guide.html#autotoc_md783", [
          [ "Echo Reply (Receiver)", "md_docs_reference_api_guide.html#autotoc_md784", null ]
        ] ]
      ] ],
      [ "Error Handling", "md_docs_reference_api_guide.html#autotoc_md786", [
        [ "Setup Error Handler", "md_docs_reference_api_guide.html#autotoc_md787", null ],
        [ "Error Levels", "md_docs_reference_api_guide.html#autotoc_md788", null ],
        [ "Error Statistics", "md_docs_reference_api_guide.html#autotoc_md789", null ],
        [ "Error Categories", "md_docs_reference_api_guide.html#autotoc_md790", null ]
      ] ],
      [ "Logging System", "md_docs_reference_api_guide.html#autotoc_md792", [
        [ "Basic Usage", "md_docs_reference_api_guide.html#autotoc_md793", null ],
        [ "Log Levels", "md_docs_reference_api_guide.html#autotoc_md794", null ],
        [ "Async Logging", "md_docs_reference_api_guide.html#autotoc_md795", null ],
        [ "Log Rotation", "md_docs_reference_api_guide.html#autotoc_md796", null ]
      ] ],
      [ "Configuration Management", "md_docs_reference_api_guide.html#autotoc_md798", [
        [ "Load Configuration from File", "md_docs_reference_api_guide.html#autotoc_md799", null ],
        [ "Configuration File Format (JSON)", "md_docs_reference_api_guide.html#autotoc_md800", null ]
      ] ],
      [ "Advanced Features", "md_docs_reference_api_guide.html#autotoc_md802", [
        [ "Memory Pool", "md_docs_reference_api_guide.html#autotoc_md803", null ],
        [ "Safe Data Buffer", "md_docs_reference_api_guide.html#autotoc_md804", null ],
        [ "Thread-Safe State", "md_docs_reference_api_guide.html#autotoc_md805", null ]
      ] ],
      [ "Best Practices", "md_docs_reference_api_guide.html#autotoc_md807", [
        [ "1. Always Handle Errors", "md_docs_reference_api_guide.html#autotoc_md808", null ],
        [ "2. Use Explicit Lifecycle Control", "md_docs_reference_api_guide.html#autotoc_md809", null ],
        [ "3. Set Appropriate Retry Intervals", "md_docs_reference_api_guide.html#autotoc_md810", null ],
        [ "4. Enable Logging for Debugging", "md_docs_reference_api_guide.html#autotoc_md811", null ],
        [ "5. Use Member Functions for OOP Design", "md_docs_reference_api_guide.html#autotoc_md812", null ]
      ] ],
      [ "Performance Tips", "md_docs_reference_api_guide.html#autotoc_md814", [
        [ "1. Use Independent Context for Testing Only", "md_docs_reference_api_guide.html#autotoc_md815", null ],
        [ "2. Enable Async Logging", "md_docs_reference_api_guide.html#autotoc_md816", null ],
        [ "3. Use Memory Pool for Frequent Allocations", "md_docs_reference_api_guide.html#autotoc_md817", null ],
        [ "4. Disable Unnecessary Features", "md_docs_reference_api_guide.html#autotoc_md818", null ]
      ] ]
    ] ],
    [ "Test Structure", "md_docs_test_structure.html", [
      [ "Overview", "md_docs_test_structure.html#autotoc_md821", null ],
      [ "Directory Structure", "md_docs_test_structure.html#autotoc_md823", null ],
      [ "Running Tests", "md_docs_test_structure.html#autotoc_md825", [
        [ "Run All Tests", "md_docs_test_structure.html#autotoc_md826", null ],
        [ "Run by Category", "md_docs_test_structure.html#autotoc_md827", null ],
        [ "Run by Component", "md_docs_test_structure.html#autotoc_md828", null ],
        [ "Run Specific Test Executable", "md_docs_test_structure.html#autotoc_md829", null ]
      ] ],
      [ "CI/CD Integration", "md_docs_test_structure.html#autotoc_md831", [
        [ "GitHub Actions Workflows", "md_docs_test_structure.html#autotoc_md832", [
          [ "1. <strong>Pull Request</strong> (Fast feedback)", "md_docs_test_structure.html#autotoc_md833", null ],
          [ "2. <strong>Main/Develop Branch</strong> (Full validation)", "md_docs_test_structure.html#autotoc_md834", null ],
          [ "3. <strong>Nightly/On-Demand</strong> (Performance)", "md_docs_test_structure.html#autotoc_md835", null ]
        ] ],
        [ "Workflow Files", "md_docs_test_structure.html#autotoc_md836", null ]
      ] ],
      [ "Test Results Summary", "md_docs_test_structure.html#autotoc_md838", null ],
      [ "Benefits of Organized Structure", "md_docs_test_structure.html#autotoc_md840", [
        [ "1. <strong>Faster Feedback</strong> ⚡", "md_docs_test_structure.html#autotoc_md841", null ],
        [ "2. **Better Maintainability** 🔧", "md_docs_test_structure.html#autotoc_md842", null ],
        [ "3. **Improved Developer Experience** 👨‍💻", "md_docs_test_structure.html#autotoc_md843", null ],
        [ "4. **CI Optimization** 🚀", "md_docs_test_structure.html#autotoc_md844", null ]
      ] ],
      [ "Adding New Tests", "md_docs_test_structure.html#autotoc_md846", [
        [ "1. Choose the Right Category", "md_docs_test_structure.html#autotoc_md847", null ],
        [ "2. Add Test File", "md_docs_test_structure.html#autotoc_md848", null ],
        [ "3. Update CMakeLists.txt", "md_docs_test_structure.html#autotoc_md849", null ],
        [ "4. Build and Run", "md_docs_test_structure.html#autotoc_md850", null ]
      ] ],
      [ "Test Best Practices", "md_docs_test_structure.html#autotoc_md852", [
        [ "Unit Tests", "md_docs_test_structure.html#autotoc_md853", null ],
        [ "Integration Tests", "md_docs_test_structure.html#autotoc_md854", null ],
        [ "E2E Tests", "md_docs_test_structure.html#autotoc_md855", null ],
        [ "Performance Tests", "md_docs_test_structure.html#autotoc_md856", null ]
      ] ],
      [ "Troubleshooting", "md_docs_test_structure.html#autotoc_md858", [
        [ "Test Failures", "md_docs_test_structure.html#autotoc_md859", null ],
        [ "Build Issues", "md_docs_test_structure.html#autotoc_md860", null ],
        [ "Timeout Issues", "md_docs_test_structure.html#autotoc_md861", null ]
      ] ],
      [ "Future Improvements", "md_docs_test_structure.html#autotoc_md863", null ],
      [ "References", "md_docs_test_structure.html#autotoc_md865", null ]
    ] ],
    [ "Tutorial 1: Getting Started with Unilink", "md_docs_tutorials_01_getting_started.html", [
      [ "What You'll Build", "md_docs_tutorials_01_getting_started.html#autotoc_md868", null ],
      [ "Step 1: Install Dependencies", "md_docs_tutorials_01_getting_started.html#autotoc_md870", [
        [ "Ubuntu/Debian", "md_docs_tutorials_01_getting_started.html#autotoc_md871", null ],
        [ "macOS", "md_docs_tutorials_01_getting_started.html#autotoc_md872", null ],
        [ "Windows (vcpkg)", "md_docs_tutorials_01_getting_started.html#autotoc_md873", null ]
      ] ],
      [ "Step 2: Install Unilink", "md_docs_tutorials_01_getting_started.html#autotoc_md875", [
        [ "Option A: From Source", "md_docs_tutorials_01_getting_started.html#autotoc_md876", null ],
        [ "Option B: Use as Subdirectory", "md_docs_tutorials_01_getting_started.html#autotoc_md877", null ]
      ] ],
      [ "Step 3: Create Your First Client", "md_docs_tutorials_01_getting_started.html#autotoc_md879", null ],
      [ "Step 4: Create CMakeLists.txt", "md_docs_tutorials_01_getting_started.html#autotoc_md881", null ],
      [ "Step 5: Build Your Application", "md_docs_tutorials_01_getting_started.html#autotoc_md883", null ],
      [ "Step 6: Test Your Application", "md_docs_tutorials_01_getting_started.html#autotoc_md885", [
        [ "Start a Test Server", "md_docs_tutorials_01_getting_started.html#autotoc_md886", null ],
        [ "Run Your Client", "md_docs_tutorials_01_getting_started.html#autotoc_md887", null ]
      ] ],
      [ "Step 7: Understanding the Code", "md_docs_tutorials_01_getting_started.html#autotoc_md889", [
        [ "1. Builder Pattern", "md_docs_tutorials_01_getting_started.html#autotoc_md890", null ],
        [ "2. Callbacks", "md_docs_tutorials_01_getting_started.html#autotoc_md891", null ],
        [ "3. Automatic Reconnection", "md_docs_tutorials_01_getting_started.html#autotoc_md892", null ],
        [ "4. Thread Safety", "md_docs_tutorials_01_getting_started.html#autotoc_md893", null ]
      ] ],
      [ "Common Issues", "md_docs_tutorials_01_getting_started.html#autotoc_md895", [
        [ "Issue 1: Connection Refused", "md_docs_tutorials_01_getting_started.html#autotoc_md896", null ],
        [ "Issue 2: Port Already in Use", "md_docs_tutorials_01_getting_started.html#autotoc_md897", null ],
        [ "Issue 3: Compilation Error - unilink not found", "md_docs_tutorials_01_getting_started.html#autotoc_md898", null ]
      ] ],
      [ "Next Steps", "md_docs_tutorials_01_getting_started.html#autotoc_md900", null ],
      [ "Full Example Code", "md_docs_tutorials_01_getting_started.html#autotoc_md902", null ]
    ] ],
    [ "Tutorial 2: Building a TCP Server", "md_docs_tutorials_02_tcp_server.html", [
      [ "What You'll Build", "md_docs_tutorials_02_tcp_server.html#autotoc_md906", null ],
      [ "Step 1: Basic Server Setup", "md_docs_tutorials_02_tcp_server.html#autotoc_md908", null ],
      [ "Step 2: Build and Test", "md_docs_tutorials_02_tcp_server.html#autotoc_md910", [
        [ "Compile", "md_docs_tutorials_02_tcp_server.html#autotoc_md911", null ],
        [ "Run Server", "md_docs_tutorials_02_tcp_server.html#autotoc_md912", null ],
        [ "Test with Multiple Clients", "md_docs_tutorials_02_tcp_server.html#autotoc_md913", null ]
      ] ],
      [ "Step 3: Add Client Management", "md_docs_tutorials_02_tcp_server.html#autotoc_md915", null ],
      [ "Step 4: Single Client Mode", "md_docs_tutorials_02_tcp_server.html#autotoc_md917", null ],
      [ "Step 5: Port Retry Logic", "md_docs_tutorials_02_tcp_server.html#autotoc_md919", null ],
      [ "Step 6: Broadcasting to All Clients", "md_docs_tutorials_02_tcp_server.html#autotoc_md921", null ],
      [ "Complete Example: Chat Server", "md_docs_tutorials_02_tcp_server.html#autotoc_md923", null ],
      [ "Testing Your Chat Server", "md_docs_tutorials_02_tcp_server.html#autotoc_md925", [
        [ "Start Server", "md_docs_tutorials_02_tcp_server.html#autotoc_md926", null ],
        [ "Connect Multiple Clients", "md_docs_tutorials_02_tcp_server.html#autotoc_md927", null ],
        [ "Try Commands", "md_docs_tutorials_02_tcp_server.html#autotoc_md928", null ]
      ] ],
      [ "Best Practices", "md_docs_tutorials_02_tcp_server.html#autotoc_md930", [
        [ "1. Always Check Server Status", "md_docs_tutorials_02_tcp_server.html#autotoc_md931", null ],
        [ "2. Use Thread-Safe Data Structures", "md_docs_tutorials_02_tcp_server.html#autotoc_md932", null ],
        [ "3. Handle Graceful Shutdown", "md_docs_tutorials_02_tcp_server.html#autotoc_md933", null ],
        [ "4. Implement Error Recovery", "md_docs_tutorials_02_tcp_server.html#autotoc_md934", null ]
      ] ],
      [ "Common Patterns", "md_docs_tutorials_02_tcp_server.html#autotoc_md936", [
        [ "Pattern 1: Command Parser", "md_docs_tutorials_02_tcp_server.html#autotoc_md937", null ],
        [ "Pattern 2: Rate Limiting", "md_docs_tutorials_02_tcp_server.html#autotoc_md938", null ]
      ] ],
      [ "Next Steps", "md_docs_tutorials_02_tcp_server.html#autotoc_md940", null ],
      [ "Full Example Code", "md_docs_tutorials_02_tcp_server.html#autotoc_md942", null ]
    ] ],
    [ "Namespaces", "namespaces.html", [
      [ "Namespace List", "namespaces.html", "namespaces_dup" ],
      [ "Namespace Members", "namespacemembers.html", [
        [ "All", "namespacemembers.html", null ],
        [ "Functions", "namespacemembers_func.html", null ],
        [ "Variables", "namespacemembers_vars.html", null ],
        [ "Typedefs", "namespacemembers_type.html", null ],
        [ "Enumerations", "namespacemembers_enum.html", null ]
      ] ]
    ] ],
    [ "Classes", "annotated.html", [
      [ "Class List", "annotated.html", "annotated_dup" ],
      [ "Class Index", "classes.html", null ],
      [ "Class Hierarchy", "hierarchy.html", "hierarchy" ],
      [ "Class Members", "functions.html", [
        [ "All", "functions.html", "functions_dup" ],
        [ "Functions", "functions_func.html", "functions_func" ],
        [ "Variables", "functions_vars.html", null ],
        [ "Typedefs", "functions_type.html", null ],
        [ "Enumerations", "functions_enum.html", null ]
      ] ]
    ] ],
    [ "Files", "files.html", [
      [ "File List", "files.html", "files_dup" ],
      [ "File Members", "globals.html", [
        [ "All", "globals.html", null ],
        [ "Macros", "globals_defs.html", null ]
      ] ]
    ] ]
  ] ]
];

var NAVTREEINDEX =
[
"annotated.html",
"classunilink_1_1config_1_1ConfigManager.html#ae4a737ef35225ae0de347f09d558d8ec",
"classunilink_1_1memory_1_1PooledBuffer.html#a94ad020d70924d2daaa865bda73d23c0",
"classunilink_1_1wrapper_1_1Serial.html#aa0f2fccb8e5d86b5e35b4cc0adebb66d",
"functions_func_v.html",
"md_docs_architecture_runtime_behavior.html#autotoc_md230",
"md_docs_guides_setup_build_guide.html#autotoc_md580",
"md_docs_tutorials_02_tcp_server.html#autotoc_md925",
"structunilink_1_1config_1_1ConfigItem.html#a74a54b9e37822c764497c802fc941cd9",
"unified__builder_8hpp.html"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';