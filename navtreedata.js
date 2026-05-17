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
    [ "Unilink Documentation", "index.html", "index" ],
    [ "unilink Logo Assets", "md_docs_2assets_2logo_2README.html", [
      [ "Files", "md_docs_2assets_2logo_2README.html#autotoc_md1", null ],
      [ "Usage", "md_docs_2assets_2logo_2README.html#autotoc_md2", null ]
    ] ],
    [ "Contributor Guide", "contrib_index.html", [
      [ "Building the Library", "contrib_index.html#autotoc_md342", null ],
      [ "Architecture", "contrib_index.html#autotoc_md344", null ],
      [ "Quick Links (User Docs)", "contrib_index.html#autotoc_md346", null ],
      [ "Build Guide", "contrib_build.html", [
        [ "Table of Contents", "contrib_build.html#autotoc_md259", null ],
        [ "Quick Build", "contrib_build.html#autotoc_md261", [
          [ "Basic Build (Recommended)", "contrib_build.html#autotoc_md262", null ]
        ] ],
        [ "Important Build Notes", "contrib_build.html#autotoc_md264", null ],
        [ "Build Configurations", "contrib_build.html#autotoc_md266", [
          [ "Minimal Build (without Configuration Management API)", "contrib_build.html#autotoc_md267", null ],
          [ "Full Build (includes Configuration Management API)", "contrib_build.html#autotoc_md269", null ]
        ] ],
        [ "Build Options Reference", "contrib_build.html#autotoc_md271", [
          [ "Core Options", "contrib_build.html#autotoc_md272", null ],
          [ "Development Options", "contrib_build.html#autotoc_md273", null ],
          [ "Installation Options", "contrib_build.html#autotoc_md274", null ]
        ] ],
        [ "Build Types Comparison", "contrib_build.html#autotoc_md276", [
          [ "Release Build (Default)", "contrib_build.html#autotoc_md277", null ],
          [ "Debug Build", "contrib_build.html#autotoc_md279", null ],
          [ "RelWithDebInfo Build", "contrib_build.html#autotoc_md281", null ]
        ] ],
        [ "Advanced Build Examples", "contrib_build.html#autotoc_md283", [
          [ "Example 1: Minimal Production Build", "contrib_build.html#autotoc_md284", null ],
          [ "Example 2: Development Build", "contrib_build.html#autotoc_md286", null ],
          [ "Example 3: Testing with Sanitizers", "contrib_build.html#autotoc_md288", null ],
          [ "Example 4: Build with Custom Boost Location", "contrib_build.html#autotoc_md290", null ],
          [ "Example 5: Build with Specific Compiler", "contrib_build.html#autotoc_md292", null ]
        ] ],
        [ "Platform-Specific Builds", "contrib_build.html#autotoc_md294", [
          [ "Ubuntu 22.04 (Recommended)", "contrib_build.html#autotoc_md295", null ],
          [ "Ubuntu 20.04 Build", "contrib_build.html#autotoc_md297", [
            [ "Prerequisites", "contrib_build.html#autotoc_md298", null ],
            [ "Build Steps", "contrib_build.html#autotoc_md299", null ],
            [ "Notes", "contrib_build.html#autotoc_md300", null ]
          ] ],
          [ "Debian 11+", "contrib_build.html#autotoc_md302", null ],
          [ "Fedora 35+", "contrib_build.html#autotoc_md304", null ],
          [ "Arch Linux", "contrib_build.html#autotoc_md306", null ]
        ] ],
        [ "Build Performance Tips", "contrib_build.html#autotoc_md308", [
          [ "Parallel Builds", "contrib_build.html#autotoc_md309", null ],
          [ "Ccache for Faster Rebuilds", "contrib_build.html#autotoc_md310", null ],
          [ "Ninja Build System (Faster than Make)", "contrib_build.html#autotoc_md311", null ]
        ] ],
        [ "Installation", "contrib_build.html#autotoc_md313", [
          [ "System-Wide Installation", "contrib_build.html#autotoc_md314", null ],
          [ "Custom Installation Directory", "contrib_build.html#autotoc_md315", null ],
          [ "Uninstall", "contrib_build.html#autotoc_md316", null ]
        ] ],
        [ "Verifying the Build", "contrib_build.html#autotoc_md318", [
          [ "Run Unit Tests", "contrib_build.html#autotoc_md319", null ],
          [ "Run Focused Tests", "contrib_build.html#autotoc_md320", null ],
          [ "Check Library Symbols", "contrib_build.html#autotoc_md321", null ]
        ] ],
        [ "Troubleshooting", "contrib_build.html#autotoc_md323", [
          [ "Problem: CMake Can't Find Boost", "contrib_build.html#autotoc_md324", null ],
          [ "Problem: Compiler Not Found", "contrib_build.html#autotoc_md325", null ],
          [ "Problem: Out of Memory During Build", "contrib_build.html#autotoc_md326", null ],
          [ "Problem: Permission Denied During Install", "contrib_build.html#autotoc_md327", null ]
        ] ],
        [ "CMake Package Integration", "contrib_build.html#autotoc_md329", [
          [ "Using the Installed Package", "contrib_build.html#autotoc_md330", null ],
          [ "Custom Installation Prefix", "contrib_build.html#autotoc_md331", null ],
          [ "Package Components", "contrib_build.html#autotoc_md332", null ],
          [ "Verification", "contrib_build.html#autotoc_md333", null ]
        ] ],
        [ "Next Steps", "contrib_build.html#autotoc_md335", null ]
      ] ],
      [ "Testing Guide", "contrib_testing.html", [
        [ "Table of Contents", "contrib_testing.html#autotoc_md382", null ],
        [ "Quick Start", "contrib_testing.html#autotoc_md384", [
          [ "Build and Run All Tests", "contrib_testing.html#autotoc_md385", null ],
          [ "Windows Build & Test Workflow", "contrib_testing.html#autotoc_md387", null ]
        ] ],
        [ "Running Tests", "contrib_testing.html#autotoc_md389", [
          [ "Run All Tests", "contrib_testing.html#autotoc_md390", null ],
          [ "Run Specific Test Categories", "contrib_testing.html#autotoc_md392", null ],
          [ "Run Tests with Verbose Output", "contrib_testing.html#autotoc_md394", null ],
          [ "Run Tests in Parallel", "contrib_testing.html#autotoc_md396", null ]
        ] ],
        [ "UDP-specific test policies", "contrib_testing.html#autotoc_md398", null ],
        [ "Test Categories", "contrib_testing.html#autotoc_md399", [
          [ "Core Tests", "contrib_testing.html#autotoc_md400", null ],
          [ "Memory Safety Tests", "contrib_testing.html#autotoc_md402", null ],
          [ "Concurrency Safety Tests", "contrib_testing.html#autotoc_md404", null ],
          [ "Benchmarking", "contrib_testing.html#autotoc_md407", null ],
          [ "Stress Tests", "contrib_testing.html#autotoc_md409", null ]
        ] ],
        [ "Memory Safety Validation", "contrib_testing.html#autotoc_md411", [
          [ "Built-in Memory Tracking", "contrib_testing.html#autotoc_md412", null ],
          [ "AddressSanitizer (ASan)", "contrib_testing.html#autotoc_md414", null ],
          [ "ThreadSanitizer (TSan)", "contrib_testing.html#autotoc_md416", null ],
          [ "Valgrind", "contrib_testing.html#autotoc_md418", null ]
        ] ],
        [ "Continuous Integration", "contrib_testing.html#autotoc_md420", [
          [ "GitHub Actions Integration", "contrib_testing.html#autotoc_md421", null ],
          [ "CI/CD Build Matrix", "contrib_testing.html#autotoc_md423", null ],
          [ "Ubuntu 20.04 Support", "contrib_testing.html#autotoc_md425", null ],
          [ "View CI/CD Results", "contrib_testing.html#autotoc_md427", null ]
        ] ],
        [ "Writing Custom Tests", "contrib_testing.html#autotoc_md429", [
          [ "Test Structure", "contrib_testing.html#autotoc_md430", null ],
          [ "Example: Custom Integration Test", "contrib_testing.html#autotoc_md432", null ],
          [ "Running Custom Tests", "contrib_testing.html#autotoc_md434", null ]
        ] ],
        [ "Test Configuration", "contrib_testing.html#autotoc_md436", [
          [ "CTest Configuration", "contrib_testing.html#autotoc_md437", null ],
          [ "Environment Variables", "contrib_testing.html#autotoc_md439", null ]
        ] ],
        [ "Troubleshooting Tests", "contrib_testing.html#autotoc_md441", [
          [ "Test Failures", "contrib_testing.html#autotoc_md442", null ],
          [ "Port Conflicts", "contrib_testing.html#autotoc_md444", null ],
          [ "Memory Issues", "contrib_testing.html#autotoc_md446", null ]
        ] ],
        [ "Performance Regression Testing", "contrib_testing.html#autotoc_md448", null ],
        [ "Code Coverage", "contrib_testing.html#autotoc_md450", [
          [ "Generate Coverage Report", "contrib_testing.html#autotoc_md451", null ],
          [ "View HTML Coverage Report", "contrib_testing.html#autotoc_md452", null ]
        ] ],
        [ "Next Steps", "contrib_testing.html#autotoc_md454", null ]
      ] ],
      [ "Implementation Status", "contrib_impl_status.html", [
        [ "Scope", "contrib_impl_status.html#autotoc_md336", null ],
        [ "C++ API Surface", "contrib_impl_status.html#autotoc_md337", null ],
        [ "Python Binding Scope", "contrib_impl_status.html#autotoc_md338", null ],
        [ "Build And Test Status", "contrib_impl_status.html#autotoc_md339", null ],
        [ "Recommended Reading Order", "contrib_impl_status.html#autotoc_md340", null ]
      ] ],
      [ "Test Structure", "contrib_test_structure.html", [
        [ "Layout", "contrib_test_structure.html#autotoc_md371", null ],
        [ "What Each Area Covers", "contrib_test_structure.html#autotoc_md372", null ],
        [ "Build-Time Controls", "contrib_test_structure.html#autotoc_md373", null ],
        [ "Running Tests", "contrib_test_structure.html#autotoc_md374", [
          [ "Run All Registered Tests", "contrib_test_structure.html#autotoc_md375", null ],
          [ "Run By Broad Category", "contrib_test_structure.html#autotoc_md376", null ],
          [ "Useful Focused Runs", "contrib_test_structure.html#autotoc_md377", null ],
          [ "Inspect What Is Currently Registered", "contrib_test_structure.html#autotoc_md378", null ]
        ] ],
        [ "Notes", "contrib_test_structure.html#autotoc_md379", null ],
        [ "CI/CD Integration", "contrib_test_structure.html#autotoc_md380", null ]
      ] ],
      [ "Unilink System Architecture", "contrib_arch.html", [
        [ "Table of Contents", "contrib_arch.html#autotoc_md98", null ],
        [ "Overview", "contrib_arch.html#autotoc_md100", [
          [ "Design Goals", "contrib_arch.html#autotoc_md101", null ]
        ] ],
        [ "Layered Architecture", "contrib_arch.html#autotoc_md103", [
          [ "Layer Responsibilities", "contrib_arch.html#autotoc_md104", [
            [ "1. Builder API Layer", "contrib_arch.html#autotoc_md105", null ],
            [ "2. Wrapper API Layer", "contrib_arch.html#autotoc_md106", null ],
            [ "3. Transport Layer", "contrib_arch.html#autotoc_md107", null ],
            [ "4. Common Utilities Layer", "contrib_arch.html#autotoc_md108", null ]
          ] ]
        ] ],
        [ "Core Components", "contrib_arch.html#autotoc_md110", [
          [ "1. Builder System", "contrib_arch.html#autotoc_md111", null ],
          [ "2. Wrapper System", "contrib_arch.html#autotoc_md112", null ],
          [ "3. Transport System", "contrib_arch.html#autotoc_md113", null ],
          [ "4. Common Utilities", "contrib_arch.html#autotoc_md114", null ]
        ] ],
        [ "Design Patterns", "contrib_arch.html#autotoc_md116", [
          [ "1. Builder Pattern", "contrib_arch.html#autotoc_md117", null ],
          [ "2. Dependency Injection", "contrib_arch.html#autotoc_md118", null ],
          [ "3. Observer Pattern", "contrib_arch.html#autotoc_md119", null ],
          [ "4. Singleton Pattern", "contrib_arch.html#autotoc_md120", null ],
          [ "5. RAII (Resource Acquisition Is Initialization)", "contrib_arch.html#autotoc_md121", null ],
          [ "6. Template Method Pattern", "contrib_arch.html#autotoc_md122", null ]
        ] ],
        [ "Threading Model", "contrib_arch.html#autotoc_md124", [
          [ "Overview", "contrib_arch.html#autotoc_md125", null ],
          [ "Thread Safety Guarantees", "contrib_arch.html#autotoc_md126", [
            [ "1. API Methods", "contrib_arch.html#autotoc_md127", null ],
            [ "2. Callbacks", "contrib_arch.html#autotoc_md128", null ],
            [ "3. Shared State", "contrib_arch.html#autotoc_md129", null ]
          ] ],
          [ "IO Context Management", "contrib_arch.html#autotoc_md130", [
            [ "Shared Context (Default)", "contrib_arch.html#autotoc_md131", null ],
            [ "Independent Context", "contrib_arch.html#autotoc_md132", null ]
          ] ]
        ] ],
        [ "Memory Management", "contrib_arch.html#autotoc_md134", [
          [ "1. Smart Pointers", "contrib_arch.html#autotoc_md135", null ],
          [ "2. Memory Pool", "contrib_arch.html#autotoc_md136", null ],
          [ "3. Memory Tracking", "contrib_arch.html#autotoc_md137", null ],
          [ "4. Safe Data Buffer", "contrib_arch.html#autotoc_md138", null ]
        ] ],
        [ "Error Handling", "contrib_arch.html#autotoc_md140", [
          [ "Error Propagation Flow", "contrib_arch.html#autotoc_md141", null ],
          [ "Error Categories", "contrib_arch.html#autotoc_md142", null ],
          [ "Error Recovery Strategies", "contrib_arch.html#autotoc_md143", [
            [ "Automatic Retry", "contrib_arch.html#autotoc_md144", null ]
          ] ]
        ] ],
        [ "Configuration System", "contrib_arch.html#autotoc_md146", [
          [ "Compile-Time Configuration", "contrib_arch.html#autotoc_md147", null ],
          [ "Runtime Configuration", "contrib_arch.html#autotoc_md148", null ]
        ] ],
        [ "Performance Considerations", "contrib_arch.html#autotoc_md150", [
          [ "1. Asynchronous I/O", "contrib_arch.html#autotoc_md151", null ],
          [ "2. Zero-Copy Operations", "contrib_arch.html#autotoc_md152", null ],
          [ "3. Memory Pooling", "contrib_arch.html#autotoc_md153", null ]
        ] ],
        [ "Extension Points", "contrib_arch.html#autotoc_md155", [
          [ "1. Custom Transports", "contrib_arch.html#autotoc_md156", null ],
          [ "2. Custom Builders", "contrib_arch.html#autotoc_md157", null ],
          [ "3. Custom Error Handlers", "contrib_arch.html#autotoc_md158", null ]
        ] ],
        [ "Development & Tooling", "contrib_arch.html#autotoc_md160", [
          [ "Documentation Generation", "contrib_arch.html#autotoc_md161", null ]
        ] ],
        [ "Testing Architecture", "contrib_arch.html#autotoc_md163", [
          [ "1. Dependency Injection", "contrib_arch.html#autotoc_md164", null ],
          [ "2. Independent Contexts", "contrib_arch.html#autotoc_md165", null ],
          [ "3. State Verification", "contrib_arch.html#autotoc_md166", null ]
        ] ],
        [ "Summary", "contrib_arch.html#autotoc_md168", null ],
        [ "Runtime Behavior Model", "contrib_arch_runtime.html", [
          [ "Table of Contents", "contrib_arch_runtime.html#autotoc_md171", null ],
          [ "Threading Model & Callback Execution", "contrib_arch_runtime.html#autotoc_md173", [
            [ "Architecture Diagram", "contrib_arch_runtime.html#autotoc_md174", null ],
            [ "Key Points", "contrib_arch_runtime.html#autotoc_md176", [
              [ "✅ Thread-Safe API Methods", "contrib_arch_runtime.html#autotoc_md177", null ],
              [ "✅ Callback Execution Context", "contrib_arch_runtime.html#autotoc_md179", null ],
              [ "⚠️ Never Block in Callbacks", "contrib_arch_runtime.html#autotoc_md181", null ],
              [ "✅ Thread-Safe State Access", "contrib_arch_runtime.html#autotoc_md183", null ]
            ] ],
            [ "Threading Model Summary", "contrib_arch_runtime.html#autotoc_md185", null ]
          ] ],
          [ "Reconnection Policy & State Machine", "contrib_arch_runtime.html#autotoc_md187", [
            [ "State Machine Diagram", "contrib_arch_runtime.html#autotoc_md188", null ],
            [ "Connection States", "contrib_arch_runtime.html#autotoc_md190", null ],
            [ "Configuration Example", "contrib_arch_runtime.html#autotoc_md192", null ],
            [ "Retry Behavior", "contrib_arch_runtime.html#autotoc_md194", [
              [ "Default Behavior", "contrib_arch_runtime.html#autotoc_md195", null ],
              [ "Retry Interval Configuration", "contrib_arch_runtime.html#autotoc_md196", null ],
              [ "State Callbacks", "contrib_arch_runtime.html#autotoc_md198", null ],
              [ "Manual Control", "contrib_arch_runtime.html#autotoc_md200", null ]
            ] ],
            [ "Reconnection Best Practices", "contrib_arch_runtime.html#autotoc_md202", [
              [ "1. Choose Appropriate Retry Interval", "contrib_arch_runtime.html#autotoc_md203", null ],
              [ "2. Handle State Transitions", "contrib_arch_runtime.html#autotoc_md205", null ],
              [ "3. Graceful Shutdown", "contrib_arch_runtime.html#autotoc_md207", null ]
            ] ]
          ] ],
          [ "Backpressure Handling", "contrib_arch_runtime.html#autotoc_md209", [
            [ "Backpressure Flow", "contrib_arch_runtime.html#autotoc_md210", null ],
            [ "Backpressure Configuration", "contrib_arch_runtime.html#autotoc_md212", null ],
            [ "Backpressure Strategies", "contrib_arch_runtime.html#autotoc_md214", [
              [ "Strategy 1: Pause Sending", "contrib_arch_runtime.html#autotoc_md215", null ],
              [ "Strategy 2: Rate Limiting", "contrib_arch_runtime.html#autotoc_md217", null ],
              [ "Strategy 3: Drop Data", "contrib_arch_runtime.html#autotoc_md219", null ]
            ] ],
            [ "Backpressure Monitoring", "contrib_arch_runtime.html#autotoc_md221", null ],
            [ "Memory Safety", "contrib_arch_runtime.html#autotoc_md223", null ]
          ] ],
          [ "Best Practices", "contrib_arch_runtime.html#autotoc_md225", [
            [ "1. Threading Best Practices", "contrib_arch_runtime.html#autotoc_md226", [
              [ "✅ DO", "contrib_arch_runtime.html#autotoc_md227", null ],
              [ "❌ DON'T", "contrib_arch_runtime.html#autotoc_md228", null ]
            ] ],
            [ "2. Reconnection Best Practices", "contrib_arch_runtime.html#autotoc_md230", [
              [ "✅ DO", "contrib_arch_runtime.html#autotoc_md231", null ],
              [ "❌ DON'T", "contrib_arch_runtime.html#autotoc_md232", null ]
            ] ],
            [ "3. Backpressure Best Practices", "contrib_arch_runtime.html#autotoc_md234", [
              [ "✅ DO", "contrib_arch_runtime.html#autotoc_md235", null ],
              [ "❌ DON'T", "contrib_arch_runtime.html#autotoc_md236", null ]
            ] ]
          ] ],
          [ "Performance Considerations", "contrib_arch_runtime.html#autotoc_md238", [
            [ "Threading Overhead", "contrib_arch_runtime.html#autotoc_md239", null ],
            [ "Reconnection Overhead", "contrib_arch_runtime.html#autotoc_md240", null ],
            [ "Backpressure Overhead", "contrib_arch_runtime.html#autotoc_md241", null ]
          ] ],
          [ "Next Steps", "contrib_arch_runtime.html#autotoc_md243", null ]
        ] ],
        [ "Memory Safety Architecture", "contrib_arch_memory.html", [
          [ "Table of Contents", "contrib_arch_memory.html#autotoc_md10", null ],
          [ "Overview", "contrib_arch_memory.html#autotoc_md12", [
            [ "Memory Safety Guarantees", "contrib_arch_memory.html#autotoc_md13", null ],
            [ "Safety Levels", "contrib_arch_memory.html#autotoc_md15", null ]
          ] ],
          [ "Safe Data Handling", "contrib_arch_memory.html#autotoc_md17", [
            [ "SafeDataBuffer", "contrib_arch_memory.html#autotoc_md18", null ],
            [ "Features", "contrib_arch_memory.html#autotoc_md20", [
              [ "1. Construction Validation", "contrib_arch_memory.html#autotoc_md21", null ],
              [ "2. Safe Type Conversions", "contrib_arch_memory.html#autotoc_md23", null ],
              [ "3. Memory Validation", "contrib_arch_memory.html#autotoc_md25", null ]
            ] ],
            [ "Safe Span", "contrib_arch_memory.html#autotoc_md27", null ]
          ] ],
          [ "Thread-Safe State Management", "contrib_arch_memory.html#autotoc_md29", [
            [ "ThreadSafeState", "contrib_arch_memory.html#autotoc_md30", null ],
            [ "AtomicState", "contrib_arch_memory.html#autotoc_md32", null ],
            [ "ThreadSafeCounter", "contrib_arch_memory.html#autotoc_md34", null ],
            [ "ThreadSafeFlag", "contrib_arch_memory.html#autotoc_md36", null ],
            [ "Thread Safety Summary", "contrib_arch_memory.html#autotoc_md38", null ]
          ] ],
          [ "Memory Tracking", "contrib_arch_memory.html#autotoc_md40", [
            [ "Built-in Memory Tracking", "contrib_arch_memory.html#autotoc_md41", null ],
            [ "Features", "contrib_arch_memory.html#autotoc_md43", [
              [ "1. Allocation Tracking", "contrib_arch_memory.html#autotoc_md44", null ],
              [ "2. Leak Detection", "contrib_arch_memory.html#autotoc_md46", null ],
              [ "3. Performance Monitoring", "contrib_arch_memory.html#autotoc_md48", null ],
              [ "4. Debug Reports", "contrib_arch_memory.html#autotoc_md50", null ]
            ] ],
            [ "Zero Overhead in Release", "contrib_arch_memory.html#autotoc_md52", null ]
          ] ],
          [ "AddressSanitizer Support", "contrib_arch_memory.html#autotoc_md54", [
            [ "Enable AddressSanitizer", "contrib_arch_memory.html#autotoc_md55", null ],
            [ "What ASan Detects", "contrib_arch_memory.html#autotoc_md57", null ],
            [ "Running with ASan", "contrib_arch_memory.html#autotoc_md59", null ],
            [ "Performance Impact", "contrib_arch_memory.html#autotoc_md61", null ]
          ] ],
          [ "Best Practices", "contrib_arch_memory.html#autotoc_md63", [
            [ "1. Buffer Management", "contrib_arch_memory.html#autotoc_md64", [
              [ "✅ DO", "contrib_arch_memory.html#autotoc_md65", null ],
              [ "❌ DON'T", "contrib_arch_memory.html#autotoc_md66", null ]
            ] ],
            [ "2. Type Conversions", "contrib_arch_memory.html#autotoc_md68", [
              [ "✅ DO", "contrib_arch_memory.html#autotoc_md69", null ],
              [ "❌ DON'T", "contrib_arch_memory.html#autotoc_md70", null ]
            ] ],
            [ "3. Thread Safety", "contrib_arch_memory.html#autotoc_md72", [
              [ "✅ DO", "contrib_arch_memory.html#autotoc_md73", null ],
              [ "❌ DON'T", "contrib_arch_memory.html#autotoc_md74", null ]
            ] ],
            [ "4. Memory Tracking", "contrib_arch_memory.html#autotoc_md76", [
              [ "✅ DO", "contrib_arch_memory.html#autotoc_md77", null ],
              [ "❌ DON'T", "contrib_arch_memory.html#autotoc_md78", null ]
            ] ],
            [ "5. Sanitizers", "contrib_arch_memory.html#autotoc_md80", [
              [ "✅ DO", "contrib_arch_memory.html#autotoc_md81", null ],
              [ "❌ DON'T", "contrib_arch_memory.html#autotoc_md82", null ]
            ] ]
          ] ],
          [ "Memory Safety Benefits", "contrib_arch_memory.html#autotoc_md84", [
            [ "Prevents Common Vulnerabilities", "contrib_arch_memory.html#autotoc_md85", null ],
            [ "Performance", "contrib_arch_memory.html#autotoc_md87", null ]
          ] ],
          [ "Testing Memory Safety", "contrib_arch_memory.html#autotoc_md89", [
            [ "Unit Tests", "contrib_arch_memory.html#autotoc_md90", null ],
            [ "Integration Tests", "contrib_arch_memory.html#autotoc_md92", null ],
            [ "Continuous Integration", "contrib_arch_memory.html#autotoc_md94", null ]
          ] ],
          [ "Next Steps", "contrib_arch_memory.html#autotoc_md96", null ]
        ] ],
        [ "Transport Channel Contract", "contrib_arch_channel.html", [
          [ "1. Introduction", "contrib_arch_channel.html#autotoc_md3", null ],
          [ "2. Core Principles", "contrib_arch_channel.html#autotoc_md4", null ],
          [ "3. Stop Semantics: No Callbacks After Stop()", "contrib_arch_channel.html#autotoc_md5", null ],
          [ "4. Backpressure Policy", "contrib_arch_channel.html#autotoc_md6", null ],
          [ "5. Error Handling Consistency", "contrib_arch_channel.html#autotoc_md7", null ],
          [ "6. State Transitions", "contrib_arch_channel.html#autotoc_md8", null ]
        ] ],
        [ "Wrapper Contract", "contrib_arch_wrapper.html", [
          [ "Scope", "contrib_arch_wrapper.html#autotoc_md244", null ],
          [ "Core Rules", "contrib_arch_wrapper.html#autotoc_md245", [
            [ "1. <tt>start()</tt> reflects real transport state", "contrib_arch_wrapper.html#autotoc_md246", null ],
            [ "2. Repeated <tt>start()</tt> and <tt>stop()</tt> are safe", "contrib_arch_wrapper.html#autotoc_md247", null ],
            [ "3. <tt>auto_start(true)</tt> follows the same startup contract", "contrib_arch_wrapper.html#autotoc_md248", null ]
          ] ],
          [ "External <tt>io_context</tt> Contract", "contrib_arch_wrapper.html#autotoc_md249", [
            [ "4. Externally supplied <tt>io_context</tt> can be reused", "contrib_arch_wrapper.html#autotoc_md250", null ],
            [ "5. Managed and unmanaged external contexts have different ownership rules", "contrib_arch_wrapper.html#autotoc_md251", null ]
          ] ],
          [ "Callback Contract", "contrib_arch_wrapper.html#autotoc_md252", [
            [ "6. Handler replacement uses the latest callback", "contrib_arch_wrapper.html#autotoc_md253", null ],
            [ "7. No wrapper callbacks after <tt>stop()</tt> returns", "contrib_arch_wrapper.html#autotoc_md254", null ],
            [ "8. Generic fallback errors are normalized", "contrib_arch_wrapper.html#autotoc_md255", null ]
          ] ],
          [ "Transport-Agnostic Expectations", "contrib_arch_wrapper.html#autotoc_md256", null ],
          [ "Testing Status", "contrib_arch_wrapper.html#autotoc_md257", null ]
        ] ]
      ] ]
    ] ],
    [ "Orin Nano Validation", "contrib_orin_nano_validation.html", [
      [ "Scope", "contrib_orin_nano_validation.html#autotoc_md349", null ],
      [ "Latest Validation Snapshot", "contrib_orin_nano_validation.html#autotoc_md350", null ],
      [ "Prerequisites", "contrib_orin_nano_validation.html#autotoc_md352", null ],
      [ "Configure And Build", "contrib_orin_nano_validation.html#autotoc_md354", null ],
      [ "Run C++ Tests", "contrib_orin_nano_validation.html#autotoc_md356", null ],
      [ "Serial Validation", "contrib_orin_nano_validation.html#autotoc_md358", [
        [ "Automated Integration Coverage", "contrib_orin_nano_validation.html#autotoc_md359", null ],
        [ "Manual Virtual Serial Pair", "contrib_orin_nano_validation.html#autotoc_md360", null ],
        [ "Physical Loopback", "contrib_orin_nano_validation.html#autotoc_md361", null ]
      ] ],
      [ "Pass Criteria", "contrib_orin_nano_validation.html#autotoc_md363", null ],
      [ "Troubleshooting", "contrib_orin_nano_validation.html#autotoc_md365", [
        [ "Boost Not Found", "contrib_orin_nano_validation.html#autotoc_md366", null ],
        [ "Serial Tests Skip", "contrib_orin_nano_validation.html#autotoc_md367", null ],
        [ "Port-Binding Failures", "contrib_orin_nano_validation.html#autotoc_md368", null ]
      ] ],
      [ "Related Docs", "contrib_orin_nano_validation.html#autotoc_md370", null ]
    ] ],
    [ "User Guide", "user_index.html", [
      [ "Getting Started", "user_index.html#autotoc_md575", null ],
      [ "API Reference", "user_index.html#autotoc_md577", null ],
      [ "Tutorials", "user_index.html#autotoc_md579", null ],
      [ "Guides", "user_index.html#autotoc_md581", null ],
      [ "Unilink Quick Start Guide", "user_quickstart.html", [
        [ "Installation", "user_quickstart.html#autotoc_md621", [
          [ "Prerequisites", "user_quickstart.html#autotoc_md622", null ],
          [ "Build & Install", "user_quickstart.html#autotoc_md623", null ]
        ] ],
        [ "Your First TCP Client", "user_quickstart.html#autotoc_md625", null ],
        [ "Your First TCP Server", "user_quickstart.html#autotoc_md627", null ],
        [ "Your First Serial Device", "user_quickstart.html#autotoc_md629", null ],
        [ "Common Patterns", "user_quickstart.html#autotoc_md631", [
          [ "Pattern 1: Auto-Reconnection", "user_quickstart.html#autotoc_md632", null ],
          [ "Pattern 2: Error Handling", "user_quickstart.html#autotoc_md633", null ],
          [ "Pattern 3: Connection Limits (optional)", "user_quickstart.html#autotoc_md634", null ]
        ] ],
        [ "Next Steps", "user_quickstart.html#autotoc_md636", null ],
        [ "Troubleshooting", "user_quickstart.html#autotoc_md638", [
          [ "Can't connect to server?", "user_quickstart.html#autotoc_md639", null ],
          [ "Port already in use?", "user_quickstart.html#autotoc_md640", null ],
          [ "Need independent IO thread?", "user_quickstart.html#autotoc_md641", null ]
        ] ],
        [ "Support", "user_quickstart.html#autotoc_md643", null ]
      ] ],
      [ "Installation Guide", "user_installation.html", [
        [ "Prerequisites", "user_installation.html#autotoc_md583", null ],
        [ "Installation Methods", "user_installation.html#autotoc_md584", [
          [ "Method 1: vcpkg (Recommended)", "user_installation.html#autotoc_md585", [
            [ "Step 1: Install via vcpkg", "user_installation.html#autotoc_md586", null ],
            [ "Step 2: Use in your project", "user_installation.html#autotoc_md587", null ]
          ] ],
          [ "Method 2: Install from Source (CMake Package)", "user_installation.html#autotoc_md588", [
            [ "Step 1: Build and install", "user_installation.html#autotoc_md589", null ],
            [ "Step 2: Use in your project", "user_installation.html#autotoc_md590", null ]
          ] ],
          [ "Method 3: Release Packages", "user_installation.html#autotoc_md591", [
            [ "Step 1: Download and extract", "user_installation.html#autotoc_md592", null ],
            [ "Step 2: Install", "user_installation.html#autotoc_md593", null ],
            [ "Step 3: Use in your project", "user_installation.html#autotoc_md594", null ]
          ] ],
          [ "Method 4: Git Submodule Integration", "user_installation.html#autotoc_md595", [
            [ "Step 1: Add submodule", "user_installation.html#autotoc_md596", null ],
            [ "Step 2: Use in CMake", "user_installation.html#autotoc_md597", null ]
          ] ]
        ] ],
        [ "Packaging Notes", "user_installation.html#autotoc_md598", null ],
        [ "Build Options (Source Builds)", "user_installation.html#autotoc_md599", null ],
        [ "Next Steps", "user_installation.html#autotoc_md600", null ]
      ] ],
      [ "System Requirements", "user_requirements.html", [
        [ "System Requirements", "user_requirements.html#autotoc_md645", [
          [ "Recommended Platform", "user_requirements.html#autotoc_md646", null ],
          [ "Supported Platforms", "user_requirements.html#autotoc_md647", null ]
        ] ],
        [ "Dependencies", "user_requirements.html#autotoc_md649", [
          [ "Core Library Dependencies", "user_requirements.html#autotoc_md650", null ],
          [ "Dependency Details", "user_requirements.html#autotoc_md651", null ]
        ] ],
        [ "Compiler Requirements", "user_requirements.html#autotoc_md653", [
          [ "Minimum Compiler Versions", "user_requirements.html#autotoc_md654", null ],
          [ "C++ Standard", "user_requirements.html#autotoc_md655", null ]
        ] ],
        [ "Runtime Requirements", "user_requirements.html#autotoc_md657", [
          [ "For Applications Using unilink", "user_requirements.html#autotoc_md658", null ],
          [ "Thread Support", "user_requirements.html#autotoc_md659", null ]
        ] ],
        [ "Platform-Specific Notes", "user_requirements.html#autotoc_md661", [
          [ "Ubuntu 22.04 LTS", "user_requirements.html#autotoc_md662", null ],
          [ "Ubuntu ARM64 / Jetson Orin Nano", "user_requirements.html#autotoc_md663", null ],
          [ "Ubuntu 20.04 LTS", "user_requirements.html#autotoc_md664", null ],
          [ "Other Linux Distributions", "user_requirements.html#autotoc_md665", null ]
        ] ],
        [ "Verifying Your Environment", "user_requirements.html#autotoc_md667", [
          [ "Check Compiler Version", "user_requirements.html#autotoc_md668", null ],
          [ "Check CMake Version", "user_requirements.html#autotoc_md669", null ],
          [ "Check Boost Version", "user_requirements.html#autotoc_md670", null ],
          [ "Quick Environment Test", "user_requirements.html#autotoc_md671", null ]
        ] ],
        [ "Troubleshooting", "user_requirements.html#autotoc_md673", [
          [ "Problem: Compiler Too Old", "user_requirements.html#autotoc_md674", null ],
          [ "Problem: Boost Not Found", "user_requirements.html#autotoc_md675", null ],
          [ "Problem: CMake Too Old", "user_requirements.html#autotoc_md676", null ]
        ] ],
        [ "Next Steps", "user_requirements.html#autotoc_md678", null ]
      ] ],
      [ "Unilink API Guide", "user_api_guide.html", [
        [ "Table of Contents", "user_api_guide.html#autotoc_md470", null ],
        [ "Builder API", "user_api_guide.html#autotoc_md472", [
          [ "Core Concept", "user_api_guide.html#autotoc_md473", null ],
          [ "Common Methods (All Builders)", "user_api_guide.html#autotoc_md474", null ],
          [ "Framed Message Handling", "user_api_guide.html#autotoc_md475", null ],
          [ "IO Context Ownership (advanced)", "user_api_guide.html#autotoc_md476", null ],
          [ "Starting Synchronously vs. Asynchronously", "user_api_guide.html#autotoc_md477", [
            [ "Asynchronous Example", "user_api_guide.html#autotoc_md478", null ]
          ] ]
        ] ],
        [ "TCP Client", "user_api_guide.html#autotoc_md480", [
          [ "Basic Usage", "user_api_guide.html#autotoc_md481", null ],
          [ "API Reference", "user_api_guide.html#autotoc_md482", [
            [ "Constructor", "user_api_guide.html#autotoc_md483", null ],
            [ "Builder Methods", "user_api_guide.html#autotoc_md484", null ],
            [ "Instance Methods", "user_api_guide.html#autotoc_md485", null ]
          ] ],
          [ "Advanced Examples", "user_api_guide.html#autotoc_md486", [
            [ "With Member Functions", "user_api_guide.html#autotoc_md487", null ],
            [ "With Lambda Capture", "user_api_guide.html#autotoc_md488", null ]
          ] ]
        ] ],
        [ "TCP Server", "user_api_guide.html#autotoc_md490", [
          [ "Basic Usage", "user_api_guide.html#autotoc_md491", null ],
          [ "API Reference", "user_api_guide.html#autotoc_md492", [
            [ "Constructor", "user_api_guide.html#autotoc_md493", null ],
            [ "Builder Methods", "user_api_guide.html#autotoc_md494", null ],
            [ "Instance Methods", "user_api_guide.html#autotoc_md495", null ]
          ] ],
          [ "Advanced Examples", "user_api_guide.html#autotoc_md496", [
            [ "Single Client Mode", "user_api_guide.html#autotoc_md497", null ],
            [ "Port Retry", "user_api_guide.html#autotoc_md498", null ],
            [ "Echo Server Pattern", "user_api_guide.html#autotoc_md499", null ]
          ] ]
        ] ],
        [ "Serial Communication", "user_api_guide.html#autotoc_md501", [
          [ "Basic Usage", "user_api_guide.html#autotoc_md502", null ],
          [ "API Reference", "user_api_guide.html#autotoc_md503", [
            [ "Constructor", "user_api_guide.html#autotoc_md504", null ],
            [ "Builder Methods", "user_api_guide.html#autotoc_md505", null ],
            [ "Instance Methods", "user_api_guide.html#autotoc_md506", null ]
          ] ],
          [ "Device Paths", "user_api_guide.html#autotoc_md507", null ],
          [ "Advanced Examples", "user_api_guide.html#autotoc_md508", [
            [ "Arduino Communication", "user_api_guide.html#autotoc_md509", null ],
            [ "GPS Module", "user_api_guide.html#autotoc_md510", null ]
          ] ]
        ] ],
        [ "UDP Communication", "user_api_guide.html#autotoc_md512", [
          [ "Basic Usage", "user_api_guide.html#autotoc_md513", [
            [ "UDP Receiver (Server)", "user_api_guide.html#autotoc_md514", null ],
            [ "UDP Sender (Client)", "user_api_guide.html#autotoc_md515", null ]
          ] ],
          [ "API Reference", "user_api_guide.html#autotoc_md516", [
            [ "Constructors", "user_api_guide.html#autotoc_md517", null ],
            [ "Builder Methods (UdpClient)", "user_api_guide.html#autotoc_md518", null ],
            [ "Builder Methods (UdpServer)", "user_api_guide.html#autotoc_md519", null ],
            [ "Instance Methods (UdpClient)", "user_api_guide.html#autotoc_md520", null ]
          ] ],
          [ "Advanced Examples", "user_api_guide.html#autotoc_md521", [
            [ "Echo Reply (Receiver)", "user_api_guide.html#autotoc_md522", null ],
            [ "UDP Server (Receive-only listener)", "user_api_guide.html#autotoc_md523", null ]
          ] ]
        ] ],
        [ "UDS Communication", "user_api_guide.html#autotoc_md525", [
          [ "Basic Usage", "user_api_guide.html#autotoc_md526", [
            [ "UDS Server", "user_api_guide.html#autotoc_md527", null ],
            [ "UDS Client", "user_api_guide.html#autotoc_md528", null ]
          ] ],
          [ "API Reference", "user_api_guide.html#autotoc_md529", [
            [ "Constructors", "user_api_guide.html#autotoc_md530", null ],
            [ "Builder Methods (UDS Server)", "user_api_guide.html#autotoc_md531", null ],
            [ "Builder Methods (UDS Client)", "user_api_guide.html#autotoc_md532", null ],
            [ "Instance Methods (UDS Client)", "user_api_guide.html#autotoc_md533", null ],
            [ "Instance Methods (UDS Server)", "user_api_guide.html#autotoc_md534", null ]
          ] ],
          [ "Notes on UDS", "user_api_guide.html#autotoc_md535", null ]
        ] ],
        [ "Error Handling", "user_api_guide.html#autotoc_md537", [
          [ "Setup Error Handler", "user_api_guide.html#autotoc_md538", null ],
          [ "Error Levels", "user_api_guide.html#autotoc_md539", null ],
          [ "Error Statistics", "user_api_guide.html#autotoc_md540", null ]
        ] ],
        [ "Logging System", "user_api_guide.html#autotoc_md542", [
          [ "Basic Usage", "user_api_guide.html#autotoc_md543", null ],
          [ "Log Levels", "user_api_guide.html#autotoc_md544", null ],
          [ "Async Logging", "user_api_guide.html#autotoc_md545", null ],
          [ "Custom Format", "user_api_guide.html#autotoc_md546", null ],
          [ "Environment", "user_api_guide.html#autotoc_md547", null ]
        ] ],
        [ "Configuration Management", "user_api_guide.html#autotoc_md549", [
          [ "Load Configuration from File", "user_api_guide.html#autotoc_md550", null ],
          [ "Configuration File Format", "user_api_guide.html#autotoc_md551", null ]
        ] ],
        [ "Best Practices", "user_api_guide.html#autotoc_md553", [
          [ "1. Always Handle Errors", "user_api_guide.html#autotoc_md554", null ],
          [ "2. Use Explicit Lifecycle Control", "user_api_guide.html#autotoc_md555", null ],
          [ "3. Set Appropriate Retry Intervals", "user_api_guide.html#autotoc_md556", null ],
          [ "4. Enable Logging for Debugging", "user_api_guide.html#autotoc_md557", null ],
          [ "5. Use Member Functions for OOP Design", "user_api_guide.html#autotoc_md558", null ]
        ] ],
        [ "Performance Tips", "user_api_guide.html#autotoc_md560", [
          [ "1. Use Independent Context for Testing Only", "user_api_guide.html#autotoc_md561", null ],
          [ "2. Enable Async Logging", "user_api_guide.html#autotoc_md562", null ]
        ] ],
        [ "Backpressure Strategy", "user_api_guide.html#autotoc_md564", [
          [ "Strategies", "user_api_guide.html#autotoc_md565", null ],
          [ "When to use each", "user_api_guide.html#autotoc_md566", null ],
          [ "C++ Usage", "user_api_guide.html#autotoc_md567", null ],
          [ "Thresholds", "user_api_guide.html#autotoc_md568", null ]
        ] ],
        [ "Security", "user_api_guide.html#autotoc_md570", [
          [ "Validate All Input", "user_api_guide.html#autotoc_md571", null ],
          [ "Rate Limiting", "user_api_guide.html#autotoc_md572", null ],
          [ "Connection Limits", "user_api_guide.html#autotoc_md573", null ]
        ] ]
      ] ],
      [ "Tutorials", "user_tutorials.html", [
        [ "Getting Started with Unilink", "tutorial_01.html", [
          [ "What You'll Build", "tutorial_01.html#autotoc_md761", null ],
          [ "Step 1: Create The Client", "tutorial_01.html#autotoc_md763", null ],
          [ "Step 2: Build With CMake", "tutorial_01.html#autotoc_md765", null ],
          [ "Step 3: Run Against A Test Server", "tutorial_01.html#autotoc_md767", null ],
          [ "API Patterns Used In This Tutorial", "tutorial_01.html#autotoc_md769", null ],
          [ "Use The Full Example If You Want More", "tutorial_01.html#autotoc_md771", null ],
          [ "Next Steps", "tutorial_01.html#autotoc_md773", null ]
        ] ],
        [ "Building a TCP Server", "tutorial_02.html", [
          [ "What You'll Build", "tutorial_02.html#autotoc_md776", null ],
          [ "Step 1: Create The Server", "tutorial_02.html#autotoc_md778", null ],
          [ "Step 2: Run It", "tutorial_02.html#autotoc_md780", null ],
          [ "Step 3: Understand The Current Server API", "tutorial_02.html#autotoc_md782", null ],
          [ "Client Limits", "tutorial_02.html#autotoc_md784", null ],
          [ "Use The Full Example Programs For More", "tutorial_02.html#autotoc_md786", null ],
          [ "Next Steps", "tutorial_02.html#autotoc_md788", null ]
        ] ],
        [ "UDS Communication", "tutorial_03.html", [
          [ "What You'll Build", "tutorial_03.html#autotoc_md791", null ],
          [ "Step 1: Create A UDS Server", "tutorial_03.html#autotoc_md793", null ],
          [ "Step 2: Create A UDS Client", "tutorial_03.html#autotoc_md795", null ],
          [ "Why Use UDS Instead Of TCP", "tutorial_03.html#autotoc_md797", null ],
          [ "Operational Notes", "tutorial_03.html#autotoc_md799", null ],
          [ "Next Steps", "tutorial_03.html#autotoc_md801", null ]
        ] ],
        [ "Serial Communication", "tutorial_04.html", [
          [ "What You'll Build", "tutorial_04.html#autotoc_md804", null ],
          [ "Step 1: Choose A Device Path", "tutorial_04.html#autotoc_md806", null ],
          [ "Step 2: Create A Minimal Serial Terminal", "tutorial_04.html#autotoc_md808", null ],
          [ "Step 3: Build And Run", "tutorial_04.html#autotoc_md810", null ],
          [ "Step 4: Test With A Second Terminal", "tutorial_04.html#autotoc_md812", null ],
          [ "Common Adjustments", "tutorial_04.html#autotoc_md814", null ],
          [ "When To Use The Example Programs Instead", "tutorial_04.html#autotoc_md816", null ],
          [ "Next Steps", "tutorial_04.html#autotoc_md818", null ]
        ] ],
        [ "UDP Communication", "tutorial_05.html", [
          [ "What You'll Build", "tutorial_05.html#autotoc_md821", null ],
          [ "Step 1: Create A Receiver", "tutorial_05.html#autotoc_md823", null ],
          [ "Step 2: Create A Sender", "tutorial_05.html#autotoc_md825", null ],
          [ "Step 3: Build The Two Programs", "tutorial_05.html#autotoc_md827", null ],
          [ "Step 4: Run Both Programs", "tutorial_05.html#autotoc_md829", null ],
          [ "What Is Different About UDP", "tutorial_05.html#autotoc_md831", null ],
          [ "Practical Notes", "tutorial_05.html#autotoc_md833", null ],
          [ "Use The Full Examples For Repeated Testing", "tutorial_05.html#autotoc_md835", null ],
          [ "Next Steps", "tutorial_05.html#autotoc_md837", null ]
        ] ]
      ] ],
      [ "Troubleshooting Guide", "user_troubleshooting.html", [
        [ "Table of Contents", "user_troubleshooting.html#autotoc_md680", null ],
        [ "Connection Issues", "user_troubleshooting.html#autotoc_md682", [
          [ "Problem: Connection Refused", "user_troubleshooting.html#autotoc_md683", [
            [ "1. Server Not Running", "user_troubleshooting.html#autotoc_md684", null ],
            [ "2. Wrong Host/Port", "user_troubleshooting.html#autotoc_md685", null ],
            [ "3. Firewall Blocking", "user_troubleshooting.html#autotoc_md686", null ]
          ] ],
          [ "Problem: Connection Timeout", "user_troubleshooting.html#autotoc_md688", [
            [ "1. Network Unreachable", "user_troubleshooting.html#autotoc_md689", null ],
            [ "2. Server Overloaded", "user_troubleshooting.html#autotoc_md690", null ]
          ] ],
          [ "Problem: Connection Drops Randomly", "user_troubleshooting.html#autotoc_md692", [
            [ "1. Network Instability", "user_troubleshooting.html#autotoc_md693", null ],
            [ "2. Server Closing Connection", "user_troubleshooting.html#autotoc_md694", null ],
            [ "3. Keep-Alive Not Set", "user_troubleshooting.html#autotoc_md695", null ]
          ] ],
          [ "Problem: Port Already in Use", "user_troubleshooting.html#autotoc_md697", [
            [ "1. Kill Existing Process", "user_troubleshooting.html#autotoc_md698", null ],
            [ "2. Use Different Port", "user_troubleshooting.html#autotoc_md699", null ],
            [ "3. Enable Port Retry", "user_troubleshooting.html#autotoc_md700", null ]
          ] ]
        ] ],
        [ "Compilation Errors", "user_troubleshooting.html#autotoc_md702", [
          [ "Problem: unilink/unilink.hpp Not Found", "user_troubleshooting.html#autotoc_md703", [
            [ "1. Install unilink", "user_troubleshooting.html#autotoc_md704", null ],
            [ "2. Add Include Path", "user_troubleshooting.html#autotoc_md705", null ],
            [ "3. Use as Subdirectory", "user_troubleshooting.html#autotoc_md706", null ]
          ] ],
          [ "Problem: Undefined Reference to unilink Symbols", "user_troubleshooting.html#autotoc_md708", [
            [ "1. Link unilink Library", "user_troubleshooting.html#autotoc_md709", null ],
            [ "2. Check Library Path", "user_troubleshooting.html#autotoc_md710", null ]
          ] ],
          [ "Problem: Boost Not Found", "user_troubleshooting.html#autotoc_md712", [
            [ "Recommended vcpkg setup", "user_troubleshooting.html#autotoc_md713", null ],
            [ "System Boost setup", "user_troubleshooting.html#autotoc_md714", null ],
            [ "Windows (vcpkg)", "user_troubleshooting.html#autotoc_md715", null ],
            [ "Manual Boost Path", "user_troubleshooting.html#autotoc_md716", null ]
          ] ]
        ] ],
        [ "Runtime Errors", "user_troubleshooting.html#autotoc_md718", [
          [ "Problem: Segmentation Fault", "user_troubleshooting.html#autotoc_md719", [
            [ "1. Enable Core Dumps", "user_troubleshooting.html#autotoc_md720", null ],
            [ "2. Common Causes", "user_troubleshooting.html#autotoc_md721", null ]
          ] ],
          [ "Problem: Callbacks Not Being Called", "user_troubleshooting.html#autotoc_md723", [
            [ "1. Callback Not Registered", "user_troubleshooting.html#autotoc_md724", null ],
            [ "2. Client Not Started", "user_troubleshooting.html#autotoc_md725", null ],
            [ "3. Application Exits Too Quickly", "user_troubleshooting.html#autotoc_md726", null ]
          ] ],
          [ "Problem: UDP with Reliable Strategy Still Drops Packets", "user_troubleshooting.html#autotoc_md728", null ]
        ] ],
        [ "Performance Issues", "user_troubleshooting.html#autotoc_md730", [
          [ "Problem: High CPU Usage", "user_troubleshooting.html#autotoc_md731", [
            [ "1. Busy Loop in Callback", "user_troubleshooting.html#autotoc_md732", null ],
            [ "2. Too Many Retries", "user_troubleshooting.html#autotoc_md733", null ],
            [ "3. Excessive Logging", "user_troubleshooting.html#autotoc_md734", null ]
          ] ],
          [ "Problem: High Memory Usage", "user_troubleshooting.html#autotoc_md736", [
            [ "1. Fix Memory Leaks", "user_troubleshooting.html#autotoc_md737", null ],
            [ "3. Limit Buffer Sizes", "user_troubleshooting.html#autotoc_md738", null ]
          ] ],
          [ "Problem: Slow Data Transfer", "user_troubleshooting.html#autotoc_md740", [
            [ "1. Batch Small Messages", "user_troubleshooting.html#autotoc_md741", null ],
            [ "2. Use Binary Protocol", "user_troubleshooting.html#autotoc_md742", null ],
            [ "3. Enable Async Logging", "user_troubleshooting.html#autotoc_md743", null ]
          ] ]
        ] ],
        [ "Memory Issues", "user_troubleshooting.html#autotoc_md745", [
          [ "Problem: Memory Leak Detected", "user_troubleshooting.html#autotoc_md746", null ]
        ] ],
        [ "Thread Safety Issues", "user_troubleshooting.html#autotoc_md748", [
          [ "Problem: Race Condition / Data Corruption", "user_troubleshooting.html#autotoc_md749", [
            [ "1. Protect Shared State", "user_troubleshooting.html#autotoc_md750", null ]
          ] ]
        ] ],
        [ "Debugging Tips", "user_troubleshooting.html#autotoc_md752", [
          [ "Enable Debug Logging", "user_troubleshooting.html#autotoc_md753", null ],
          [ "Use GDB for Debugging", "user_troubleshooting.html#autotoc_md754", null ],
          [ "Network Debugging with tcpdump", "user_troubleshooting.html#autotoc_md755", null ],
          [ "Test with netcat", "user_troubleshooting.html#autotoc_md756", null ]
        ] ],
        [ "Getting Help", "user_troubleshooting.html#autotoc_md758", null ]
      ] ],
      [ "Python Bindings", "user_python_bindings.html", null ],
      [ "Performance Guide", "user_performance.html", [
        [ "Table of Contents", "user_performance.html#autotoc_md602", null ],
        [ "Runtime Optimization", "user_performance.html#autotoc_md604", [
          [ "1. Threading Model & IO Context", "user_performance.html#autotoc_md605", null ],
          [ "2. Async Logging", "user_performance.html#autotoc_md606", null ],
          [ "3. Non-Blocking Callbacks", "user_performance.html#autotoc_md607", null ]
        ] ],
        [ "Memory Optimization", "user_performance.html#autotoc_md609", [
          [ "1. Avoid Data Copies", "user_performance.html#autotoc_md610", null ],
          [ "2. Reserve Vector Capacity", "user_performance.html#autotoc_md611", null ]
        ] ],
        [ "Network Optimization", "user_performance.html#autotoc_md613", [
          [ "1. Batch Small Messages", "user_performance.html#autotoc_md614", null ],
          [ "2. Connection Reuse", "user_performance.html#autotoc_md615", null ],
          [ "3. Socket Tuning", "user_performance.html#autotoc_md616", null ]
        ] ],
        [ "Backpressure Management", "user_performance.html#backpressure-management", [
          [ "1. Choosing a Strategy", "user_performance.html#autotoc_md618", null ],
          [ "2. High-Throughput Sensors (LiDAR/Camera)", "user_performance.html#autotoc_md619", null ],
          [ "3. Critical Reliable Data", "user_performance.html#autotoc_md620", null ]
        ] ]
      ] ]
    ] ],
    [ "unilink Documentation", "md_docs_2README.html", [
      [ "Directory Structure", "md_docs_2README.html#autotoc_md462", null ],
      [ "Where To Start", "md_docs_2README.html#autotoc_md463", null ],
      [ "Generating Documentation", "md_docs_2README.html#autotoc_md464", [
        [ "Prerequisites", "md_docs_2README.html#autotoc_md465", null ],
        [ "Supported Methods", "md_docs_2README.html#autotoc_md466", null ]
      ] ],
      [ "Viewing Generated HTML", "md_docs_2README.html#autotoc_md467", null ],
      [ "Maintenance Notes", "md_docs_2README.html#autotoc_md468", null ]
    ] ],
    [ "Asynchronous Programming Patterns", "user_tutorial_async.html", [
      [ "1. Non-Blocking Startup", "user_tutorial_async.html#autotoc_md840", [
        [ "The Async Pattern", "user_tutorial_async.html#autotoc_md841", null ]
      ] ],
      [ "2. Shared Ownership in Callbacks", "user_tutorial_async.html#autotoc_md843", [
        [ "Safe Capture Pattern", "user_tutorial_async.html#autotoc_md844", null ]
      ] ],
      [ "3. Parallel Initialization", "user_tutorial_async.html#autotoc_md846", null ],
      [ "4. When to Use Async vs Sync", "user_tutorial_async.html#autotoc_md848", null ],
      [ "Summary", "user_tutorial_async.html#autotoc_md850", null ]
    ] ],
    [ "unilink tests", "md_test_2README.html", [
      [ "Running", "md_test_2README.html#autotoc_md852", null ],
      [ "Naming", "md_test_2README.html#autotoc_md853", null ]
    ] ],
    [ "unilink", "md_README.html", [
      [ "Description", "md_README.html#autotoc_md855", null ],
      [ "Feature Highlights", "md_README.html#autotoc_md856", null ],
      [ "Requirements", "md_README.html#autotoc_md857", null ],
      [ "📦 Installation", "md_README.html#autotoc_md858", [
        [ "vcpkg (recommended)", "md_README.html#autotoc_md859", null ],
        [ "Contributor Development Setup", "md_README.html#autotoc_md860", null ]
      ] ],
      [ "📚 Documentation", "md_README.html#autotoc_md861", [
        [ "📖 For Library Users", "md_README.html#autotoc_md862", null ],
        [ "🔧 For Contributors", "md_README.html#autotoc_md863", null ],
        [ "💡 Examples", "md_README.html#autotoc_md864", null ]
      ] ],
      [ "📄 License", "md_README.html#autotoc_md866", null ]
    ] ],
    [ "Namespaces", "namespaces.html", [
      [ "Namespace List", "namespaces.html", "namespaces_dup" ],
      [ "Namespace Members", "namespacemembers.html", [
        [ "All", "namespacemembers.html", null ],
        [ "Functions", "namespacemembers_func.html", null ],
        [ "Variables", "namespacemembers_vars.html", null ],
        [ "Typedefs", "namespacemembers_type.html", null ],
        [ "Enumerations", "namespacemembers_enum.html", null ],
        [ "Enumerator", "namespacemembers_eval.html", null ]
      ] ]
    ] ],
    [ "Concepts", "concepts.html", "concepts" ],
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
        [ "Related Symbols", "functions_rela.html", null ]
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
"classunilink_1_1builder_1_1UdpClientBuilder.html#a168a445050dba774ad3c573ecc9ff436",
"classunilink_1_1concurrency_1_1ThreadSafeState.html#a801aa228c55ce37a501c9313137140b2",
"classunilink_1_1diagnostics_1_1Logger.html#ada26b971ae75266aff644f17ecb670e8",
"classunilink_1_1memory_1_1GlobalMemoryPool.html",
"classunilink_1_1transport_1_1BoostSerialPort.html#af914e71e312c12837af7259113e6f26a",
"classunilink_1_1transport_1_1TcpServerSession.html#a6f2e8e32e9d48cf6e310bd2c129ac780",
"classunilink_1_1util_1_1InputValidator.html#a916c2974db54fded555dbfe4615d6239",
"classunilink_1_1wrapper_1_1ServerInterface.html#ad18e4d24ce196ade73fac5c7b83a2270",
"classunilink_1_1wrapper_1_1UdpServer.html#a48e99fec83c6ed3d7c6fd3d9e71f5e50",
"config__factory_8cc_source.html",
"contrib_build.html#autotoc_md271",
"functions_func_p.html",
"namespaceunilink_1_1base_1_1constants.html#a05abdf71a9089b67af9c1c7f5a8c30b9",
"structunilink_1_1config_1_1ConfigItem.html#a5d42b01ee9135405a2c2070cdafa1160",
"structunilink_1_1diagnostics_1_1ErrorInfo.html#a9faaa444de9d98cd0e75d6879daaaff3",
"structunilink_1_1transport_1_1Serial_1_1Impl.html#afd063f7e1b097e7cef807ea4a9fac3f5",
"structunilink_1_1transport_1_1UdpChannel_1_1Impl.html#abb6764ea5c652eaaaf0d431650c2fbab",
"structunilink_1_1wrapper_1_1Serial_1_1Impl.html#a5505f2fd78751a7a19fdffa03aadd443",
"structunilink_1_1wrapper_1_1TcpServer_1_1Impl.html#a8b8ab1b95d8a945e14a5d161828e729d",
"structunilink_1_1wrapper_1_1UdpServer_1_1Impl_1_1SessionEntry.html#aa578b746f75d7edfeb39e2bb89f4eb8c",
"transport_2tcp__server_2tcp__server_8hpp.html",
"user_quickstart.html#autotoc_md632"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';