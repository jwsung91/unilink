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
    [ "Unilink System Architecture", "index.html", "index" ],
    [ "Transport Channel Contract", "md_docs_2architecture_2channel__contract.html", [
      [ "1. Introduction", "md_docs_2architecture_2channel__contract.html#autotoc_md1", null ],
      [ "2. Core Principles", "md_docs_2architecture_2channel__contract.html#autotoc_md2", null ],
      [ "3. Stop Semantics: No Callbacks After Stop()", "md_docs_2architecture_2channel__contract.html#autotoc_md3", null ],
      [ "4. Backpressure Policy", "md_docs_2architecture_2channel__contract.html#autotoc_md4", null ],
      [ "5. Error Handling Consistency", "md_docs_2architecture_2channel__contract.html#autotoc_md5", null ],
      [ "6. API Whitelist and State Transitions", "md_docs_2architecture_2channel__contract.html#autotoc_md6", null ],
      [ "7. Versioning and Evolution", "md_docs_2architecture_2channel__contract.html#autotoc_md7", null ]
    ] ],
    [ "Memory Safety Architecture", "md_docs_2architecture_2memory__safety.html", [
      [ "Table of Contents", "md_docs_2architecture_2memory__safety.html#autotoc_md10", null ],
      [ "Overview", "md_docs_2architecture_2memory__safety.html#autotoc_md12", [
        [ "Memory Safety Guarantees", "md_docs_2architecture_2memory__safety.html#autotoc_md13", null ],
        [ "Safety Levels", "md_docs_2architecture_2memory__safety.html#autotoc_md15", null ]
      ] ],
      [ "Safe Data Handling", "md_docs_2architecture_2memory__safety.html#autotoc_md17", [
        [ "SafeDataBuffer", "md_docs_2architecture_2memory__safety.html#autotoc_md18", null ],
        [ "Features", "md_docs_2architecture_2memory__safety.html#autotoc_md20", [
          [ "1. Construction Validation", "md_docs_2architecture_2memory__safety.html#autotoc_md21", null ],
          [ "2. Safe Type Conversions", "md_docs_2architecture_2memory__safety.html#autotoc_md23", null ],
          [ "3. Memory Validation", "md_docs_2architecture_2memory__safety.html#autotoc_md25", null ]
        ] ],
        [ "Safe Span (C++17 Compatible)", "md_docs_2architecture_2memory__safety.html#autotoc_md27", null ]
      ] ],
      [ "Thread-Safe State Management", "md_docs_2architecture_2memory__safety.html#autotoc_md29", [
        [ "ThreadSafeState", "md_docs_2architecture_2memory__safety.html#autotoc_md30", null ],
        [ "AtomicState", "md_docs_2architecture_2memory__safety.html#autotoc_md32", null ],
        [ "ThreadSafeCounter", "md_docs_2architecture_2memory__safety.html#autotoc_md34", null ],
        [ "ThreadSafeFlag", "md_docs_2architecture_2memory__safety.html#autotoc_md36", null ],
        [ "Thread Safety Summary", "md_docs_2architecture_2memory__safety.html#autotoc_md38", null ]
      ] ],
      [ "Memory Tracking", "md_docs_2architecture_2memory__safety.html#autotoc_md40", [
        [ "Built-in Memory Tracking", "md_docs_2architecture_2memory__safety.html#autotoc_md41", null ],
        [ "Features", "md_docs_2architecture_2memory__safety.html#autotoc_md43", [
          [ "1. Allocation Tracking", "md_docs_2architecture_2memory__safety.html#autotoc_md44", null ],
          [ "2. Leak Detection", "md_docs_2architecture_2memory__safety.html#autotoc_md46", null ],
          [ "3. Performance Monitoring", "md_docs_2architecture_2memory__safety.html#autotoc_md48", null ],
          [ "4. Debug Reports", "md_docs_2architecture_2memory__safety.html#autotoc_md50", null ]
        ] ],
        [ "Zero Overhead in Release", "md_docs_2architecture_2memory__safety.html#autotoc_md52", null ]
      ] ],
      [ "AddressSanitizer Support", "md_docs_2architecture_2memory__safety.html#autotoc_md54", [
        [ "Enable AddressSanitizer", "md_docs_2architecture_2memory__safety.html#autotoc_md55", null ],
        [ "What ASan Detects", "md_docs_2architecture_2memory__safety.html#autotoc_md57", null ],
        [ "Running with ASan", "md_docs_2architecture_2memory__safety.html#autotoc_md59", null ],
        [ "Performance Impact", "md_docs_2architecture_2memory__safety.html#autotoc_md61", null ]
      ] ],
      [ "Best Practices", "md_docs_2architecture_2memory__safety.html#autotoc_md63", [
        [ "1. Buffer Management", "md_docs_2architecture_2memory__safety.html#autotoc_md64", [
          [ "✅ DO", "md_docs_2architecture_2memory__safety.html#autotoc_md65", null ],
          [ "❌ DON'T", "md_docs_2architecture_2memory__safety.html#autotoc_md66", null ]
        ] ],
        [ "2. Type Conversions", "md_docs_2architecture_2memory__safety.html#autotoc_md68", [
          [ "✅ DO", "md_docs_2architecture_2memory__safety.html#autotoc_md69", null ],
          [ "❌ DON'T", "md_docs_2architecture_2memory__safety.html#autotoc_md70", null ]
        ] ],
        [ "3. Thread Safety", "md_docs_2architecture_2memory__safety.html#autotoc_md72", [
          [ "✅ DO", "md_docs_2architecture_2memory__safety.html#autotoc_md73", null ],
          [ "❌ DON'T", "md_docs_2architecture_2memory__safety.html#autotoc_md74", null ]
        ] ],
        [ "4. Memory Tracking", "md_docs_2architecture_2memory__safety.html#autotoc_md76", [
          [ "✅ DO", "md_docs_2architecture_2memory__safety.html#autotoc_md77", null ],
          [ "❌ DON'T", "md_docs_2architecture_2memory__safety.html#autotoc_md78", null ]
        ] ],
        [ "5. Sanitizers", "md_docs_2architecture_2memory__safety.html#autotoc_md80", [
          [ "✅ DO", "md_docs_2architecture_2memory__safety.html#autotoc_md81", null ],
          [ "❌ DON'T", "md_docs_2architecture_2memory__safety.html#autotoc_md82", null ]
        ] ]
      ] ],
      [ "Memory Safety Benefits", "md_docs_2architecture_2memory__safety.html#autotoc_md84", [
        [ "Prevents Common Vulnerabilities", "md_docs_2architecture_2memory__safety.html#autotoc_md85", null ],
        [ "Performance", "md_docs_2architecture_2memory__safety.html#autotoc_md87", null ]
      ] ],
      [ "Testing Memory Safety", "md_docs_2architecture_2memory__safety.html#autotoc_md89", [
        [ "Unit Tests", "md_docs_2architecture_2memory__safety.html#autotoc_md90", null ],
        [ "Integration Tests", "md_docs_2architecture_2memory__safety.html#autotoc_md92", null ],
        [ "Continuous Integration", "md_docs_2architecture_2memory__safety.html#autotoc_md94", null ]
      ] ],
      [ "Next Steps", "md_docs_2architecture_2memory__safety.html#autotoc_md96", null ]
    ] ],
    [ "Runtime Behavior Model", "md_docs_2architecture_2runtime__behavior.html", [
      [ "Table of Contents", "md_docs_2architecture_2runtime__behavior.html#autotoc_md176", null ],
      [ "Threading Model & Callback Execution", "md_docs_2architecture_2runtime__behavior.html#autotoc_md178", [
        [ "Architecture Diagram", "md_docs_2architecture_2runtime__behavior.html#autotoc_md179", null ],
        [ "Key Points", "md_docs_2architecture_2runtime__behavior.html#autotoc_md181", [
          [ "✅ Thread-Safe API Methods", "md_docs_2architecture_2runtime__behavior.html#autotoc_md182", null ],
          [ "✅ Callback Execution Context", "md_docs_2architecture_2runtime__behavior.html#autotoc_md184", null ],
          [ "⚠️ Never Block in Callbacks", "md_docs_2architecture_2runtime__behavior.html#autotoc_md186", null ],
          [ "✅ Thread-Safe State Access", "md_docs_2architecture_2runtime__behavior.html#autotoc_md188", null ]
        ] ],
        [ "Threading Model Summary", "md_docs_2architecture_2runtime__behavior.html#autotoc_md190", null ]
      ] ],
      [ "Reconnection Policy & State Machine", "md_docs_2architecture_2runtime__behavior.html#autotoc_md192", [
        [ "State Machine Diagram", "md_docs_2architecture_2runtime__behavior.html#autotoc_md193", null ],
        [ "Connection States", "md_docs_2architecture_2runtime__behavior.html#autotoc_md195", null ],
        [ "Configuration Example", "md_docs_2architecture_2runtime__behavior.html#autotoc_md197", null ],
        [ "Retry Behavior", "md_docs_2architecture_2runtime__behavior.html#autotoc_md199", [
          [ "Default Behavior", "md_docs_2architecture_2runtime__behavior.html#autotoc_md200", null ],
          [ "Retry Interval Configuration", "md_docs_2architecture_2runtime__behavior.html#autotoc_md201", null ],
          [ "State Callbacks", "md_docs_2architecture_2runtime__behavior.html#autotoc_md203", null ],
          [ "Manual Control", "md_docs_2architecture_2runtime__behavior.html#autotoc_md205", null ]
        ] ],
        [ "Reconnection Best Practices", "md_docs_2architecture_2runtime__behavior.html#autotoc_md207", [
          [ "1. Choose Appropriate Retry Interval", "md_docs_2architecture_2runtime__behavior.html#autotoc_md208", null ],
          [ "2. Handle State Transitions", "md_docs_2architecture_2runtime__behavior.html#autotoc_md210", null ],
          [ "3. Graceful Shutdown", "md_docs_2architecture_2runtime__behavior.html#autotoc_md212", null ]
        ] ]
      ] ],
      [ "Backpressure Handling", "md_docs_2architecture_2runtime__behavior.html#autotoc_md214", [
        [ "Backpressure Flow", "md_docs_2architecture_2runtime__behavior.html#autotoc_md215", null ],
        [ "Backpressure Configuration", "md_docs_2architecture_2runtime__behavior.html#autotoc_md217", null ],
        [ "Backpressure Strategies", "md_docs_2architecture_2runtime__behavior.html#autotoc_md219", [
          [ "Strategy 1: Pause Sending", "md_docs_2architecture_2runtime__behavior.html#autotoc_md220", null ],
          [ "Strategy 2: Rate Limiting", "md_docs_2architecture_2runtime__behavior.html#autotoc_md222", null ],
          [ "Strategy 3: Drop Data", "md_docs_2architecture_2runtime__behavior.html#autotoc_md224", null ]
        ] ],
        [ "Backpressure Monitoring", "md_docs_2architecture_2runtime__behavior.html#autotoc_md226", null ],
        [ "Memory Safety", "md_docs_2architecture_2runtime__behavior.html#autotoc_md228", null ]
      ] ],
      [ "Best Practices", "md_docs_2architecture_2runtime__behavior.html#autotoc_md230", [
        [ "1. Threading Best Practices", "md_docs_2architecture_2runtime__behavior.html#autotoc_md231", [
          [ "✅ DO", "md_docs_2architecture_2runtime__behavior.html#autotoc_md232", null ],
          [ "❌ DON'T", "md_docs_2architecture_2runtime__behavior.html#autotoc_md233", null ]
        ] ],
        [ "2. Reconnection Best Practices", "md_docs_2architecture_2runtime__behavior.html#autotoc_md235", [
          [ "✅ DO", "md_docs_2architecture_2runtime__behavior.html#autotoc_md236", null ],
          [ "❌ DON'T", "md_docs_2architecture_2runtime__behavior.html#autotoc_md237", null ]
        ] ],
        [ "3. Backpressure Best Practices", "md_docs_2architecture_2runtime__behavior.html#autotoc_md239", [
          [ "✅ DO", "md_docs_2architecture_2runtime__behavior.html#autotoc_md240", null ],
          [ "❌ DON'T", "md_docs_2architecture_2runtime__behavior.html#autotoc_md241", null ]
        ] ]
      ] ],
      [ "Performance Considerations", "md_docs_2architecture_2runtime__behavior.html#autotoc_md243", [
        [ "Threading Overhead", "md_docs_2architecture_2runtime__behavior.html#autotoc_md244", null ],
        [ "Reconnection Overhead", "md_docs_2architecture_2runtime__behavior.html#autotoc_md245", null ],
        [ "Backpressure Overhead", "md_docs_2architecture_2runtime__behavior.html#autotoc_md246", null ]
      ] ],
      [ "Next Steps", "md_docs_2architecture_2runtime__behavior.html#autotoc_md248", null ]
    ] ],
    [ "Wrapper Contract", "md_docs_2architecture_2wrapper__contract.html", [
      [ "Scope", "md_docs_2architecture_2wrapper__contract.html#autotoc_md250", null ],
      [ "Core Rules", "md_docs_2architecture_2wrapper__contract.html#autotoc_md251", [
        [ "1. <tt>start()</tt> reflects real transport state", "md_docs_2architecture_2wrapper__contract.html#autotoc_md252", null ],
        [ "2. Repeated <tt>start()</tt> and <tt>stop()</tt> are safe", "md_docs_2architecture_2wrapper__contract.html#autotoc_md253", null ],
        [ "3. <tt>auto_start(true)</tt> follows the same startup contract", "md_docs_2architecture_2wrapper__contract.html#autotoc_md254", null ]
      ] ],
      [ "External <tt>io_context</tt> Contract", "md_docs_2architecture_2wrapper__contract.html#autotoc_md255", [
        [ "4. Externally supplied <tt>io_context</tt> can be reused", "md_docs_2architecture_2wrapper__contract.html#autotoc_md256", null ],
        [ "5. Managed and unmanaged external contexts have different ownership rules", "md_docs_2architecture_2wrapper__contract.html#autotoc_md257", null ]
      ] ],
      [ "Callback Contract", "md_docs_2architecture_2wrapper__contract.html#autotoc_md258", [
        [ "6. Handler replacement uses the latest callback", "md_docs_2architecture_2wrapper__contract.html#autotoc_md259", null ],
        [ "7. No wrapper callbacks after <tt>stop()</tt> returns", "md_docs_2architecture_2wrapper__contract.html#autotoc_md260", null ],
        [ "8. Generic fallback errors are normalized", "md_docs_2architecture_2wrapper__contract.html#autotoc_md261", null ]
      ] ],
      [ "Transport-Agnostic Expectations", "md_docs_2architecture_2wrapper__contract.html#autotoc_md262", null ],
      [ "Testing Status", "md_docs_2architecture_2wrapper__contract.html#autotoc_md263", null ]
    ] ],
    [ "Performance Guide", "md_docs_2guides_2advanced_2performance.html", [
      [ "Table of Contents", "md_docs_2guides_2advanced_2performance.html#autotoc_md266", null ],
      [ "Performance Overview", "md_docs_2guides_2advanced_2performance.html#autotoc_md268", [
        [ "Characteristics & Goals", "md_docs_2guides_2advanced_2performance.html#autotoc_md269", null ]
      ] ],
      [ "Build Configuration Optimization", "md_docs_2guides_2advanced_2performance.html#autotoc_md271", [
        [ "Minimal Build vs Full Build", "md_docs_2guides_2advanced_2performance.html#autotoc_md272", null ],
        [ "Compiler Optimization Levels", "md_docs_2guides_2advanced_2performance.html#autotoc_md273", null ]
      ] ],
      [ "Runtime Optimization", "md_docs_2guides_2advanced_2performance.html#autotoc_md275", [
        [ "1. Threading Model & IO Context", "md_docs_2guides_2advanced_2performance.html#autotoc_md276", null ],
        [ "2. Async Logging", "md_docs_2guides_2advanced_2performance.html#autotoc_md277", null ],
        [ "3. Non-Blocking Callbacks", "md_docs_2guides_2advanced_2performance.html#autotoc_md278", null ]
      ] ],
      [ "Memory Optimization", "md_docs_2guides_2advanced_2performance.html#autotoc_md280", [
        [ "1. Memory Pool Usage", "md_docs_2guides_2advanced_2performance.html#autotoc_md281", null ],
        [ "2. Avoid Data Copies", "md_docs_2guides_2advanced_2performance.html#autotoc_md282", null ],
        [ "3. Reserve Vector Capacity", "md_docs_2guides_2advanced_2performance.html#autotoc_md283", null ]
      ] ],
      [ "Network Optimization", "md_docs_2guides_2advanced_2performance.html#autotoc_md285", [
        [ "1. Batch Small Messages", "md_docs_2guides_2advanced_2performance.html#autotoc_md286", null ],
        [ "2. Connection Reuse", "md_docs_2guides_2advanced_2performance.html#autotoc_md287", null ],
        [ "3. Socket Tuning", "md_docs_2guides_2advanced_2performance.html#autotoc_md288", null ]
      ] ],
      [ "Benchmarking & Profiling", "md_docs_2guides_2advanced_2performance.html#autotoc_md290", [
        [ "Built-in Performance Tests", "md_docs_2guides_2advanced_2performance.html#autotoc_md291", null ],
        [ "Profiling with Tools", "md_docs_2guides_2advanced_2performance.html#autotoc_md292", null ],
        [ "Simple Throughput Benchmark Code", "md_docs_2guides_2advanced_2performance.html#autotoc_md293", null ]
      ] ],
      [ "Real-World Case Studies", "md_docs_2guides_2advanced_2performance.html#autotoc_md295", [
        [ "Case Study 1: High-Throughput Data Streaming", "md_docs_2guides_2advanced_2performance.html#autotoc_md296", null ],
        [ "Case Study 2: Low-Latency Trading System", "md_docs_2guides_2advanced_2performance.html#autotoc_md297", null ],
        [ "Case Study 3: IoT Gateway (1000+ Connections)", "md_docs_2guides_2advanced_2performance.html#autotoc_md298", null ]
      ] ]
    ] ],
    [ "Unilink Best Practices Guide", "md_docs_2guides_2core_2best__practices.html", [
      [ "Table of Contents", "md_docs_2guides_2core_2best__practices.html#autotoc_md301", null ],
      [ "Error Handling", "md_docs_2guides_2core_2best__practices.html#autotoc_md303", [
        [ "✅ DO: Always Register Error Callbacks", "md_docs_2guides_2core_2best__practices.html#autotoc_md304", null ],
        [ "✅ DO: Check Connection Status Before Sending", "md_docs_2guides_2core_2best__practices.html#autotoc_md305", null ],
        [ "✅ DO: Implement Graceful Error Recovery", "md_docs_2guides_2core_2best__practices.html#autotoc_md306", null ],
        [ "✅ DO: Use Centralized Error Handler", "md_docs_2guides_2core_2best__practices.html#autotoc_md307", null ]
      ] ],
      [ "Resource Management", "md_docs_2guides_2core_2best__practices.html#autotoc_md309", [
        [ "✅ DO: Use RAII and Smart Pointers", "md_docs_2guides_2core_2best__practices.html#autotoc_md310", null ],
        [ "✅ DO: Stop Connections Before Shutdown", "md_docs_2guides_2core_2best__practices.html#autotoc_md311", null ],
        [ "✅ DO: Reuse Connections When Possible", "md_docs_2guides_2core_2best__practices.html#autotoc_md312", null ]
      ] ],
      [ "Thread Safety", "md_docs_2guides_2core_2best__practices.html#autotoc_md314", [
        [ "✅ DO: Protect Shared State", "md_docs_2guides_2core_2best__practices.html#autotoc_md315", null ],
        [ "✅ DO: Use ThreadSafeState for Complex States", "md_docs_2guides_2core_2best__practices.html#autotoc_md316", null ],
        [ "❌ DON'T: Block in Callbacks", "md_docs_2guides_2core_2best__practices.html#autotoc_md317", null ]
      ] ],
      [ "Performance Optimization", "md_docs_2guides_2core_2best__practices.html#autotoc_md319", [
        [ "✅ DO: Use Move Semantics", "md_docs_2guides_2core_2best__practices.html#autotoc_md320", null ],
        [ "✅ DO: Enable Async Logging", "md_docs_2guides_2core_2best__practices.html#autotoc_md321", null ],
        [ "✅ DO: Use Shared IO Context (Default)", "md_docs_2guides_2core_2best__practices.html#autotoc_md322", null ],
        [ "✅ DO: Batch Small Messages", "md_docs_2guides_2core_2best__practices.html#autotoc_md323", null ]
      ] ],
      [ "Code Organization", "md_docs_2guides_2core_2best__practices.html#autotoc_md325", [
        [ "✅ DO: Use Classes for Complex Logic", "md_docs_2guides_2core_2best__practices.html#autotoc_md326", null ],
        [ "✅ DO: Separate Concerns", "md_docs_2guides_2core_2best__practices.html#autotoc_md327", null ]
      ] ],
      [ "Testing", "md_docs_2guides_2core_2best__practices.html#autotoc_md329", [
        [ "✅ DO: Use Dependency Injection", "md_docs_2guides_2core_2best__practices.html#autotoc_md330", null ],
        [ "✅ DO: Test Error Scenarios", "md_docs_2guides_2core_2best__practices.html#autotoc_md331", null ],
        [ "✅ DO: Use Independent Context for Tests", "md_docs_2guides_2core_2best__practices.html#autotoc_md332", null ]
      ] ],
      [ "Security", "md_docs_2guides_2core_2best__practices.html#autotoc_md334", [
        [ "✅ DO: Validate All Input", "md_docs_2guides_2core_2best__practices.html#autotoc_md335", null ],
        [ "✅ DO: Implement Rate Limiting", "md_docs_2guides_2core_2best__practices.html#autotoc_md336", null ],
        [ "✅ DO: Set Connection Limits", "md_docs_2guides_2core_2best__practices.html#autotoc_md337", null ]
      ] ],
      [ "Logging and Debugging", "md_docs_2guides_2core_2best__practices.html#autotoc_md339", [
        [ "✅ DO: Use Appropriate Log Levels", "md_docs_2guides_2core_2best__practices.html#autotoc_md340", null ],
        [ "✅ DO: Enable Debug Logging During Development", "md_docs_2guides_2core_2best__practices.html#autotoc_md341", null ],
        [ "✅ DO: Log Context Information", "md_docs_2guides_2core_2best__practices.html#autotoc_md342", null ]
      ] ]
    ] ],
    [ "Python Bindings Guide", "md_docs_2guides_2core_2python__bindings.html", [
      [ "🚀 Getting Started", "md_docs_2guides_2core_2python__bindings.html#autotoc_md345", [
        [ "Installation", "md_docs_2guides_2core_2python__bindings.html#autotoc_md346", null ]
      ] ],
      [ "🔌 TCP Client", "md_docs_2guides_2core_2python__bindings.html#autotoc_md348", [
        [ "Basic Usage", "md_docs_2guides_2core_2python__bindings.html#autotoc_md349", null ]
      ] ],
      [ "🖥️ TCP Server", "md_docs_2guides_2core_2python__bindings.html#autotoc_md351", [
        [ "Basic Usage", "md_docs_2guides_2core_2python__bindings.html#autotoc_md352", null ]
      ] ],
      [ "📟 Serial Communication", "md_docs_2guides_2core_2python__bindings.html#autotoc_md354", [
        [ "Basic Usage", "md_docs_2guides_2core_2python__bindings.html#autotoc_md355", null ]
      ] ],
      [ "🌐 UDP Communication", "md_docs_2guides_2core_2python__bindings.html#autotoc_md357", [
        [ "Basic Usage", "md_docs_2guides_2core_2python__bindings.html#autotoc_md358", null ]
      ] ],
      [ "📂 UDS Communication", "md_docs_2guides_2core_2python__bindings.html#autotoc_md360", [
        [ "Basic Usage", "md_docs_2guides_2core_2python__bindings.html#autotoc_md361", null ]
      ] ],
      [ "🛠️ Advanced Features", "md_docs_2guides_2core_2python__bindings.html#autotoc_md363", [
        [ "Message Framing (Line/Packet)", "md_docs_2guides_2core_2python__bindings.html#autotoc_md364", null ],
        [ "Threading & GIL", "md_docs_2guides_2core_2python__bindings.html#autotoc_md365", null ],
        [ "Lifecycle Management", "md_docs_2guides_2core_2python__bindings.html#autotoc_md366", null ],
        [ "Configuration", "md_docs_2guides_2core_2python__bindings.html#autotoc_md367", null ]
      ] ]
    ] ],
    [ "Unilink Quick Start Guide", "md_docs_2guides_2core_2quickstart.html", [
      [ "Installation", "md_docs_2guides_2core_2quickstart.html#autotoc_md369", [
        [ "Prerequisites", "md_docs_2guides_2core_2quickstart.html#autotoc_md370", null ],
        [ "Build & Install", "md_docs_2guides_2core_2quickstart.html#autotoc_md371", null ]
      ] ],
      [ "Your First TCP Client (30 seconds)", "md_docs_2guides_2core_2quickstart.html#autotoc_md373", null ],
      [ "Your First TCP Server (30 seconds)", "md_docs_2guides_2core_2quickstart.html#autotoc_md375", null ],
      [ "Your First Serial Device (30 seconds)", "md_docs_2guides_2core_2quickstart.html#autotoc_md377", null ],
      [ "Common Patterns", "md_docs_2guides_2core_2quickstart.html#autotoc_md379", [
        [ "Pattern 1: Auto-Reconnection", "md_docs_2guides_2core_2quickstart.html#autotoc_md380", null ],
        [ "Pattern 2: Error Handling", "md_docs_2guides_2core_2quickstart.html#autotoc_md381", null ],
        [ "Pattern 3: Single vs Multi-Client Server (optional)", "md_docs_2guides_2core_2quickstart.html#autotoc_md382", null ]
      ] ],
      [ "Next Steps", "md_docs_2guides_2core_2quickstart.html#autotoc_md384", null ],
      [ "Troubleshooting", "md_docs_2guides_2core_2quickstart.html#autotoc_md386", [
        [ "Can't connect to server?", "md_docs_2guides_2core_2quickstart.html#autotoc_md387", null ],
        [ "Port already in use?", "md_docs_2guides_2core_2quickstart.html#autotoc_md388", null ],
        [ "Need independent IO thread?", "md_docs_2guides_2core_2quickstart.html#autotoc_md389", null ]
      ] ],
      [ "Support", "md_docs_2guides_2core_2quickstart.html#autotoc_md391", null ]
    ] ],
    [ "Testing Guide", "md_docs_2guides_2core_2testing.html", [
      [ "Table of Contents", "md_docs_2guides_2core_2testing.html#autotoc_md394", null ],
      [ "Quick Start", "md_docs_2guides_2core_2testing.html#autotoc_md396", [
        [ "Build and Run All Tests", "md_docs_2guides_2core_2testing.html#autotoc_md397", null ],
        [ "Windows Build & Test Workflow", "md_docs_2guides_2core_2testing.html#autotoc_md399", null ]
      ] ],
      [ "Running Tests", "md_docs_2guides_2core_2testing.html#autotoc_md401", [
        [ "Run All Tests", "md_docs_2guides_2core_2testing.html#autotoc_md402", null ],
        [ "Run Specific Test Categories", "md_docs_2guides_2core_2testing.html#autotoc_md404", null ],
        [ "Run Tests with Verbose Output", "md_docs_2guides_2core_2testing.html#autotoc_md406", null ],
        [ "Run Tests in Parallel", "md_docs_2guides_2core_2testing.html#autotoc_md408", null ]
      ] ],
      [ "UDP-specific test policies", "md_docs_2guides_2core_2testing.html#autotoc_md410", null ],
      [ "Test Categories", "md_docs_2guides_2core_2testing.html#autotoc_md411", [
        [ "Core Tests", "md_docs_2guides_2core_2testing.html#autotoc_md412", null ],
        [ "Integration Tests", "md_docs_2guides_2core_2testing.html#autotoc_md414", null ],
        [ "Memory Safety Tests", "md_docs_2guides_2core_2testing.html#autotoc_md416", null ],
        [ "Concurrency Safety Tests", "md_docs_2guides_2core_2testing.html#autotoc_md418", null ],
        [ "Performance Tests", "md_docs_2guides_2core_2testing.html#autotoc_md421", null ],
        [ "Stress Tests", "md_docs_2guides_2core_2testing.html#autotoc_md423", null ]
      ] ],
      [ "Memory Safety Validation", "md_docs_2guides_2core_2testing.html#autotoc_md425", [
        [ "Built-in Memory Tracking", "md_docs_2guides_2core_2testing.html#autotoc_md426", null ],
        [ "AddressSanitizer (ASan)", "md_docs_2guides_2core_2testing.html#autotoc_md428", null ],
        [ "ThreadSanitizer (TSan)", "md_docs_2guides_2core_2testing.html#autotoc_md430", null ],
        [ "Valgrind", "md_docs_2guides_2core_2testing.html#autotoc_md432", null ]
      ] ],
      [ "Continuous Integration", "md_docs_2guides_2core_2testing.html#autotoc_md434", [
        [ "GitHub Actions Integration", "md_docs_2guides_2core_2testing.html#autotoc_md435", null ],
        [ "CI/CD Build Matrix", "md_docs_2guides_2core_2testing.html#autotoc_md437", null ],
        [ "Ubuntu 20.04 Support", "md_docs_2guides_2core_2testing.html#autotoc_md439", null ],
        [ "View CI/CD Results", "md_docs_2guides_2core_2testing.html#autotoc_md441", null ]
      ] ],
      [ "Writing Custom Tests", "md_docs_2guides_2core_2testing.html#autotoc_md443", [
        [ "Test Structure", "md_docs_2guides_2core_2testing.html#autotoc_md444", null ],
        [ "Example: Custom Integration Test", "md_docs_2guides_2core_2testing.html#autotoc_md446", null ],
        [ "Running Custom Tests", "md_docs_2guides_2core_2testing.html#autotoc_md448", null ]
      ] ],
      [ "Test Configuration", "md_docs_2guides_2core_2testing.html#autotoc_md450", [
        [ "CTest Configuration", "md_docs_2guides_2core_2testing.html#autotoc_md451", null ],
        [ "Environment Variables", "md_docs_2guides_2core_2testing.html#autotoc_md453", null ]
      ] ],
      [ "Troubleshooting Tests", "md_docs_2guides_2core_2testing.html#autotoc_md455", [
        [ "Test Failures", "md_docs_2guides_2core_2testing.html#autotoc_md456", null ],
        [ "Port Conflicts", "md_docs_2guides_2core_2testing.html#autotoc_md458", null ],
        [ "Memory Issues", "md_docs_2guides_2core_2testing.html#autotoc_md460", null ]
      ] ],
      [ "Performance Regression Testing", "md_docs_2guides_2core_2testing.html#autotoc_md462", [
        [ "Benchmark Baseline", "md_docs_2guides_2core_2testing.html#autotoc_md463", null ],
        [ "Compare Against Baseline", "md_docs_2guides_2core_2testing.html#autotoc_md464", null ],
        [ "Automated Regression Detection", "md_docs_2guides_2core_2testing.html#autotoc_md465", null ]
      ] ],
      [ "Code Coverage", "md_docs_2guides_2core_2testing.html#autotoc_md467", [
        [ "Generate Coverage Report", "md_docs_2guides_2core_2testing.html#autotoc_md468", null ],
        [ "View HTML Coverage Report", "md_docs_2guides_2core_2testing.html#autotoc_md469", null ]
      ] ],
      [ "Next Steps", "md_docs_2guides_2core_2testing.html#autotoc_md471", null ]
    ] ],
    [ "Troubleshooting Guide", "md_docs_2guides_2core_2troubleshooting.html", [
      [ "Table of Contents", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md474", null ],
      [ "Connection Issues", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md476", [
        [ "Problem: Connection Refused", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md477", [
          [ "1. Server Not Running", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md478", null ],
          [ "2. Wrong Host/Port", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md479", null ],
          [ "3. Firewall Blocking", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md480", null ]
        ] ],
        [ "Problem: Connection Timeout", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md482", [
          [ "1. Network Unreachable", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md483", null ],
          [ "2. Server Overloaded", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md484", null ],
          [ "3. Slow Network", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md485", null ]
        ] ],
        [ "Problem: Connection Drops Randomly", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md487", [
          [ "1. Network Instability", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md488", null ],
          [ "2. Server Closing Connection", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md489", null ],
          [ "3. Keep-Alive Not Set", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md490", null ]
        ] ],
        [ "Problem: Port Already in Use", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md492", [
          [ "1. Kill Existing Process", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md493", null ],
          [ "2. Use Different Port", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md494", null ],
          [ "3. Enable Port Retry", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md495", null ],
          [ "4. Set SO_REUSEADDR (Advanced)", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md496", null ]
        ] ]
      ] ],
      [ "Compilation Errors", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md498", [
        [ "Problem: unilink/unilink.hpp Not Found", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md499", [
          [ "1. Install unilink", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md500", null ],
          [ "2. Add Include Path", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md501", null ],
          [ "3. Use as Subdirectory", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md502", null ]
        ] ],
        [ "Problem: Undefined Reference to unilink Symbols", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md504", [
          [ "1. Link unilink Library", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md505", null ],
          [ "2. Check Library Path", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md506", null ]
        ] ],
        [ "Problem: Boost Not Found", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md508", [
          [ "Ubuntu/Debian", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md509", null ],
          [ "macOS", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md510", null ],
          [ "Windows (vcpkg)", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md511", null ],
          [ "Manual Boost Path", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md512", null ]
        ] ]
      ] ],
      [ "Runtime Errors", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md514", [
        [ "Problem: Segmentation Fault", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md515", [
          [ "1. Enable Core Dumps", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md516", null ],
          [ "2. Use AddressSanitizer", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md517", null ],
          [ "3. Common Causes", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md518", null ]
        ] ],
        [ "Problem: Callbacks Not Being Called", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md520", [
          [ "1. Callback Not Registered", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md521", null ],
          [ "2. Client Not Started", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md522", null ],
          [ "3. Application Exits Too Quickly", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md523", null ]
        ] ]
      ] ],
      [ "Performance Issues", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md525", [
        [ "Problem: High CPU Usage", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md526", [
          [ "1. Busy Loop in Callback", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md527", null ],
          [ "2. Too Many Retries", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md528", null ],
          [ "3. Excessive Logging", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md529", null ]
        ] ],
        [ "Problem: High Memory Usage", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md531", [
          [ "1. Enable Memory Tracking (Debug)", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md532", null ],
          [ "2. Fix Memory Leaks", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md533", null ],
          [ "3. Limit Buffer Sizes", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md534", null ]
        ] ],
        [ "Problem: Slow Data Transfer", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md536", [
          [ "1. Batch Small Messages", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md537", null ],
          [ "2. Use Binary Protocol", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md538", null ],
          [ "3. Enable Async Logging", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md539", null ]
        ] ]
      ] ],
      [ "Memory Issues", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md541", [
        [ "Problem: Memory Leak Detected", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md542", null ]
      ] ],
      [ "Thread Safety Issues", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md544", [
        [ "Problem: Race Condition / Data Corruption", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md545", [
          [ "1. Protect Shared State", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md546", null ],
          [ "2. Use Thread-Safe Containers", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md547", null ]
        ] ]
      ] ],
      [ "Debugging Tips", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md549", [
        [ "Enable Debug Logging", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md550", null ],
        [ "Use GDB for Debugging", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md551", null ],
        [ "Network Debugging with tcpdump", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md552", null ],
        [ "Test with netcat", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md553", null ]
      ] ],
      [ "Getting Help", "md_docs_2guides_2core_2troubleshooting.html#autotoc_md555", null ]
    ] ],
    [ "Build Guide", "md_docs_2guides_2setup_2build__guide.html", [
      [ "</blockquote>", "md_docs_2guides_2setup_2build__guide.html#autotoc_md558", null ],
      [ "Table of Contents", "md_docs_2guides_2setup_2build__guide.html#autotoc_md559", null ],
      [ "Quick Build", "md_docs_2guides_2setup_2build__guide.html#autotoc_md561", [
        [ "Basic Build (Recommended)", "md_docs_2guides_2setup_2build__guide.html#autotoc_md562", null ]
      ] ],
      [ "Important Build Notes", "md_docs_2guides_2setup_2build__guide.html#autotoc_md564", null ],
      [ "Build Configurations", "md_docs_2guides_2setup_2build__guide.html#autotoc_md566", [
        [ "Minimal Build (without Configuration Management API)", "md_docs_2guides_2setup_2build__guide.html#autotoc_md567", null ],
        [ "Full Build (includes Configuration Management API)", "md_docs_2guides_2setup_2build__guide.html#autotoc_md569", null ]
      ] ],
      [ "Build Options Reference", "md_docs_2guides_2setup_2build__guide.html#autotoc_md571", [
        [ "Core Options", "md_docs_2guides_2setup_2build__guide.html#autotoc_md572", null ],
        [ "Development Options", "md_docs_2guides_2setup_2build__guide.html#autotoc_md573", null ],
        [ "Installation Options", "md_docs_2guides_2setup_2build__guide.html#autotoc_md574", null ]
      ] ],
      [ "Build Types Comparison", "md_docs_2guides_2setup_2build__guide.html#autotoc_md576", [
        [ "Release Build (Default)", "md_docs_2guides_2setup_2build__guide.html#autotoc_md577", null ],
        [ "Debug Build", "md_docs_2guides_2setup_2build__guide.html#autotoc_md579", null ],
        [ "RelWithDebInfo Build", "md_docs_2guides_2setup_2build__guide.html#autotoc_md581", null ]
      ] ],
      [ "Advanced Build Examples", "md_docs_2guides_2setup_2build__guide.html#autotoc_md583", [
        [ "Example 1: Minimal Production Build", "md_docs_2guides_2setup_2build__guide.html#autotoc_md584", null ],
        [ "Example 2: Development Build with Examples", "md_docs_2guides_2setup_2build__guide.html#autotoc_md586", null ],
        [ "Example 3: Testing with Sanitizers", "md_docs_2guides_2setup_2build__guide.html#autotoc_md588", null ],
        [ "Example 4: Build with Custom Boost Location", "md_docs_2guides_2setup_2build__guide.html#autotoc_md590", null ],
        [ "Example 5: Build with Specific Compiler", "md_docs_2guides_2setup_2build__guide.html#autotoc_md592", null ]
      ] ],
      [ "Platform-Specific Builds", "md_docs_2guides_2setup_2build__guide.html#autotoc_md594", [
        [ "Ubuntu 22.04 (Recommended)", "md_docs_2guides_2setup_2build__guide.html#autotoc_md595", null ],
        [ "Ubuntu 20.04 Build", "md_docs_2guides_2setup_2build__guide.html#autotoc_md597", [
          [ "Prerequisites", "md_docs_2guides_2setup_2build__guide.html#autotoc_md598", null ],
          [ "Build Steps", "md_docs_2guides_2setup_2build__guide.html#autotoc_md599", null ],
          [ "Notes", "md_docs_2guides_2setup_2build__guide.html#autotoc_md600", null ]
        ] ],
        [ "Debian 11+", "md_docs_2guides_2setup_2build__guide.html#autotoc_md602", null ],
        [ "Fedora 35+", "md_docs_2guides_2setup_2build__guide.html#autotoc_md604", null ],
        [ "Arch Linux", "md_docs_2guides_2setup_2build__guide.html#autotoc_md606", null ]
      ] ],
      [ "Build Performance Tips", "md_docs_2guides_2setup_2build__guide.html#autotoc_md608", [
        [ "Parallel Builds", "md_docs_2guides_2setup_2build__guide.html#autotoc_md609", null ],
        [ "Ccache for Faster Rebuilds", "md_docs_2guides_2setup_2build__guide.html#autotoc_md610", null ],
        [ "Ninja Build System (Faster than Make)", "md_docs_2guides_2setup_2build__guide.html#autotoc_md611", null ]
      ] ],
      [ "Installation", "md_docs_2guides_2setup_2build__guide.html#autotoc_md613", [
        [ "System-Wide Installation", "md_docs_2guides_2setup_2build__guide.html#autotoc_md614", null ],
        [ "Custom Installation Directory", "md_docs_2guides_2setup_2build__guide.html#autotoc_md615", null ],
        [ "Uninstall", "md_docs_2guides_2setup_2build__guide.html#autotoc_md616", null ]
      ] ],
      [ "Verifying the Build", "md_docs_2guides_2setup_2build__guide.html#autotoc_md618", [
        [ "Run Unit Tests", "md_docs_2guides_2setup_2build__guide.html#autotoc_md619", null ],
        [ "Run Examples", "md_docs_2guides_2setup_2build__guide.html#autotoc_md620", null ],
        [ "Check Library Symbols", "md_docs_2guides_2setup_2build__guide.html#autotoc_md621", null ]
      ] ],
      [ "Troubleshooting", "md_docs_2guides_2setup_2build__guide.html#autotoc_md623", [
        [ "Problem: CMake Can't Find Boost", "md_docs_2guides_2setup_2build__guide.html#autotoc_md624", null ],
        [ "Problem: Compiler Not Found", "md_docs_2guides_2setup_2build__guide.html#autotoc_md625", null ],
        [ "Problem: Out of Memory During Build", "md_docs_2guides_2setup_2build__guide.html#autotoc_md626", null ],
        [ "Problem: Permission Denied During Install", "md_docs_2guides_2setup_2build__guide.html#autotoc_md627", null ]
      ] ],
      [ "CMake Package Integration", "md_docs_2guides_2setup_2build__guide.html#autotoc_md629", [
        [ "Using the Installed Package", "md_docs_2guides_2setup_2build__guide.html#autotoc_md630", null ],
        [ "Custom Installation Prefix", "md_docs_2guides_2setup_2build__guide.html#autotoc_md631", null ],
        [ "Package Components", "md_docs_2guides_2setup_2build__guide.html#autotoc_md632", null ],
        [ "Verification", "md_docs_2guides_2setup_2build__guide.html#autotoc_md633", null ]
      ] ],
      [ "Next Steps", "md_docs_2guides_2setup_2build__guide.html#autotoc_md635", null ]
    ] ],
    [ "Installation Guide", "md_docs_2guides_2setup_2installation.html", [
      [ "Prerequisites", "md_docs_2guides_2setup_2installation.html#autotoc_md637", null ],
      [ "Installation Methods", "md_docs_2guides_2setup_2installation.html#autotoc_md638", [
        [ "Method 1: vcpkg (Recommended)", "md_docs_2guides_2setup_2installation.html#autotoc_md639", [
          [ "Step 1: Install via vcpkg", "md_docs_2guides_2setup_2installation.html#autotoc_md640", null ],
          [ "Step 2: Use in your project", "md_docs_2guides_2setup_2installation.html#autotoc_md641", null ]
        ] ],
        [ "Method 2: Install from Source (CMake Package)", "md_docs_2guides_2setup_2installation.html#autotoc_md642", [
          [ "Step 1: Build and install", "md_docs_2guides_2setup_2installation.html#autotoc_md643", null ],
          [ "Step 2: Use in your project", "md_docs_2guides_2setup_2installation.html#autotoc_md644", null ]
        ] ],
        [ "Method 3: Release Packages", "md_docs_2guides_2setup_2installation.html#autotoc_md645", [
          [ "Step 1: Download and extract", "md_docs_2guides_2setup_2installation.html#autotoc_md646", null ],
          [ "Step 2: Install", "md_docs_2guides_2setup_2installation.html#autotoc_md647", null ],
          [ "Step 3: Use in your project", "md_docs_2guides_2setup_2installation.html#autotoc_md648", null ]
        ] ],
        [ "Method 4: Git Submodule Integration", "md_docs_2guides_2setup_2installation.html#autotoc_md649", [
          [ "Step 1: Add submodule", "md_docs_2guides_2setup_2installation.html#autotoc_md650", null ],
          [ "Step 2: Use in CMake", "md_docs_2guides_2setup_2installation.html#autotoc_md651", null ]
        ] ]
      ] ],
      [ "Packaging Notes", "md_docs_2guides_2setup_2installation.html#autotoc_md652", null ],
      [ "Build Options (Source Builds)", "md_docs_2guides_2setup_2installation.html#autotoc_md653", null ],
      [ "Next Steps", "md_docs_2guides_2setup_2installation.html#autotoc_md654", null ]
    ] ],
    [ "System Requirements", "md_docs_2guides_2setup_2requirements.html", [
      [ "System Requirements", "md_docs_2guides_2setup_2requirements.html#autotoc_md657", [
        [ "Recommended Platform", "md_docs_2guides_2setup_2requirements.html#autotoc_md658", null ],
        [ "Supported Platforms", "md_docs_2guides_2setup_2requirements.html#autotoc_md659", null ]
      ] ],
      [ "Dependencies", "md_docs_2guides_2setup_2requirements.html#autotoc_md661", [
        [ "Core Library Dependencies", "md_docs_2guides_2setup_2requirements.html#autotoc_md662", null ],
        [ "Dependency Details", "md_docs_2guides_2setup_2requirements.html#autotoc_md663", null ]
      ] ],
      [ "Optional Dependencies", "md_docs_2guides_2setup_2requirements.html#autotoc_md665", [
        [ "For Documentation Generation", "md_docs_2guides_2setup_2requirements.html#autotoc_md666", null ],
        [ "For Development", "md_docs_2guides_2setup_2requirements.html#autotoc_md667", null ]
      ] ],
      [ "Compiler Requirements", "md_docs_2guides_2setup_2requirements.html#autotoc_md669", [
        [ "Minimum Compiler Versions", "md_docs_2guides_2setup_2requirements.html#autotoc_md670", null ],
        [ "C++ Standard", "md_docs_2guides_2setup_2requirements.html#autotoc_md671", null ],
        [ "Compiler Features Required", "md_docs_2guides_2setup_2requirements.html#autotoc_md672", null ]
      ] ],
      [ "Build Environment", "md_docs_2guides_2setup_2requirements.html#autotoc_md674", [
        [ "Disk Space", "md_docs_2guides_2setup_2requirements.html#autotoc_md675", null ],
        [ "Memory Requirements", "md_docs_2guides_2setup_2requirements.html#autotoc_md676", null ],
        [ "CPU", "md_docs_2guides_2setup_2requirements.html#autotoc_md677", null ]
      ] ],
      [ "Runtime Requirements", "md_docs_2guides_2setup_2requirements.html#autotoc_md679", [
        [ "For Applications Using unilink", "md_docs_2guides_2setup_2requirements.html#autotoc_md680", null ],
        [ "Thread Support", "md_docs_2guides_2setup_2requirements.html#autotoc_md681", null ]
      ] ],
      [ "Platform-Specific Notes", "md_docs_2guides_2setup_2requirements.html#autotoc_md683", [
        [ "Ubuntu 22.04 LTS", "md_docs_2guides_2setup_2requirements.html#autotoc_md684", null ],
        [ "Ubuntu 20.04 LTS", "md_docs_2guides_2setup_2requirements.html#autotoc_md685", null ],
        [ "Other Linux Distributions", "md_docs_2guides_2setup_2requirements.html#autotoc_md686", null ]
      ] ],
      [ "Verifying Your Environment", "md_docs_2guides_2setup_2requirements.html#autotoc_md688", [
        [ "Check Compiler Version", "md_docs_2guides_2setup_2requirements.html#autotoc_md689", null ],
        [ "Check CMake Version", "md_docs_2guides_2setup_2requirements.html#autotoc_md690", null ],
        [ "Check Boost Version", "md_docs_2guides_2setup_2requirements.html#autotoc_md691", null ],
        [ "Quick Environment Test", "md_docs_2guides_2setup_2requirements.html#autotoc_md692", null ]
      ] ],
      [ "Troubleshooting", "md_docs_2guides_2setup_2requirements.html#autotoc_md694", [
        [ "Problem: Compiler Too Old", "md_docs_2guides_2setup_2requirements.html#autotoc_md695", null ],
        [ "Problem: Boost Not Found", "md_docs_2guides_2setup_2requirements.html#autotoc_md696", null ],
        [ "Problem: CMake Too Old", "md_docs_2guides_2setup_2requirements.html#autotoc_md697", null ]
      ] ],
      [ "Next Steps", "md_docs_2guides_2setup_2requirements.html#autotoc_md699", null ]
    ] ],
    [ "Implementation Status", "md_docs_2implementation__status.html", [
      [ "Scope", "md_docs_2implementation__status.html#autotoc_md701", null ],
      [ "C++ API Surface", "md_docs_2implementation__status.html#autotoc_md702", null ],
      [ "Python Binding Scope", "md_docs_2implementation__status.html#autotoc_md703", null ],
      [ "Build And Test Status", "md_docs_2implementation__status.html#autotoc_md704", null ],
      [ "Recommended Reading Order", "md_docs_2implementation__status.html#autotoc_md705", null ]
    ] ],
    [ "Unilink Documentation Index", "md_docs_2index.html", [
      [ "Start Here", "md_docs_2index.html#autotoc_md707", null ],
      [ "Core Guides", "md_docs_2index.html#autotoc_md708", null ],
      [ "Tutorials", "md_docs_2index.html#autotoc_md709", null ],
      [ "Architecture Notes", "md_docs_2index.html#autotoc_md710", null ],
      [ "Examples and Tests", "md_docs_2index.html#autotoc_md711", null ],
      [ "Notes For Maintainers", "md_docs_2index.html#autotoc_md712", null ]
    ] ],
    [ "Mermaid Test Page", "md_docs_2mermaid__test.html", [
      [ "Flowchart Test", "md_docs_2mermaid__test.html#autotoc_md714", null ],
      [ "Sequence Diagram Test", "md_docs_2mermaid__test.html#autotoc_md715", null ]
    ] ],
    [ "Unilink API Guide", "md_docs_2reference_2api__guide.html", [
      [ "Table of Contents", "md_docs_2reference_2api__guide.html#autotoc_md726", null ],
      [ "Builder API", "md_docs_2reference_2api__guide.html#autotoc_md728", [
        [ "Core Concept", "md_docs_2reference_2api__guide.html#autotoc_md729", null ],
        [ "Common Methods (All Builders)", "md_docs_2reference_2api__guide.html#autotoc_md730", null ],
        [ "Framed Message Handling", "md_docs_2reference_2api__guide.html#autotoc_md731", null ],
        [ "IO Context Ownership (advanced)", "md_docs_2reference_2api__guide.html#autotoc_md732", null ]
      ] ],
      [ "TCP Client", "md_docs_2reference_2api__guide.html#autotoc_md734", [
        [ "Basic Usage", "md_docs_2reference_2api__guide.html#autotoc_md735", null ],
        [ "API Reference", "md_docs_2reference_2api__guide.html#autotoc_md736", [
          [ "Constructor", "md_docs_2reference_2api__guide.html#autotoc_md737", null ],
          [ "Builder Methods", "md_docs_2reference_2api__guide.html#autotoc_md738", null ],
          [ "Instance Methods", "md_docs_2reference_2api__guide.html#autotoc_md739", null ]
        ] ],
        [ "Advanced Examples", "md_docs_2reference_2api__guide.html#autotoc_md740", [
          [ "With Member Functions", "md_docs_2reference_2api__guide.html#autotoc_md741", null ],
          [ "With Lambda Capture", "md_docs_2reference_2api__guide.html#autotoc_md742", null ],
          [ "Custom Reconnect Policy (Transport Layer)", "md_docs_2reference_2api__guide.html#autotoc_md743", null ]
        ] ]
      ] ],
      [ "TCP Server", "md_docs_2reference_2api__guide.html#autotoc_md745", [
        [ "Basic Usage", "md_docs_2reference_2api__guide.html#autotoc_md746", null ],
        [ "API Reference", "md_docs_2reference_2api__guide.html#autotoc_md747", [
          [ "Constructor", "md_docs_2reference_2api__guide.html#autotoc_md748", null ],
          [ "Builder Methods", "md_docs_2reference_2api__guide.html#autotoc_md749", null ],
          [ "Instance Methods", "md_docs_2reference_2api__guide.html#autotoc_md750", null ]
        ] ],
        [ "Advanced Examples", "md_docs_2reference_2api__guide.html#autotoc_md751", [
          [ "Single Client Mode", "md_docs_2reference_2api__guide.html#autotoc_md752", null ],
          [ "Port Retry", "md_docs_2reference_2api__guide.html#autotoc_md753", null ],
          [ "Echo Server Pattern", "md_docs_2reference_2api__guide.html#autotoc_md754", null ]
        ] ]
      ] ],
      [ "Serial Communication", "md_docs_2reference_2api__guide.html#autotoc_md756", [
        [ "Basic Usage", "md_docs_2reference_2api__guide.html#autotoc_md757", null ],
        [ "API Reference", "md_docs_2reference_2api__guide.html#autotoc_md758", [
          [ "Constructor", "md_docs_2reference_2api__guide.html#autotoc_md759", null ],
          [ "Builder Methods", "md_docs_2reference_2api__guide.html#autotoc_md760", null ],
          [ "Instance Methods", "md_docs_2reference_2api__guide.html#autotoc_md761", null ]
        ] ],
        [ "Device Paths", "md_docs_2reference_2api__guide.html#autotoc_md762", null ],
        [ "Advanced Examples", "md_docs_2reference_2api__guide.html#autotoc_md763", [
          [ "Arduino Communication", "md_docs_2reference_2api__guide.html#autotoc_md764", null ],
          [ "GPS Module", "md_docs_2reference_2api__guide.html#autotoc_md765", null ]
        ] ]
      ] ],
      [ "UDP Communication", "md_docs_2reference_2api__guide.html#autotoc_md767", [
        [ "Basic Usage", "md_docs_2reference_2api__guide.html#autotoc_md768", [
          [ "UDP Receiver (Server)", "md_docs_2reference_2api__guide.html#autotoc_md769", null ],
          [ "UDP Sender (Client)", "md_docs_2reference_2api__guide.html#autotoc_md770", null ]
        ] ],
        [ "API Reference", "md_docs_2reference_2api__guide.html#autotoc_md771", [
          [ "Constructor", "md_docs_2reference_2api__guide.html#autotoc_md772", null ],
          [ "Builder Methods", "md_docs_2reference_2api__guide.html#autotoc_md773", null ],
          [ "Instance Methods", "md_docs_2reference_2api__guide.html#autotoc_md774", null ]
        ] ],
        [ "Advanced Examples", "md_docs_2reference_2api__guide.html#autotoc_md775", [
          [ "Echo Reply (Receiver)", "md_docs_2reference_2api__guide.html#autotoc_md776", null ]
        ] ]
      ] ],
      [ "UDS Communication", "md_docs_2reference_2api__guide.html#autotoc_md778", [
        [ "Basic Usage", "md_docs_2reference_2api__guide.html#autotoc_md779", [
          [ "UDS Server", "md_docs_2reference_2api__guide.html#autotoc_md780", null ],
          [ "UDS Client", "md_docs_2reference_2api__guide.html#autotoc_md781", null ]
        ] ],
        [ "API Reference", "md_docs_2reference_2api__guide.html#autotoc_md782", [
          [ "Constructors", "md_docs_2reference_2api__guide.html#autotoc_md783", null ],
          [ "Builder Methods (UDS Server)", "md_docs_2reference_2api__guide.html#autotoc_md784", null ],
          [ "Builder Methods (UDS Client)", "md_docs_2reference_2api__guide.html#autotoc_md785", null ],
          [ "Instance Methods (UDS Client)", "md_docs_2reference_2api__guide.html#autotoc_md786", null ],
          [ "Instance Methods (UDS Server)", "md_docs_2reference_2api__guide.html#autotoc_md787", null ]
        ] ],
        [ "Notes on UDS", "md_docs_2reference_2api__guide.html#autotoc_md788", null ]
      ] ],
      [ "Error Handling", "md_docs_2reference_2api__guide.html#autotoc_md790", [
        [ "Setup Error Handler", "md_docs_2reference_2api__guide.html#autotoc_md791", null ],
        [ "Error Levels", "md_docs_2reference_2api__guide.html#autotoc_md792", null ],
        [ "Error Statistics", "md_docs_2reference_2api__guide.html#autotoc_md793", null ],
        [ "Error Categories", "md_docs_2reference_2api__guide.html#autotoc_md794", null ]
      ] ],
      [ "Logging System", "md_docs_2reference_2api__guide.html#autotoc_md796", [
        [ "Basic Usage", "md_docs_2reference_2api__guide.html#autotoc_md797", null ],
        [ "Log Levels", "md_docs_2reference_2api__guide.html#autotoc_md798", null ],
        [ "Async Logging", "md_docs_2reference_2api__guide.html#autotoc_md799", null ],
        [ "Log Rotation", "md_docs_2reference_2api__guide.html#autotoc_md800", null ]
      ] ],
      [ "Configuration Management", "md_docs_2reference_2api__guide.html#autotoc_md802", [
        [ "Load Configuration from File", "md_docs_2reference_2api__guide.html#autotoc_md803", null ],
        [ "Configuration File Format", "md_docs_2reference_2api__guide.html#autotoc_md804", null ]
      ] ],
      [ "Advanced Features", "md_docs_2reference_2api__guide.html#autotoc_md806", [
        [ "Memory Pool", "md_docs_2reference_2api__guide.html#autotoc_md807", null ],
        [ "Safe Data Buffer", "md_docs_2reference_2api__guide.html#autotoc_md808", null ],
        [ "Thread-Safe State", "md_docs_2reference_2api__guide.html#autotoc_md809", null ]
      ] ],
      [ "Best Practices", "md_docs_2reference_2api__guide.html#autotoc_md811", [
        [ "1. Always Handle Errors", "md_docs_2reference_2api__guide.html#autotoc_md812", null ],
        [ "2. Use Explicit Lifecycle Control", "md_docs_2reference_2api__guide.html#autotoc_md813", null ],
        [ "3. Set Appropriate Retry Intervals", "md_docs_2reference_2api__guide.html#autotoc_md814", null ],
        [ "4. Enable Logging for Debugging", "md_docs_2reference_2api__guide.html#autotoc_md815", null ],
        [ "5. Use Member Functions for OOP Design", "md_docs_2reference_2api__guide.html#autotoc_md816", null ]
      ] ],
      [ "Performance Tips", "md_docs_2reference_2api__guide.html#autotoc_md818", [
        [ "1. Use Independent Context for Testing Only", "md_docs_2reference_2api__guide.html#autotoc_md819", null ],
        [ "2. Enable Async Logging", "md_docs_2reference_2api__guide.html#autotoc_md820", null ],
        [ "3. Use Memory Pool for Frequent Allocations", "md_docs_2reference_2api__guide.html#autotoc_md821", null ],
        [ "4. Disable Unnecessary Features", "md_docs_2reference_2api__guide.html#autotoc_md822", null ]
      ] ]
    ] ],
    [ "Test Structure", "md_docs_2test__structure.html", [
      [ "Layout", "md_docs_2test__structure.html#autotoc_md824", null ],
      [ "What Each Area Covers", "md_docs_2test__structure.html#autotoc_md825", null ],
      [ "Build-Time Controls", "md_docs_2test__structure.html#autotoc_md826", null ],
      [ "Running Tests", "md_docs_2test__structure.html#autotoc_md827", [
        [ "Run All Registered Tests", "md_docs_2test__structure.html#autotoc_md828", null ],
        [ "Run By Broad Category", "md_docs_2test__structure.html#autotoc_md829", null ],
        [ "Useful Focused Runs", "md_docs_2test__structure.html#autotoc_md830", null ],
        [ "Inspect What Is Currently Registered", "md_docs_2test__structure.html#autotoc_md831", null ]
      ] ],
      [ "Notes", "md_docs_2test__structure.html#autotoc_md832", null ],
      [ "CI/CD Integration", "md_docs_2test__structure.html#autotoc_md833", null ]
    ] ],
    [ "Getting Started with Unilink", "md_docs_2tutorials_201__getting__started.html", [
      [ "What You'll Build", "md_docs_2tutorials_201__getting__started.html#autotoc_md836", null ],
      [ "Step 1: Create The Client", "md_docs_2tutorials_201__getting__started.html#autotoc_md838", null ],
      [ "Step 2: Build With CMake", "md_docs_2tutorials_201__getting__started.html#autotoc_md840", null ],
      [ "Step 3: Run Against A Test Server", "md_docs_2tutorials_201__getting__started.html#autotoc_md842", null ],
      [ "What Changed In The Current API", "md_docs_2tutorials_201__getting__started.html#autotoc_md844", null ],
      [ "Use The Full Example If You Want More", "md_docs_2tutorials_201__getting__started.html#autotoc_md846", null ],
      [ "Next Steps", "md_docs_2tutorials_201__getting__started.html#autotoc_md848", null ]
    ] ],
    [ "Building a TCP Server", "md_docs_2tutorials_202__tcp__server.html", [
      [ "What You'll Build", "md_docs_2tutorials_202__tcp__server.html#autotoc_md852", null ],
      [ "Step 1: Create The Server", "md_docs_2tutorials_202__tcp__server.html#autotoc_md854", null ],
      [ "Step 2: Run It", "md_docs_2tutorials_202__tcp__server.html#autotoc_md856", null ],
      [ "Step 3: Understand The Current Server API", "md_docs_2tutorials_202__tcp__server.html#autotoc_md858", null ],
      [ "Client Limits", "md_docs_2tutorials_202__tcp__server.html#autotoc_md860", null ],
      [ "Use The Full Example Programs For More", "md_docs_2tutorials_202__tcp__server.html#autotoc_md862", null ],
      [ "Next Steps", "md_docs_2tutorials_202__tcp__server.html#autotoc_md864", null ]
    ] ],
    [ "UDS Communication", "md_docs_2tutorials_203__uds__communication.html", [
      [ "What You'll Build", "md_docs_2tutorials_203__uds__communication.html#autotoc_md868", null ],
      [ "Step 1: Create A UDS Server", "md_docs_2tutorials_203__uds__communication.html#autotoc_md870", null ],
      [ "Step 2: Create A UDS Client", "md_docs_2tutorials_203__uds__communication.html#autotoc_md872", null ],
      [ "Why Use UDS Instead Of TCP", "md_docs_2tutorials_203__uds__communication.html#autotoc_md874", null ],
      [ "Operational Notes", "md_docs_2tutorials_203__uds__communication.html#autotoc_md876", null ],
      [ "Next Steps", "md_docs_2tutorials_203__uds__communication.html#autotoc_md878", null ]
    ] ],
    [ "Serial Communication", "md_docs_2tutorials_204__serial__communication.html", [
      [ "What You'll Build", "md_docs_2tutorials_204__serial__communication.html#autotoc_md882", null ],
      [ "Step 1: Choose A Device Path", "md_docs_2tutorials_204__serial__communication.html#autotoc_md884", null ],
      [ "Step 2: Create A Minimal Serial Terminal", "md_docs_2tutorials_204__serial__communication.html#autotoc_md886", null ],
      [ "Step 3: Build And Run", "md_docs_2tutorials_204__serial__communication.html#autotoc_md888", null ],
      [ "Step 4: Test With A Second Terminal", "md_docs_2tutorials_204__serial__communication.html#autotoc_md890", null ],
      [ "Common Adjustments", "md_docs_2tutorials_204__serial__communication.html#autotoc_md892", null ],
      [ "When To Use The Example Programs Instead", "md_docs_2tutorials_204__serial__communication.html#autotoc_md894", null ],
      [ "Next Steps", "md_docs_2tutorials_204__serial__communication.html#autotoc_md896", null ]
    ] ],
    [ "UDP Communication", "md_docs_2tutorials_205__udp__communication.html", [
      [ "What You'll Build", "md_docs_2tutorials_205__udp__communication.html#autotoc_md900", null ],
      [ "Step 1: Create A Receiver", "md_docs_2tutorials_205__udp__communication.html#autotoc_md902", null ],
      [ "Step 2: Create A Sender", "md_docs_2tutorials_205__udp__communication.html#autotoc_md904", null ],
      [ "Step 3: Build The Two Programs", "md_docs_2tutorials_205__udp__communication.html#autotoc_md906", null ],
      [ "Step 4: Run Both Programs", "md_docs_2tutorials_205__udp__communication.html#autotoc_md908", null ],
      [ "What Is Different About UDP", "md_docs_2tutorials_205__udp__communication.html#autotoc_md910", null ],
      [ "Practical Notes", "md_docs_2tutorials_205__udp__communication.html#autotoc_md912", null ],
      [ "Use The Full Examples For Repeated Testing", "md_docs_2tutorials_205__udp__communication.html#autotoc_md914", null ],
      [ "Next Steps", "md_docs_2tutorials_205__udp__communication.html#autotoc_md916", null ]
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
        [ "Variables", "functions_vars.html", "functions_vars" ],
        [ "Typedefs", "functions_type.html", null ],
        [ "Enumerations", "functions_enum.html", null ],
        [ "Enumerator", "functions_eval.html", null ]
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
"classunilink_1_1builder_1_1UdsServerBuilder.html#a3913c8955226959a4787b071e84f3969",
"classunilink_1_1config_1_1ConfigManagerInterface.html#af77cd459050f281987bf8b08b7e8596f",
"classunilink_1_1interface_1_1Channel.html#a4b6549386c1be6c9768fbf882dac6b29",
"classunilink_1_1memory_1_1PooledBuffer.html#a66bb837b33bf0a93b494a73abf1dfd0a",
"classunilink_1_1transport_1_1Serial.html",
"classunilink_1_1transport_1_1UdsClient.html#a7149f26ed27adf289cf4ff1485fa5084",
"classunilink_1_1wrapper_1_1Serial.html#a2fbdec2d253d8a75114501fcccec5d06",
"classunilink_1_1wrapper_1_1UdpServer.html#a97ddb6517358a2e82a5c44a205fbba57",
"dir_e3d1c031bcdc60380978ce5471ce3438.html",
"line__framer_8cc_source.html",
"md_docs_2guides_2core_2quickstart.html#autotoc_md389",
"md_docs_2reference_2api__guide.html#autotoc_md736",
"namespaceunilink_1_1base_1_1constants.html#aa09a917cdd7f238dcf5f207bafd97a00",
"structunilink_1_1config_1_1SerialConfig.html#aa93fec717b98438e20efe4a6c7bf6b54",
"structunilink_1_1diagnostics_1_1LogEntry.html#a2179681d8f79db0705453d858814019b",
"structunilink_1_1transport_1_1Serial_1_1Impl.html#acdcdebe610cfcff07bad20b93515437f",
"structunilink_1_1transport_1_1UdpChannel_1_1Impl.html#abc6fdc541d5e28f498f3ca001749e6e0",
"structunilink_1_1wrapper_1_1Serial_1_1Impl.html#ac0d7c161fba59697abfc902e66b47628",
"structunilink_1_1wrapper_1_1UdpServer_1_1Impl.html#a6b1bd018ecf73e66fe2b96bff2b8abe9",
"uds__server__session_8hpp.html"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';