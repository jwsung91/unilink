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
      [ "Table of Contents", "index.html#autotoc_md91", null ],
      [ "Overview", "index.html#autotoc_md93", [
        [ "Design Goals", "index.html#autotoc_md94", null ]
      ] ],
      [ "Layered Architecture", "index.html#autotoc_md96", [
        [ "Layer Responsibilities", "index.html#autotoc_md97", [
          [ "1. Builder API Layer", "index.html#autotoc_md98", null ],
          [ "2. Wrapper API Layer", "index.html#autotoc_md99", null ],
          [ "3. Transport Layer", "index.html#autotoc_md100", null ],
          [ "4. Common Utilities Layer", "index.html#autotoc_md101", null ]
        ] ]
      ] ],
      [ "Core Components", "index.html#autotoc_md103", [
        [ "1. Builder System", "index.html#autotoc_md104", null ],
        [ "2. Wrapper System", "index.html#autotoc_md105", null ],
        [ "3. Transport System", "index.html#autotoc_md106", null ],
        [ "4. Common Utilities", "index.html#autotoc_md107", null ]
      ] ],
      [ "Design Patterns", "index.html#autotoc_md109", [
        [ "1. Builder Pattern", "index.html#autotoc_md110", null ],
        [ "2. Dependency Injection", "index.html#autotoc_md111", null ],
        [ "3. Observer Pattern", "index.html#autotoc_md112", null ],
        [ "4. Singleton Pattern", "index.html#autotoc_md113", null ],
        [ "5. RAII (Resource Acquisition Is Initialization)", "index.html#autotoc_md114", null ],
        [ "6. Template Method Pattern", "index.html#autotoc_md115", null ]
      ] ],
      [ "Threading Model", "index.html#autotoc_md117", [
        [ "Overview", "index.html#autotoc_md118", null ],
        [ "Thread Safety Guarantees", "index.html#autotoc_md119", [
          [ "1. API Methods", "index.html#autotoc_md120", null ],
          [ "2. Callbacks", "index.html#autotoc_md121", null ],
          [ "3. Shared State", "index.html#autotoc_md122", null ]
        ] ],
        [ "IO Context Management", "index.html#autotoc_md123", [
          [ "Shared Context (Default)", "index.html#autotoc_md124", null ],
          [ "Independent Context", "index.html#autotoc_md125", null ]
        ] ]
      ] ],
      [ "Memory Management", "index.html#autotoc_md127", [
        [ "1. Smart Pointers", "index.html#autotoc_md128", null ],
        [ "2. Memory Pool", "index.html#autotoc_md129", null ],
        [ "3. Memory Tracking", "index.html#autotoc_md130", null ],
        [ "4. Safe Data Buffer", "index.html#autotoc_md131", null ]
      ] ],
      [ "Error Handling", "index.html#autotoc_md133", [
        [ "Error Propagation Flow", "index.html#autotoc_md134", null ],
        [ "Error Categories", "index.html#autotoc_md135", null ],
        [ "Error Recovery Strategies", "index.html#autotoc_md136", [
          [ "1. Automatic Retry", "index.html#autotoc_md137", null ],
          [ "2. Circuit Breaker (Planned)", "index.html#autotoc_md138", null ]
        ] ]
      ] ],
      [ "Configuration System", "index.html#autotoc_md140", [
        [ "Compile-Time Configuration", "index.html#autotoc_md141", null ],
        [ "Runtime Configuration", "index.html#autotoc_md142", null ]
      ] ],
      [ "Performance Considerations", "index.html#autotoc_md144", [
        [ "1. Asynchronous I/O", "index.html#autotoc_md145", null ],
        [ "2. Zero-Copy Operations", "index.html#autotoc_md146", null ],
        [ "3. Connection Pooling (Planned)", "index.html#autotoc_md147", null ],
        [ "4. Memory Pooling", "index.html#autotoc_md148", null ]
      ] ],
      [ "Extension Points", "index.html#autotoc_md150", [
        [ "1. Custom Transports", "index.html#autotoc_md151", null ],
        [ "2. Custom Builders", "index.html#autotoc_md152", null ],
        [ "3. Custom Error Handlers", "index.html#autotoc_md153", null ]
      ] ],
      [ "Development & Tooling", "index.html#autotoc_md155", [
        [ "1. Docker-based Environment", "index.html#autotoc_md156", null ],
        [ "2. Documentation Generation", "index.html#autotoc_md157", null ]
      ] ],
      [ "Testing Architecture", "index.html#autotoc_md159", [
        [ "1. Dependency Injection", "index.html#autotoc_md160", null ],
        [ "2. Independent Contexts", "index.html#autotoc_md161", null ],
        [ "3. State Verification", "index.html#autotoc_md162", null ]
      ] ],
      [ "Summary", "index.html#autotoc_md164", null ]
    ] ],
    [ "Memory Safety Architecture", "md_docs_architecture_memory_safety.html", [
      [ "Table of Contents", "md_docs_architecture_memory_safety.html#autotoc_md2", null ],
      [ "Overview", "md_docs_architecture_memory_safety.html#autotoc_md4", [
        [ "Memory Safety Guarantees", "md_docs_architecture_memory_safety.html#autotoc_md5", null ],
        [ "Safety Levels", "md_docs_architecture_memory_safety.html#autotoc_md7", null ]
      ] ],
      [ "Safe Data Handling", "md_docs_architecture_memory_safety.html#autotoc_md9", [
        [ "SafeDataBuffer", "md_docs_architecture_memory_safety.html#autotoc_md10", null ],
        [ "Features", "md_docs_architecture_memory_safety.html#autotoc_md12", [
          [ "1. Construction Validation", "md_docs_architecture_memory_safety.html#autotoc_md13", null ],
          [ "2. Safe Type Conversions", "md_docs_architecture_memory_safety.html#autotoc_md15", null ],
          [ "3. Memory Validation", "md_docs_architecture_memory_safety.html#autotoc_md17", null ]
        ] ],
        [ "Safe Span (C++17 Compatible)", "md_docs_architecture_memory_safety.html#autotoc_md19", null ]
      ] ],
      [ "Thread-Safe State Management", "md_docs_architecture_memory_safety.html#autotoc_md21", [
        [ "ThreadSafeState", "md_docs_architecture_memory_safety.html#autotoc_md22", null ],
        [ "AtomicState", "md_docs_architecture_memory_safety.html#autotoc_md24", null ],
        [ "ThreadSafeCounter", "md_docs_architecture_memory_safety.html#autotoc_md26", null ],
        [ "ThreadSafeFlag", "md_docs_architecture_memory_safety.html#autotoc_md28", null ],
        [ "Thread Safety Summary", "md_docs_architecture_memory_safety.html#autotoc_md30", null ]
      ] ],
      [ "Memory Tracking", "md_docs_architecture_memory_safety.html#autotoc_md32", [
        [ "Built-in Memory Tracking", "md_docs_architecture_memory_safety.html#autotoc_md33", null ],
        [ "Features", "md_docs_architecture_memory_safety.html#autotoc_md35", [
          [ "1. Allocation Tracking", "md_docs_architecture_memory_safety.html#autotoc_md36", null ],
          [ "2. Leak Detection", "md_docs_architecture_memory_safety.html#autotoc_md38", null ],
          [ "3. Performance Monitoring", "md_docs_architecture_memory_safety.html#autotoc_md40", null ],
          [ "4. Debug Reports", "md_docs_architecture_memory_safety.html#autotoc_md42", null ]
        ] ],
        [ "Zero Overhead in Release", "md_docs_architecture_memory_safety.html#autotoc_md44", null ]
      ] ],
      [ "AddressSanitizer Support", "md_docs_architecture_memory_safety.html#autotoc_md46", [
        [ "Enable AddressSanitizer", "md_docs_architecture_memory_safety.html#autotoc_md47", null ],
        [ "What ASan Detects", "md_docs_architecture_memory_safety.html#autotoc_md49", null ],
        [ "Running with ASan", "md_docs_architecture_memory_safety.html#autotoc_md51", null ],
        [ "Performance Impact", "md_docs_architecture_memory_safety.html#autotoc_md53", null ]
      ] ],
      [ "Best Practices", "md_docs_architecture_memory_safety.html#autotoc_md55", [
        [ "1. Buffer Management", "md_docs_architecture_memory_safety.html#autotoc_md56", [
          [ "✅ DO", "md_docs_architecture_memory_safety.html#autotoc_md57", null ],
          [ "❌ DON'T", "md_docs_architecture_memory_safety.html#autotoc_md58", null ]
        ] ],
        [ "2. Type Conversions", "md_docs_architecture_memory_safety.html#autotoc_md60", [
          [ "✅ DO", "md_docs_architecture_memory_safety.html#autotoc_md61", null ],
          [ "❌ DON'T", "md_docs_architecture_memory_safety.html#autotoc_md62", null ]
        ] ],
        [ "3. Thread Safety", "md_docs_architecture_memory_safety.html#autotoc_md64", [
          [ "✅ DO", "md_docs_architecture_memory_safety.html#autotoc_md65", null ],
          [ "❌ DON'T", "md_docs_architecture_memory_safety.html#autotoc_md66", null ]
        ] ],
        [ "4. Memory Tracking", "md_docs_architecture_memory_safety.html#autotoc_md68", [
          [ "✅ DO", "md_docs_architecture_memory_safety.html#autotoc_md69", null ],
          [ "❌ DON'T", "md_docs_architecture_memory_safety.html#autotoc_md70", null ]
        ] ],
        [ "5. Sanitizers", "md_docs_architecture_memory_safety.html#autotoc_md72", [
          [ "✅ DO", "md_docs_architecture_memory_safety.html#autotoc_md73", null ],
          [ "❌ DON'T", "md_docs_architecture_memory_safety.html#autotoc_md74", null ]
        ] ]
      ] ],
      [ "Memory Safety Benefits", "md_docs_architecture_memory_safety.html#autotoc_md76", [
        [ "Prevents Common Vulnerabilities", "md_docs_architecture_memory_safety.html#autotoc_md77", null ],
        [ "Performance", "md_docs_architecture_memory_safety.html#autotoc_md79", null ]
      ] ],
      [ "Testing Memory Safety", "md_docs_architecture_memory_safety.html#autotoc_md81", [
        [ "Unit Tests", "md_docs_architecture_memory_safety.html#autotoc_md82", null ],
        [ "Integration Tests", "md_docs_architecture_memory_safety.html#autotoc_md84", null ],
        [ "Continuous Integration", "md_docs_architecture_memory_safety.html#autotoc_md86", null ]
      ] ],
      [ "Next Steps", "md_docs_architecture_memory_safety.html#autotoc_md88", null ]
    ] ],
    [ "Runtime Behavior Model", "md_docs_architecture_runtime_behavior.html", [
      [ "Table of Contents", "md_docs_architecture_runtime_behavior.html#autotoc_md168", null ],
      [ "Threading Model & Callback Execution", "md_docs_architecture_runtime_behavior.html#autotoc_md170", [
        [ "Architecture Diagram", "md_docs_architecture_runtime_behavior.html#autotoc_md171", null ],
        [ "Key Points", "md_docs_architecture_runtime_behavior.html#autotoc_md173", [
          [ "✅ Thread-Safe API Methods", "md_docs_architecture_runtime_behavior.html#autotoc_md174", null ],
          [ "✅ Callback Execution Context", "md_docs_architecture_runtime_behavior.html#autotoc_md176", null ],
          [ "⚠️ Never Block in Callbacks", "md_docs_architecture_runtime_behavior.html#autotoc_md178", null ],
          [ "✅ Thread-Safe State Access", "md_docs_architecture_runtime_behavior.html#autotoc_md180", null ]
        ] ],
        [ "Threading Model Summary", "md_docs_architecture_runtime_behavior.html#autotoc_md182", null ]
      ] ],
      [ "Reconnection Policy & State Machine", "md_docs_architecture_runtime_behavior.html#autotoc_md184", [
        [ "State Machine Diagram", "md_docs_architecture_runtime_behavior.html#autotoc_md185", null ],
        [ "Connection States", "md_docs_architecture_runtime_behavior.html#autotoc_md187", null ],
        [ "Configuration Example", "md_docs_architecture_runtime_behavior.html#autotoc_md189", null ],
        [ "Retry Behavior", "md_docs_architecture_runtime_behavior.html#autotoc_md191", [
          [ "Default Behavior", "md_docs_architecture_runtime_behavior.html#autotoc_md192", null ],
          [ "Retry Interval Configuration", "md_docs_architecture_runtime_behavior.html#autotoc_md193", null ],
          [ "State Callbacks", "md_docs_architecture_runtime_behavior.html#autotoc_md195", null ],
          [ "Manual Control", "md_docs_architecture_runtime_behavior.html#autotoc_md197", null ]
        ] ],
        [ "Reconnection Best Practices", "md_docs_architecture_runtime_behavior.html#autotoc_md199", [
          [ "1. Choose Appropriate Retry Interval", "md_docs_architecture_runtime_behavior.html#autotoc_md200", null ],
          [ "2. Handle State Transitions", "md_docs_architecture_runtime_behavior.html#autotoc_md202", null ],
          [ "3. Graceful Shutdown", "md_docs_architecture_runtime_behavior.html#autotoc_md204", null ]
        ] ]
      ] ],
      [ "Backpressure Handling", "md_docs_architecture_runtime_behavior.html#autotoc_md206", [
        [ "Backpressure Flow", "md_docs_architecture_runtime_behavior.html#autotoc_md207", null ],
        [ "Backpressure Configuration", "md_docs_architecture_runtime_behavior.html#autotoc_md209", null ],
        [ "Backpressure Strategies", "md_docs_architecture_runtime_behavior.html#autotoc_md211", [
          [ "Strategy 1: Pause Sending", "md_docs_architecture_runtime_behavior.html#autotoc_md212", null ],
          [ "Strategy 2: Rate Limiting", "md_docs_architecture_runtime_behavior.html#autotoc_md214", null ],
          [ "Strategy 3: Drop Data", "md_docs_architecture_runtime_behavior.html#autotoc_md216", null ]
        ] ],
        [ "Backpressure Monitoring", "md_docs_architecture_runtime_behavior.html#autotoc_md218", null ],
        [ "Memory Safety", "md_docs_architecture_runtime_behavior.html#autotoc_md220", null ]
      ] ],
      [ "Best Practices", "md_docs_architecture_runtime_behavior.html#autotoc_md222", [
        [ "1. Threading Best Practices", "md_docs_architecture_runtime_behavior.html#autotoc_md223", [
          [ "✅ DO", "md_docs_architecture_runtime_behavior.html#autotoc_md224", null ],
          [ "❌ DON'T", "md_docs_architecture_runtime_behavior.html#autotoc_md225", null ]
        ] ],
        [ "2. Reconnection Best Practices", "md_docs_architecture_runtime_behavior.html#autotoc_md227", [
          [ "✅ DO", "md_docs_architecture_runtime_behavior.html#autotoc_md228", null ],
          [ "❌ DON'T", "md_docs_architecture_runtime_behavior.html#autotoc_md229", null ]
        ] ],
        [ "3. Backpressure Best Practices", "md_docs_architecture_runtime_behavior.html#autotoc_md231", [
          [ "✅ DO", "md_docs_architecture_runtime_behavior.html#autotoc_md232", null ],
          [ "❌ DON'T", "md_docs_architecture_runtime_behavior.html#autotoc_md233", null ]
        ] ]
      ] ],
      [ "Performance Considerations", "md_docs_architecture_runtime_behavior.html#autotoc_md235", [
        [ "Threading Overhead", "md_docs_architecture_runtime_behavior.html#autotoc_md236", null ],
        [ "Reconnection Overhead", "md_docs_architecture_runtime_behavior.html#autotoc_md237", null ],
        [ "Backpressure Overhead", "md_docs_architecture_runtime_behavior.html#autotoc_md238", null ]
      ] ],
      [ "Next Steps", "md_docs_architecture_runtime_behavior.html#autotoc_md240", null ]
    ] ],
    [ "Performance Guide", "md_docs_guides_advanced_performance.html", [
      [ "Table of Contents", "md_docs_guides_advanced_performance.html#autotoc_md243", null ],
      [ "Performance Overview", "md_docs_guides_advanced_performance.html#autotoc_md245", [
        [ "Characteristics & Goals", "md_docs_guides_advanced_performance.html#autotoc_md246", null ]
      ] ],
      [ "Build Configuration Optimization", "md_docs_guides_advanced_performance.html#autotoc_md248", [
        [ "Minimal Build vs Full Build", "md_docs_guides_advanced_performance.html#autotoc_md249", null ],
        [ "Compiler Optimization Levels", "md_docs_guides_advanced_performance.html#autotoc_md250", null ]
      ] ],
      [ "Runtime Optimization", "md_docs_guides_advanced_performance.html#autotoc_md252", [
        [ "1. Threading Model & IO Context", "md_docs_guides_advanced_performance.html#autotoc_md253", null ],
        [ "2. Async Logging", "md_docs_guides_advanced_performance.html#autotoc_md254", null ],
        [ "3. Non-Blocking Callbacks", "md_docs_guides_advanced_performance.html#autotoc_md255", null ]
      ] ],
      [ "Memory Optimization", "md_docs_guides_advanced_performance.html#autotoc_md257", [
        [ "1. Memory Pool Usage", "md_docs_guides_advanced_performance.html#autotoc_md258", null ],
        [ "2. Avoid Data Copies", "md_docs_guides_advanced_performance.html#autotoc_md259", null ],
        [ "3. Reserve Vector Capacity", "md_docs_guides_advanced_performance.html#autotoc_md260", null ]
      ] ],
      [ "Network Optimization", "md_docs_guides_advanced_performance.html#autotoc_md262", [
        [ "1. Batch Small Messages", "md_docs_guides_advanced_performance.html#autotoc_md263", null ],
        [ "2. Connection Reuse", "md_docs_guides_advanced_performance.html#autotoc_md264", null ],
        [ "3. Socket Tuning", "md_docs_guides_advanced_performance.html#autotoc_md265", null ]
      ] ],
      [ "Benchmarking & Profiling", "md_docs_guides_advanced_performance.html#autotoc_md267", [
        [ "Built-in Performance Tests", "md_docs_guides_advanced_performance.html#autotoc_md268", null ],
        [ "Profiling with Tools", "md_docs_guides_advanced_performance.html#autotoc_md269", null ],
        [ "Simple Throughput Benchmark Code", "md_docs_guides_advanced_performance.html#autotoc_md270", null ]
      ] ],
      [ "Real-World Case Studies", "md_docs_guides_advanced_performance.html#autotoc_md272", [
        [ "Case Study 1: High-Throughput Data Streaming", "md_docs_guides_advanced_performance.html#autotoc_md273", null ],
        [ "Case Study 2: Low-Latency Trading System", "md_docs_guides_advanced_performance.html#autotoc_md274", null ],
        [ "Case Study 3: IoT Gateway (1000+ Connections)", "md_docs_guides_advanced_performance.html#autotoc_md275", null ]
      ] ]
    ] ],
    [ "Unilink Best Practices Guide", "md_docs_guides_core_best_practices.html", [
      [ "Table of Contents", "md_docs_guides_core_best_practices.html#autotoc_md278", null ],
      [ "Error Handling", "md_docs_guides_core_best_practices.html#autotoc_md280", [
        [ "✅ DO: Always Register Error Callbacks", "md_docs_guides_core_best_practices.html#autotoc_md281", null ],
        [ "✅ DO: Check Connection Status Before Sending", "md_docs_guides_core_best_practices.html#autotoc_md282", null ],
        [ "✅ DO: Implement Graceful Error Recovery", "md_docs_guides_core_best_practices.html#autotoc_md283", null ],
        [ "✅ DO: Use Centralized Error Handler", "md_docs_guides_core_best_practices.html#autotoc_md284", null ]
      ] ],
      [ "Resource Management", "md_docs_guides_core_best_practices.html#autotoc_md286", [
        [ "✅ DO: Use RAII and Smart Pointers", "md_docs_guides_core_best_practices.html#autotoc_md287", null ],
        [ "✅ DO: Stop Connections Before Shutdown", "md_docs_guides_core_best_practices.html#autotoc_md288", null ],
        [ "✅ DO: Reuse Connections When Possible", "md_docs_guides_core_best_practices.html#autotoc_md289", null ]
      ] ],
      [ "Thread Safety", "md_docs_guides_core_best_practices.html#autotoc_md291", [
        [ "✅ DO: Protect Shared State", "md_docs_guides_core_best_practices.html#autotoc_md292", null ],
        [ "✅ DO: Use ThreadSafeState for Complex States", "md_docs_guides_core_best_practices.html#autotoc_md293", null ],
        [ "❌ DON'T: Block in Callbacks", "md_docs_guides_core_best_practices.html#autotoc_md294", null ]
      ] ],
      [ "Performance Optimization", "md_docs_guides_core_best_practices.html#autotoc_md296", [
        [ "✅ DO: Use Move Semantics", "md_docs_guides_core_best_practices.html#autotoc_md297", null ],
        [ "✅ DO: Enable Async Logging", "md_docs_guides_core_best_practices.html#autotoc_md298", null ],
        [ "✅ DO: Use Shared IO Context (Default)", "md_docs_guides_core_best_practices.html#autotoc_md299", null ],
        [ "✅ DO: Batch Small Messages", "md_docs_guides_core_best_practices.html#autotoc_md300", null ]
      ] ],
      [ "Code Organization", "md_docs_guides_core_best_practices.html#autotoc_md302", [
        [ "✅ DO: Use Classes for Complex Logic", "md_docs_guides_core_best_practices.html#autotoc_md303", null ],
        [ "✅ DO: Separate Concerns", "md_docs_guides_core_best_practices.html#autotoc_md304", null ]
      ] ],
      [ "Testing", "md_docs_guides_core_best_practices.html#autotoc_md306", [
        [ "✅ DO: Use Dependency Injection", "md_docs_guides_core_best_practices.html#autotoc_md307", null ],
        [ "✅ DO: Test Error Scenarios", "md_docs_guides_core_best_practices.html#autotoc_md308", null ],
        [ "✅ DO: Use Independent Context for Tests", "md_docs_guides_core_best_practices.html#autotoc_md309", null ]
      ] ],
      [ "Security", "md_docs_guides_core_best_practices.html#autotoc_md311", [
        [ "✅ DO: Validate All Input", "md_docs_guides_core_best_practices.html#autotoc_md312", null ],
        [ "✅ DO: Implement Rate Limiting", "md_docs_guides_core_best_practices.html#autotoc_md313", null ],
        [ "✅ DO: Set Connection Limits", "md_docs_guides_core_best_practices.html#autotoc_md314", null ]
      ] ],
      [ "Logging and Debugging", "md_docs_guides_core_best_practices.html#autotoc_md316", [
        [ "✅ DO: Use Appropriate Log Levels", "md_docs_guides_core_best_practices.html#autotoc_md317", null ],
        [ "✅ DO: Enable Debug Logging During Development", "md_docs_guides_core_best_practices.html#autotoc_md318", null ],
        [ "✅ DO: Log Context Information", "md_docs_guides_core_best_practices.html#autotoc_md319", null ]
      ] ]
    ] ],
    [ "Unilink Quick Start Guide", "md_docs_guides_core_quickstart.html", [
      [ "Installation", "md_docs_guides_core_quickstart.html#autotoc_md321", [
        [ "Prerequisites", "md_docs_guides_core_quickstart.html#autotoc_md322", null ],
        [ "Build & Install", "md_docs_guides_core_quickstart.html#autotoc_md323", null ]
      ] ],
      [ "Your First TCP Client (30 seconds)", "md_docs_guides_core_quickstart.html#autotoc_md325", null ],
      [ "Your First TCP Server (30 seconds)", "md_docs_guides_core_quickstart.html#autotoc_md327", null ],
      [ "Your First Serial Device (30 seconds)", "md_docs_guides_core_quickstart.html#autotoc_md329", null ],
      [ "Common Patterns", "md_docs_guides_core_quickstart.html#autotoc_md331", [
        [ "Pattern 1: Auto-Reconnection", "md_docs_guides_core_quickstart.html#autotoc_md332", null ],
        [ "Pattern 2: Error Handling", "md_docs_guides_core_quickstart.html#autotoc_md333", null ],
        [ "Pattern 3: Member Function Callbacks", "md_docs_guides_core_quickstart.html#autotoc_md334", null ],
        [ "Pattern 4: Single vs Multi-Client Server (choose one)", "md_docs_guides_core_quickstart.html#autotoc_md335", null ]
      ] ],
      [ "Next Steps", "md_docs_guides_core_quickstart.html#autotoc_md337", null ],
      [ "Troubleshooting", "md_docs_guides_core_quickstart.html#autotoc_md339", [
        [ "Can't connect to server?", "md_docs_guides_core_quickstart.html#autotoc_md340", null ],
        [ "Port already in use?", "md_docs_guides_core_quickstart.html#autotoc_md341", null ],
        [ "Need independent IO thread?", "md_docs_guides_core_quickstart.html#autotoc_md342", null ]
      ] ],
      [ "Support", "md_docs_guides_core_quickstart.html#autotoc_md344", null ]
    ] ],
    [ "Testing Guide", "md_docs_guides_core_testing.html", [
      [ "Table of Contents", "md_docs_guides_core_testing.html#autotoc_md347", null ],
      [ "Quick Start", "md_docs_guides_core_testing.html#autotoc_md349", [
        [ "Build and Run All Tests", "md_docs_guides_core_testing.html#autotoc_md350", null ],
        [ "Windows Build & Test Workflow", "md_docs_guides_core_testing.html#autotoc_md352", null ]
      ] ],
      [ "Running Tests", "md_docs_guides_core_testing.html#autotoc_md354", [
        [ "Run All Tests", "md_docs_guides_core_testing.html#autotoc_md355", null ],
        [ "Run Specific Test Categories", "md_docs_guides_core_testing.html#autotoc_md357", null ],
        [ "Run Tests with Verbose Output", "md_docs_guides_core_testing.html#autotoc_md359", null ],
        [ "Run Tests in Parallel", "md_docs_guides_core_testing.html#autotoc_md361", null ]
      ] ],
      [ "UDP-specific test policies", "md_docs_guides_core_testing.html#autotoc_md363", null ],
      [ "Test Categories", "md_docs_guides_core_testing.html#autotoc_md364", [
        [ "Core Tests", "md_docs_guides_core_testing.html#autotoc_md365", null ],
        [ "Integration Tests", "md_docs_guides_core_testing.html#autotoc_md367", null ],
        [ "Memory Safety Tests", "md_docs_guides_core_testing.html#autotoc_md369", null ],
        [ "Concurrency Safety Tests", "md_docs_guides_core_testing.html#autotoc_md371", null ],
        [ "Performance Tests", "md_docs_guides_core_testing.html#autotoc_md374", null ],
        [ "Stress Tests", "md_docs_guides_core_testing.html#autotoc_md376", null ]
      ] ],
      [ "Memory Safety Validation", "md_docs_guides_core_testing.html#autotoc_md378", [
        [ "Built-in Memory Tracking", "md_docs_guides_core_testing.html#autotoc_md379", null ],
        [ "AddressSanitizer (ASan)", "md_docs_guides_core_testing.html#autotoc_md381", null ],
        [ "ThreadSanitizer (TSan)", "md_docs_guides_core_testing.html#autotoc_md383", null ],
        [ "Valgrind", "md_docs_guides_core_testing.html#autotoc_md385", null ]
      ] ],
      [ "Continuous Integration", "md_docs_guides_core_testing.html#autotoc_md387", [
        [ "GitHub Actions Integration", "md_docs_guides_core_testing.html#autotoc_md388", null ],
        [ "CI/CD Build Matrix", "md_docs_guides_core_testing.html#autotoc_md390", null ],
        [ "Ubuntu 20.04 Support", "md_docs_guides_core_testing.html#autotoc_md392", null ],
        [ "View CI/CD Results", "md_docs_guides_core_testing.html#autotoc_md394", null ]
      ] ],
      [ "Writing Custom Tests", "md_docs_guides_core_testing.html#autotoc_md396", [
        [ "Test Structure", "md_docs_guides_core_testing.html#autotoc_md397", null ],
        [ "Example: Custom Integration Test", "md_docs_guides_core_testing.html#autotoc_md399", null ],
        [ "Running Custom Tests", "md_docs_guides_core_testing.html#autotoc_md401", null ]
      ] ],
      [ "Test Configuration", "md_docs_guides_core_testing.html#autotoc_md403", [
        [ "CTest Configuration", "md_docs_guides_core_testing.html#autotoc_md404", null ],
        [ "Environment Variables", "md_docs_guides_core_testing.html#autotoc_md406", null ]
      ] ],
      [ "Troubleshooting Tests", "md_docs_guides_core_testing.html#autotoc_md408", [
        [ "Test Failures", "md_docs_guides_core_testing.html#autotoc_md409", null ],
        [ "Port Conflicts", "md_docs_guides_core_testing.html#autotoc_md411", null ],
        [ "Memory Issues", "md_docs_guides_core_testing.html#autotoc_md413", null ]
      ] ],
      [ "Performance Regression Testing", "md_docs_guides_core_testing.html#autotoc_md415", [
        [ "Benchmark Baseline", "md_docs_guides_core_testing.html#autotoc_md416", null ],
        [ "Compare Against Baseline", "md_docs_guides_core_testing.html#autotoc_md417", null ],
        [ "Automated Regression Detection", "md_docs_guides_core_testing.html#autotoc_md418", null ]
      ] ],
      [ "Code Coverage", "md_docs_guides_core_testing.html#autotoc_md420", [
        [ "Generate Coverage Report", "md_docs_guides_core_testing.html#autotoc_md421", null ],
        [ "View HTML Coverage Report", "md_docs_guides_core_testing.html#autotoc_md422", null ]
      ] ],
      [ "Next Steps", "md_docs_guides_core_testing.html#autotoc_md424", null ]
    ] ],
    [ "Troubleshooting Guide", "md_docs_guides_core_troubleshooting.html", [
      [ "Table of Contents", "md_docs_guides_core_troubleshooting.html#autotoc_md427", null ],
      [ "Connection Issues", "md_docs_guides_core_troubleshooting.html#autotoc_md429", [
        [ "Problem: Connection Refused", "md_docs_guides_core_troubleshooting.html#autotoc_md430", [
          [ "1. Server Not Running", "md_docs_guides_core_troubleshooting.html#autotoc_md431", null ],
          [ "2. Wrong Host/Port", "md_docs_guides_core_troubleshooting.html#autotoc_md432", null ],
          [ "3. Firewall Blocking", "md_docs_guides_core_troubleshooting.html#autotoc_md433", null ]
        ] ],
        [ "Problem: Connection Timeout", "md_docs_guides_core_troubleshooting.html#autotoc_md435", [
          [ "1. Network Unreachable", "md_docs_guides_core_troubleshooting.html#autotoc_md436", null ],
          [ "2. Server Overloaded", "md_docs_guides_core_troubleshooting.html#autotoc_md437", null ],
          [ "3. Slow Network", "md_docs_guides_core_troubleshooting.html#autotoc_md438", null ]
        ] ],
        [ "Problem: Connection Drops Randomly", "md_docs_guides_core_troubleshooting.html#autotoc_md440", [
          [ "1. Network Instability", "md_docs_guides_core_troubleshooting.html#autotoc_md441", null ],
          [ "2. Server Closing Connection", "md_docs_guides_core_troubleshooting.html#autotoc_md442", null ],
          [ "3. Keep-Alive Not Set", "md_docs_guides_core_troubleshooting.html#autotoc_md443", null ]
        ] ],
        [ "Problem: Port Already in Use", "md_docs_guides_core_troubleshooting.html#autotoc_md445", [
          [ "1. Kill Existing Process", "md_docs_guides_core_troubleshooting.html#autotoc_md446", null ],
          [ "2. Use Different Port", "md_docs_guides_core_troubleshooting.html#autotoc_md447", null ],
          [ "3. Enable Port Retry", "md_docs_guides_core_troubleshooting.html#autotoc_md448", null ],
          [ "4. Set SO_REUSEADDR (Advanced)", "md_docs_guides_core_troubleshooting.html#autotoc_md449", null ]
        ] ]
      ] ],
      [ "Compilation Errors", "md_docs_guides_core_troubleshooting.html#autotoc_md451", [
        [ "Problem: unilink/unilink.hpp Not Found", "md_docs_guides_core_troubleshooting.html#autotoc_md452", [
          [ "1. Install unilink", "md_docs_guides_core_troubleshooting.html#autotoc_md453", null ],
          [ "2. Add Include Path", "md_docs_guides_core_troubleshooting.html#autotoc_md454", null ],
          [ "3. Use as Subdirectory", "md_docs_guides_core_troubleshooting.html#autotoc_md455", null ]
        ] ],
        [ "Problem: Undefined Reference to unilink Symbols", "md_docs_guides_core_troubleshooting.html#autotoc_md457", [
          [ "1. Link unilink Library", "md_docs_guides_core_troubleshooting.html#autotoc_md458", null ],
          [ "2. Check Library Path", "md_docs_guides_core_troubleshooting.html#autotoc_md459", null ]
        ] ],
        [ "Problem: Boost Not Found", "md_docs_guides_core_troubleshooting.html#autotoc_md461", [
          [ "Ubuntu/Debian", "md_docs_guides_core_troubleshooting.html#autotoc_md462", null ],
          [ "macOS", "md_docs_guides_core_troubleshooting.html#autotoc_md463", null ],
          [ "Windows (vcpkg)", "md_docs_guides_core_troubleshooting.html#autotoc_md464", null ],
          [ "Manual Boost Path", "md_docs_guides_core_troubleshooting.html#autotoc_md465", null ]
        ] ]
      ] ],
      [ "Runtime Errors", "md_docs_guides_core_troubleshooting.html#autotoc_md467", [
        [ "Problem: Segmentation Fault", "md_docs_guides_core_troubleshooting.html#autotoc_md468", [
          [ "1. Enable Core Dumps", "md_docs_guides_core_troubleshooting.html#autotoc_md469", null ],
          [ "2. Use AddressSanitizer", "md_docs_guides_core_troubleshooting.html#autotoc_md470", null ],
          [ "3. Common Causes", "md_docs_guides_core_troubleshooting.html#autotoc_md471", null ]
        ] ],
        [ "Problem: Callbacks Not Being Called", "md_docs_guides_core_troubleshooting.html#autotoc_md473", [
          [ "1. Callback Not Registered", "md_docs_guides_core_troubleshooting.html#autotoc_md474", null ],
          [ "2. Client Not Started", "md_docs_guides_core_troubleshooting.html#autotoc_md475", null ],
          [ "3. Application Exits Too Quickly", "md_docs_guides_core_troubleshooting.html#autotoc_md476", null ]
        ] ]
      ] ],
      [ "Performance Issues", "md_docs_guides_core_troubleshooting.html#autotoc_md478", [
        [ "Problem: High CPU Usage", "md_docs_guides_core_troubleshooting.html#autotoc_md479", [
          [ "1. Busy Loop in Callback", "md_docs_guides_core_troubleshooting.html#autotoc_md480", null ],
          [ "2. Too Many Retries", "md_docs_guides_core_troubleshooting.html#autotoc_md481", null ],
          [ "3. Excessive Logging", "md_docs_guides_core_troubleshooting.html#autotoc_md482", null ]
        ] ],
        [ "Problem: High Memory Usage", "md_docs_guides_core_troubleshooting.html#autotoc_md484", [
          [ "1. Enable Memory Tracking (Debug)", "md_docs_guides_core_troubleshooting.html#autotoc_md485", null ],
          [ "2. Fix Memory Leaks", "md_docs_guides_core_troubleshooting.html#autotoc_md486", null ],
          [ "3. Limit Buffer Sizes", "md_docs_guides_core_troubleshooting.html#autotoc_md487", null ]
        ] ],
        [ "Problem: Slow Data Transfer", "md_docs_guides_core_troubleshooting.html#autotoc_md489", [
          [ "1. Batch Small Messages", "md_docs_guides_core_troubleshooting.html#autotoc_md490", null ],
          [ "2. Use Binary Protocol", "md_docs_guides_core_troubleshooting.html#autotoc_md491", null ],
          [ "3. Enable Async Logging", "md_docs_guides_core_troubleshooting.html#autotoc_md492", null ]
        ] ]
      ] ],
      [ "Memory Issues", "md_docs_guides_core_troubleshooting.html#autotoc_md494", [
        [ "Problem: Memory Leak Detected", "md_docs_guides_core_troubleshooting.html#autotoc_md495", null ]
      ] ],
      [ "Thread Safety Issues", "md_docs_guides_core_troubleshooting.html#autotoc_md497", [
        [ "Problem: Race Condition / Data Corruption", "md_docs_guides_core_troubleshooting.html#autotoc_md498", [
          [ "1. Protect Shared State", "md_docs_guides_core_troubleshooting.html#autotoc_md499", null ],
          [ "2. Use Thread-Safe Containers", "md_docs_guides_core_troubleshooting.html#autotoc_md500", null ]
        ] ]
      ] ],
      [ "Debugging Tips", "md_docs_guides_core_troubleshooting.html#autotoc_md502", [
        [ "Enable Debug Logging", "md_docs_guides_core_troubleshooting.html#autotoc_md503", null ],
        [ "Use GDB for Debugging", "md_docs_guides_core_troubleshooting.html#autotoc_md504", null ],
        [ "Network Debugging with tcpdump", "md_docs_guides_core_troubleshooting.html#autotoc_md505", null ],
        [ "Test with netcat", "md_docs_guides_core_troubleshooting.html#autotoc_md506", null ]
      ] ],
      [ "Getting Help", "md_docs_guides_core_troubleshooting.html#autotoc_md508", null ]
    ] ],
    [ "Build Guide", "md_docs_guides_setup_build_guide.html", [
      [ "</blockquote>", "md_docs_guides_setup_build_guide.html#autotoc_md511", null ],
      [ "Table of Contents", "md_docs_guides_setup_build_guide.html#autotoc_md512", null ],
      [ "Quick Build", "md_docs_guides_setup_build_guide.html#autotoc_md514", [
        [ "Basic Build (Recommended)", "md_docs_guides_setup_build_guide.html#autotoc_md515", null ]
      ] ],
      [ "Build Configurations", "md_docs_guides_setup_build_guide.html#autotoc_md517", [
        [ "Minimal Build (Builder API only)", "md_docs_guides_setup_build_guide.html#autotoc_md518", null ],
        [ "Full Build (includes Configuration Management API)", "md_docs_guides_setup_build_guide.html#autotoc_md520", null ]
      ] ],
      [ "Build Options Reference", "md_docs_guides_setup_build_guide.html#autotoc_md522", [
        [ "Core Options", "md_docs_guides_setup_build_guide.html#autotoc_md523", null ],
        [ "Development Options", "md_docs_guides_setup_build_guide.html#autotoc_md524", null ],
        [ "Installation Options", "md_docs_guides_setup_build_guide.html#autotoc_md525", null ]
      ] ],
      [ "Build Types Comparison", "md_docs_guides_setup_build_guide.html#autotoc_md527", [
        [ "Release Build (Default)", "md_docs_guides_setup_build_guide.html#autotoc_md528", null ],
        [ "Debug Build", "md_docs_guides_setup_build_guide.html#autotoc_md530", null ],
        [ "RelWithDebInfo Build", "md_docs_guides_setup_build_guide.html#autotoc_md532", null ]
      ] ],
      [ "Advanced Build Examples", "md_docs_guides_setup_build_guide.html#autotoc_md534", [
        [ "Example 1: Minimal Production Build", "md_docs_guides_setup_build_guide.html#autotoc_md535", null ],
        [ "Example 2: Development Build with Examples", "md_docs_guides_setup_build_guide.html#autotoc_md537", null ],
        [ "Example 3: Testing with Sanitizers", "md_docs_guides_setup_build_guide.html#autotoc_md539", null ],
        [ "Example 4: Build with Custom Boost Location", "md_docs_guides_setup_build_guide.html#autotoc_md541", null ],
        [ "Example 5: Build with Specific Compiler", "md_docs_guides_setup_build_guide.html#autotoc_md543", null ]
      ] ],
      [ "Platform-Specific Builds", "md_docs_guides_setup_build_guide.html#autotoc_md545", [
        [ "Ubuntu 22.04 (Recommended)", "md_docs_guides_setup_build_guide.html#autotoc_md546", null ],
        [ "Ubuntu 20.04 Build", "md_docs_guides_setup_build_guide.html#autotoc_md548", [
          [ "Prerequisites", "md_docs_guides_setup_build_guide.html#autotoc_md549", null ],
          [ "Build Steps", "md_docs_guides_setup_build_guide.html#autotoc_md550", null ],
          [ "Notes", "md_docs_guides_setup_build_guide.html#autotoc_md551", null ]
        ] ],
        [ "Debian 11+", "md_docs_guides_setup_build_guide.html#autotoc_md553", null ],
        [ "Fedora 35+", "md_docs_guides_setup_build_guide.html#autotoc_md555", null ],
        [ "Arch Linux", "md_docs_guides_setup_build_guide.html#autotoc_md557", null ]
      ] ],
      [ "Build Performance Tips", "md_docs_guides_setup_build_guide.html#autotoc_md559", [
        [ "Parallel Builds", "md_docs_guides_setup_build_guide.html#autotoc_md560", null ],
        [ "Ccache for Faster Rebuilds", "md_docs_guides_setup_build_guide.html#autotoc_md561", null ],
        [ "Ninja Build System (Faster than Make)", "md_docs_guides_setup_build_guide.html#autotoc_md562", null ]
      ] ],
      [ "Installation", "md_docs_guides_setup_build_guide.html#autotoc_md564", [
        [ "System-Wide Installation", "md_docs_guides_setup_build_guide.html#autotoc_md565", null ],
        [ "Custom Installation Directory", "md_docs_guides_setup_build_guide.html#autotoc_md566", null ],
        [ "Uninstall", "md_docs_guides_setup_build_guide.html#autotoc_md567", null ]
      ] ],
      [ "Verifying the Build", "md_docs_guides_setup_build_guide.html#autotoc_md569", [
        [ "Run Unit Tests", "md_docs_guides_setup_build_guide.html#autotoc_md570", null ],
        [ "Run Examples", "md_docs_guides_setup_build_guide.html#autotoc_md571", null ],
        [ "Check Library Symbols", "md_docs_guides_setup_build_guide.html#autotoc_md572", null ]
      ] ],
      [ "Troubleshooting", "md_docs_guides_setup_build_guide.html#autotoc_md574", [
        [ "Problem: CMake Can't Find Boost", "md_docs_guides_setup_build_guide.html#autotoc_md575", null ],
        [ "Problem: Compiler Not Found", "md_docs_guides_setup_build_guide.html#autotoc_md576", null ],
        [ "Problem: Out of Memory During Build", "md_docs_guides_setup_build_guide.html#autotoc_md577", null ],
        [ "Problem: Permission Denied During Install", "md_docs_guides_setup_build_guide.html#autotoc_md578", null ]
      ] ],
      [ "CMake Package Integration", "md_docs_guides_setup_build_guide.html#autotoc_md580", [
        [ "Using the Installed Package", "md_docs_guides_setup_build_guide.html#autotoc_md581", null ],
        [ "Custom Installation Prefix", "md_docs_guides_setup_build_guide.html#autotoc_md582", null ],
        [ "Package Components", "md_docs_guides_setup_build_guide.html#autotoc_md583", null ],
        [ "Verification", "md_docs_guides_setup_build_guide.html#autotoc_md584", null ]
      ] ],
      [ "Next Steps", "md_docs_guides_setup_build_guide.html#autotoc_md586", null ]
    ] ],
    [ "Installation Guide", "md_docs_guides_setup_installation.html", [
      [ "Prerequisites", "md_docs_guides_setup_installation.html#autotoc_md588", null ],
      [ "Installation Methods", "md_docs_guides_setup_installation.html#autotoc_md589", [
        [ "Method 1: vcpkg (Recommended)", "md_docs_guides_setup_installation.html#autotoc_md590", [
          [ "Step 1: Install via vcpkg", "md_docs_guides_setup_installation.html#autotoc_md591", null ],
          [ "Step 2: Use in your project", "md_docs_guides_setup_installation.html#autotoc_md592", null ]
        ] ],
        [ "Method 2: Install from Source (CMake Package)", "md_docs_guides_setup_installation.html#autotoc_md593", [
          [ "Step 1: Build and install", "md_docs_guides_setup_installation.html#autotoc_md594", null ],
          [ "Step 2: Use in your project", "md_docs_guides_setup_installation.html#autotoc_md595", null ]
        ] ],
        [ "Method 3: Release Packages", "md_docs_guides_setup_installation.html#autotoc_md596", [
          [ "Step 1: Download and extract", "md_docs_guides_setup_installation.html#autotoc_md597", null ],
          [ "Step 2: Install", "md_docs_guides_setup_installation.html#autotoc_md598", null ],
          [ "Step 3: Use in your project", "md_docs_guides_setup_installation.html#autotoc_md599", null ]
        ] ],
        [ "Method 4: Git Submodule Integration", "md_docs_guides_setup_installation.html#autotoc_md600", [
          [ "Step 1: Add submodule", "md_docs_guides_setup_installation.html#autotoc_md601", null ],
          [ "Step 2: Use in CMake", "md_docs_guides_setup_installation.html#autotoc_md602", null ]
        ] ]
      ] ],
      [ "Packaging Notes", "md_docs_guides_setup_installation.html#autotoc_md603", null ],
      [ "Build Options (Source Builds)", "md_docs_guides_setup_installation.html#autotoc_md604", null ],
      [ "Next Steps", "md_docs_guides_setup_installation.html#autotoc_md605", null ]
    ] ],
    [ "System Requirements", "md_docs_guides_setup_requirements.html", [
      [ "System Requirements", "md_docs_guides_setup_requirements.html#autotoc_md608", [
        [ "Recommended Platform", "md_docs_guides_setup_requirements.html#autotoc_md609", null ],
        [ "Supported Platforms", "md_docs_guides_setup_requirements.html#autotoc_md610", null ]
      ] ],
      [ "Dependencies", "md_docs_guides_setup_requirements.html#autotoc_md612", [
        [ "Core Library Dependencies", "md_docs_guides_setup_requirements.html#autotoc_md613", null ],
        [ "Dependency Details", "md_docs_guides_setup_requirements.html#autotoc_md614", null ]
      ] ],
      [ "Optional Dependencies", "md_docs_guides_setup_requirements.html#autotoc_md616", [
        [ "For Documentation Generation", "md_docs_guides_setup_requirements.html#autotoc_md617", null ],
        [ "For Development", "md_docs_guides_setup_requirements.html#autotoc_md618", null ]
      ] ],
      [ "Compiler Requirements", "md_docs_guides_setup_requirements.html#autotoc_md620", [
        [ "Minimum Compiler Versions", "md_docs_guides_setup_requirements.html#autotoc_md621", null ],
        [ "C++ Standard", "md_docs_guides_setup_requirements.html#autotoc_md622", null ],
        [ "Compiler Features Required", "md_docs_guides_setup_requirements.html#autotoc_md623", null ]
      ] ],
      [ "Build Environment", "md_docs_guides_setup_requirements.html#autotoc_md625", [
        [ "Disk Space", "md_docs_guides_setup_requirements.html#autotoc_md626", null ],
        [ "Memory Requirements", "md_docs_guides_setup_requirements.html#autotoc_md627", null ],
        [ "CPU", "md_docs_guides_setup_requirements.html#autotoc_md628", null ]
      ] ],
      [ "Runtime Requirements", "md_docs_guides_setup_requirements.html#autotoc_md630", [
        [ "For Applications Using unilink", "md_docs_guides_setup_requirements.html#autotoc_md631", null ],
        [ "Thread Support", "md_docs_guides_setup_requirements.html#autotoc_md632", null ]
      ] ],
      [ "Platform-Specific Notes", "md_docs_guides_setup_requirements.html#autotoc_md634", [
        [ "Ubuntu 22.04 LTS", "md_docs_guides_setup_requirements.html#autotoc_md635", null ],
        [ "Ubuntu 20.04 LTS", "md_docs_guides_setup_requirements.html#autotoc_md636", null ],
        [ "Other Linux Distributions", "md_docs_guides_setup_requirements.html#autotoc_md637", null ]
      ] ],
      [ "Verifying Your Environment", "md_docs_guides_setup_requirements.html#autotoc_md639", [
        [ "Check Compiler Version", "md_docs_guides_setup_requirements.html#autotoc_md640", null ],
        [ "Check CMake Version", "md_docs_guides_setup_requirements.html#autotoc_md641", null ],
        [ "Check Boost Version", "md_docs_guides_setup_requirements.html#autotoc_md642", null ],
        [ "Quick Environment Test", "md_docs_guides_setup_requirements.html#autotoc_md643", null ]
      ] ],
      [ "Troubleshooting", "md_docs_guides_setup_requirements.html#autotoc_md645", [
        [ "Problem: Compiler Too Old", "md_docs_guides_setup_requirements.html#autotoc_md646", null ],
        [ "Problem: Boost Not Found", "md_docs_guides_setup_requirements.html#autotoc_md647", null ],
        [ "Problem: CMake Too Old", "md_docs_guides_setup_requirements.html#autotoc_md648", null ]
      ] ],
      [ "Next Steps", "md_docs_guides_setup_requirements.html#autotoc_md650", null ]
    ] ],
    [ "Unilink Documentation Index", "md_docs_index.html", [
      [ "📚 Documentation Structure", "md_docs_index.html#autotoc_md653", null ],
      [ "🚀 Getting Started", "md_docs_index.html#autotoc_md655", [
        [ "New to Unilink?", "md_docs_index.html#autotoc_md656", null ],
        [ "Core Documentation", "md_docs_index.html#autotoc_md657", null ]
      ] ],
      [ "📖 Tutorials", "md_docs_index.html#autotoc_md659", [
        [ "Beginner Tutorials", "md_docs_index.html#autotoc_md660", null ],
        [ "Coming Soon", "md_docs_index.html#autotoc_md661", null ]
      ] ],
      [ "📋 Guides", "md_docs_index.html#autotoc_md663", [
        [ "Setup Guides", "md_docs_index.html#autotoc_md664", null ],
        [ "Core Guides", "md_docs_index.html#autotoc_md665", null ],
        [ "Advanced Guides", "md_docs_index.html#autotoc_md666", null ],
        [ "Quick Reference", "md_docs_index.html#autotoc_md667", null ]
      ] ],
      [ "📚 API Reference", "md_docs_index.html#autotoc_md669", [
        [ "Core APIs", "md_docs_index.html#autotoc_md670", null ],
        [ "Advanced Features", "md_docs_index.html#autotoc_md671", null ]
      ] ],
      [ "🏗️ Architecture", "md_docs_index.html#autotoc_md673", [
        [ "Architecture Documentation", "md_docs_index.html#autotoc_md674", null ],
        [ "Key Concepts", "md_docs_index.html#autotoc_md675", null ]
      ] ],
      [ "🔧 Development", "md_docs_index.html#autotoc_md677", [
        [ "Development Documentation", "md_docs_index.html#autotoc_md678", null ],
        [ "Build Options", "md_docs_index.html#autotoc_md679", null ]
      ] ],
      [ "💡 Examples", "md_docs_index.html#autotoc_md681", [
        [ "Example Applications", "md_docs_index.html#autotoc_md682", null ],
        [ "Code Snippets", "md_docs_index.html#autotoc_md683", null ]
      ] ],
      [ "🔍 Search & Find", "md_docs_index.html#autotoc_md685", [
        [ "By Topic", "md_docs_index.html#autotoc_md686", null ],
        [ "By Use Case", "md_docs_index.html#autotoc_md687", null ]
      ] ],
      [ "📊 Documentation Stats", "md_docs_index.html#autotoc_md689", null ],
      [ "🆘 Need Help?", "md_docs_index.html#autotoc_md691", [
        [ "Quick Links", "md_docs_index.html#autotoc_md692", null ],
        [ "Learning Path", "md_docs_index.html#autotoc_md693", null ]
      ] ],
      [ "📝 Document History", "md_docs_index.html#autotoc_md695", null ],
      [ "🤝 Contributing", "md_docs_index.html#autotoc_md697", null ]
    ] ],
    [ "Unilink API Guide", "md_docs_reference_api_guide.html", [
      [ "Table of Contents", "md_docs_reference_api_guide.html#autotoc_md725", null ],
      [ "Builder API", "md_docs_reference_api_guide.html#autotoc_md727", [
        [ "Core Concept", "md_docs_reference_api_guide.html#autotoc_md728", null ],
        [ "Common Methods (All Builders)", "md_docs_reference_api_guide.html#autotoc_md729", null ],
        [ "IO Context Ownership (advanced)", "md_docs_reference_api_guide.html#autotoc_md730", null ]
      ] ],
      [ "TCP Client", "md_docs_reference_api_guide.html#autotoc_md732", [
        [ "Basic Usage", "md_docs_reference_api_guide.html#autotoc_md733", null ],
        [ "API Reference", "md_docs_reference_api_guide.html#autotoc_md734", [
          [ "Constructor", "md_docs_reference_api_guide.html#autotoc_md735", null ],
          [ "Builder Methods", "md_docs_reference_api_guide.html#autotoc_md736", null ],
          [ "Instance Methods", "md_docs_reference_api_guide.html#autotoc_md737", null ]
        ] ],
        [ "Advanced Examples", "md_docs_reference_api_guide.html#autotoc_md738", [
          [ "With Member Functions", "md_docs_reference_api_guide.html#autotoc_md739", null ],
          [ "With Lambda Capture", "md_docs_reference_api_guide.html#autotoc_md740", null ]
        ] ]
      ] ],
      [ "TCP Server", "md_docs_reference_api_guide.html#autotoc_md742", [
        [ "Basic Usage", "md_docs_reference_api_guide.html#autotoc_md743", null ],
        [ "API Reference", "md_docs_reference_api_guide.html#autotoc_md744", [
          [ "Constructor", "md_docs_reference_api_guide.html#autotoc_md745", null ],
          [ "Builder Methods", "md_docs_reference_api_guide.html#autotoc_md746", null ],
          [ "Instance Methods", "md_docs_reference_api_guide.html#autotoc_md747", null ]
        ] ],
        [ "Advanced Examples", "md_docs_reference_api_guide.html#autotoc_md748", [
          [ "Single Client Mode", "md_docs_reference_api_guide.html#autotoc_md749", null ],
          [ "Port Retry", "md_docs_reference_api_guide.html#autotoc_md750", null ],
          [ "Echo Server Pattern", "md_docs_reference_api_guide.html#autotoc_md751", null ]
        ] ]
      ] ],
      [ "Serial Communication", "md_docs_reference_api_guide.html#autotoc_md753", [
        [ "Basic Usage", "md_docs_reference_api_guide.html#autotoc_md754", null ],
        [ "API Reference", "md_docs_reference_api_guide.html#autotoc_md755", [
          [ "Constructor", "md_docs_reference_api_guide.html#autotoc_md756", null ],
          [ "Builder Methods", "md_docs_reference_api_guide.html#autotoc_md757", null ],
          [ "Instance Methods", "md_docs_reference_api_guide.html#autotoc_md758", null ]
        ] ],
        [ "Device Paths", "md_docs_reference_api_guide.html#autotoc_md759", null ],
        [ "Advanced Examples", "md_docs_reference_api_guide.html#autotoc_md760", [
          [ "Arduino Communication", "md_docs_reference_api_guide.html#autotoc_md761", null ],
          [ "GPS Module", "md_docs_reference_api_guide.html#autotoc_md762", null ]
        ] ]
      ] ],
      [ "UDP Communication", "md_docs_reference_api_guide.html#autotoc_md764", [
        [ "Basic Usage", "md_docs_reference_api_guide.html#autotoc_md765", [
          [ "UDP Receiver (Server)", "md_docs_reference_api_guide.html#autotoc_md766", null ],
          [ "UDP Sender (Client)", "md_docs_reference_api_guide.html#autotoc_md767", null ]
        ] ],
        [ "API Reference", "md_docs_reference_api_guide.html#autotoc_md768", [
          [ "Constructor", "md_docs_reference_api_guide.html#autotoc_md769", null ],
          [ "Builder Methods", "md_docs_reference_api_guide.html#autotoc_md770", null ],
          [ "Instance Methods", "md_docs_reference_api_guide.html#autotoc_md771", null ]
        ] ],
        [ "Advanced Examples", "md_docs_reference_api_guide.html#autotoc_md772", [
          [ "Echo Reply (Receiver)", "md_docs_reference_api_guide.html#autotoc_md773", null ]
        ] ]
      ] ],
      [ "Error Handling", "md_docs_reference_api_guide.html#autotoc_md775", [
        [ "Setup Error Handler", "md_docs_reference_api_guide.html#autotoc_md776", null ],
        [ "Error Levels", "md_docs_reference_api_guide.html#autotoc_md777", null ],
        [ "Error Statistics", "md_docs_reference_api_guide.html#autotoc_md778", null ],
        [ "Error Categories", "md_docs_reference_api_guide.html#autotoc_md779", null ]
      ] ],
      [ "Logging System", "md_docs_reference_api_guide.html#autotoc_md781", [
        [ "Basic Usage", "md_docs_reference_api_guide.html#autotoc_md782", null ],
        [ "Log Levels", "md_docs_reference_api_guide.html#autotoc_md783", null ],
        [ "Async Logging", "md_docs_reference_api_guide.html#autotoc_md784", null ],
        [ "Log Rotation", "md_docs_reference_api_guide.html#autotoc_md785", null ]
      ] ],
      [ "Configuration Management", "md_docs_reference_api_guide.html#autotoc_md787", [
        [ "Load Configuration from File", "md_docs_reference_api_guide.html#autotoc_md788", null ],
        [ "Configuration File Format (JSON)", "md_docs_reference_api_guide.html#autotoc_md789", null ]
      ] ],
      [ "Advanced Features", "md_docs_reference_api_guide.html#autotoc_md791", [
        [ "Memory Pool", "md_docs_reference_api_guide.html#autotoc_md792", null ],
        [ "Safe Data Buffer", "md_docs_reference_api_guide.html#autotoc_md793", null ],
        [ "Thread-Safe State", "md_docs_reference_api_guide.html#autotoc_md794", null ]
      ] ],
      [ "Best Practices", "md_docs_reference_api_guide.html#autotoc_md796", [
        [ "1. Always Handle Errors", "md_docs_reference_api_guide.html#autotoc_md797", null ],
        [ "2. Use Explicit Lifecycle Control", "md_docs_reference_api_guide.html#autotoc_md798", null ],
        [ "3. Set Appropriate Retry Intervals", "md_docs_reference_api_guide.html#autotoc_md799", null ],
        [ "4. Enable Logging for Debugging", "md_docs_reference_api_guide.html#autotoc_md800", null ],
        [ "5. Use Member Functions for OOP Design", "md_docs_reference_api_guide.html#autotoc_md801", null ]
      ] ],
      [ "Performance Tips", "md_docs_reference_api_guide.html#autotoc_md803", [
        [ "1. Use Independent Context for Testing Only", "md_docs_reference_api_guide.html#autotoc_md804", null ],
        [ "2. Enable Async Logging", "md_docs_reference_api_guide.html#autotoc_md805", null ],
        [ "3. Use Memory Pool for Frequent Allocations", "md_docs_reference_api_guide.html#autotoc_md806", null ],
        [ "4. Disable Unnecessary Features", "md_docs_reference_api_guide.html#autotoc_md807", null ]
      ] ]
    ] ],
    [ "Test Structure", "md_docs_test_structure.html", [
      [ "Overview", "md_docs_test_structure.html#autotoc_md810", null ],
      [ "Directory Structure", "md_docs_test_structure.html#autotoc_md812", null ],
      [ "Running Tests", "md_docs_test_structure.html#autotoc_md814", [
        [ "Run All Tests", "md_docs_test_structure.html#autotoc_md815", null ],
        [ "Run by Category", "md_docs_test_structure.html#autotoc_md816", null ],
        [ "Run by Component", "md_docs_test_structure.html#autotoc_md817", null ],
        [ "Run Specific Test Executable", "md_docs_test_structure.html#autotoc_md818", null ]
      ] ],
      [ "CI/CD Integration", "md_docs_test_structure.html#autotoc_md820", [
        [ "GitHub Actions Workflows", "md_docs_test_structure.html#autotoc_md821", [
          [ "1. <strong>Pull Request</strong> (Fast feedback)", "md_docs_test_structure.html#autotoc_md822", null ],
          [ "2. <strong>Main/Develop Branch</strong> (Full validation)", "md_docs_test_structure.html#autotoc_md823", null ],
          [ "3. <strong>Nightly/On-Demand</strong> (Performance)", "md_docs_test_structure.html#autotoc_md824", null ]
        ] ],
        [ "Workflow Files", "md_docs_test_structure.html#autotoc_md825", null ]
      ] ],
      [ "Test Results Summary", "md_docs_test_structure.html#autotoc_md827", null ],
      [ "Benefits of Organized Structure", "md_docs_test_structure.html#autotoc_md829", [
        [ "1. <strong>Faster Feedback</strong> ⚡", "md_docs_test_structure.html#autotoc_md830", null ],
        [ "2. **Better Maintainability** 🔧", "md_docs_test_structure.html#autotoc_md831", null ],
        [ "3. **Improved Developer Experience** 👨‍💻", "md_docs_test_structure.html#autotoc_md832", null ],
        [ "4. **CI Optimization** 🚀", "md_docs_test_structure.html#autotoc_md833", null ]
      ] ],
      [ "Adding New Tests", "md_docs_test_structure.html#autotoc_md835", [
        [ "1. Choose the Right Category", "md_docs_test_structure.html#autotoc_md836", null ],
        [ "2. Add Test File", "md_docs_test_structure.html#autotoc_md837", null ],
        [ "3. Update CMakeLists.txt", "md_docs_test_structure.html#autotoc_md838", null ],
        [ "4. Build and Run", "md_docs_test_structure.html#autotoc_md839", null ]
      ] ],
      [ "Test Best Practices", "md_docs_test_structure.html#autotoc_md841", [
        [ "Unit Tests", "md_docs_test_structure.html#autotoc_md842", null ],
        [ "Integration Tests", "md_docs_test_structure.html#autotoc_md843", null ],
        [ "E2E Tests", "md_docs_test_structure.html#autotoc_md844", null ],
        [ "Performance Tests", "md_docs_test_structure.html#autotoc_md845", null ]
      ] ],
      [ "Troubleshooting", "md_docs_test_structure.html#autotoc_md847", [
        [ "Test Failures", "md_docs_test_structure.html#autotoc_md848", null ],
        [ "Build Issues", "md_docs_test_structure.html#autotoc_md849", null ],
        [ "Timeout Issues", "md_docs_test_structure.html#autotoc_md850", null ]
      ] ],
      [ "Future Improvements", "md_docs_test_structure.html#autotoc_md852", null ],
      [ "References", "md_docs_test_structure.html#autotoc_md854", null ]
    ] ],
    [ "Tutorial 1: Getting Started with Unilink", "md_docs_tutorials_01_getting_started.html", [
      [ "What You'll Build", "md_docs_tutorials_01_getting_started.html#autotoc_md857", null ],
      [ "Step 1: Install Dependencies", "md_docs_tutorials_01_getting_started.html#autotoc_md859", [
        [ "Ubuntu/Debian", "md_docs_tutorials_01_getting_started.html#autotoc_md860", null ],
        [ "macOS", "md_docs_tutorials_01_getting_started.html#autotoc_md861", null ],
        [ "Windows (vcpkg)", "md_docs_tutorials_01_getting_started.html#autotoc_md862", null ]
      ] ],
      [ "Step 2: Install Unilink", "md_docs_tutorials_01_getting_started.html#autotoc_md864", [
        [ "Option A: From Source", "md_docs_tutorials_01_getting_started.html#autotoc_md865", null ],
        [ "Option B: Use as Subdirectory", "md_docs_tutorials_01_getting_started.html#autotoc_md866", null ]
      ] ],
      [ "Step 3: Create Your First Client", "md_docs_tutorials_01_getting_started.html#autotoc_md868", null ],
      [ "Step 4: Create CMakeLists.txt", "md_docs_tutorials_01_getting_started.html#autotoc_md870", null ],
      [ "Step 5: Build Your Application", "md_docs_tutorials_01_getting_started.html#autotoc_md872", null ],
      [ "Step 6: Test Your Application", "md_docs_tutorials_01_getting_started.html#autotoc_md874", [
        [ "Start a Test Server", "md_docs_tutorials_01_getting_started.html#autotoc_md875", null ],
        [ "Run Your Client", "md_docs_tutorials_01_getting_started.html#autotoc_md876", null ]
      ] ],
      [ "Step 7: Understanding the Code", "md_docs_tutorials_01_getting_started.html#autotoc_md878", [
        [ "1. Builder Pattern", "md_docs_tutorials_01_getting_started.html#autotoc_md879", null ],
        [ "2. Callbacks", "md_docs_tutorials_01_getting_started.html#autotoc_md880", null ],
        [ "3. Automatic Reconnection", "md_docs_tutorials_01_getting_started.html#autotoc_md881", null ],
        [ "4. Thread Safety", "md_docs_tutorials_01_getting_started.html#autotoc_md882", null ]
      ] ],
      [ "Common Issues", "md_docs_tutorials_01_getting_started.html#autotoc_md884", [
        [ "Issue 1: Connection Refused", "md_docs_tutorials_01_getting_started.html#autotoc_md885", null ],
        [ "Issue 2: Port Already in Use", "md_docs_tutorials_01_getting_started.html#autotoc_md886", null ],
        [ "Issue 3: Compilation Error - unilink not found", "md_docs_tutorials_01_getting_started.html#autotoc_md887", null ]
      ] ],
      [ "Next Steps", "md_docs_tutorials_01_getting_started.html#autotoc_md889", null ],
      [ "Full Example Code", "md_docs_tutorials_01_getting_started.html#autotoc_md891", null ]
    ] ],
    [ "Tutorial 2: Building a TCP Server", "md_docs_tutorials_02_tcp_server.html", [
      [ "What You'll Build", "md_docs_tutorials_02_tcp_server.html#autotoc_md895", null ],
      [ "Step 1: Basic Server Setup", "md_docs_tutorials_02_tcp_server.html#autotoc_md897", null ],
      [ "Step 2: Build and Test", "md_docs_tutorials_02_tcp_server.html#autotoc_md899", [
        [ "Compile", "md_docs_tutorials_02_tcp_server.html#autotoc_md900", null ],
        [ "Run Server", "md_docs_tutorials_02_tcp_server.html#autotoc_md901", null ],
        [ "Test with Multiple Clients", "md_docs_tutorials_02_tcp_server.html#autotoc_md902", null ]
      ] ],
      [ "Step 3: Add Client Management", "md_docs_tutorials_02_tcp_server.html#autotoc_md904", null ],
      [ "Step 4: Single Client Mode", "md_docs_tutorials_02_tcp_server.html#autotoc_md906", null ],
      [ "Step 5: Port Retry Logic", "md_docs_tutorials_02_tcp_server.html#autotoc_md908", null ],
      [ "Step 6: Broadcasting to All Clients", "md_docs_tutorials_02_tcp_server.html#autotoc_md910", null ],
      [ "Complete Example: Chat Server", "md_docs_tutorials_02_tcp_server.html#autotoc_md912", null ],
      [ "Testing Your Chat Server", "md_docs_tutorials_02_tcp_server.html#autotoc_md914", [
        [ "Start Server", "md_docs_tutorials_02_tcp_server.html#autotoc_md915", null ],
        [ "Connect Multiple Clients", "md_docs_tutorials_02_tcp_server.html#autotoc_md916", null ],
        [ "Try Commands", "md_docs_tutorials_02_tcp_server.html#autotoc_md917", null ]
      ] ],
      [ "Best Practices", "md_docs_tutorials_02_tcp_server.html#autotoc_md919", [
        [ "1. Always Check Server Status", "md_docs_tutorials_02_tcp_server.html#autotoc_md920", null ],
        [ "2. Use Thread-Safe Data Structures", "md_docs_tutorials_02_tcp_server.html#autotoc_md921", null ],
        [ "3. Handle Graceful Shutdown", "md_docs_tutorials_02_tcp_server.html#autotoc_md922", null ],
        [ "4. Implement Error Recovery", "md_docs_tutorials_02_tcp_server.html#autotoc_md923", null ]
      ] ],
      [ "Common Patterns", "md_docs_tutorials_02_tcp_server.html#autotoc_md925", [
        [ "Pattern 1: Command Parser", "md_docs_tutorials_02_tcp_server.html#autotoc_md926", null ],
        [ "Pattern 2: Rate Limiting", "md_docs_tutorials_02_tcp_server.html#autotoc_md927", null ]
      ] ],
      [ "Next Steps", "md_docs_tutorials_02_tcp_server.html#autotoc_md929", null ],
      [ "Full Example Code", "md_docs_tutorials_02_tcp_server.html#autotoc_md931", null ]
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
"classunilink_1_1common_1_1MemoryPool.html",
"classunilink_1_1interface_1_1Channel.html#a77068c3759acd3355ffeecf652661505",
"classunilink_1_1wrapper_1_1Udp.html#a8e83e46303c4cedf045f96bb22c5446b",
"index.html#autotoc_md146",
"md_docs_guides_core_testing.html#autotoc_md347",
"md_docs_index.html#autotoc_md691",
"namespaceunilink_1_1common_1_1constants.html#a02d5f5d0f5bb76414da33078a6888c32",
"structunilink_1_1config_1_1TcpClientConfig.html#acba30b670711d398bb87ba94c3eb56f3"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';