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
    [ "Unilink Documentation", "docs_index.html", [
      [ "📖 For Library Users", "docs_index.html#autotoc_md436", null ],
      [ "🔧 For Contributors", "docs_index.html#autotoc_md438", null ],
      [ "Examples and Tests", "docs_index.html#autotoc_md440", null ],
      [ "User Guide", "user_index.html", [
        [ "Getting Started", "user_index.html#autotoc_md545", null ],
        [ "API Reference", "user_index.html#autotoc_md547", null ],
        [ "Tutorials", "user_index.html#autotoc_md549", null ],
        [ "Guides", "user_index.html#autotoc_md551", null ],
        [ "Unilink Quick Start Guide", "user_quickstart.html", [
          [ "Installation", "user_quickstart.html#autotoc_md611", [
            [ "Prerequisites", "user_quickstart.html#autotoc_md612", null ],
            [ "Build & Install", "user_quickstart.html#autotoc_md613", null ]
          ] ],
          [ "Your First TCP Client", "user_quickstart.html#autotoc_md615", null ],
          [ "Your First TCP Server", "user_quickstart.html#autotoc_md617", null ],
          [ "Your First Serial Device", "user_quickstart.html#autotoc_md619", null ],
          [ "Common Patterns", "user_quickstart.html#autotoc_md621", [
            [ "Pattern 1: Auto-Reconnection", "user_quickstart.html#autotoc_md622", null ],
            [ "Pattern 2: Error Handling", "user_quickstart.html#autotoc_md623", null ],
            [ "Pattern 3: Single vs Multi-Client Server (optional)", "user_quickstart.html#autotoc_md624", null ]
          ] ],
          [ "Next Steps", "user_quickstart.html#autotoc_md626", null ],
          [ "Troubleshooting", "user_quickstart.html#autotoc_md628", [
            [ "Can't connect to server?", "user_quickstart.html#autotoc_md629", null ],
            [ "Port already in use?", "user_quickstart.html#autotoc_md630", null ],
            [ "Need independent IO thread?", "user_quickstart.html#autotoc_md631", null ]
          ] ],
          [ "Support", "user_quickstart.html#autotoc_md633", null ]
        ] ],
        [ "Installation Guide", "user_installation.html", [
          [ "Prerequisites", "user_installation.html#autotoc_md553", null ],
          [ "Installation Methods", "user_installation.html#autotoc_md554", [
            [ "Method 1: vcpkg (Recommended)", "user_installation.html#autotoc_md555", [
              [ "Step 1: Install via vcpkg", "user_installation.html#autotoc_md556", null ],
              [ "Step 2: Use in your project", "user_installation.html#autotoc_md557", null ]
            ] ],
            [ "Method 2: Install from Source (CMake Package)", "user_installation.html#autotoc_md558", [
              [ "Step 1: Build and install", "user_installation.html#autotoc_md559", null ],
              [ "Step 2: Use in your project", "user_installation.html#autotoc_md560", null ]
            ] ],
            [ "Method 3: Release Packages", "user_installation.html#autotoc_md561", [
              [ "Step 1: Download and extract", "user_installation.html#autotoc_md562", null ],
              [ "Step 2: Install", "user_installation.html#autotoc_md563", null ],
              [ "Step 3: Use in your project", "user_installation.html#autotoc_md564", null ]
            ] ],
            [ "Method 4: Git Submodule Integration", "user_installation.html#autotoc_md565", [
              [ "Step 1: Add submodule", "user_installation.html#autotoc_md566", null ],
              [ "Step 2: Use in CMake", "user_installation.html#autotoc_md567", null ]
            ] ]
          ] ],
          [ "Packaging Notes", "user_installation.html#autotoc_md568", null ],
          [ "Build Options (Source Builds)", "user_installation.html#autotoc_md569", null ],
          [ "Next Steps", "user_installation.html#autotoc_md570", null ]
        ] ],
        [ "System Requirements", "user_requirements.html", [
          [ "System Requirements", "user_requirements.html#autotoc_md635", [
            [ "Recommended Platform", "user_requirements.html#autotoc_md636", null ],
            [ "Supported Platforms", "user_requirements.html#autotoc_md637", null ]
          ] ],
          [ "Dependencies", "user_requirements.html#autotoc_md639", [
            [ "Core Library Dependencies", "user_requirements.html#autotoc_md640", null ],
            [ "Dependency Details", "user_requirements.html#autotoc_md641", null ]
          ] ],
          [ "Compiler Requirements", "user_requirements.html#autotoc_md643", [
            [ "Minimum Compiler Versions", "user_requirements.html#autotoc_md644", null ],
            [ "C++ Standard", "user_requirements.html#autotoc_md645", null ]
          ] ],
          [ "Runtime Requirements", "user_requirements.html#autotoc_md647", [
            [ "For Applications Using unilink", "user_requirements.html#autotoc_md648", null ],
            [ "Thread Support", "user_requirements.html#autotoc_md649", null ]
          ] ],
          [ "Platform-Specific Notes", "user_requirements.html#autotoc_md651", [
            [ "Ubuntu 22.04 LTS", "user_requirements.html#autotoc_md652", null ],
            [ "Ubuntu 20.04 LTS", "user_requirements.html#autotoc_md653", null ],
            [ "Other Linux Distributions", "user_requirements.html#autotoc_md654", null ]
          ] ],
          [ "Verifying Your Environment", "user_requirements.html#autotoc_md656", [
            [ "Check Compiler Version", "user_requirements.html#autotoc_md657", null ],
            [ "Check CMake Version", "user_requirements.html#autotoc_md658", null ],
            [ "Check Boost Version", "user_requirements.html#autotoc_md659", null ],
            [ "Quick Environment Test", "user_requirements.html#autotoc_md660", null ]
          ] ],
          [ "Troubleshooting", "user_requirements.html#autotoc_md662", [
            [ "Problem: Compiler Too Old", "user_requirements.html#autotoc_md663", null ],
            [ "Problem: Boost Not Found", "user_requirements.html#autotoc_md664", null ],
            [ "Problem: CMake Too Old", "user_requirements.html#autotoc_md665", null ]
          ] ],
          [ "Next Steps", "user_requirements.html#autotoc_md667", null ]
        ] ],
        [ "Unilink API Guide", "user_api_guide.html", [
          [ "Table of Contents", "user_api_guide.html#autotoc_md450", null ],
          [ "Builder API", "user_api_guide.html#autotoc_md452", [
            [ "Core Concept", "user_api_guide.html#autotoc_md453", null ],
            [ "Common Methods (All Builders)", "user_api_guide.html#autotoc_md454", null ],
            [ "Framed Message Handling", "user_api_guide.html#autotoc_md455", null ],
            [ "IO Context Ownership (advanced)", "user_api_guide.html#autotoc_md456", null ]
          ] ],
          [ "TCP Client", "user_api_guide.html#autotoc_md458", [
            [ "Basic Usage", "user_api_guide.html#autotoc_md459", null ],
            [ "API Reference", "user_api_guide.html#autotoc_md460", [
              [ "Constructor", "user_api_guide.html#autotoc_md461", null ],
              [ "Builder Methods", "user_api_guide.html#autotoc_md462", null ],
              [ "Instance Methods", "user_api_guide.html#autotoc_md463", null ]
            ] ],
            [ "Advanced Examples", "user_api_guide.html#autotoc_md464", [
              [ "With Member Functions", "user_api_guide.html#autotoc_md465", null ],
              [ "With Lambda Capture", "user_api_guide.html#autotoc_md466", null ]
            ] ]
          ] ],
          [ "TCP Server", "user_api_guide.html#autotoc_md468", [
            [ "Basic Usage", "user_api_guide.html#autotoc_md469", null ],
            [ "API Reference", "user_api_guide.html#autotoc_md470", [
              [ "Constructor", "user_api_guide.html#autotoc_md471", null ],
              [ "Builder Methods", "user_api_guide.html#autotoc_md472", null ],
              [ "Instance Methods", "user_api_guide.html#autotoc_md473", null ]
            ] ],
            [ "Advanced Examples", "user_api_guide.html#autotoc_md474", [
              [ "Single Client Mode", "user_api_guide.html#autotoc_md475", null ],
              [ "Port Retry", "user_api_guide.html#autotoc_md476", null ],
              [ "Echo Server Pattern", "user_api_guide.html#autotoc_md477", null ]
            ] ]
          ] ],
          [ "Serial Communication", "user_api_guide.html#autotoc_md479", [
            [ "Basic Usage", "user_api_guide.html#autotoc_md480", null ],
            [ "API Reference", "user_api_guide.html#autotoc_md481", [
              [ "Constructor", "user_api_guide.html#autotoc_md482", null ],
              [ "Builder Methods", "user_api_guide.html#autotoc_md483", null ],
              [ "Instance Methods", "user_api_guide.html#autotoc_md484", null ]
            ] ],
            [ "Device Paths", "user_api_guide.html#autotoc_md485", null ],
            [ "Advanced Examples", "user_api_guide.html#autotoc_md486", [
              [ "Arduino Communication", "user_api_guide.html#autotoc_md487", null ],
              [ "GPS Module", "user_api_guide.html#autotoc_md488", null ]
            ] ]
          ] ],
          [ "UDP Communication", "user_api_guide.html#autotoc_md490", [
            [ "Basic Usage", "user_api_guide.html#autotoc_md491", [
              [ "UDP Receiver (Server)", "user_api_guide.html#autotoc_md492", null ],
              [ "UDP Sender (Client)", "user_api_guide.html#autotoc_md493", null ]
            ] ],
            [ "API Reference", "user_api_guide.html#autotoc_md494", [
              [ "Constructors", "user_api_guide.html#autotoc_md495", null ],
              [ "Builder Methods (UdpClient)", "user_api_guide.html#autotoc_md496", null ],
              [ "Builder Methods (UdpServer)", "user_api_guide.html#autotoc_md497", null ],
              [ "Instance Methods (UdpClient)", "user_api_guide.html#autotoc_md498", null ]
            ] ],
            [ "Advanced Examples", "user_api_guide.html#autotoc_md499", [
              [ "Echo Reply (Receiver)", "user_api_guide.html#autotoc_md500", null ],
              [ "UDP Server (Receive-only listener)", "user_api_guide.html#autotoc_md501", null ]
            ] ]
          ] ],
          [ "UDS Communication", "user_api_guide.html#autotoc_md503", [
            [ "Basic Usage", "user_api_guide.html#autotoc_md504", [
              [ "UDS Server", "user_api_guide.html#autotoc_md505", null ],
              [ "UDS Client", "user_api_guide.html#autotoc_md506", null ]
            ] ],
            [ "API Reference", "user_api_guide.html#autotoc_md507", [
              [ "Constructors", "user_api_guide.html#autotoc_md508", null ],
              [ "Builder Methods (UDS Server)", "user_api_guide.html#autotoc_md509", null ],
              [ "Builder Methods (UDS Client)", "user_api_guide.html#autotoc_md510", null ],
              [ "Instance Methods (UDS Client)", "user_api_guide.html#autotoc_md511", null ],
              [ "Instance Methods (UDS Server)", "user_api_guide.html#autotoc_md512", null ]
            ] ],
            [ "Notes on UDS", "user_api_guide.html#autotoc_md513", null ]
          ] ],
          [ "Error Handling", "user_api_guide.html#autotoc_md515", [
            [ "Setup Error Handler", "user_api_guide.html#autotoc_md516", null ],
            [ "Error Levels", "user_api_guide.html#autotoc_md517", null ],
            [ "Error Statistics", "user_api_guide.html#autotoc_md518", null ]
          ] ],
          [ "Logging System", "user_api_guide.html#autotoc_md520", [
            [ "Basic Usage", "user_api_guide.html#autotoc_md521", null ],
            [ "Log Levels", "user_api_guide.html#autotoc_md522", null ],
            [ "Async Logging", "user_api_guide.html#autotoc_md523", null ]
          ] ],
          [ "Configuration Management", "user_api_guide.html#autotoc_md525", [
            [ "Load Configuration from File", "user_api_guide.html#autotoc_md526", null ],
            [ "Configuration File Format", "user_api_guide.html#autotoc_md527", null ]
          ] ],
          [ "Best Practices", "user_api_guide.html#autotoc_md529", [
            [ "1. Always Handle Errors", "user_api_guide.html#autotoc_md530", null ],
            [ "2. Use Explicit Lifecycle Control", "user_api_guide.html#autotoc_md531", null ],
            [ "3. Set Appropriate Retry Intervals", "user_api_guide.html#autotoc_md532", null ],
            [ "4. Enable Logging for Debugging", "user_api_guide.html#autotoc_md533", null ],
            [ "5. Use Member Functions for OOP Design", "user_api_guide.html#autotoc_md534", null ]
          ] ],
          [ "Performance Tips", "user_api_guide.html#autotoc_md536", [
            [ "1. Use Independent Context for Testing Only", "user_api_guide.html#autotoc_md537", null ],
            [ "2. Enable Async Logging", "user_api_guide.html#autotoc_md538", null ]
          ] ],
          [ "Security", "user_api_guide.html#autotoc_md540", [
            [ "Validate All Input", "user_api_guide.html#autotoc_md541", null ],
            [ "Rate Limiting", "user_api_guide.html#autotoc_md542", null ],
            [ "Connection Limits", "user_api_guide.html#autotoc_md543", null ]
          ] ]
        ] ],
        [ "Tutorials", "user_tutorials.html", [
          [ "Getting Started with Unilink", "tutorial_01.html", [
            [ "What You'll Build", "tutorial_01.html#autotoc_md748", null ],
            [ "Step 1: Create The Client", "tutorial_01.html#autotoc_md750", null ],
            [ "Step 2: Build With CMake", "tutorial_01.html#autotoc_md752", null ],
            [ "Step 3: Run Against A Test Server", "tutorial_01.html#autotoc_md754", null ],
            [ "API Patterns Used In This Tutorial", "tutorial_01.html#autotoc_md756", null ],
            [ "Use The Full Example If You Want More", "tutorial_01.html#autotoc_md758", null ],
            [ "Next Steps", "tutorial_01.html#autotoc_md760", null ]
          ] ],
          [ "Building a TCP Server", "tutorial_02.html", [
            [ "What You'll Build", "tutorial_02.html#autotoc_md763", null ],
            [ "Step 1: Create The Server", "tutorial_02.html#autotoc_md765", null ],
            [ "Step 2: Run It", "tutorial_02.html#autotoc_md767", null ],
            [ "Step 3: Understand The Current Server API", "tutorial_02.html#autotoc_md769", null ],
            [ "Client Limits", "tutorial_02.html#autotoc_md771", null ],
            [ "Use The Full Example Programs For More", "tutorial_02.html#autotoc_md773", null ],
            [ "Next Steps", "tutorial_02.html#autotoc_md775", null ]
          ] ],
          [ "UDS Communication", "tutorial_03.html", [
            [ "What You'll Build", "tutorial_03.html#autotoc_md778", null ],
            [ "Step 1: Create A UDS Server", "tutorial_03.html#autotoc_md780", null ],
            [ "Step 2: Create A UDS Client", "tutorial_03.html#autotoc_md782", null ],
            [ "Why Use UDS Instead Of TCP", "tutorial_03.html#autotoc_md784", null ],
            [ "Operational Notes", "tutorial_03.html#autotoc_md786", null ],
            [ "Next Steps", "tutorial_03.html#autotoc_md788", null ]
          ] ],
          [ "Serial Communication", "tutorial_04.html", [
            [ "What You'll Build", "tutorial_04.html#autotoc_md791", null ],
            [ "Step 1: Choose A Device Path", "tutorial_04.html#autotoc_md793", null ],
            [ "Step 2: Create A Minimal Serial Terminal", "tutorial_04.html#autotoc_md795", null ],
            [ "Step 3: Build And Run", "tutorial_04.html#autotoc_md797", null ],
            [ "Step 4: Test With A Second Terminal", "tutorial_04.html#autotoc_md799", null ],
            [ "Common Adjustments", "tutorial_04.html#autotoc_md801", null ],
            [ "When To Use The Example Programs Instead", "tutorial_04.html#autotoc_md803", null ],
            [ "Next Steps", "tutorial_04.html#autotoc_md805", null ]
          ] ],
          [ "UDP Communication", "tutorial_05.html", [
            [ "What You'll Build", "tutorial_05.html#autotoc_md808", null ],
            [ "Step 1: Create A Receiver", "tutorial_05.html#autotoc_md810", null ],
            [ "Step 2: Create A Sender", "tutorial_05.html#autotoc_md812", null ],
            [ "Step 3: Build The Two Programs", "tutorial_05.html#autotoc_md814", null ],
            [ "Step 4: Run Both Programs", "tutorial_05.html#autotoc_md816", null ],
            [ "What Is Different About UDP", "tutorial_05.html#autotoc_md818", null ],
            [ "Practical Notes", "tutorial_05.html#autotoc_md820", null ],
            [ "Use The Full Examples For Repeated Testing", "tutorial_05.html#autotoc_md822", null ],
            [ "Next Steps", "tutorial_05.html#autotoc_md824", null ]
          ] ]
        ] ],
        [ "Troubleshooting Guide", "user_troubleshooting.html", [
          [ "Table of Contents", "user_troubleshooting.html#autotoc_md669", null ],
          [ "Connection Issues", "user_troubleshooting.html#autotoc_md671", [
            [ "Problem: Connection Refused", "user_troubleshooting.html#autotoc_md672", [
              [ "1. Server Not Running", "user_troubleshooting.html#autotoc_md673", null ],
              [ "2. Wrong Host/Port", "user_troubleshooting.html#autotoc_md674", null ],
              [ "3. Firewall Blocking", "user_troubleshooting.html#autotoc_md675", null ]
            ] ],
            [ "Problem: Connection Timeout", "user_troubleshooting.html#autotoc_md677", [
              [ "1. Network Unreachable", "user_troubleshooting.html#autotoc_md678", null ],
              [ "2. Server Overloaded", "user_troubleshooting.html#autotoc_md679", null ]
            ] ],
            [ "Problem: Connection Drops Randomly", "user_troubleshooting.html#autotoc_md681", [
              [ "1. Network Instability", "user_troubleshooting.html#autotoc_md682", null ],
              [ "2. Server Closing Connection", "user_troubleshooting.html#autotoc_md683", null ],
              [ "3. Keep-Alive Not Set", "user_troubleshooting.html#autotoc_md684", null ]
            ] ],
            [ "Problem: Port Already in Use", "user_troubleshooting.html#autotoc_md686", [
              [ "1. Kill Existing Process", "user_troubleshooting.html#autotoc_md687", null ],
              [ "2. Use Different Port", "user_troubleshooting.html#autotoc_md688", null ],
              [ "3. Enable Port Retry", "user_troubleshooting.html#autotoc_md689", null ]
            ] ]
          ] ],
          [ "Compilation Errors", "user_troubleshooting.html#autotoc_md691", [
            [ "Problem: unilink/unilink.hpp Not Found", "user_troubleshooting.html#autotoc_md692", [
              [ "1. Install unilink", "user_troubleshooting.html#autotoc_md693", null ],
              [ "2. Add Include Path", "user_troubleshooting.html#autotoc_md694", null ],
              [ "3. Use as Subdirectory", "user_troubleshooting.html#autotoc_md695", null ]
            ] ],
            [ "Problem: Undefined Reference to unilink Symbols", "user_troubleshooting.html#autotoc_md697", [
              [ "1. Link unilink Library", "user_troubleshooting.html#autotoc_md698", null ],
              [ "2. Check Library Path", "user_troubleshooting.html#autotoc_md699", null ]
            ] ],
            [ "Problem: Boost Not Found", "user_troubleshooting.html#autotoc_md701", [
              [ "Ubuntu/Debian", "user_troubleshooting.html#autotoc_md702", null ],
              [ "macOS", "user_troubleshooting.html#autotoc_md703", null ],
              [ "Windows (vcpkg)", "user_troubleshooting.html#autotoc_md704", null ],
              [ "Manual Boost Path", "user_troubleshooting.html#autotoc_md705", null ]
            ] ]
          ] ],
          [ "Runtime Errors", "user_troubleshooting.html#autotoc_md707", [
            [ "Problem: Segmentation Fault", "user_troubleshooting.html#autotoc_md708", [
              [ "1. Enable Core Dumps", "user_troubleshooting.html#autotoc_md709", null ],
              [ "2. Common Causes", "user_troubleshooting.html#autotoc_md710", null ]
            ] ],
            [ "Problem: Callbacks Not Being Called", "user_troubleshooting.html#autotoc_md712", [
              [ "1. Callback Not Registered", "user_troubleshooting.html#autotoc_md713", null ],
              [ "2. Client Not Started", "user_troubleshooting.html#autotoc_md714", null ],
              [ "3. Application Exits Too Quickly", "user_troubleshooting.html#autotoc_md715", null ]
            ] ]
          ] ],
          [ "Performance Issues", "user_troubleshooting.html#autotoc_md717", [
            [ "Problem: High CPU Usage", "user_troubleshooting.html#autotoc_md718", [
              [ "1. Busy Loop in Callback", "user_troubleshooting.html#autotoc_md719", null ],
              [ "2. Too Many Retries", "user_troubleshooting.html#autotoc_md720", null ],
              [ "3. Excessive Logging", "user_troubleshooting.html#autotoc_md721", null ]
            ] ],
            [ "Problem: High Memory Usage", "user_troubleshooting.html#autotoc_md723", [
              [ "1. Fix Memory Leaks", "user_troubleshooting.html#autotoc_md724", null ],
              [ "3. Limit Buffer Sizes", "user_troubleshooting.html#autotoc_md725", null ]
            ] ],
            [ "Problem: Slow Data Transfer", "user_troubleshooting.html#autotoc_md727", [
              [ "1. Batch Small Messages", "user_troubleshooting.html#autotoc_md728", null ],
              [ "2. Use Binary Protocol", "user_troubleshooting.html#autotoc_md729", null ],
              [ "3. Enable Async Logging", "user_troubleshooting.html#autotoc_md730", null ]
            ] ]
          ] ],
          [ "Memory Issues", "user_troubleshooting.html#autotoc_md732", [
            [ "Problem: Memory Leak Detected", "user_troubleshooting.html#autotoc_md733", null ]
          ] ],
          [ "Thread Safety Issues", "user_troubleshooting.html#autotoc_md735", [
            [ "Problem: Race Condition / Data Corruption", "user_troubleshooting.html#autotoc_md736", [
              [ "1. Protect Shared State", "user_troubleshooting.html#autotoc_md737", null ]
            ] ]
          ] ],
          [ "Debugging Tips", "user_troubleshooting.html#autotoc_md739", [
            [ "Enable Debug Logging", "user_troubleshooting.html#autotoc_md740", null ],
            [ "Use GDB for Debugging", "user_troubleshooting.html#autotoc_md741", null ],
            [ "Network Debugging with tcpdump", "user_troubleshooting.html#autotoc_md742", null ],
            [ "Test with netcat", "user_troubleshooting.html#autotoc_md743", null ]
          ] ],
          [ "Getting Help", "user_troubleshooting.html#autotoc_md745", null ]
        ] ],
        [ "Python Bindings Guide", "user_python_bindings.html", [
          [ "🚀 Getting Started", "user_python_bindings.html#autotoc_md588", [
            [ "Installation", "user_python_bindings.html#autotoc_md589", null ]
          ] ],
          [ "🔌 TCP Client", "user_python_bindings.html#autotoc_md591", [
            [ "Basic Usage", "user_python_bindings.html#autotoc_md592", null ]
          ] ],
          [ "🖥️ TCP Server", "user_python_bindings.html#autotoc_md594", [
            [ "Basic Usage", "user_python_bindings.html#autotoc_md595", null ]
          ] ],
          [ "📟 Serial Communication", "user_python_bindings.html#autotoc_md597", [
            [ "Basic Usage", "user_python_bindings.html#autotoc_md598", null ]
          ] ],
          [ "🌐 UDP Communication", "user_python_bindings.html#autotoc_md600", [
            [ "Basic Usage", "user_python_bindings.html#autotoc_md601", null ]
          ] ],
          [ "📂 UDS Communication", "user_python_bindings.html#autotoc_md603", [
            [ "Basic Usage", "user_python_bindings.html#autotoc_md604", null ]
          ] ],
          [ "🛠️ Advanced Features", "user_python_bindings.html#autotoc_md606", [
            [ "Message Framing (Line/Packet)", "user_python_bindings.html#autotoc_md607", null ],
            [ "Threading & GIL", "user_python_bindings.html#autotoc_md608", null ],
            [ "Lifecycle Management", "user_python_bindings.html#autotoc_md609", null ],
            [ "Configuration", "user_python_bindings.html#autotoc_md610", null ]
          ] ]
        ] ],
        [ "Performance Guide", "user_performance.html", [
          [ "Table of Contents", "user_performance.html#autotoc_md572", null ],
          [ "Runtime Optimization", "user_performance.html#autotoc_md574", [
            [ "1. Threading Model & IO Context", "user_performance.html#autotoc_md575", null ],
            [ "2. Async Logging", "user_performance.html#autotoc_md576", null ],
            [ "3. Non-Blocking Callbacks", "user_performance.html#autotoc_md577", null ]
          ] ],
          [ "Memory Optimization", "user_performance.html#autotoc_md579", [
            [ "1. Avoid Data Copies", "user_performance.html#autotoc_md580", null ],
            [ "2. Reserve Vector Capacity", "user_performance.html#autotoc_md581", null ]
          ] ],
          [ "Network Optimization", "user_performance.html#autotoc_md583", [
            [ "1. Batch Small Messages", "user_performance.html#autotoc_md584", null ],
            [ "2. Connection Reuse", "user_performance.html#autotoc_md585", null ],
            [ "3. Socket Tuning", "user_performance.html#autotoc_md586", null ]
          ] ]
        ] ]
      ] ],
      [ "Contributor Guide", "contrib_index.html", [
        [ "Building the Library", "contrib_index.html#autotoc_md340", null ],
        [ "Architecture", "contrib_index.html#autotoc_md342", null ],
        [ "Quick Links (User Docs)", "contrib_index.html#autotoc_md344", null ],
        [ "Build Guide", "contrib_build.html", [
          [ "Table of Contents", "contrib_build.html#autotoc_md257", null ],
          [ "Quick Build", "contrib_build.html#autotoc_md259", [
            [ "Basic Build (Recommended)", "contrib_build.html#autotoc_md260", null ]
          ] ],
          [ "Important Build Notes", "contrib_build.html#autotoc_md262", null ],
          [ "Build Configurations", "contrib_build.html#autotoc_md264", [
            [ "Minimal Build (without Configuration Management API)", "contrib_build.html#autotoc_md265", null ],
            [ "Full Build (includes Configuration Management API)", "contrib_build.html#autotoc_md267", null ]
          ] ],
          [ "Build Options Reference", "contrib_build.html#autotoc_md269", [
            [ "Core Options", "contrib_build.html#autotoc_md270", null ],
            [ "Development Options", "contrib_build.html#autotoc_md271", null ],
            [ "Installation Options", "contrib_build.html#autotoc_md272", null ]
          ] ],
          [ "Build Types Comparison", "contrib_build.html#autotoc_md274", [
            [ "Release Build (Default)", "contrib_build.html#autotoc_md275", null ],
            [ "Debug Build", "contrib_build.html#autotoc_md277", null ],
            [ "RelWithDebInfo Build", "contrib_build.html#autotoc_md279", null ]
          ] ],
          [ "Advanced Build Examples", "contrib_build.html#autotoc_md281", [
            [ "Example 1: Minimal Production Build", "contrib_build.html#autotoc_md282", null ],
            [ "Example 2: Development Build with Examples", "contrib_build.html#autotoc_md284", null ],
            [ "Example 3: Testing with Sanitizers", "contrib_build.html#autotoc_md286", null ],
            [ "Example 4: Build with Custom Boost Location", "contrib_build.html#autotoc_md288", null ],
            [ "Example 5: Build with Specific Compiler", "contrib_build.html#autotoc_md290", null ]
          ] ],
          [ "Platform-Specific Builds", "contrib_build.html#autotoc_md292", [
            [ "Ubuntu 22.04 (Recommended)", "contrib_build.html#autotoc_md293", null ],
            [ "Ubuntu 20.04 Build", "contrib_build.html#autotoc_md295", [
              [ "Prerequisites", "contrib_build.html#autotoc_md296", null ],
              [ "Build Steps", "contrib_build.html#autotoc_md297", null ],
              [ "Notes", "contrib_build.html#autotoc_md298", null ]
            ] ],
            [ "Debian 11+", "contrib_build.html#autotoc_md300", null ],
            [ "Fedora 35+", "contrib_build.html#autotoc_md302", null ],
            [ "Arch Linux", "contrib_build.html#autotoc_md304", null ]
          ] ],
          [ "Build Performance Tips", "contrib_build.html#autotoc_md306", [
            [ "Parallel Builds", "contrib_build.html#autotoc_md307", null ],
            [ "Ccache for Faster Rebuilds", "contrib_build.html#autotoc_md308", null ],
            [ "Ninja Build System (Faster than Make)", "contrib_build.html#autotoc_md309", null ]
          ] ],
          [ "Installation", "contrib_build.html#autotoc_md311", [
            [ "System-Wide Installation", "contrib_build.html#autotoc_md312", null ],
            [ "Custom Installation Directory", "contrib_build.html#autotoc_md313", null ],
            [ "Uninstall", "contrib_build.html#autotoc_md314", null ]
          ] ],
          [ "Verifying the Build", "contrib_build.html#autotoc_md316", [
            [ "Run Unit Tests", "contrib_build.html#autotoc_md317", null ],
            [ "Run Examples", "contrib_build.html#autotoc_md318", null ],
            [ "Check Library Symbols", "contrib_build.html#autotoc_md319", null ]
          ] ],
          [ "Troubleshooting", "contrib_build.html#autotoc_md321", [
            [ "Problem: CMake Can't Find Boost", "contrib_build.html#autotoc_md322", null ],
            [ "Problem: Compiler Not Found", "contrib_build.html#autotoc_md323", null ],
            [ "Problem: Out of Memory During Build", "contrib_build.html#autotoc_md324", null ],
            [ "Problem: Permission Denied During Install", "contrib_build.html#autotoc_md325", null ]
          ] ],
          [ "CMake Package Integration", "contrib_build.html#autotoc_md327", [
            [ "Using the Installed Package", "contrib_build.html#autotoc_md328", null ],
            [ "Custom Installation Prefix", "contrib_build.html#autotoc_md329", null ],
            [ "Package Components", "contrib_build.html#autotoc_md330", null ],
            [ "Verification", "contrib_build.html#autotoc_md331", null ]
          ] ],
          [ "Next Steps", "contrib_build.html#autotoc_md333", null ]
        ] ],
        [ "Testing Guide", "contrib_testing.html", [
          [ "Table of Contents", "contrib_testing.html#autotoc_md357", null ],
          [ "Quick Start", "contrib_testing.html#autotoc_md359", [
            [ "Build and Run All Tests", "contrib_testing.html#autotoc_md360", null ],
            [ "Windows Build & Test Workflow", "contrib_testing.html#autotoc_md362", null ]
          ] ],
          [ "Running Tests", "contrib_testing.html#autotoc_md364", [
            [ "Run All Tests", "contrib_testing.html#autotoc_md365", null ],
            [ "Run Specific Test Categories", "contrib_testing.html#autotoc_md367", null ],
            [ "Run Tests with Verbose Output", "contrib_testing.html#autotoc_md369", null ],
            [ "Run Tests in Parallel", "contrib_testing.html#autotoc_md371", null ]
          ] ],
          [ "UDP-specific test policies", "contrib_testing.html#autotoc_md373", null ],
          [ "Test Categories", "contrib_testing.html#autotoc_md374", [
            [ "Core Tests", "contrib_testing.html#autotoc_md375", null ],
            [ "Integration Tests", "contrib_testing.html#autotoc_md377", null ],
            [ "Memory Safety Tests", "contrib_testing.html#autotoc_md379", null ],
            [ "Concurrency Safety Tests", "contrib_testing.html#autotoc_md381", null ],
            [ "Performance Tests", "contrib_testing.html#autotoc_md384", null ],
            [ "Stress Tests", "contrib_testing.html#autotoc_md386", null ]
          ] ],
          [ "Memory Safety Validation", "contrib_testing.html#autotoc_md388", [
            [ "Built-in Memory Tracking", "contrib_testing.html#autotoc_md389", null ],
            [ "AddressSanitizer (ASan)", "contrib_testing.html#autotoc_md391", null ],
            [ "ThreadSanitizer (TSan)", "contrib_testing.html#autotoc_md393", null ],
            [ "Valgrind", "contrib_testing.html#autotoc_md395", null ]
          ] ],
          [ "Continuous Integration", "contrib_testing.html#autotoc_md397", [
            [ "GitHub Actions Integration", "contrib_testing.html#autotoc_md398", null ],
            [ "CI/CD Build Matrix", "contrib_testing.html#autotoc_md400", null ],
            [ "Ubuntu 20.04 Support", "contrib_testing.html#autotoc_md402", null ],
            [ "View CI/CD Results", "contrib_testing.html#autotoc_md404", null ]
          ] ],
          [ "Writing Custom Tests", "contrib_testing.html#autotoc_md406", [
            [ "Test Structure", "contrib_testing.html#autotoc_md407", null ],
            [ "Example: Custom Integration Test", "contrib_testing.html#autotoc_md409", null ],
            [ "Running Custom Tests", "contrib_testing.html#autotoc_md411", null ]
          ] ],
          [ "Test Configuration", "contrib_testing.html#autotoc_md413", [
            [ "CTest Configuration", "contrib_testing.html#autotoc_md414", null ],
            [ "Environment Variables", "contrib_testing.html#autotoc_md416", null ]
          ] ],
          [ "Troubleshooting Tests", "contrib_testing.html#autotoc_md418", [
            [ "Test Failures", "contrib_testing.html#autotoc_md419", null ],
            [ "Port Conflicts", "contrib_testing.html#autotoc_md421", null ],
            [ "Memory Issues", "contrib_testing.html#autotoc_md423", null ]
          ] ],
          [ "Performance Regression Testing", "contrib_testing.html#autotoc_md425", [
            [ "Benchmark Baseline", "contrib_testing.html#autotoc_md426", null ],
            [ "Compare Against Baseline", "contrib_testing.html#autotoc_md427", null ],
            [ "Automated Regression Detection", "contrib_testing.html#autotoc_md428", null ]
          ] ],
          [ "Code Coverage", "contrib_testing.html#autotoc_md430", [
            [ "Generate Coverage Report", "contrib_testing.html#autotoc_md431", null ],
            [ "View HTML Coverage Report", "contrib_testing.html#autotoc_md432", null ]
          ] ],
          [ "Next Steps", "contrib_testing.html#autotoc_md434", null ]
        ] ],
        [ "Implementation Status", "contrib_impl_status.html", [
          [ "Scope", "contrib_impl_status.html#autotoc_md334", null ],
          [ "C++ API Surface", "contrib_impl_status.html#autotoc_md335", null ],
          [ "Python Binding Scope", "contrib_impl_status.html#autotoc_md336", null ],
          [ "Build And Test Status", "contrib_impl_status.html#autotoc_md337", null ],
          [ "Recommended Reading Order", "contrib_impl_status.html#autotoc_md338", null ]
        ] ],
        [ "Test Structure", "contrib_test_structure.html", [
          [ "Layout", "contrib_test_structure.html#autotoc_md346", null ],
          [ "What Each Area Covers", "contrib_test_structure.html#autotoc_md347", null ],
          [ "Build-Time Controls", "contrib_test_structure.html#autotoc_md348", null ],
          [ "Running Tests", "contrib_test_structure.html#autotoc_md349", [
            [ "Run All Registered Tests", "contrib_test_structure.html#autotoc_md350", null ],
            [ "Run By Broad Category", "contrib_test_structure.html#autotoc_md351", null ],
            [ "Useful Focused Runs", "contrib_test_structure.html#autotoc_md352", null ],
            [ "Inspect What Is Currently Registered", "contrib_test_structure.html#autotoc_md353", null ]
          ] ],
          [ "Notes", "contrib_test_structure.html#autotoc_md354", null ],
          [ "CI/CD Integration", "contrib_test_structure.html#autotoc_md355", null ]
        ] ],
        [ "Unilink System Architecture", "contrib_arch.html", [
          [ "Runtime Behavior Model", "contrib_arch_runtime.html", [
            [ "Table of Contents", "contrib_arch_runtime.html#autotoc_md169", null ],
            [ "Threading Model & Callback Execution", "contrib_arch_runtime.html#autotoc_md171", [
              [ "Architecture Diagram", "contrib_arch_runtime.html#autotoc_md172", null ],
              [ "Key Points", "contrib_arch_runtime.html#autotoc_md174", [
                [ "✅ Thread-Safe API Methods", "contrib_arch_runtime.html#autotoc_md175", null ],
                [ "✅ Callback Execution Context", "contrib_arch_runtime.html#autotoc_md177", null ],
                [ "⚠️ Never Block in Callbacks", "contrib_arch_runtime.html#autotoc_md179", null ],
                [ "✅ Thread-Safe State Access", "contrib_arch_runtime.html#autotoc_md181", null ]
              ] ],
              [ "Threading Model Summary", "contrib_arch_runtime.html#autotoc_md183", null ]
            ] ],
            [ "Reconnection Policy & State Machine", "contrib_arch_runtime.html#autotoc_md185", [
              [ "State Machine Diagram", "contrib_arch_runtime.html#autotoc_md186", null ],
              [ "Connection States", "contrib_arch_runtime.html#autotoc_md188", null ],
              [ "Configuration Example", "contrib_arch_runtime.html#autotoc_md190", null ],
              [ "Retry Behavior", "contrib_arch_runtime.html#autotoc_md192", [
                [ "Default Behavior", "contrib_arch_runtime.html#autotoc_md193", null ],
                [ "Retry Interval Configuration", "contrib_arch_runtime.html#autotoc_md194", null ],
                [ "State Callbacks", "contrib_arch_runtime.html#autotoc_md196", null ],
                [ "Manual Control", "contrib_arch_runtime.html#autotoc_md198", null ]
              ] ],
              [ "Reconnection Best Practices", "contrib_arch_runtime.html#autotoc_md200", [
                [ "1. Choose Appropriate Retry Interval", "contrib_arch_runtime.html#autotoc_md201", null ],
                [ "2. Handle State Transitions", "contrib_arch_runtime.html#autotoc_md203", null ],
                [ "3. Graceful Shutdown", "contrib_arch_runtime.html#autotoc_md205", null ]
              ] ]
            ] ],
            [ "Backpressure Handling", "contrib_arch_runtime.html#autotoc_md207", [
              [ "Backpressure Flow", "contrib_arch_runtime.html#autotoc_md208", null ],
              [ "Backpressure Configuration", "contrib_arch_runtime.html#autotoc_md210", null ],
              [ "Backpressure Strategies", "contrib_arch_runtime.html#autotoc_md212", [
                [ "Strategy 1: Pause Sending", "contrib_arch_runtime.html#autotoc_md213", null ],
                [ "Strategy 2: Rate Limiting", "contrib_arch_runtime.html#autotoc_md215", null ],
                [ "Strategy 3: Drop Data", "contrib_arch_runtime.html#autotoc_md217", null ]
              ] ],
              [ "Backpressure Monitoring", "contrib_arch_runtime.html#autotoc_md219", null ],
              [ "Memory Safety", "contrib_arch_runtime.html#autotoc_md221", null ]
            ] ],
            [ "Best Practices", "contrib_arch_runtime.html#autotoc_md223", [
              [ "1. Threading Best Practices", "contrib_arch_runtime.html#autotoc_md224", [
                [ "✅ DO", "contrib_arch_runtime.html#autotoc_md225", null ],
                [ "❌ DON'T", "contrib_arch_runtime.html#autotoc_md226", null ]
              ] ],
              [ "2. Reconnection Best Practices", "contrib_arch_runtime.html#autotoc_md228", [
                [ "✅ DO", "contrib_arch_runtime.html#autotoc_md229", null ],
                [ "❌ DON'T", "contrib_arch_runtime.html#autotoc_md230", null ]
              ] ],
              [ "3. Backpressure Best Practices", "contrib_arch_runtime.html#autotoc_md232", [
                [ "✅ DO", "contrib_arch_runtime.html#autotoc_md233", null ],
                [ "❌ DON'T", "contrib_arch_runtime.html#autotoc_md234", null ]
              ] ]
            ] ],
            [ "Performance Considerations", "contrib_arch_runtime.html#autotoc_md236", [
              [ "Threading Overhead", "contrib_arch_runtime.html#autotoc_md237", null ],
              [ "Reconnection Overhead", "contrib_arch_runtime.html#autotoc_md238", null ],
              [ "Backpressure Overhead", "contrib_arch_runtime.html#autotoc_md239", null ]
            ] ],
            [ "Next Steps", "contrib_arch_runtime.html#autotoc_md241", null ]
          ] ],
          [ "Memory Safety Architecture", "contrib_arch_memory.html", [
            [ "Table of Contents", "contrib_arch_memory.html#autotoc_md7", null ],
            [ "Overview", "contrib_arch_memory.html#autotoc_md9", [
              [ "Memory Safety Guarantees", "contrib_arch_memory.html#autotoc_md10", null ],
              [ "Safety Levels", "contrib_arch_memory.html#autotoc_md12", null ]
            ] ],
            [ "Safe Data Handling", "contrib_arch_memory.html#autotoc_md14", [
              [ "SafeDataBuffer", "contrib_arch_memory.html#autotoc_md15", null ],
              [ "Features", "contrib_arch_memory.html#autotoc_md17", [
                [ "1. Construction Validation", "contrib_arch_memory.html#autotoc_md18", null ],
                [ "2. Safe Type Conversions", "contrib_arch_memory.html#autotoc_md20", null ],
                [ "3. Memory Validation", "contrib_arch_memory.html#autotoc_md22", null ]
              ] ],
              [ "Safe Span (C++17 Compatible)", "contrib_arch_memory.html#autotoc_md24", null ]
            ] ],
            [ "Thread-Safe State Management", "contrib_arch_memory.html#autotoc_md26", [
              [ "ThreadSafeState", "contrib_arch_memory.html#autotoc_md27", null ],
              [ "AtomicState", "contrib_arch_memory.html#autotoc_md29", null ],
              [ "ThreadSafeCounter", "contrib_arch_memory.html#autotoc_md31", null ],
              [ "ThreadSafeFlag", "contrib_arch_memory.html#autotoc_md33", null ],
              [ "Thread Safety Summary", "contrib_arch_memory.html#autotoc_md35", null ]
            ] ],
            [ "Memory Tracking", "contrib_arch_memory.html#autotoc_md37", [
              [ "Built-in Memory Tracking", "contrib_arch_memory.html#autotoc_md38", null ],
              [ "Features", "contrib_arch_memory.html#autotoc_md40", [
                [ "1. Allocation Tracking", "contrib_arch_memory.html#autotoc_md41", null ],
                [ "2. Leak Detection", "contrib_arch_memory.html#autotoc_md43", null ],
                [ "3. Performance Monitoring", "contrib_arch_memory.html#autotoc_md45", null ],
                [ "4. Debug Reports", "contrib_arch_memory.html#autotoc_md47", null ]
              ] ],
              [ "Zero Overhead in Release", "contrib_arch_memory.html#autotoc_md49", null ]
            ] ],
            [ "AddressSanitizer Support", "contrib_arch_memory.html#autotoc_md51", [
              [ "Enable AddressSanitizer", "contrib_arch_memory.html#autotoc_md52", null ],
              [ "What ASan Detects", "contrib_arch_memory.html#autotoc_md54", null ],
              [ "Running with ASan", "contrib_arch_memory.html#autotoc_md56", null ],
              [ "Performance Impact", "contrib_arch_memory.html#autotoc_md58", null ]
            ] ],
            [ "Best Practices", "contrib_arch_memory.html#autotoc_md60", [
              [ "1. Buffer Management", "contrib_arch_memory.html#autotoc_md61", [
                [ "✅ DO", "contrib_arch_memory.html#autotoc_md62", null ],
                [ "❌ DON'T", "contrib_arch_memory.html#autotoc_md63", null ]
              ] ],
              [ "2. Type Conversions", "contrib_arch_memory.html#autotoc_md65", [
                [ "✅ DO", "contrib_arch_memory.html#autotoc_md66", null ],
                [ "❌ DON'T", "contrib_arch_memory.html#autotoc_md67", null ]
              ] ],
              [ "3. Thread Safety", "contrib_arch_memory.html#autotoc_md69", [
                [ "✅ DO", "contrib_arch_memory.html#autotoc_md70", null ],
                [ "❌ DON'T", "contrib_arch_memory.html#autotoc_md71", null ]
              ] ],
              [ "4. Memory Tracking", "contrib_arch_memory.html#autotoc_md73", [
                [ "✅ DO", "contrib_arch_memory.html#autotoc_md74", null ],
                [ "❌ DON'T", "contrib_arch_memory.html#autotoc_md75", null ]
              ] ],
              [ "5. Sanitizers", "contrib_arch_memory.html#autotoc_md77", [
                [ "✅ DO", "contrib_arch_memory.html#autotoc_md78", null ],
                [ "❌ DON'T", "contrib_arch_memory.html#autotoc_md79", null ]
              ] ]
            ] ],
            [ "Memory Safety Benefits", "contrib_arch_memory.html#autotoc_md81", [
              [ "Prevents Common Vulnerabilities", "contrib_arch_memory.html#autotoc_md82", null ],
              [ "Performance", "contrib_arch_memory.html#autotoc_md84", null ]
            ] ],
            [ "Testing Memory Safety", "contrib_arch_memory.html#autotoc_md86", [
              [ "Unit Tests", "contrib_arch_memory.html#autotoc_md87", null ],
              [ "Integration Tests", "contrib_arch_memory.html#autotoc_md89", null ],
              [ "Continuous Integration", "contrib_arch_memory.html#autotoc_md91", null ]
            ] ],
            [ "Next Steps", "contrib_arch_memory.html#autotoc_md93", null ]
          ] ],
          [ "Transport Channel Contract", "contrib_arch_channel.html", [
            [ "1. Introduction", "contrib_arch_channel.html#autotoc_md0", null ],
            [ "2. Core Principles", "contrib_arch_channel.html#autotoc_md1", null ],
            [ "3. Stop Semantics: No Callbacks After Stop()", "contrib_arch_channel.html#autotoc_md2", null ],
            [ "4. Backpressure Policy", "contrib_arch_channel.html#autotoc_md3", null ],
            [ "5. Error Handling Consistency", "contrib_arch_channel.html#autotoc_md4", null ],
            [ "6. State Transitions", "contrib_arch_channel.html#autotoc_md5", null ]
          ] ],
          [ "Wrapper Contract", "contrib_arch_wrapper.html", [
            [ "Scope", "contrib_arch_wrapper.html#autotoc_md242", null ],
            [ "Core Rules", "contrib_arch_wrapper.html#autotoc_md243", [
              [ "1. <tt>start()</tt> reflects real transport state", "contrib_arch_wrapper.html#autotoc_md244", null ],
              [ "2. Repeated <tt>start()</tt> and <tt>stop()</tt> are safe", "contrib_arch_wrapper.html#autotoc_md245", null ],
              [ "3. <tt>auto_start(true)</tt> follows the same startup contract", "contrib_arch_wrapper.html#autotoc_md246", null ]
            ] ],
            [ "External <tt>io_context</tt> Contract", "contrib_arch_wrapper.html#autotoc_md247", [
              [ "4. Externally supplied <tt>io_context</tt> can be reused", "contrib_arch_wrapper.html#autotoc_md248", null ],
              [ "5. Managed and unmanaged external contexts have different ownership rules", "contrib_arch_wrapper.html#autotoc_md249", null ]
            ] ],
            [ "Callback Contract", "contrib_arch_wrapper.html#autotoc_md250", [
              [ "6. Handler replacement uses the latest callback", "contrib_arch_wrapper.html#autotoc_md251", null ],
              [ "7. No wrapper callbacks after <tt>stop()</tt> returns", "contrib_arch_wrapper.html#autotoc_md252", null ],
              [ "8. Generic fallback errors are normalized", "contrib_arch_wrapper.html#autotoc_md253", null ]
            ] ],
            [ "Transport-Agnostic Expectations", "contrib_arch_wrapper.html#autotoc_md254", null ],
            [ "Testing Status", "contrib_arch_wrapper.html#autotoc_md255", null ]
          ] ]
        ] ]
      ] ]
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
"classunilink_1_1memory_1_1PooledBuffer.html#a5e365743f105fa8fc0dde10154044722",
"classunilink_1_1transport_1_1BoostUdsSocket.html#ac757a6f588479225a140b0e0d11e9343",
"classunilink_1_1transport_1_1UdsClient.html#a3d6266cd525005baddcbb2f3a68d1394",
"classunilink_1_1wrapper_1_1Serial.html",
"classunilink_1_1wrapper_1_1UdpClient.html#ab5fdcb6f6998e30c96c5dc5e8b9f9e4c",
"constants_8hpp.html#a7e7b61bde076c73dbcf8a77de61f9f46",
"contrib_testing.html#autotoc_md411",
"index.html#autotoc_md145",
"namespaceunilink_1_1diagnostics.html#ab6a79961fe30d5f3931ccf667666f4c0abb1ca97ec761fc37101737ba0aa2e7c5",
"structunilink_1_1config_1_1TcpServerConfig.html#a23e893ec9a547edc4b3bf9f63309643e",
"structunilink_1_1diagnostics_1_1Logger_1_1Impl.html#a4440cb13b53b0af86dc97a374a880a72",
"structunilink_1_1transport_1_1TcpClient_1_1Impl.html#a4e933115e6ad0d8a381ad415015d0ef1",
"structunilink_1_1transport_1_1UdsClient_1_1Impl.html#a1a47f1f2a3f6eb794592ebcf0bd97714",
"structunilink_1_1wrapper_1_1TcpClient_1_1Impl.html#a2af2fc4ff23dc83bb75ae8fc21540d99",
"structunilink_1_1wrapper_1_1UdpServer_1_1Impl.html#a0036f0adbe3084ead3dcd497511f0d2f",
"structunilink_1_1wrapper_1_1UdsServer_1_1Impl.html#ae21ba9040a7052fe4f62e113136d7416",
"user_installation.html#autotoc_md564"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';